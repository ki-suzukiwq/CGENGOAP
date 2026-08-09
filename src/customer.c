#include "customer.h"
#include "db_layer.h"
#include <ctype.h>
#include <string.h>

/*
 * db_layer.h の DbResult（DB層のエラーコード）を、
 * customer.h の CustomerResult（ビジネスロジック層のエラーコード）に変換する関数。
 * static を付けているので、この customer.c ファイルの中だけで使える「内部関数」になる
 * （他のファイルから呼び出すことはできない＝公開APIではない）。
 * design.md 8.1節: switch文で1対1にマッピングする。defaultは想定外値のフォールバックで、
 * 「DB側の想定外エラー」の定義に合わせて CUSTOMER_RESULT_DB_ERROR を返す。
 */
static CustomerResult convert_db_result(DbResult db_result) {
    switch (db_result) {
        case DB_RESULT_OK:
            return CUSTOMER_RESULT_OK;
        case DB_RESULT_NOT_FOUND:
            return CUSTOMER_RESULT_NOT_FOUND;
        case DB_RESULT_CONNECTION_ERROR:
            return CUSTOMER_RESULT_DB_CONNECTION_ERROR;
        case DB_RESULT_DB_ERROR:
            return CUSTOMER_RESULT_DB_ERROR;
        case DB_RESULT_DATA_CONFLICT:
            return CUSTOMER_RESULT_DATA_CONFLICT;
        default:
            return CUSTOMER_RESULT_DB_ERROR;
    }
}

/*
 * エラー時の共通処理: out_customerの全フィールドをゼロクリアしてから
 * resultフィールドにだけエラーコードを設定する（requirements.md 3.2参照）。
 * out_customerがNULLでないことは呼び出し元（customer_get）で保証済み。
 */
static void set_error_result(Customer *out_customer, CustomerResult result) {
    memset(out_customer, 0, sizeof(*out_customer));
    out_customer->result = result;
}

/**
 * PERF: O(1) - DB1件検索のみ、ループ処理なし（customer_id長は24桁固定のため文字種チェックも定数時間）
 *
 * @brief   顧客IDをキーに顧客情報を1件取得する（本APの唯一の公開API）
 * @param   customer_id  検索対象の顧客ID（英数字24桁、大文字のみ）
 * @param   out_customer 取得結果を格納する出力先構造体（呼び出し元が確保したメモリを渡す）。
 *                       NULLの場合は書き込みを行わず CUSTOMER_RESULT_INVALID_ARGUMENT を返す。
 * @return  CUSTOMER_RESULT_OK ほか、上記CustomerResultのエラーコード
 *          （out_customerがNULLでない場合、out_customer->result にも同じ値が入る）
 */
CustomerResult customer_get(const char *customer_id, Customer *out_customer) {
    /* design.md 8.2 Step0: out_customer自体がNULLなら、書き込み先が無いため戻り値のみで通知する */
    if (out_customer == NULL) {
        return CUSTOMER_RESULT_INVALID_ARGUMENT;
    }

    /* design.md 8.2 Step1-1: NULLチェック */
    if (customer_id == NULL) {
        set_error_result(out_customer, CUSTOMER_RESULT_INVALID_FORMAT);
        return CUSTOMER_RESULT_INVALID_FORMAT;
    }

    /* design.md 8.2 Step1-2: 桁数チェック。
     * strnlenで25バイトを上限に走査することで、極端に長い文字列や
     * 万一NUL終端されていない不正な入力に対しても無制限に走査しない。
     * 空文字・23桁・25桁・1000文字はいずれもこの条件式1本で検出する。 */
    if (strnlen(customer_id, 25) != 24) {
        set_error_result(out_customer, CUSTOMER_RESULT_INVALID_FORMAT);
        return CUSTOMER_RESULT_INVALID_FORMAT;
    }

    /* design.md 8.2 Step1-3: 文字種チェック（英数字、英字は大文字のみ許容） */
    for (size_t i = 0; i < 24; i++) {
        unsigned char c = (unsigned char)customer_id[i];
        if (!(isupper(c) || isdigit(c))) {
            set_error_result(out_customer, CUSTOMER_RESULT_INVALID_FORMAT);
            return CUSTOMER_RESULT_INVALID_FORMAT;
        }
    }

    /* design.md 8.2 Step2: db_layerへの検索委譲 */
    DbCustomerRecord db_record;
    DbResult db_result = db_layer_find_customer(customer_id, &db_record);

    /* design.md 8.2 Step3: DB層のエラーコードをビジネスロジック層のエラーコードに変換 */
    CustomerResult customer_result = convert_db_result(db_result);

    /* design.md 8.2 Step4a: 異常時は早期return */
    if (customer_result != CUSTOMER_RESULT_OK) {
        set_error_result(out_customer, customer_result);
        return customer_result;
    }

    /* design.md 8.2 Step4b: 正常時はDbCustomerRecordの全項目をCustomerへコピー */
    memcpy(out_customer->customer_id, db_record.customer_id, sizeof(out_customer->customer_id));
    memcpy(out_customer->name, db_record.name, sizeof(out_customer->name));
    memcpy(out_customer->birth_date, db_record.birth_date, sizeof(out_customer->birth_date));
    memcpy(out_customer->address, db_record.address, sizeof(out_customer->address));
    memcpy(out_customer->phone_number, db_record.phone_number, sizeof(out_customer->phone_number));
    memcpy(out_customer->email, db_record.email, sizeof(out_customer->email));
    memcpy(out_customer->inserted_at, db_record.inserted_at, sizeof(out_customer->inserted_at));
    memcpy(out_customer->updated_at, db_record.updated_at, sizeof(out_customer->updated_at));
    out_customer->service_a = db_record.service_a;
    out_customer->service_b = db_record.service_b;
    out_customer->service_c = db_record.service_c;
    out_customer->result = CUSTOMER_RESULT_OK;

    return CUSTOMER_RESULT_OK;
}
