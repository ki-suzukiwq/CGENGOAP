# ディレクトリ構成

- `src/db_layer.c/.h` : DBアクセス層。本番用インターフェース実装。
  現状はJSON方式の暫定実装（Phase 4完了）。将来Oracle/Pro*C実装に差し替え予定（未着手）。
- `src/db_layer_internal.h` : `db_layer_load_from_path()` をテストから直接呼べるようにする非公開ヘッダ（`db_layer.h`には含めない）
- `src/customer.c/.h` : ビジネスロジック層。`db_layer.h` の関数を呼び出す。
- `src/main.c` : エントリポイント。
- `src/data/customers.json` : `db_layer.c`が読み込む疑似DBデータ（本番/開発ビルド用）。
- `src/vendor/cjson/` : ベンダリングしたcJSON本体（v1.7.19）。`test/unity`と同じ方式（サブモジュールではない）。**変更禁止**。
- `test/unity/` : Unity (ThrowTheSwitch) テストフレームワーク本体。GitHubから直接ソース配置。**変更禁止**。
- `test/mocks/mock_db_layer.c/.h` : `db_layer.h` のモック実装。
- `test/test_customer.c` : `customer.c` に対するUnit Testケース（`db_layer`はモックに差し替え）。全13ケース。
- `test/integration/test_db_layer.c` : `db_layer.c`（JSON方式）自体に対するIntegration Testケース（実ファイルI/O・cJSONパース・iconv変換を経由）。全6ケース。
- `test/integration/fixtures/db_layer/` : 上記テスト用フィクスチャJSON。

`src/vendor/` と `test/unity/` の編集は `.claude/settings.json` のフックでブロックされる。

## モック差し替え方式

サブモジュール／CMock自動生成は使わず、**リンク時差し替え方式**を採用。
`db_layer.h` は共通インターフェースとして `src/customer.c` から呼ばれるが、
実体（`.c`）は以下のようにビルド対象によって切り替える。

- 通常ビルド (`make`) : `src/db_layer.c`（本番実装＝JSON方式暫定実装）をリンク
- テストビルド (`make test`) : `test/mocks/mock_db_layer.c`（モック実装）をリンク

同一のヘッダ・同一のシンボル名を実装した2つの `.c` ファイルのうち、
リンクする側だけをMakefileのソースリストで切り替えることで、テスト時のみDBアクセスをモックに差し替える。
`db_layer.c`自体（JSON方式）を検証する `make test_db_layer` は、このリンク差し替えの対象外で
`db_layer_load_from_path()` を `db_layer_internal.h` 経由で直接呼び出す別系統のテスト。

詳細な呼び出し関係は `function_diagram.md` のmermaid図を参照。
