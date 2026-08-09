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

## Git運用ルール

### ブランチ運用
- mainブランチへの直接コミット・pushは禁止（branch protectionにより技術的にもブロックされる）
- 新しい作業は必ずfeatureブランチを作成してから行う
  - 命名規則: `feature/{作業内容の英語短縮}`（新機能・改修）
  - 命名規則: `fix/{修正内容の英語短縮}`（バグ修正・事故対応）

### 作業開始前の手順（必須）
新しい作業を始める前は、必ず以下を実行してmainを最新化してから
featureブランチを作成すること。

```
git checkout main
git pull origin main
git checkout -b feature/{作業内容}
```

これを怠ると、featureブランチがmainから乖離し、PRマージ時に
「Require branches to be up to date before merging」の判定で
待たされる、または追加のupdate作業が必要になる。

### コミット・PR運用
- 1タスク（変更内容単位）ごとに1コミットを意識する
- 作業が完了したらpushし、Pull Requestを作成する
  - gh CLIが使える場合は `gh pr create` を使用する
- PR作成後は完了を報告し、マージの実行は必ずユーザーの確認を
  待つこと（Claude Code側で勝手にマージしない）

### マージ後の後片付け（必須）
PRがマージされたら、以下を実行してローカル環境をクリーンな
状態に保つこと。

```
git checkout main
git pull origin main
git branch -d {マージ済みのfeatureブランチ名}
git fetch --prune
```

「git branch」の結果、mainのみが残っている状態を基本とする。

### 禁止事項
- force pushは使用しない
- ユーザーの確認なしに、branch protectionの設定変更や
  リポジトリの公開設定（visibility）を変更しない
- ビルド成果物（build/配下）や `.claude/settings.local.json` を
  コミットしない（.gitignoreで除外済みのはずだが、念のため
  コミット前に `git status` で確認すること）
