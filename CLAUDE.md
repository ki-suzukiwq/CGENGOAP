# customer_app

顧客管理機能のC言語プロジェクト。顧客情報**参照専用**のバックエンドAP。
本番DBはOracleを想定するが、`db_layer.c`は暫定的にローカルJSON方式で実装済み（詳細は下記参照）。

## 作業前に読むファイル（タスクに応じて必要な分だけ）

このCLAUDE.mdは毎回自動で読み込まれるため要点のみに絞っている。詳細は以下を**該当するものだけ**読むこと。
関係ない設計書・節まで全部読み込まない（トークンの無駄遣いを避ける）。

| 知りたいこと | 読むファイル |
|---|---|
| プロジェクトのスコープ・DB実装の位置づけ | `.claude/steering/product.md` |
| ディレクトリ構成・モック差し替え方式 | `.claude/steering/structure.md` |
| ビルド/テストコマンド・前提ツール | `.claude/steering/tech.md` |
| コーディング規約・命名規則 | `.claude/steering/conventions.md` |
| 入出力仕様・エラーコード定義（要件） | `requirements.md`（Phase 1） |
| `customer_get()`のビジネスロジック設計 | `design.md`（Phase 2） |
| `db_layer.c`のJSON方式暫定実装の設計 | `design_db_layer.md`（Phase 4） |
| 関数の呼び出し関係 | `function_diagram.md` |
| 実装タスクの一覧・進捗 | `tasks.md` |

## 常に守るルール

1. `test/unity/` と `src/vendor/` はベンダリングされた外部ライブラリ（Unity, cJSON）。**変更禁止**
   （`.claude/settings.json`のフックでEdit/Write/NotebookEditをブロック済み。変更が必要な場合は必ずユーザーに確認する）。
2. Oracle DB本体・Pro*C実装は対象外。`db_layer.h`のインターフェース（関数シグネチャ）を変えない限り、
   `db_layer.c`の内部実装（現状JSON方式）は変更してよい。
3. 登録・更新機能、結合テスト・E2Eテスト、非機能要件（性能・可用性等）は対象外。参照UTのみがスコープ。
4. モック差し替えはMakefileのソースリスト切り替えによるリンク時差し替え方式。CMock自動生成やサブモジュールは使わない。

## ビルド/テスト クイックリファレンス
- `make` : `build/main` を生成
- `make test` : `customer_get()`のUT（mock使用、13ケース）
- `make test_db_layer` : `db_layer.c`（JSON方式）自体のUT（6ケース）
- `make clean` : `build/` を削除

スラッシュコマンド `/build` `/test` でも実行可能（`.claude/commands/`）。

## .claude/ 構成
- `.claude/settings.json` : make系コマンドの許可リスト、ベンダリングコード編集ブロックのフック
- `.claude/steering/` : このプロジェクトの構造化ノート（オンデマンド参照、常時ロードしない）
- `.claude/commands/` : スラッシュコマンド（`/build`, `/test`）
