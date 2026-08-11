# コーディング規約・命名規則

出典: `requirements.md` 7節、`design.md` 6節

## 一般規約
- 一般的なC言語コーディング規約に準拠
- 関数名: snake_case（既存踏襲）
- エラーコード（enum値）: UPPER_SNAKE_CASE

## 関数コメント（PERF + Doxygen風DocString）
公開関数には以下の形式でコメントを付与する。

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

## テストケース命名規則（test_customer.c）
- 命名規則: `test_customer_get_<状況>_should_<期待結果>`
- 1ファイルに関連する全ケースをまとめる（`test/test_customer.c`（Unit Test）、`test/integration/test_db_layer.c`（Integration Test）のように対象単位で分割）
- TC ID（design.md参照）: 正常系は`TC-N`、異常系は`TC-E`、境界値は`TC-B`、db_layer固有は`TC-DB-`のprefixを使う

## エラー時の応答構造体の扱い
- エラー発生時、応答構造体（`Customer`）の他フィールド（氏名・住所等）はゼロクリアする
- ただし`out_customer`自体がNULLの場合はこの限りではない（書き込み対象が存在しないため、戻り値でのみエラーを通知する）
