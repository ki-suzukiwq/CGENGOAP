#include "unity.h"
#include "customer.h"
#include "mock_db_layer.h"
#include <string.h>

/* 全テストで使い回す、形式として正しい顧客ID（英数字24桁、大文字のみ） */
static const char *VALID_ID = "ABCD1234EFGH5678IJKL9012";

/*
 * Unity（テストフレームワーク）の決まり事:
 * - setUp() は「各テスト関数が実行される直前」に毎回自動的に呼ばれる
 * - tearDown() は「各テスト関数が実行された直後」に毎回自動的に呼ばれる
 * ここでは setUp() で毎回モックの状態をリセットし、
 * 「前のテストで設定した戻り値が次のテストに残ってしまう」事故を防いでいる。
 */
void setUp(void) {
    mock_db_layer_reset();
}

void tearDown(void) {
}

/*
 * エラー系テスト共通のヘルパー: out_customerの全フィールドが
 * ゼロクリアされ、resultだけが期待エラーコードになっていることを検証する。
 * Customer構造体をまるごとゼロ埋めした期待値と、実際の出力をメモリ比較することで、
 * 「他のフィールドに1バイトでもゴミが残っていないか」を1回のアサーションで確認できる。
 */
static void assert_error_result(const Customer *out_customer, CustomerResult expected_result) {
    Customer expected;
    memset(&expected, 0, sizeof(expected));
    expected.result = expected_result;
    TEST_ASSERT_EQUAL_MEMORY(&expected, out_customer, sizeof(Customer));
}

/* TC-N01: 正常取得（サービスA/B/C混在） */
void test_customer_get_valid_id_with_mixed_services_should_return_ok(void) {
    DbCustomerRecord record;
    memset(&record, 0, sizeof(record));
    strcpy(record.customer_id, VALID_ID);
    strcpy(record.name, "YAMADA TAROU");
    strcpy(record.birth_date, "19900101");
    strcpy(record.address, "TOKYO-TO CHIYODA-KU 1-1-1");
    strcpy(record.phone_number, "0312345678");
    strcpy(record.email, "taro.yamada@example.com");
    strcpy(record.inserted_at, "20240101120000");
    strcpy(record.updated_at, "20240102130000");
    record.service_a = 1;
    record.service_b = 0;
    record.service_c = 1;
    mock_db_layer_set_find_customer_result(DB_RESULT_OK);
    mock_db_layer_set_find_customer_record(&record);

    Customer out_customer;
    CustomerResult result = customer_get(VALID_ID, &out_customer);

    TEST_ASSERT_EQUAL(CUSTOMER_RESULT_OK, result);
    TEST_ASSERT_EQUAL(CUSTOMER_RESULT_OK, out_customer.result);
    TEST_ASSERT_EQUAL_STRING(VALID_ID, out_customer.customer_id);
    TEST_ASSERT_EQUAL_STRING("YAMADA TAROU", out_customer.name);
    TEST_ASSERT_EQUAL_STRING("19900101", out_customer.birth_date);
    TEST_ASSERT_EQUAL_STRING("TOKYO-TO CHIYODA-KU 1-1-1", out_customer.address);
    TEST_ASSERT_EQUAL_STRING("0312345678", out_customer.phone_number);
    TEST_ASSERT_EQUAL_STRING("taro.yamada@example.com", out_customer.email);
    TEST_ASSERT_EQUAL_STRING("20240101120000", out_customer.inserted_at);
    TEST_ASSERT_EQUAL_STRING("20240102130000", out_customer.updated_at);
    TEST_ASSERT_EQUAL(1, out_customer.service_a);
    TEST_ASSERT_EQUAL(0, out_customer.service_b);
    TEST_ASSERT_EQUAL(1, out_customer.service_c);
}

/* TC-E01: 存在しない顧客ID */
void test_customer_get_nonexistent_id_should_return_not_found(void) {
    mock_db_layer_set_find_customer_result(DB_RESULT_NOT_FOUND);

    Customer out_customer;
    CustomerResult result = customer_get(VALID_ID, &out_customer);

    TEST_ASSERT_EQUAL(CUSTOMER_RESULT_NOT_FOUND, result);
    assert_error_result(&out_customer, CUSTOMER_RESULT_NOT_FOUND);
}

/* TC-E02: 形式不正（小文字混入） */
void test_customer_get_lowercase_included_should_return_invalid_format(void) {
    Customer out_customer;
    CustomerResult result = customer_get("aBCD1234EFGH5678IJKL9012", &out_customer);

    TEST_ASSERT_EQUAL(CUSTOMER_RESULT_INVALID_FORMAT, result);
    assert_error_result(&out_customer, CUSTOMER_RESULT_INVALID_FORMAT);
}

/* TC-E03: 形式不正（記号混入） */
void test_customer_get_symbol_included_should_return_invalid_format(void) {
    Customer out_customer;
    CustomerResult result = customer_get("-BCD1234EFGH5678IJKL9012", &out_customer);

    TEST_ASSERT_EQUAL(CUSTOMER_RESULT_INVALID_FORMAT, result);
    assert_error_result(&out_customer, CUSTOMER_RESULT_INVALID_FORMAT);
}

/* TC-E04: DB接続断 */
void test_customer_get_db_connection_error_should_return_db_connection_error(void) {
    mock_db_layer_set_find_customer_result(DB_RESULT_CONNECTION_ERROR);

    Customer out_customer;
    CustomerResult result = customer_get(VALID_ID, &out_customer);

    TEST_ASSERT_EQUAL(CUSTOMER_RESULT_DB_CONNECTION_ERROR, result);
    assert_error_result(&out_customer, CUSTOMER_RESULT_DB_CONNECTION_ERROR);
}

/* TC-E05: DB想定外エラー */
void test_customer_get_db_unexpected_error_should_return_db_error(void) {
    mock_db_layer_set_find_customer_result(DB_RESULT_DB_ERROR);

    Customer out_customer;
    CustomerResult result = customer_get(VALID_ID, &out_customer);

    TEST_ASSERT_EQUAL(CUSTOMER_RESULT_DB_ERROR, result);
    assert_error_result(&out_customer, CUSTOMER_RESULT_DB_ERROR);
}

/* TC-E06: 整合性異常 */
void test_customer_get_data_conflict_should_return_data_conflict(void) {
    mock_db_layer_set_find_customer_result(DB_RESULT_DATA_CONFLICT);

    Customer out_customer;
    CustomerResult result = customer_get(VALID_ID, &out_customer);

    TEST_ASSERT_EQUAL(CUSTOMER_RESULT_DATA_CONFLICT, result);
    assert_error_result(&out_customer, CUSTOMER_RESULT_DATA_CONFLICT);
}

/* TC-E07: out_customerがNULL（design.md 8.2 Step0） */
void test_customer_get_null_out_customer_should_return_invalid_argument(void) {
    CustomerResult result = customer_get(VALID_ID, NULL);

    TEST_ASSERT_EQUAL(CUSTOMER_RESULT_INVALID_ARGUMENT, result);
}

/* TC-B01: 23桁（1桁不足） */
void test_customer_get_23_digit_id_should_return_invalid_format(void) {
    Customer out_customer;
    CustomerResult result = customer_get("ABCD1234EFGH5678IJKL901", &out_customer);

    TEST_ASSERT_EQUAL(CUSTOMER_RESULT_INVALID_FORMAT, result);
    assert_error_result(&out_customer, CUSTOMER_RESULT_INVALID_FORMAT);
}

/* TC-B02: 25桁（1桁超過） */
void test_customer_get_25_digit_id_should_return_invalid_format(void) {
    Customer out_customer;
    CustomerResult result = customer_get("ABCD1234EFGH5678IJKL90123", &out_customer);

    TEST_ASSERT_EQUAL(CUSTOMER_RESULT_INVALID_FORMAT, result);
    assert_error_result(&out_customer, CUSTOMER_RESULT_INVALID_FORMAT);
}

/* TC-B03: 空文字 */
void test_customer_get_empty_string_id_should_return_invalid_format(void) {
    Customer out_customer;
    CustomerResult result = customer_get("", &out_customer);

    TEST_ASSERT_EQUAL(CUSTOMER_RESULT_INVALID_FORMAT, result);
    assert_error_result(&out_customer, CUSTOMER_RESULT_INVALID_FORMAT);
}

/* TC-B04: NULL入力 */
void test_customer_get_null_id_should_return_invalid_format(void) {
    Customer out_customer;
    CustomerResult result = customer_get(NULL, &out_customer);

    TEST_ASSERT_EQUAL(CUSTOMER_RESULT_INVALID_FORMAT, result);
    assert_error_result(&out_customer, CUSTOMER_RESULT_INVALID_FORMAT);
}

/* TC-B05: 極端に長い文字列（1000文字） */
void test_customer_get_extremely_long_id_should_return_invalid_format(void) {
    char long_id[1001];
    memset(long_id, 'A', sizeof(long_id) - 1);
    long_id[sizeof(long_id) - 1] = '\0';

    Customer out_customer;
    CustomerResult result = customer_get(long_id, &out_customer);

    TEST_ASSERT_EQUAL(CUSTOMER_RESULT_INVALID_FORMAT, result);
    assert_error_result(&out_customer, CUSTOMER_RESULT_INVALID_FORMAT);
}

/*
 * テストの実行エントリポイント。
 * UNITY_BEGIN() / UNITY_END() の間に RUN_TEST() でテスト関数を1つずつ登録して実行する。
 * UNITY_END() は失敗したテスト数を戻り値として返すため、
 * この main() の戻り値がそのままテスト成否（0 = 全て成功）になる。
 */
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_customer_get_valid_id_with_mixed_services_should_return_ok);
    RUN_TEST(test_customer_get_nonexistent_id_should_return_not_found);
    RUN_TEST(test_customer_get_lowercase_included_should_return_invalid_format);
    RUN_TEST(test_customer_get_symbol_included_should_return_invalid_format);
    RUN_TEST(test_customer_get_db_connection_error_should_return_db_connection_error);
    RUN_TEST(test_customer_get_db_unexpected_error_should_return_db_error);
    RUN_TEST(test_customer_get_data_conflict_should_return_data_conflict);
    RUN_TEST(test_customer_get_null_out_customer_should_return_invalid_argument);
    RUN_TEST(test_customer_get_23_digit_id_should_return_invalid_format);
    RUN_TEST(test_customer_get_25_digit_id_should_return_invalid_format);
    RUN_TEST(test_customer_get_empty_string_id_should_return_invalid_format);
    RUN_TEST(test_customer_get_null_id_should_return_invalid_format);
    RUN_TEST(test_customer_get_extremely_long_id_should_return_invalid_format);
    return UNITY_END();
}
