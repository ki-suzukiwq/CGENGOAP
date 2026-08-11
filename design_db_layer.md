# 顧客情報参照AP DB層 暫定実装（JSON方式）設計書（Phase 4）

## 0. 位置づけ

現状 `src/db_layer.c` は常に `DB_RESULT_OK` を返すだけの空実装（雛形）。
本設計は、**Oracle/Pro*C接続による本実装が入るまでの暫定実装**として、
ローカルJSONファイルを疑似DBとして読み込む方式で `db_layer.c` の中身を実装するもの。

- JSON方式はPro*C(SQL)実装とは**別物のロジック**（SQLを一切使わず、ファイルI/O+メモリ内検索）。
- 将来Oracle/Pro*C実装に差し替える際は、`db_layer.h`のインターフェース（関数シグネチャ）はそのまま流用できるが、
  本設計書に基づく内部実装（JSON読み込み・キャッシュ・検索ロジック）は丸ごと置き換える前提。
- CLAUDE.md・requirements.mdは「Oracle DB本体の実装は対象外」としているが、
  この暫定実装はOracleそのものには接続しないため、その前提と矛盾しない
  （別途、CLAUDE.md等に「暫定実装としてJSON方式のdb_layer.cが存在する」旨の追記が必要 — 8節参照）。

## 1. スコープ

### 対象
- `db_layer_init()` / `db_layer_close()` / `db_layer_find_customer()` の実ロジック実装（JSON方式）
- JSON読み込み・パース（cJSONベンダリング）
- 文字コード変換（UTF-8→SJIS、iconv使用）
- `db_layer_find_customer()` 自体の新規UT（`test/integration/test_db_layer.c`。9節参照）

### 対象外
- Oracle/Pro*C実装そのもの（引き続き別フェーズ）
- `customer_get()` 側の変更（`customer.c`は無改修。`db_layer.h`のインターフェースが変わらないため）
- 書き込み系（登録・更新）— requirements.md同様、参照のみがスコープ

## 2. ファイル配置

| パス | 役割 |
|---|---|
| `src/vendor/cjson/cJSON.c` `.h` | ベンダリングしたcJSON本体（test/unityと同じ方式：ソース直接配置、サブモジュールなし。v1.7.19） |
| `src/db_layer_internal.h` | `db_layer_load_from_path()`をテストから直接呼べるようにする非公開ヘッダ（`db_layer.h`には含めない） |
| `src/data/customers.json` | 本番/開発ビルド（`make`）が読み込む疑似DBデータ |
| `test/integration/test_db_layer.c` | `db_layer_find_customer()`自体（JSON実装）のUT本体。実ファイルI/O・cJSONパース・iconv変換を経由するためIntegration Test的に`test/integration/`配下に置く（9節参照） |
| `test/integration/fixtures/db_layer/customers_main.json` | `test_db_layer.c`用：OK / NOT_FOUND / DATA_CONFLICT検証用データ |
| `test/integration/fixtures/db_layer/customers_malformed.json` | `test_db_layer.c`用：DB_ERROR（パース失敗）検証用データ |
| `test/integration/fixtures/db_layer/customers_overflow.json` | `test_db_layer.c`用：文字コード変換後バッファ超過によるDB_ERROR検証用データ（4.1節） |
| （フィクスチャなし） | CONNECTION_ERROR検証用：存在しないパス（`customers_does_not_exist.json`、実ファイルは作らない）を直接指定 |

## 3. JSONスキーマ

`DbCustomerRecord`のフィールド名とJSONキー名を1対1対応させる。

```json
{
  "customer_id": "AAAA1111BBBB2222CCCC3333",
  "name": "山田太郎",
  "birth_date": "19900101",
  "address": "東京都千代田区1-1-1",
  "phone_number": "0312345678",
  "email": "taro.yamada@example.com",
  "inserted_at": "20240101120000",
  "updated_at": "20240102130000",
  "service_a": 1,
  "service_b": 0,
  "service_c": 1
}
```

- `name` / `address` はJSON内はUTF-8の日本語文字列で記述する（2.5節参照、読み込み時にSJISへ変換）
- `service_a/b/c` はJSON数値（0/1）。`DbCustomerRecord`の`int`にそのまま格納
- ルート要素は `{"_comment": "...", "customers": [ {...}, ... ]}` の形。
  `_comment` および各レコード内の `_test_case` はテストケースの説明用メタデータで、
  パース時はキー名で読むため無視される（cJSONの未知キーは単に読み飛ばす）

## 4. ロード戦略・インターフェース

- `db_layer.h` の関数シグネチャは変更しない（呼び出し側`customer.c`への影響ゼロを維持）
  ```c
  DbResult db_layer_init(void);
  void db_layer_close(void);
  DbResult db_layer_find_customer(const char *customer_id, DbCustomerRecord *out_record);
  ```
- JSONファイルパスは `db_layer.c` 内の `static const char *` 定数としてハードコードする
  （`CUSTOMER_DATA_JSON_PATH = "src/data/customers.json"`）
- 実際の読み込み処理は `DbResult db_layer_load_from_path(const char *json_path)` という、パスを引数に取る関数として実装し、
  `db_layer_init()` はこれを固定パスで呼ぶだけのラッパーにする。
  この関数は `db_layer_internal.h`（非公開ヘッダ）経由で`test_db_layer.c`から直接呼び出し、
  フィクスチャごとに異なるパスを指定してテストする（7節参照）。
- `db_layer_init()`:
  1. 上記パスのJSONファイルを読み込み、cJSONでパース
  2. パース失敗（ファイル内容が壊れている）→ `DB_RESULT_DB_ERROR` を返す
  3. ファイルが開けない（存在しない・権限なし）→ `DB_RESULT_CONNECTION_ERROR` を返す
  4. 成功時は各レコードを `DbCustomerRecord` の配列（静的にヒープ確保 or 固定長配列）としてメモリにキャッシュし、`DB_RESULT_OK` を返す
  5. 各レコードの`name`/`address`は、キャッシュに格納する際に`iconv`でUTF-8→SJISへ変換する（4.1節参照）
- `db_layer_find_customer()`: キャッシュ配列を先頭から線形探索し、
  - `customer_id`が一致するレコードが1件 → そのレコードをコピーして`DB_RESULT_OK`
  - 一致するレコードが0件 → `DB_RESULT_NOT_FOUND`
  - 一致するレコードが2件以上 → `DB_RESULT_DATA_CONFLICT`
- `db_layer_close()`: キャッシュ配列を解放

### 4.1 文字コード変換とバッファ超過時の扱い
- `iconv(from="UTF-8", to="SJIS//TRANSLIT" または "SJIS")` でJSON文字列を変換し、`DbCustomerRecord.name`（40バイト）・`address`（100バイト）に格納する
- 変換後のバイト長が確保サイズを超える場合は、そのレコードを**不正データとして扱い、そのレコードの読み込み時点で`DB_RESULT_DB_ERROR`を返す**（「DB側の想定外エラー」の定義に合わせたフォールバック。8.3節の`convert_db_result`のdefault分岐と同じ考え方）

## 5. エラーケースとDbResultの対応表

| ケース | 再現方法 | DbResult |
|---|---|---|
| 正常 | `customers_main.json`内の一意なID(`AAAA1111BBBB2222CCCC3333`)で検索 | `DB_RESULT_OK` |
| 該当なし | `customers_main.json`に存在しないID(`ZZZZ9999ZZZZ9999ZZZZ9999`)で検索 | `DB_RESULT_NOT_FOUND` |
| 整合性異常 | `customers_main.json`内の重複ID(`DDDD4444EEEE5555FFFF6666`、2件収録)で検索 | `DB_RESULT_DATA_CONFLICT` |
| 接続断 | 存在しないパス（例: `test/integration/fixtures/db_layer/customers_does_not_exist.json`）を指定して`db_layer_init()`相当の処理を実行 | `DB_RESULT_CONNECTION_ERROR` |
| DB想定外エラー | `customers_malformed.json`（意図的に構文を破損）を指定 | `DB_RESULT_DB_ERROR` |

## 6. cJSONベンダリング方針

- [cJSON (DaveGamble/cJSON)](https://github.com/DaveGamble/cJSON) の `cJSON.c` / `cJSON.h` を `src/vendor/cjson/` に直接配置
- `test/unity/` と同じ方式（GitHubから直接ソース配置、gitサブモジュールは使わない）
- Makefileのソースリスト・インクルードパスに `src/vendor/cjson` を追加

## 7. テスト方針

- 新規ファイル `test/integration/test_db_layer.c` を追加。**`db_layer_find_customer()`自体（JSON実装）が検証対象**（`customer.c`は経由しない）
- 既存 `test/test_customer.c`（`customer_get()`対象、mock使用）とは独立したテストバイナリとする
- Makefileに新規ターゲット `make test_db_layer`（`build/test_db_layer_runner`を生成・実行）を追加。既存`make test`は無改修
- Unit TestとIntegration Testの役割分担・ディレクトリ構成については9.3節で改めて整理する
- テストケース一覧（5節の対応表と対）

| TC ID | 内容 | 期待結果 |
|---|---|---|
| TC-DB-N01 | 正常取得 | `DB_RESULT_OK`、フィールド内容が`customers_main.json`の内容と一致 |
| TC-DB-E01 | 該当ID無し | `DB_RESULT_NOT_FOUND` |
| TC-DB-E02 | 整合性異常（重複ID） | `DB_RESULT_DATA_CONFLICT` |
| TC-DB-E03 | 接続断（ファイルが開けない） | `DB_RESULT_CONNECTION_ERROR` |
| TC-DB-E04 | DB想定外エラー（JSON破損） | `DB_RESULT_DB_ERROR` |
| TC-DB-E05 | 文字コード変換後バッファ超過 | `DB_RESULT_DB_ERROR`（4.1節） |

TC-DB-N01では、`name`/`address`がUTF-8→SJISへ正しく変換されていることを、
テスト側で逆方向（SJIS→UTF-8）にiconv変換し、元のUTF-8文字列と一致するかで検証する
（Cソース上に生のSJISバイト列を直接埋め込むのを避けるため）。

## 8. 既存ドキュメントへの反映・実装状況

- `CLAUDE.md` / `requirements.md` / `tasks.md`: 反映済み（tasks.md Phase 4, T19）
- 実装: 完了（2026-07-26）。`make` / `make test`（既存13ケース） / `make test_db_layer`（新規6ケース）いずれも成功を確認済み
  - `make`: `build/main`生成、`db_layer_init()`が`src/data/customers.json`を正しく読み込むことを別途アドホック検証済み
  - `make test_db_layer`: 6 Tests 0 Failures 0 Ignored（TC-DB-N01, E01〜E05）
- Oracle/Pro*C本実装への差し替えは引き続き別フェーズ（未着手）
- `test/test_db_layer.c`・`test/fixtures/db_layer/`は、9.3節の方針に基づき`test/integration/`配下へ移動済み
  （2026-08-11。パス変更のみで、テスト内容・件数に変更なし。`make test_db_layer`で6 Tests 0 Failuresを再確認済み）

## 9. 顧客状態（state_id）対応の拡張方針【requirements.md 10節 対応、試験導入】

### 9.0 位置づけ

- `requirements.md` 10節で追加された、顧客の契約・利用ライフサイクル状態（全16パターン、
  今回は代表5パターンS01〜S05）に応じた応答変動要件を受けて、JSON DB（本ファイルが対象とする
  `db_layer.c`のJSON方式暫定実装）側のスキーマ・実装方針を検討する。
- 状態の正式な16パターン定義（Excel状態遷移図由来のJSON）はまだ提供されていないため、
  本節も1〜8節と同様に**試験的な設計**であり、実データ提供後に差し替え・拡張する前提。
- 本節は設計レベルの記述のみを対象とし、`db_layer.c`・`customer.c`等の実コード変更、
  `src/data/customers.json`の実データ更新は行わない（別タスク）。

### 9.1 JSONスキーマ拡張案

3節のJSONスキーマに、状態を表す`state_id`フィールドを追加する。値は`requirements.md` 10.1節の
状態コード（`"S01"`〜`"S05"`）と同じ**文字列コード**とする。

```json
{
  "customer_id": "AAAA1111BBBB2222CCCC3333",
  "name": "山田太郎",
  "birth_date": "19900101",
  "address": "東京都千代田区1-1-1",
  "phone_number": "0312345678",
  "email": "taro.yamada@example.com",
  "inserted_at": "20240101120000",
  "updated_at": "20240102130000",
  "service_a": 1,
  "service_b": 0,
  "service_c": 1,
  "state_id": "S01"
}
```

- `state_id`は`DbCustomerRecord`（`db_layer.h`）に新規フィールド`char state_id[2 + 1]`
  （"S01"〜"S99"を想定した2桁+終端）として追加する想定（未実装、別タスク）。
- 状態名・状態ごとのマスク対象フィールド等の**振る舞いの定義**まで`customers.json`側に持たせるか
  （データ駆動）、`customer.c`側にハードコードするか（コード駆動）は9.4節「未確定事項」を参照。
  本節では、まずは「状態コード自体をJSONデータとして持つ」という最小限の拡張のみを設計対象とする。

### 9.2 cJSON導入方針

cJSONは既に`src/vendor/cjson/`にベンダリング済みであり（6節参照）、Makefile
（`CJSON_DIR`、`APP_SRCS`、`TEST_DB_LAYER_SRCS`）・GitHub Actions（`.github/workflows/build.yml`、
`make`/`make test`/`make test_db_layer`いずれも成功確認済み）ともに対応済みである。

`state_id`フィールドの追加は、既存レコードに新しいJSONキーが1つ増えるだけであり、
cJSONの読み込みロジック（キー名で値を取得する既存の方式）をそのまま流用できる。
そのため、**cJSONライブラリ自体の新規導入・Actions側への新しいインストールステップの追加は不要**。

### 9.3 db_layer.c実装方針・層分離の健全性チェック

- `db_layer.h`の**関数シグネチャ**（`db_layer_init()` / `db_layer_close()` / `db_layer_find_customer()`）は
  変更しない。4節の設計方針（「JSONファイルパスは固定、読み込み処理は`db_layer_load_from_path()`」）も
  そのまま踏襲する。`state_id`はJSON読み込み時に`DbCustomerRecord`へマッピングする1フィールドが増えるだけで、
  読み込み・線形探索のロジック自体（4節）に構造的な変更は生じない。
- ただし、**「customer.c側のロジックには一切変更が不要」という主張は、正確には成立しない**点を明記する
  （層分離の健全性チェック結果）。
  - **変更不要な部分**: `db_layer.h`の関数シグネチャ、リンク時差し替えの仕組み（`make`/`make test`での
    本番実装/モック切り替え）、`customer.c`から`db_layer_find_customer()`を呼び出す既存コード自体。
  - **変更が必要になる見込みの部分**: `DbCustomerRecord`（`db_layer.h`が定義する構造体）へ`state_id`
    フィールドを追加する必要がある。また、`requirements.md` REQ-S01〜S05の振る舞い
    （フィールドのマスク、状態別のエラーコード返却）を実現するには、`customer.c`の`customer_get()`が
    `db_record.state_id`を読み取り、状態に応じて応答を分岐させるロジックを追加する必要がある
    （design.md側の詳細は同ファイル参照）。
  - つまり、**「関数呼び出しのインターフェース」は不変だが、「インターフェースが運ぶデータの形（構造体）」と
    「呼び出し元のビジネスロジック」は状態対応のために拡張が必要**、というのが正確な整理である。
    層分離の設計自体（DB層とビジネスロジック層の責務分割）は健全に保たれており、
    大規模な作り直しは不要という点は担保されている。

### 9.4 テスト方針の整理（Unit Test / Integration Test）

既存のMock（`test/mocks/mock_db_layer.c`）と、JSON DBを使ったテスト（`test/integration/test_db_layer.c`）は、
検証対象・検証レイヤーが異なるため、両方を維持する。

| | Unit Test | Integration Test |
|---|---|---|
| ファイル | `test/test_customer.c` | `test/integration/test_db_layer.c` |
| 検証対象 | `customer.c`の`customer_get()`（ビジネスロジック） | `db_layer.c`の`db_layer_find_customer()`等（JSON方式実装） |
| DBアクセス | `test/mocks/mock_db_layer.c`に差し替え（実I/Oなし） | 実際のJSONファイル（`test/integration/fixtures/db_layer/`）をファイルI/Oで読み込む |
| 検証の目的 | 入力バリデーション・エラーコード変換・状態に応じた分岐など、ビジネスロジックが正しいか（I/Oの実装詳細から独立して検証） | JSON読み込み・パース・cJSON連携・iconv文字コード変換・線形探索など、実装の詳細（I/O・外部ライブラリ連携）が正しいか |
| 実行速度・安定性 | 高速・決定的（外部要因に依存しない） | ファイルI/O・文字コード変換を伴うため、Unit Testよりわずかに重い。ただし外部ネットワーク等には依存しないためCIでの安定性は保たれる |
| ディレクトリ | `test/`直下（`test/mocks/`にモック実装） | `test/integration/`（テスト本体・フィクスチャとも） |

- ディレクトリ構成: `test/integration/`を新設し、`test_db_layer.c`本体と、そのフィクスチャ
  （`test/integration/fixtures/db_layer/`）をまとめて配置する（2026-08-11実施）。
  `test/mocks/`・`test/test_customer.c`（Unit Test側）は既存の配置のまま変更しない。
- 状態（`state_id`）関連の新規テストケースも、DB層自体を対象とする限りは
  `test/integration/test_db_layer.c`に追加する想定（未実装、別タスク）。
  `customer_get()`側で状態に応じた分岐ロジックを追加した場合のテストは、
  引き続き`test/test_customer.c`（Unit Test、mock使用）に追加する。

### 9.5 未確定事項（実装フェーズ着手前に確定が必要）

- `DbCustomerRecord`・`Customer`構造体への`state_id`（または対応するフィールド）追加の具体的な型・サイズ
- 状態ごとの振る舞い（マスク対象フィールド、エラーコード）を、JSONデータ側に持たせるか
  （例: 状態マスタをJSONで別途定義）、`customer.c`側にハードコードするかの方針
- `requirements.md` 10.4節に記載の未確定事項（`CUSTOMER_RESULT_CANCELLED`等の仮称エラーコードの正式化等）
- `src/data/customers.json`（本番/開発ビルド用データ）・`test/integration/fixtures/db_layer/`配下の
  各フィクスチャへの`state_id`実データ追加
