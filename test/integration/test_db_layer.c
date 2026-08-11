#include "unity.h"
#include "db_layer.h"
#include "db_layer_internal.h"
#include <iconv.h>
#include <string.h>

/*
 * db_layer.c（JSON方式暫定実装）自体に対するテスト。
 * test_customer.c（customer.c対象、db_layerはmockに差し替え）とは独立したテストバイナリ。
 * design_db_layer.md 5, 7節参照。
 */

#define FIXTURE_MAIN "test/integration/fixtures/db_layer/customers_main.json"
#define FIXTURE_MALFORMED "test/integration/fixtures/db_layer/customers_malformed.json"
#define FIXTURE_OVERFLOW "test/integration/fixtures/db_layer/customers_overflow.json"
#define FIXTURE_MISSING "test/integration/fixtures/db_layer/customers_does_not_exist.json"

void setUp(void) {
}

/* 各テスト後にキャッシュを解放し、次のテストへ影響が残らないようにする */
void tearDown(void) {
    db_layer_close();
}

/*
 * db_layer.c内でUTF-8→SJISに変換されたフィールドを検証するためのヘルパー。
 * 逆方向(SJIS→UTF-8)にiconvで変換し、元のUTF-8文字列と一致することを確認することで、
 * 変換が正しく行われたことを検証する。
 */
static void assert_sjis_field_equals_utf8(const char *sjis_bytes, const char *expected_utf8) {
    iconv_t cd = iconv_open("UTF-8", "SHIFT_JIS");
    TEST_ASSERT_TRUE(cd != (iconv_t)-1);

    char in_buf[256];
    memset(in_buf, 0, sizeof(in_buf));
    strncpy(in_buf, sjis_bytes, sizeof(in_buf) - 1);
    size_t in_bytes_left = strlen(in_buf);
    char *in_ptr = in_buf;

    char out_buf[256];
    memset(out_buf, 0, sizeof(out_buf));
    char *out_ptr = out_buf;
    size_t out_bytes_left = sizeof(out_buf) - 1;

    size_t rc = iconv(cd, &in_ptr, &in_bytes_left, &out_ptr, &out_bytes_left);
    iconv_close(cd);

    TEST_ASSERT_TRUE(rc != (size_t)-1);
    TEST_ASSERT_EQUAL_STRING(expected_utf8, out_buf);
}

/* TC-DB-N01: 正常取得（フィールドマッピング・SJIS変換の検証を含む） */
void test_db_layer_find_customer_valid_id_should_return_ok_with_fields(void) {
    DbResult init_result = db_layer_load_from_path(FIXTURE_MAIN);
    TEST_ASSERT_EQUAL(DB_RESULT_OK, init_result);

    DbCustomerRecord record;
    DbResult result = db_layer_find_customer("AAAA1111BBBB2222CCCC3333", &record);

    TEST_ASSERT_EQUAL(DB_RESULT_OK, result);
    TEST_ASSERT_EQUAL_STRING("AAAA1111BBBB2222CCCC3333", record.customer_id);
    assert_sjis_field_equals_utf8(record.name, "山田太郎");
    TEST_ASSERT_EQUAL_STRING("19900101", record.birth_date);
    assert_sjis_field_equals_utf8(record.address, "東京都千代田区1-1-1");
    TEST_ASSERT_EQUAL_STRING("0312345678", record.phone_number);
    TEST_ASSERT_EQUAL_STRING("taro.yamada@example.com", record.email);
    TEST_ASSERT_EQUAL_STRING("20240101120000", record.inserted_at);
    TEST_ASSERT_EQUAL_STRING("20240102130000", record.updated_at);
    TEST_ASSERT_EQUAL(1, record.service_a);
    TEST_ASSERT_EQUAL(0, record.service_b);
    TEST_ASSERT_EQUAL(1, record.service_c);
}

/* TC-DB-E01: 該当ID無し */
void test_db_layer_find_customer_nonexistent_id_should_return_not_found(void) {
    DbResult init_result = db_layer_load_from_path(FIXTURE_MAIN);
    TEST_ASSERT_EQUAL(DB_RESULT_OK, init_result);

    DbCustomerRecord record;
    DbResult result = db_layer_find_customer("ZZZZ9999ZZZZ9999ZZZZ9999", &record);

    TEST_ASSERT_EQUAL(DB_RESULT_NOT_FOUND, result);
}

/* TC-DB-E02: 整合性異常（重複ID） */
void test_db_layer_find_customer_duplicate_id_should_return_data_conflict(void) {
    DbResult init_result = db_layer_load_from_path(FIXTURE_MAIN);
    TEST_ASSERT_EQUAL(DB_RESULT_OK, init_result);

    DbCustomerRecord record;
    DbResult result = db_layer_find_customer("DDDD4444EEEE5555FFFF6666", &record);

    TEST_ASSERT_EQUAL(DB_RESULT_DATA_CONFLICT, result);
}

/* TC-DB-E03: 接続断（ファイルが開けない） */
void test_db_layer_load_missing_file_should_return_connection_error(void) {
    DbResult init_result = db_layer_load_from_path(FIXTURE_MISSING);

    TEST_ASSERT_EQUAL(DB_RESULT_CONNECTION_ERROR, init_result);
}

/* TC-DB-E04: DB想定外エラー（JSON破損） */
void test_db_layer_load_malformed_json_should_return_db_error(void) {
    DbResult init_result = db_layer_load_from_path(FIXTURE_MALFORMED);

    TEST_ASSERT_EQUAL(DB_RESULT_DB_ERROR, init_result);
}

/* TC-DB-E05: 文字コード変換後のバッファ超過 */
void test_db_layer_load_field_overflow_should_return_db_error(void) {
    DbResult init_result = db_layer_load_from_path(FIXTURE_OVERFLOW);

    TEST_ASSERT_EQUAL(DB_RESULT_DB_ERROR, init_result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_db_layer_find_customer_valid_id_should_return_ok_with_fields);
    RUN_TEST(test_db_layer_find_customer_nonexistent_id_should_return_not_found);
    RUN_TEST(test_db_layer_find_customer_duplicate_id_should_return_data_conflict);
    RUN_TEST(test_db_layer_load_missing_file_should_return_connection_error);
    RUN_TEST(test_db_layer_load_malformed_json_should_return_db_error);
    RUN_TEST(test_db_layer_load_field_overflow_should_return_db_error);
    return UNITY_END();
}
