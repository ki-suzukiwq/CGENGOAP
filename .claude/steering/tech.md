# 技術スタック・ビルド/テスト

## 前提ツール
- コンパイラ: clang (Xcode Command Line Tools)
- テストフレームワーク: Unity (ThrowTheSwitch)、`test/unity/`にベンダリング済み
- JSONパース: cJSON、`src/vendor/cjson/`にベンダリング済み（追加インストール不要）
- 文字コード変換: iconv（OS標準ライブラリ、追加インストール不要。UTF-8→SJIS変換に使用）
- 実DBライブラリのインストールは不要（DB層はJSON方式の暫定実装、または本番実装が入るまではモックのみで完結）

## ビルド/テストコマンド
- `make` : `build/main` を生成（本番実装＝`src/db_layer.c`をリンク）
- `make test` : `build/test_runner` を生成して実行（`customer_get()`のUT、mock使用、13ケース）
- `make test_db_layer` : `build/test_db_layer_runner` を生成して実行（`db_layer.c`自体のJSON方式実装のUT、6ケース）
- `make clean` : `build/` を削除

これらのコマンドは `.claude/settings.json` の permissions.allow で許可済み（確認プロンプトなしで実行可）。

## 現状の実装状況
- `customer_get()` / `convert_db_result()`: 実装済み（design.md 8節の処理フロー通り）
- `db_layer.c`（JSON方式暫定実装）: 実装済み（Phase 4完了、`make`/`make test`/`make test_db_layer`いずれも成功確認済み）
- Oracle/Pro*C本実装: 未着手（要件定義上、引き続き対象外）
