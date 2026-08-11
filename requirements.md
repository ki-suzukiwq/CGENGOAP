# 顧客情報参照AP Requirements定義書（Phase 1）

## 1. 対象範囲
- 対象: 顧客情報参照AP（バックエンドAP）のみ
- 対象外: ポータル系CP、ミドルウェアCP、Oracle DB本体の実装
  - ※Oracle/Pro*C実装が入るまでの暫定実装として、`db_layer.c`をローカルJSONファイル読み込み方式で
    実装する計画がある（`design_db_layer.md`参照）。これはSQLを使わない別ロジックであり、
    「Oracle DB本体の実装」を代替するものではなく、上記の対象外方針とは矛盾しない。
- 機能範囲: 顧客情報の参照機能のみ（登録・更新機能は対象外）
- データモデル: 顧客IDによる単一引き。加えて、顧客の契約・利用ライフサイクル状態（全16パターンを想定）に応じて
  応答項目が変動する要件を追加する（10節参照。JSON DB導入検証のため、今回は代表5パターンのみを対象とした試験的な要件化）
- 非機能要件（性能・可用性等）: 対象外
- テストレベル: UT（関数のバリエーションテスト）のみ。結合テスト・E2Eテストは対象外

**REQ-U01（普遍的要件）**:
THE SYSTEM SHALL 顧客情報の参照（顧客IDによる単一引き）機能のみを提供する（登録・更新機能は提供しない）

## 2. 入力仕様

### 2.1 顧客ID
| 項目 | 内容 |
|---|---|
| 文字種 | 英数字 |
| 桁数 | 固定長24桁 |
| 英字の大小 | 大文字のみ許容（小文字は形式不正扱い） |

### 2.2 呼び出しインターフェース
- 入力パラメータは顧客IDのみ
- 関数シグネチャ（案）:
```c
int customer_get(const char *customer_id, Customer *out_customer);
```
- `out_customer`（応答格納先）はNULL不可。NULLで呼び出された場合は`CUSTOMER_RESULT_INVALID_ARGUMENT`を返す（3.2節参照）。
  この場合、書き込み先が存在しないため応答構造体への書き込みは行わない。

## 3. 応答仕様

### 3.1 応答項目一覧

| No | 項目 | 必須/任意 | フォーマット（仮） |
|---|---|---|---|
| 1 | 顧客ID | 必須 | 英数字24桁固定 |
| 2 | 氏名 | 必須 | SJIS、最大40バイト |
| 3 | 生年月日 | 必須 | `YYYYMMDD` 数字8桁文字列 |
| 4 | 住所 | 必須 | SJIS、最大100バイト |
| 5 | 電話番号 | 必須 | 数字のみ、最大15バイト |
| 6 | メールアドレス | 必須 | ASCII、最大256バイト |
| 7 | システム挿入日時 | 必須 | `YYYYMMDDHHMMSS` 数字14桁文字列 |
| 8 | システム更新日時 | 必須 | `YYYYMMDDHHMMSS` 数字14桁文字列 |
| 9 | サービスA契約状態 | 任意 | 1桁数字（0/1、初期値0） |
| 10 | サービスB契約状態 | 任意 | 1桁数字（0/1、初期値0） |
| 11 | サービスC契約状態 | 任意 | 1桁数字（0/1、初期値0） |

正常終了時は、DBから取得した値が上記11項目すべてについて
欠落・誤マッピングなく応答構造体に反映されていることをUTで検証する。

**REQ-N01（正常系）**:
WHEN 顧客IDの形式が正しく、該当する顧客が1件のみ存在する
THE SYSTEM SHALL 上記11項目（No.1〜11）を欠落・誤マッピングなく応答構造体に反映し、`CUSTOMER_RESULT_OK`を返す

### 3.2 戻り値・エラー表現

**REQ-C01（横断的要件）**:
WHEN `customer_get()`が`CUSTOMER_RESULT_OK`以外の結果を返し、かつ`out_customer`がNULLでない
THE SYSTEM SHALL `out_customer`の`result`フィールド以外の全フィールドをゼロクリアする

**REQ-C02（横断的要件）**:
WHEN `out_customer`がNULLでない
THE SYSTEM SHALL `customer_get()`の戻り値と`out_customer->result`フィールドに同一の`CustomerResult`値を設定する

（`out_customer`自体がNULLの場合はREQ-C01・REQ-C02の対象外。書き込み対象が存在しないため、戻り値でのみエラーを通知する。4節 REQ-E07参照）

| エラーコード（仮） | 意味 |
|---|---|
| `CUSTOMER_RESULT_OK` | 正常終了 |
| `CUSTOMER_RESULT_INVALID_FORMAT` | 顧客IDの形式不正（桁数不正・文字種不正・空文字/NULL・境界値超過を含む） |
| `CUSTOMER_RESULT_NOT_FOUND` | 該当する顧客が存在しない |
| `CUSTOMER_RESULT_DB_CONNECTION_ERROR` | DB接続断・未接続 |
| `CUSTOMER_RESULT_DB_ERROR` | DB側の想定外エラー（DB層が想定外の値を返した場合のフォールバック先も含む） |
| `CUSTOMER_RESULT_DATA_CONFLICT` | 複数件ヒットなどのデータ整合性異常 |
| `CUSTOMER_RESULT_INVALID_ARGUMENT` | `out_customer`（応答格納先ポインタ）がNULL |

## 4. 異常系（受け入れ条件・EARS記法）

1. **REQ-E01**: WHEN 顧客IDの形式は正しいが、該当する顧客が存在しない
   THE SYSTEM SHALL `CUSTOMER_RESULT_NOT_FOUND`を返す
2. **REQ-E02**: WHEN 顧客IDに大文字英数字以外の文字（小文字・記号等）が含まれる
   THE SYSTEM SHALL `CUSTOMER_RESULT_INVALID_FORMAT`を返す
3. **REQ-E03**: WHEN 顧客ID（`customer_id`）が空文字またはNULLである
   THE SYSTEM SHALL `CUSTOMER_RESULT_INVALID_FORMAT`を返す
   （5節 REQ-B03・REQ-B04として、それぞれの具体的な条件を個別に定義する）
4. **REQ-E04**: WHILE DBが接続断または未接続の状態にある
   THE SYSTEM SHALL 顧客情報の検索要求に対し`CUSTOMER_RESULT_DB_CONNECTION_ERROR`を返す
5. **REQ-E05**: WHEN DB側で想定外のエラー（SQLエラー、権限エラー等）が発生する
   THE SYSTEM SHALL `CUSTOMER_RESULT_DB_ERROR`を返す
6. **REQ-E06**: WHEN 同一の顧客IDに一致するレコードがDB側に複数件存在する
   THE SYSTEM SHALL `CUSTOMER_RESULT_DATA_CONFLICT`を返す
7. **REQ-E07**: WHEN `out_customer`（応答格納先ポインタ）がNULLである
   THE SYSTEM SHALL `out_customer`への書き込みを一切行わずに`CUSTOMER_RESULT_INVALID_ARGUMENT`を返す

※DBタイムアウトは非機能要件寄りのため対象外

## 5. 境界値（受け入れ条件・EARS記法）

1. **REQ-B01**: WHEN 顧客ID（`customer_id`）の桁数が23桁（1桁不足）である
   THE SYSTEM SHALL `CUSTOMER_RESULT_INVALID_FORMAT`を返す
2. **REQ-B02**: WHEN 顧客ID（`customer_id`）の桁数が25桁（1桁超過）である
   THE SYSTEM SHALL `CUSTOMER_RESULT_INVALID_FORMAT`を返す
3. **REQ-B03**: WHEN 顧客ID（`customer_id`）が空文字である
   THE SYSTEM SHALL `CUSTOMER_RESULT_INVALID_FORMAT`を返す
4. **REQ-B04**: WHEN 顧客ID（`customer_id`）がNULLである
   THE SYSTEM SHALL `CUSTOMER_RESULT_INVALID_FORMAT`を返す
5. **REQ-B05**: WHEN 顧客IDとして24桁を大幅に超える極端に長い文字列（例: 1000文字）が渡される
   THE SYSTEM SHALL バッファオーバーフロー等を起こすことなく`CUSTOMER_RESULT_INVALID_FORMAT`を返す

※応答項目側の境界値（DBからの想定外データ）はモックで完全制御されるため対象外

## 6. モック化・テスト方針
- モック化の粒度: 関数単位（`db_layer_find_customer`のみ。`db_layer_save_customer`はスコープ外のため削除）
- ビジネスロジック層も参照系のみ（`customer_get`のみ。`customer_register`はスコープ外のため削除）
- モックの戻り値切り替え: セッター関数方式（例: `mock_db_layer_set_return_value(...)`）
- UT対象粒度: `customer_get`（ビジネスロジック層の公開関数）のみ。モック自体はテスト対象外
- テストフレームワーク: Unity（ThrowTheSwitch）

## 7. コーディング規約
- 一般的なC言語コーディング規約に準拠
- 関数にはPERFコメント＋DocString相当のコメント（Doxygen風）を付与

```c
/**
 * PERF: O(1) - DB1件検索のみ、ループ処理なし
 *
 * @brief   顧客IDをキーに顧客情報を1件取得する
 * @param   customer_id  検索対象の顧客ID（英数字24桁、大文字のみ）
 * @param   out_customer 取得結果を格納する出力先構造体（NULL不可）
 * @return  CUSTOMER_RESULT_OK ほか、上記エラーコード
 */
```

- 命名規則: 関数はsnake_case（既存踏襲）、エラーコードはUPPER_SNAKE_CASE

## 8. Phase 2以降へ持ち越す事項
- Oracle DB側のテーブル・カラム構成（スキーマ設計）
- 非機能要件（性能・可用性等）
- `db_layer.c`のJSON方式暫定実装（Phase 4、`design_db_layer.md`参照。Oracle/Pro*C本実装への差し替えは別途）
- 顧客状態（10節）の残り11パターンの定義・要件化。Excel状態遷移図を構造化したJSONが外部から提供され次第、
  10節の代表5パターン（S01〜S05）を実データに差し替え・拡張する
- 10節（顧客状態に応じた応答変動要件）に対応するdesign.md/design_db_layer.mdの設計変更、
  `customer.h`/`db_layer.h`の構造体・関数シグネチャ変更、テストケースの追加（未着手、別タスク）

## 9. テストケース対応表（EARS要件 ⇔ 既存テストケース）

対応先は `test/test_customer.c`（実装済み・全13ケース、`make test`で13 Tests 0 Failuresを確認済み）。
TC IDは `design.md` 5節と同一のものを使用する。

**件数についての補足**: `tasks.md` Phase 3時点（T9, T11）では「12ケース」と記載されているが、
これは`out_customer`がNULLの場合の検証（現在のREQ-E07 / TC-E07）が後から追加される前の記述であり、
現時点の実装・要件は本表の通り**13ケース**が正。tasks.mdの当該記述は更新されていない点に注意。

| REQ ID | 種別 | 内容（要約） | 対応TC ID | カバレッジ |
|---|---|---|---|---|
| REQ-N01 | WHEN | 正常取得・全項目マッピング | TC-N01 | ✅ 検証済み |
| REQ-E01 | WHEN | 該当顧客なし | TC-E01 | ✅ 検証済み |
| REQ-E02 | WHEN | 形式不正（文字種：小文字／記号） | TC-E02, TC-E03 | ✅ 検証済み（2ケースで具体例を検証） |
| REQ-E03 | WHEN | 空文字／NULL入力（総称） | TC-B03, TC-B04 | ✅ 検証済み（5節の個別ケースに分解して検証） |
| REQ-E04 | WHILE | DB接続断・未接続 | TC-E04 | ✅ 検証済み |
| REQ-E05 | WHEN | DB想定外エラー | TC-E05 | ✅ 検証済み |
| REQ-E06 | WHEN | 整合性異常（複数件ヒット） | TC-E06 | ✅ 検証済み |
| REQ-E07 | WHEN | `out_customer`がNULL | TC-E07 | ✅ 検証済み |
| REQ-B01 | WHEN | 23桁（1桁不足） | TC-B01 | ✅ 検証済み |
| REQ-B02 | WHEN | 25桁（1桁超過） | TC-B02 | ✅ 検証済み |
| REQ-B03 | WHEN | 空文字 | TC-B03 | ✅ 検証済み |
| REQ-B04 | WHEN | NULL入力 | TC-B04 | ✅ 検証済み |
| REQ-B05 | WHEN | 極端に長い文字列（1000文字） | TC-B05 | ✅ 検証済み |
| REQ-C01 | WHEN | エラー時、`result`以外を全てゼロクリア | TC-E01〜E06, TC-B01〜B05 | ✅ 検証済み（各TCの`assert_error_result`ヘルパーで共通検証。専用TCなし） |
| REQ-C02 | WHEN | 戻り値と`out_customer->result`が同一値 | TC-N01, TC-E01〜E06, TC-B01〜B05 | ✅ 検証済み（`out_customer`がNULLでない全TCで検証。TC-E07は`out_customer`がNULLのため対象外） |
| REQ-U01 | 普遍的要件 | 参照機能のみを提供（登録・更新なし） | なし | ⚠️ **未検証（対応TCなし）** — `customer_get()`が唯一の公開APIであるという設計・実装上の制約であり、単体テストで直接検証できる性質の要件ではない。UTでは検証不可、レビュー・設計時点での担保に留まる |

### 不足の明示
- 個別のUTとして直接対応するテストケースが存在しない要件は **REQ-U01のみ**（理由は上表参照。UT対象外として意図的に未検証）。
- それ以外の全13要件（REQ-N01, REQ-E01〜E07, REQ-B01〜B05, REQ-C01, REQ-C02）は既存の13テストケースでカバー済みで、不足なし。
- `convert_db_result()`のswitch文における`DbResult`想定外値へのdefault分岐（`CUSTOMER_RESULT_DB_ERROR`にフォールバック）は、design.md 8.3節の通り実装上の防御的分岐であり、本要件定義書（4節・5節）が列挙する異常系のいずれにも直接対応しないため、意図的にUT対象外としている（本表のREQ一覧にも含めていない）。

## 10. 顧客状態（契約・利用ライフサイクル）に応じた応答変動要件【JSON DB検証における試験導入】

### 10.0 位置づけ・前提

- 1節のデータモデルを拡張し、顧客の**契約・利用ライフサイクル状態**（以下「状態」）に応じて、
  `customer_get()`の応答内容（応答項目の値・戻り値・返却されるフィールド構成）が変動するという要件を追加する。
- 状態は全16パターンを想定するが、パターンの正式な定義（状態コード・状態名・遷移条件等）は、
  Excelで管理されている状態遷移図を構造化したJSONとして、別途外部から提供される想定である。
  **本要件定義書の作成時点では、このJSONはまだ提供されていない。**
- 今回はJSON DB方式（`design_db_layer.md`参照）を用いた検証の導入が主目的のため、
  16パターン全てではなく、代表的な**5パターン（S01〜S05）**に絞って試験的に要件化する。
  状態コード・状態名・変動内容は全て**仮のプレースホルダ**であり、実際の業務要件や
  Excel状態遷移図の内容を反映したものではない。実データのJSONが提供され次第、本節は差し替え・拡張する。
- 本節は要件レベルの記述のみを対象とし、design.md/design_db_layer.mdの設計変更、
  `customer.h`/`db_layer.h`の構造体・関数シグネチャ変更、実装、テストケースの追加は
  本タスクの対象外（8節「Phase 2以降へ持ち越す事項」参照、別タスクで実施）。

### 10.1 仮の代表状態パターン（S01〜S05）

| 状態コード | 状態名（仮） | 概要（仮） |
|---|---|---|
| S01 | 有効 | 通常の契約中顧客。3節の応答項目一覧を通常通り全て返却する（既存のREQ-N01に相当） |
| S02 | 休止中 | 一時的にサービス利用を停止している顧客。個人情報系の一部フィールドをマスク（ゼロクリア）した上で返却する |
| S03 | 解約済み | 契約を解約した顧客。応答フィールドを返却せず、専用のエラーコードを返す |
| S04 | 仮登録（未本登録） | 本登録前の顧客。応答フィールドを返却せず、専用のエラーコードを返す |
| S05 | 退会済み | 完全に退会した顧客。該当顧客が存在しない場合（REQ-E01）と同様に扱う |

### 10.2 EARS記法による受け入れ条件（代表5パターン）

**REQ-S01**: WHEN 顧客の状態がS01（有効）である
THE SYSTEM SHALL 3節の応答項目一覧（No.1〜11）を通常通り全て返却し、`CUSTOMER_RESULT_OK`を返す
（既存のREQ-N01と同一の振る舞い。S01は「状態という概念を導入した場合の基準点」としての位置付け）

**REQ-S02**: WHEN 顧客の状態がS02（休止中）である
THE SYSTEM SHALL 氏名・住所・電話番号・メールアドレスをゼロクリアした上で、
その他の項目（顧客ID・生年月日・システム挿入日時・システム更新日時・サービスA/B/C契約状態）を返却し、
`CUSTOMER_RESULT_OK`を返す
（マスク対象フィールドの具体的な選定は仮。実際の要件確定後に見直す）

**REQ-S03**: WHEN 顧客の状態がS03（解約済み）である
THE SYSTEM SHALL 応答構造体の`result`以外の全フィールドをゼロクリアした上で、
`CUSTOMER_RESULT_CANCELLED`（仮称、新設するエラーコード）を返す
（REQ-C01・REQ-C02の横断的要件と整合する振る舞いとする）

**REQ-S04**: WHEN 顧客の状態がS04（仮登録・未本登録）である
THE SYSTEM SHALL 応答構造体の`result`以外の全フィールドをゼロクリアした上で、
`CUSTOMER_RESULT_PROVISIONAL`（仮称、新設するエラーコード）を返す
（REQ-C01・REQ-C02の横断的要件と整合する振る舞いとする）

**REQ-S05**: WHEN 顧客の状態がS05（退会済み）である
THE SYSTEM SHALL 4節のREQ-E01（該当する顧客が存在しない）と同一の振る舞いとして、`CUSTOMER_RESULT_NOT_FOUND`を返す

### 10.3 既存要件（顧客IDバリデーション系）との関係

- 4節・5節に定義された異常系・境界値の受け入れ条件（REQ-E01〜E07, REQ-B01〜B05）は、
  顧客IDの形式チェック・DB接続・整合性異常など、**顧客IDの入力段階で発生する検証**であり、
  本節（10節）で定義する「該当顧客が見つかった後、その状態に応じて応答内容を決定する」処理とは
  独立したレイヤーである。
- `customer_get()`の処理順序は、概念的には
  「(1) 顧客IDフォーマット検証 → (2) DB検索（該当有無・整合性確認） → (3) 状態に応じた応答内容決定」
  という段階を経る想定であり、本節の拡張によって(1)(2)の処理・受け入れ条件は変更されない。
- したがって、既存の`test/test_customer.c`（TC-N01, TC-E01〜E07, TC-B01〜B05の**13ケース**）は、
  本節の拡張による影響を受けず、変更・削除の対象にはならない見込みである。
  - 参考: 本タスクの依頼文中では「既存の12テストケース」と記載されていたが、
    `TC-E07`（`out_customer`がNULLの場合の検証）が後から追加されたことにより、
    現時点の正しいケース数は9節の対応表の通り**13ケース**である（`tasks.md`にも同様の経緯を記載済み）。
- 既存のTC-N01（正常系、サービスA/B/C混在）は、状態という概念を導入した場合、本節のS01（有効状態）に
  相当するケースとして解釈できる。実装フェーズでは、TC-N01のモックデータに状態フィールド（S01相当）を
  追加する形で自然に統合できる見込みだが、これは実装フェーズの話であり、本要件追加では実施しない。

### 10.4 未確定事項（実装フェーズ着手前に確定が必要）

- 状態を表現するデータ項目（フィールド名・型・DbCustomerRecord/Customerへの追加方法）
- `CUSTOMER_RESULT_CANCELLED`・`CUSTOMER_RESULT_PROVISIONAL`という仮称エラーコードの正式名称・要否
- S02のマスク対象フィールドの正式な選定基準
- 16パターン全体の正式な状態コード・状態名・状態ごとの応答変動内容（Excel状態遷移図由来のJSON提供待ち）
