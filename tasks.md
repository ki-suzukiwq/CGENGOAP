# 顧客情報参照AP タスク一覧（Phase 3）

design.mdの内容を、変更内容単位のタスクに分解したもの。
各タスクの完了条件は「設計通りに更新されている（コンパイルが通る空実装）」こと。
実ロジック（入力バリデーション、DB連携本体、変換処理の中身等）の実装はPhase 4以降に分離し、
本フェーズでは対象としない。

| ID | 内容 | 対応design.mdセクション | ステータス |
|---|---|---|---|
| T1 | `db_layer.h`: 型定義追加（`DbResult`, `DbCustomerRecord`） | 1.2, 1.3 | 完了 |
| T2 | `db_layer.h`: 関数シグネチャ更新（`db_layer_save_customer`宣言を削除、`db_layer_init`/`db_layer_find_customer`の戻り値を`DbResult`に変更） | 2.1 | 完了 |
| T3 | `db_layer.c`: 実装更新（`db_layer_save_customer`実装を削除、新シグネチャに合わせた空実装に更新） | 2.1, 7 | 完了（中身は雛形のまま。実DB接続処理は要件定義上スコープ外のため未実装） |
| T4 | `customer.h`: 型定義追加（`CustomerResult`, `Customer`） | 1.1, 1.4 | 完了 |
| T5 | `customer.h`: 関数シグネチャ更新（`customer_register`宣言を削除、`customer_get`を確定） | 2.2 | 完了 |
| T6 | `customer.c`: 実装更新（`customer_register`実装を削除、`convert_db_result`の雛形追加、`customer_get`を新シグネチャの空実装に更新） | 2.2, 4, 7 | 完了（当初想定の空実装ではなく、design.md 8節の処理フロー通り本ロジックまで実装済み） |
| T7 | `mock_db_layer.h`: セッター関数・`reset`関数の宣言追加（`mock_db_layer_set_find_customer_result`, `mock_db_layer_set_find_customer_record`, `mock_db_layer_reset`） | 3.1 | 完了 |
| T8 | `mock_db_layer.c`: 内部状態変数（次回戻り値・レコード）追加、セッター関数実装、`db_layer_find_customer`モック実装を内部状態に基づく形に更新 | 3.1, 3.2, 7 | 完了 |
| T9 | `test_customer.c`: 13ケース分のテスト関数雛形を追加（`TEST_IGNORE`で保留、命名規則に従い`RUN_TEST`へ登録、`setUp`で`mock_db_layer_reset`を呼び出す） | 5, 6, 7 | 完了（`TEST_IGNORE`での保留ではなく、13ケース全て本実装まで完了） |
| T10 | `main.c`/`Makefile`: 型・シグネチャ変更に伴う影響確認（コンパイルエラーが出ないか確認し、必要なら修正） | 7 | 完了（`make`/`make test`双方のビルド成功で影響がないことを確認） |
| T11 | 全体ビルド確認（`make`でbuild/mainが生成される、`make test`でbuild/test_runnerが生成・実行され13ケースが表示されること） | （Phase 0ゴールの再確認） | 完了（`make`でbuild/main生成、`make test`で13 Tests 0 Failures 0 Ignoredを確認） |

# 顧客情報参照AP タスク一覧（Phase 4: db_layer JSON方式暫定実装）

`design_db_layer.md`の内容を、変更内容単位のタスクに分解したもの。
Oracle/Pro*C本実装への差し替えは対象外（別フェーズ）。

| ID | 内容 | 対応design_db_layer.mdセクション | ステータス |
|---|---|---|---|
| T12 | cJSONのベンダリング（`src/vendor/cjson/`にソース直接配置、v1.7.19） | 6 | 完了 |
| T13 | `db_layer.c`: JSON読み込み・パース・メモリキャッシュ・線形探索ロジックの実装（`db_layer_init`/`db_layer_find_customer`/`db_layer_close`/`db_layer_load_from_path`、`src/db_layer_internal.h`追加） | 4 | 完了 |
| T14 | `db_layer.c`: iconvによるUTF-8→SJIS変換の実装（`name`/`address`、バッファ超過時は`DB_RESULT_DB_ERROR`） | 4.1 | 完了 |
| T15 | `src/data/customers.json`（本番/開発ビルド用データ）の作成 | 2, 3 | 完了 |
| T16 | `test/test_db_layer.c`新規作成（TC-DB-N01, E01〜E05の6ケース、`test/fixtures/db_layer/`のフィクスチャを使用） | 5, 7 | 完了（`customers_overflow.json`フィクスチャも追加。2026-08-11: `test/integration/test_db_layer.c`・`test/integration/fixtures/db_layer/`へ移動済み。design_db_layer.md 9.4節参照） |
| T17 | `Makefile`: `make test_db_layer`ターゲット追加（既存`make test`は無改修、`-liconv`リンク追加） | 7 | 完了 |
| T18 | 全体ビルド確認（`make`/`make test`/`make test_db_layer`が全て成功すること） | （ゴールの再確認） | 完了（`make test`: 13 Tests 0 Failures、`make test_db_layer`: 6 Tests 0 Failures。`make`成果物での実データ読み込みもアドホック確認済み） |
| T19 | `CLAUDE.md`/`requirements.md`/`tasks.md`のドキュメント反映 | 8 | 完了 |
