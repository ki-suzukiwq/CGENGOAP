#ifndef MOCK_DB_LAYER_H
#define MOCK_DB_LAYER_H

/*
 * db_layer.h と同一シグネチャを満たすモック実装。
 * テストビルド時は src/db_layer.c の代わりにこちらをリンクする。
 */
#include "db_layer.h"

/*
 * 以下3つはテストコード専用の「セッター関数」。
 * 本番の db_layer.h には存在しない、モック実装だけが持つ追加のAPI。
 * テスト側から「次に db_layer_find_customer() が呼ばれたら何を返すか」を
 * あらかじめ設定しておくために使う。
 */

/**
 * @brief   次回の db_layer_find_customer() 呼び出しが返す DbResult を設定する
 * @param   result  次回呼び出し時に返したい結果コード（例: DB_RESULT_NOT_FOUND）
 */
void mock_db_layer_set_find_customer_result(DbResult result);

/**
 * @brief   次回の db_layer_find_customer() 呼び出しが out_record にコピーするレコードを設定する
 * @param   record  次回呼び出し時に返したい顧客データ
 */
void mock_db_layer_set_find_customer_record(const DbCustomerRecord *record);

/**
 * @brief   モックの内部状態を初期値に戻す（結果=DB_RESULT_OK、レコード=全項目ゼロクリア）
 *          各テストの実行前（Unityのsetup関数）で必ず呼び出し、テスト間で状態が
 *          引き継がれてしまう（前のテストの設定が残る）ことを防ぐ。
 */
void mock_db_layer_reset(void);

#endif /* MOCK_DB_LAYER_H */
