#include "db_layer.h"
#include "db_layer_internal.h"
#include "cJSON.h"
#include <errno.h>
#include <iconv.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * db_layer.c: JSON方式の暫定実装。
 * Oracle/Pro*C本実装が入るまでの繋ぎとして、ローカルJSONファイルを疑似DBとして読み込み、
 * メモリ上にキャッシュした配列を線形探索することで db_layer_find_customer() を実現する。
 * SQLは一切使わない。design_db_layer.md 参照。
 */

/* db_layer_init() が読み込む本番/開発用データのパス（固定）。design_db_layer.md 4節。 */
static const char *CUSTOMER_DATA_JSON_PATH = "src/data/customers.json";

/* JSONから読み込んだ顧客レコードをプロセス終了までキャッシュしておく静的な配列。 */
static DbCustomerRecord *g_cached_records = NULL;
static size_t g_cached_count = 0;

/*
 * customer_id/birth_date/phone_number/email/inserted_at/updated_at用のコピー関数。
 * これらはいずれもASCII/数字のみのフィールドのため、文字コード変換は不要で、
 * バイト長チェックのみ行う。
 */
static int copy_string_field(char *dest, size_t dest_field_size, const cJSON *obj, const char *key) {
    const cJSON *field = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!cJSON_IsString(field) || field->valuestring == NULL) {
        return -1;
    }
    size_t len = strlen(field->valuestring);
    if (len > dest_field_size - 1) {
        return -1;
    }
    memset(dest, 0, dest_field_size);
    memcpy(dest, field->valuestring, len);
    return 0;
}

static int copy_int_field(int *dest, const cJSON *obj, const char *key) {
    const cJSON *field = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!cJSON_IsNumber(field)) {
        return -1;
    }
    *dest = field->valueint;
    return 0;
}

/*
 * design.md 4.1節: name/addressはJSON内ではUTF-8だが、格納先はSJIS(Shift_JIS)前提のフィールドのため、
 * iconvでUTF-8→SJISに変換してからコピーする。
 * 変換後のバイト長がdest_field_sizeに収まらない場合（E2BIG）、
 * および不正なUTF-8シーケンスの場合（EILSEQ）は、いずれも呼び出し元でDB_ERROR扱いにするため -1 を返す。
 */
static int convert_utf8_to_sjis(const char *utf8_str, char *dest, size_t dest_field_size) {
    iconv_t cd = iconv_open("SHIFT_JIS", "UTF-8");
    if (cd == (iconv_t)-1) {
        return -1;
    }

    size_t in_bytes_left = strlen(utf8_str);
    /* iconv()の第2引数はPOSIX/glibc/macOSいずれもchar**（constではない）ため、明示的にキャストする。 */
    char *in_ptr = (char *)utf8_str;
    char *out_ptr = dest;
    size_t out_bytes_left = dest_field_size - 1; /* 終端の'\0'分は残す */

    memset(dest, 0, dest_field_size);
    size_t rc = iconv(cd, &in_ptr, &in_bytes_left, &out_ptr, &out_bytes_left);
    iconv_close(cd);

    if (rc == (size_t)-1) {
        return -1;
    }
    return 0;
}

static int copy_sjis_field(char *dest, size_t dest_field_size, const cJSON *obj, const char *key) {
    const cJSON *field = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!cJSON_IsString(field) || field->valuestring == NULL) {
        return -1;
    }
    return convert_utf8_to_sjis(field->valuestring, dest, dest_field_size);
}

/* 1レコード分のJSONオブジェクトをDbCustomerRecordへ変換する。1項目でも不正ならDB_ERROR扱い（-1）。 */
static int parse_record(const cJSON *json_record, DbCustomerRecord *out) {
    memset(out, 0, sizeof(*out));

    if (copy_string_field(out->customer_id, sizeof(out->customer_id), json_record, "customer_id") != 0) {
        return -1;
    }
    if (copy_sjis_field(out->name, sizeof(out->name), json_record, "name") != 0) {
        return -1;
    }
    if (copy_string_field(out->birth_date, sizeof(out->birth_date), json_record, "birth_date") != 0) {
        return -1;
    }
    if (copy_sjis_field(out->address, sizeof(out->address), json_record, "address") != 0) {
        return -1;
    }
    if (copy_string_field(out->phone_number, sizeof(out->phone_number), json_record, "phone_number") != 0) {
        return -1;
    }
    if (copy_string_field(out->email, sizeof(out->email), json_record, "email") != 0) {
        return -1;
    }
    if (copy_string_field(out->inserted_at, sizeof(out->inserted_at), json_record, "inserted_at") != 0) {
        return -1;
    }
    if (copy_string_field(out->updated_at, sizeof(out->updated_at), json_record, "updated_at") != 0) {
        return -1;
    }
    if (copy_int_field(&out->service_a, json_record, "service_a") != 0) {
        return -1;
    }
    if (copy_int_field(&out->service_b, json_record, "service_b") != 0) {
        return -1;
    }
    if (copy_int_field(&out->service_c, json_record, "service_c") != 0) {
        return -1;
    }

    return 0;
}

void db_layer_close(void) {
    free(g_cached_records);
    g_cached_records = NULL;
    g_cached_count = 0;
}

DbResult db_layer_load_from_path(const char *json_path) {
    /* 前回分のキャッシュが残っていれば解放してから読み直す（テストで複数フィクスチャを切り替える際も安全にするため） */
    db_layer_close();

    FILE *fp = fopen(json_path, "rb");
    if (fp == NULL) {
        return DB_RESULT_CONNECTION_ERROR;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return DB_RESULT_CONNECTION_ERROR;
    }
    long file_size = ftell(fp);
    if (file_size < 0) {
        fclose(fp);
        return DB_RESULT_CONNECTION_ERROR;
    }
    rewind(fp);

    char *buffer = malloc((size_t)file_size + 1);
    if (buffer == NULL) {
        fclose(fp);
        return DB_RESULT_DB_ERROR;
    }
    size_t read_size = fread(buffer, 1, (size_t)file_size, fp);
    fclose(fp);
    buffer[read_size] = '\0';

    cJSON *root = cJSON_Parse(buffer);
    free(buffer);
    if (root == NULL) {
        /* JSONとして構文が壊れている場合。design_db_layer.md 5節のDB_ERRORケース。 */
        return DB_RESULT_DB_ERROR;
    }

    const cJSON *customers = cJSON_GetObjectItemCaseSensitive(root, "customers");
    if (!cJSON_IsArray(customers)) {
        cJSON_Delete(root);
        return DB_RESULT_DB_ERROR;
    }

    int count = cJSON_GetArraySize(customers);
    DbCustomerRecord *records = NULL;
    if (count > 0) {
        records = malloc((size_t)count * sizeof(DbCustomerRecord));
        if (records == NULL) {
            cJSON_Delete(root);
            return DB_RESULT_DB_ERROR;
        }
    }

    int i = 0;
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, customers) {
        if (parse_record(item, &records[i]) != 0) {
            free(records);
            cJSON_Delete(root);
            return DB_RESULT_DB_ERROR;
        }
        i++;
    }

    cJSON_Delete(root);

    g_cached_records = records;
    g_cached_count = (size_t)count;
    return DB_RESULT_OK;
}

DbResult db_layer_init(void) {
    return db_layer_load_from_path(CUSTOMER_DATA_JSON_PATH);
}

DbResult db_layer_find_customer(const char *customer_id, DbCustomerRecord *out_record) {
    size_t match_count = 0;
    const DbCustomerRecord *matched = NULL;

    for (size_t i = 0; i < g_cached_count; i++) {
        if (strcmp(g_cached_records[i].customer_id, customer_id) == 0) {
            match_count++;
            matched = &g_cached_records[i];
        }
    }

    if (match_count == 0) {
        return DB_RESULT_NOT_FOUND;
    }
    if (match_count >= 2) {
        return DB_RESULT_DATA_CONFLICT;
    }

    *out_record = *matched;
    return DB_RESULT_OK;
}
