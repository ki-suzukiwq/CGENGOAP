# 顧客情報参照AP 設計書（Phase 2）

## 1. データ構造

### 1.1 CustomerResult（ビジネスロジック層エラーコード）
```c
typedef enum {
    CUSTOMER_RESULT_OK = 0,
    CUSTOMER_RESULT_INVALID_FORMAT,
    CUSTOMER_RESULT_NOT_FOUND,
    CUSTOMER_RESULT_DB_CONNECTION_ERROR,
    CUSTOMER_RESULT_DB_ERROR,
    CUSTOMER_RESULT_DATA_CONFLICT,
    CUSTOMER_RESULT_INVALID_ARGUMENT
} CustomerResult;
```
※`CUSTOMER_RESULT_INVALID_ARGUMENT`は8節（処理フロー）検討時に追加。`out_customer`がNULLの場合に返す。
現時点の`src/customer.h`にはまだ反映されていない（コード反映は別タスク）。

### 1.2 DbResult（DB層エラーコード）
```c
typedef enum {
    DB_RESULT_OK = 0,
    DB_RESULT_NOT_FOUND,
    DB_RESULT_CONNECTION_ERROR,
    DB_RESULT_DB_ERROR,
    DB_RESULT_DATA_CONFLICT
} DbResult;
```

### 1.3 DbCustomerRecord（DB層構造体）
```c
typedef struct {
    char customer_id[24 + 1];
    char name[40 + 1];
    char birth_date[8 + 1];
    char address[100 + 1];
    char phone_number[15 + 1];
    char email[256 + 1];
    char inserted_at[14 + 1];
    char updated_at[14 + 1];
    int service_a;
    int service_b;
    int service_c;
} DbCustomerRecord;
```

### 1.4 Customer（ビジネスロジック層構造体、応答用）
```c
typedef struct {
    char customer_id[24 + 1];
    char name[40 + 1];
    char birth_date[8 + 1];
    char address[100 + 1];
    char phone_number[15 + 1];
    char email[256 + 1];
    char inserted_at[14 + 1];
    char updated_at[14 + 1];
    int service_a;
    int service_b;
    int service_c;
    CustomerResult result;
} Customer;
```

## 2. インターフェース定義

### 2.1 db_layer.h
```c
DbResult db_layer_init(void);
void db_layer_close(void);
DbResult db_layer_find_customer(const char *customer_id, DbCustomerRecord *out_record);
```
※`db_layer_save_customer`はスコープ外のため削除

### 2.2 customer.h
```c
CustomerResult customer_get(const char *customer_id, Customer *out_customer);
```
※`customer_register`はスコープ外のため削除

## 3. モック設計

### 3.1 mock_db_layer.h（セッター関数）
```c
void mock_db_layer_set_find_customer_result(DbResult result);
void mock_db_layer_set_find_customer_record(const DbCustomerRecord *record);
void mock_db_layer_reset(void);
```

### 3.2 内部状態管理方針
- `mock_db_layer.c`内の`static`変数で次回呼び出し時の戻り値・レコードを保持
- 各テストの`setUp`で`mock_db_layer_reset()`を呼び、`DB_RESULT_OK`・ゼロクリアされたレコードに初期化

## 4. DbResult → CustomerResult 変換方針
`customer.c`内にstatic変換関数を用意し、switch文で1対1マッピングする。

```c
static CustomerResult convert_db_result(DbResult db_result);
```

## 5. テストケース一覧（全13件）

### 正常系
| TC ID | 内容 | 期待結果 |
|---|---|---|
| TC-N01 | 正常取得（サービスA/B/C混在） | `CUSTOMER_RESULT_OK` |

### 異常系
| TC ID | 内容 | 期待結果 |
|---|---|---|
| TC-E01 | 存在しない顧客ID | `CUSTOMER_RESULT_NOT_FOUND` |
| TC-E02 | 形式不正（小文字混入） | `CUSTOMER_RESULT_INVALID_FORMAT` |
| TC-E03 | 形式不正（記号混入） | `CUSTOMER_RESULT_INVALID_FORMAT` |
| TC-E04 | DB接続断 | `CUSTOMER_RESULT_DB_CONNECTION_ERROR` |
| TC-E05 | DB想定外エラー | `CUSTOMER_RESULT_DB_ERROR` |
| TC-E06 | 整合性異常 | `CUSTOMER_RESULT_DATA_CONFLICT` |
| TC-E07 | `out_customer`がNULL | `CUSTOMER_RESULT_INVALID_ARGUMENT` |

### 境界値
| TC ID | 内容 | 期待結果 |
|---|---|---|
| TC-B01 | 23桁（1桁不足） | `CUSTOMER_RESULT_INVALID_FORMAT` |
| TC-B02 | 25桁（1桁超過） | `CUSTOMER_RESULT_INVALID_FORMAT` |
| TC-B03 | 空文字 | `CUSTOMER_RESULT_INVALID_FORMAT` |
| TC-B04 | NULL入力 | `CUSTOMER_RESULT_INVALID_FORMAT` |
| TC-B05 | 極端に長い文字列（1000文字） | `CUSTOMER_RESULT_INVALID_FORMAT` |

TC-E07を除く全ケースにおいて、エラー時は`Customer`構造体の他フィールドがゼロクリアされていることも検証する。
TC-E07は`out_customer`自体がNULLのため、書き込み内容の検証は行わず、戻り値のみを検証する。

## 6. テストファイル構成・命名規則
- `test/test_customer.c`1ファイルに全13ケースをまとめる
- 命名規則: `test_customer_get_<状況>_should_<期待結果>`

## 7. 既存雛形の改修方針
- `src/db_layer.h/.c`: `db_layer_save_customer`削除、`DbResult`/`DbCustomerRecord`を更新、`db_layer_find_customer`は雛形のまま（中身はPhase 3で実装）
- `src/customer.h/.c`: `customer_register`削除、`CustomerResult`/`Customer`を更新、`customer_get`は8節の処理フロー通りに実装済み
- `test/mocks/mock_db_layer.h/.c`: セッター関数・内部状態管理を追加（雛形段階では最低限の実装）
- `test/test_customer.c`: 13個のテスト関数（TC-N01, TC-E01〜E07, TC-B01〜B05）を8節の処理フローに基づき実装済み
- `Makefile`: 既存のビルドルールを流用（変更なし見込み）

※TC-E07（`out_customer`がNULL）は`src/customer.h`への`CUSTOMER_RESULT_INVALID_ARGUMENT`追加、
`test/test_customer.c`へのテスト関数追加ともに反映済み（`make test`で13 Tests 0 Failures 0 Ignoredを確認）。

## 8. 処理フロー

### 8.1 convert_db_result()

`DbResult`の値ごとに、`CustomerResult`への変換を1対1で対応させる（switch文）。

| DbResult | CustomerResult | 根拠 |
|---|---|---|
| `DB_RESULT_OK` | `CUSTOMER_RESULT_OK` | requirements.md 3.2表 |
| `DB_RESULT_NOT_FOUND` | `CUSTOMER_RESULT_NOT_FOUND` | requirements.md 3.2表・4節-1 |
| `DB_RESULT_CONNECTION_ERROR` | `CUSTOMER_RESULT_DB_CONNECTION_ERROR` | requirements.md 3.2表・4節-4 |
| `DB_RESULT_DB_ERROR` | `CUSTOMER_RESULT_DB_ERROR` | requirements.md 3.2表・4節-5 |
| `DB_RESULT_DATA_CONFLICT` | `CUSTOMER_RESULT_DATA_CONFLICT` | requirements.md 3.2表・4節-6 |
| （上記5値以外の想定外の値） | `CUSTOMER_RESULT_DB_ERROR`（switchのdefault） | 「DB側の想定外エラー」の定義に合わせたフォールバック（要件定義には分岐そのものの明記なし、8.3節参照） |

擬似コード:
```c
static CustomerResult convert_db_result(DbResult db_result) {
    switch (db_result) {
        case DB_RESULT_OK:                return CUSTOMER_RESULT_OK;
        case DB_RESULT_NOT_FOUND:         return CUSTOMER_RESULT_NOT_FOUND;
        case DB_RESULT_CONNECTION_ERROR:  return CUSTOMER_RESULT_DB_CONNECTION_ERROR;
        case DB_RESULT_DB_ERROR:          return CUSTOMER_RESULT_DB_ERROR;
        case DB_RESULT_DATA_CONFLICT:     return CUSTOMER_RESULT_DATA_CONFLICT;
        default:                          return CUSTOMER_RESULT_DB_ERROR;
    }
}
```

### 8.2 customer_get()

以下の順序で処理する。早期return条件に1つでも合致したら、以降のステップは実行しない。

#### Step 0: out_customerのNULLチェック
- 条件: `out_customer == NULL`
- 挙動: `CUSTOMER_RESULT_INVALID_ARGUMENT`を返してreturn。
  `out_customer`への書き込みは一切行わない（書き込み先が存在しないため）。
- 根拠: requirements.md 2.2・3.2・4節-7

#### Step 1: customer_idの入力バリデーション
以降の3チェックは全て「NG時は`out_customer`をゼロクリアした上で
`result = CUSTOMER_RESULT_INVALID_FORMAT`を設定し、returnする」という共通の挙動を取る。

1. NULLチェック
   - 条件式: `customer_id == NULL`
   - 根拠: requirements.md 5節-4
2. 桁数チェック
   - 条件式: `strnlen(customer_id, 25) != 24`
   - 25バイトを上限に走査することで、極端に長い文字列や万一NUL終端されていない
     不正な入力に対しても無制限に走査しない（ユーザー確認済み方針）
   - 空文字（長さ0）・23桁・25桁・1000文字のいずれもこの条件式1本で検出する
   - 根拠: requirements.md 2.1「桁数: 固定長24桁」、5節-1, 2, 3, 5
3. 文字種チェック
   - 条件式: 24文字全てについて `isupper((unsigned char)c) || isdigit((unsigned char)c)` を満たすこと。
     1文字でも満たさない文字があれば不正（小文字・記号・スペース等）。
   - 根拠: requirements.md 2.1「文字種: 英数字」「英字の大小: 大文字のみ許容」、4節-2

#### Step 2: db_layer_find_customer()の呼び出し
```c
DbCustomerRecord db_record;
DbResult db_result = db_layer_find_customer(customer_id, &db_record);
```

#### Step 3: convert_db_result()による変換
```c
CustomerResult customer_result = convert_db_result(db_result);
```

#### Step 4a: 異常時（customer_result != CUSTOMER_RESULT_OK）
- `out_customer`をゼロクリアした上で`result = customer_result`を設定し、return。
- 根拠: requirements.md 3.2「エラー発生時、応答構造体の他フィールドはゼロクリアする」

#### Step 4b: 正常時（customer_result == CUSTOMER_RESULT_OK）のフィールドコピー

| DbCustomerRecordのフィールド | Customerのフィールド | コピー方法 |
|---|---|---|
| `customer_id` | `customer_id` | memcpy（同一サイズの固定長配列） |
| `name` | `name` | memcpy |
| `birth_date` | `birth_date` | memcpy |
| `address` | `address` | memcpy |
| `phone_number` | `phone_number` | memcpy |
| `email` | `email` | memcpy |
| `inserted_at` | `inserted_at` | memcpy |
| `updated_at` | `updated_at` | memcpy |
| `service_a` | `service_a` | 直接代入 |
| `service_b` | `service_b` | 直接代入 |
| `service_c` | `service_c` | 直接代入 |
| （該当なし） | `result` | `CUSTOMER_RESULT_OK`を設定 |

- 根拠: requirements.md 3.1（「正常終了時は…欠落・誤マッピングなく反映されていることをUTで検証する」の追記を含む）
- 全項目コピー後、`result = CUSTOMER_RESULT_OK`を設定してreturn

### 8.3 整合性確認: test_customer.cとの対応

| 処理フロー上の分岐 | 対応するTC | 状態 |
|---|---|---|
| Step 0: `out_customer == NULL` | TC-E07 | 対応あり |
| Step 1-1: `customer_id == NULL` | TC-B04 | 対応あり |
| Step 1-2: 桁数不正（空文字/23桁/25桁/1000文字） | TC-B01, TC-B02, TC-B03, TC-B05 | 対応あり |
| Step 1-3: 文字種不正（小文字/記号） | TC-E02, TC-E03 | 対応あり |
| Step 3: `DB_RESULT_NOT_FOUND` | TC-E01 | 対応あり |
| Step 3: `DB_RESULT_CONNECTION_ERROR` | TC-E04 | 対応あり |
| Step 3: `DB_RESULT_DB_ERROR` | TC-E05 | 対応あり |
| Step 3: `DB_RESULT_DATA_CONFLICT` | TC-E06 | 対応あり |
| Step 3: 想定外`DbResult`値→default | なし | **意図的にテスト対象外**（下記参照） |
| Step 4b: 正常系フィールドコピー | TC-N01 | 対応あり |

#### convert_db_result()のdefault分岐について
`DbResult`は現状5つの値しか正規には取り得ないため、この分岐はrequirements.mdが
列挙する異常系（4節）のいずれにも直接対応しない、実装上の防御的分岐という位置付けになる。
ユーザー確認の結果、UTのテスト対象外として割り切る方針とした
（テストする場合は`mock_db_layer_set_find_customer_result((DbResult)不正な整数値)`のように
enumの範囲外の値を強制的にキャストして渡す必要があるが、今回は行わない）。

#### 逆方向の確認
`test_customer.c`の13ケース（TC-N01, TC-E01〜E07, TC-B01〜B05）は全て上表のいずれかの分岐に
対応しており、処理フローに存在しない条件を検証しているケースはない。
（`make test`で13 Tests 0 Failures 0 Ignoredを確認済み）
