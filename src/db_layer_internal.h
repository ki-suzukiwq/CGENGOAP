#ifndef DB_LAYER_INTERNAL_H
#define DB_LAYER_INTERNAL_H

/*
 * db_layer.c（JSON方式暫定実装）の内部実装をテストから直接呼び出すための非公開ヘッダ。
 * 本番コード（customer.c等）はdb_layer.hのみを使い、これは使わない。
 * design_db_layer.md 4節参照。
 */
#include "db_layer.h"

/**
 * @brief   指定したパスのJSONファイルを読み込み、内部キャッシュに格納する
 *          （`db_layer_init()`は本関数を固定パスで呼び出すラッパー）。
 *          テストコードから任意のフィクスチャパスを指定して呼び出すために公開している。
 * @param   json_path  読み込むJSONファイルのパス
 * @return  DB_RESULT_OK ほか、上記DbResultのエラーコード
 */
DbResult db_layer_load_from_path(const char *json_path);

#endif /* DB_LAYER_INTERNAL_H */
