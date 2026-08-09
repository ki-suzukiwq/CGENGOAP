#include "mock_db_layer.h"
#include <stddef.h>
#include <string.h>

/*
 * static変数は「このファイルの中だけで有効なグローバル変数」。
 * 関数を抜けても値が消えず、次にこのファイル内の関数が呼ばれたときも値を覚えている。
 * ここでは「次に db_layer_find_customer() が呼ばれたときに何を返すか」を
 * 一時的に覚えておくための箱として使っている。
 */
static DbResult next_find_customer_result = DB_RESULT_OK;   /* 次回の戻り値 */
static DbCustomerRecord next_find_customer_record;           /* 次回のout_recordの中身 */

/* テストコードから呼ばれ、上のstatic変数に値をセットするだけの単純な関数 */
void mock_db_layer_set_find_customer_result(DbResult result) {
    next_find_customer_result = result;
}

void mock_db_layer_set_find_customer_record(const DbCustomerRecord *record) {
    /* 構造体はそのまま代入すると中身が丸ごとコピーされる（ポインタのコピーではない） */
    next_find_customer_record = *record;
}

/* テストの setUp() から毎回呼ばれ、状態を初期値に戻す */
void mock_db_layer_reset(void) {
    next_find_customer_result = DB_RESULT_OK;
    /* memsetで構造体の全バイトを0にする＝全ての文字列フィールドが空文字列、数値フィールドが0になる */
    memset(&next_find_customer_record, 0, sizeof(next_find_customer_record));
}

/*
 * ここから下は db_layer.h と同じシグネチャを持つモック実装本体。
 * Makefileの設定により、テストビルド（make test）のときだけ
 * 本番実装（src/db_layer.c）の代わりにこちらがリンクされる。
 */

DbResult db_layer_init(void) {
    return DB_RESULT_OK;
}

void db_layer_close(void) {
}

DbResult db_layer_find_customer(const char *customer_id, DbCustomerRecord *out_record) {
    (void)customer_id;
    /* 実際のDB検索は行わず、テスト側が事前にセットしておいた値をそのまま返すだけ */
    *out_record = next_find_customer_record;
    return next_find_customer_result;
}
