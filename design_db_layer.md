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
- `db_layer_find_customer()` 自体の新規UT（`test/test_db_layer.c`）

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
| `test/fixtures/db_layer/customers_main.json` | `test_db_layer.c`用：OK / NOT_FOUND / DATA_CONFLICT検証用データ |
| `test/fixtures/db_layer/customers_malformed.json` | `test_db_layer.c`用：DB_ERROR（パース失敗）検証用データ |
| `test/fixtures/db_layer/customers_overflow.json` | `test_db_layer.c`用：文字コード変換後バッファ超過によるDB_ERROR検証用データ（4.1節） |
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
| 接続断 | 存在しないパス（例: `test/fixtures/db_layer/customers_does_not_exist.json`）を指定して`db_layer_init()`相当の処理を実行 | `DB_RESULT_CONNECTION_ERROR` |
| DB想定外エラー | `customers_malformed.json`（意図的に構文を破損）を指定 | `DB_RESULT_DB_ERROR` |

## 6. cJSONベンダリング方針

- [cJSON (DaveGamble/cJSON)](https://github.com/DaveGamble/cJSON) の `cJSON.c` / `cJSON.h` を `src/vendor/cjson/` に直接配置
- `test/unity/` と同じ方式（GitHubから直接ソース配置、gitサブモジュールは使わない）
- Makefileのソースリスト・インクルードパスに `src/vendor/cjson` を追加

## 7. テスト方針

- 新規ファイル `test/test_db_layer.c` を追加。**`db_layer_find_customer()`自体（JSON実装）が検証対象**（`customer.c`は経由しない）
- 既存 `test/test_customer.c`（`customer_get()`対象、mock使用）とは独立したテストバイナリとする
- Makefileに新規ターゲット `make test_db_layer`（`build/test_db_layer_runner`を生成・実行）を追加。既存`make test`は無改修
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
