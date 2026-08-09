#ifndef CUSTOMER_H
#define CUSTOMER_H

/*
 * ビジネスロジック層（customer_get の呼び出し元）から見たエラーコード。
 * db_layer.h の DbResult とよく似ているが、こちらは「顧客IDの形式不正」という
 * ビジネスロジック層特有のエラー（CUSTOMER_RESULT_INVALID_FORMAT）を持つ点が異なる。
 * DbResultから変換されるルールは customer.c の convert_db_result を参照。
 */
typedef enum {
    CUSTOMER_RESULT_OK = 0,                 /* 正常終了 */
    CUSTOMER_RESULT_INVALID_FORMAT,         /* 顧客IDの形式不正（桁数・文字種・空文字/NULL等） */
    CUSTOMER_RESULT_NOT_FOUND,              /* 該当する顧客が存在しない */
    CUSTOMER_RESULT_DB_CONNECTION_ERROR,    /* DB接続断・未接続 */
    CUSTOMER_RESULT_DB_ERROR,               /* DB側の想定外エラー */
    CUSTOMER_RESULT_DATA_CONFLICT,          /* データ整合性異常（複数件ヒット等） */
    CUSTOMER_RESULT_INVALID_ARGUMENT        /* out_customer（出力先ポインタ）がNULL */
} CustomerResult;

/*
 * customer_get の呼び出し元に返す応答用構造体。
 * DbCustomerRecord（db_layer.h）とほぼ同じ項目を持つが、
 * 最後に result フィールドを持ち、処理結果（エラーコード）もここに格納する。
 * エラー発生時は result 以外の全フィールドをゼロクリアする（requirements.md 3.2参照）。
 */
typedef struct {
    char customer_id[24 + 1];   /* 顧客ID（英数字24桁） */
    char name[40 + 1];          /* 氏名（SJIS、最大40バイト） */
    char birth_date[8 + 1];     /* 生年月日（YYYYMMDD、数字8桁） */
    char address[100 + 1];      /* 住所（SJIS、最大100バイト） */
    char phone_number[15 + 1];  /* 電話番号（数字のみ、最大15バイト） */
    char email[256 + 1];        /* メールアドレス（ASCII、最大256バイト） */
    char inserted_at[14 + 1];   /* システム挿入日時（YYYYMMDDHHMMSS） */
    char updated_at[14 + 1];    /* システム更新日時（YYYYMMDDHHMMSS） */
    int service_a;               /* サービスA契約状態（0/1） */
    int service_b;               /* サービスB契約状態（0/1） */
    int service_c;               /* サービスC契約状態（0/1） */
    CustomerResult result;        /* 処理結果（エラーコード） */
} Customer;

/**
 * PERF: O(1) - DB1件検索のみ、ループ処理なし
 *
 * @brief   顧客IDをキーに顧客情報を1件取得する（本APの唯一の公開API）
 * @param   customer_id  検索対象の顧客ID（英数字24桁、大文字のみ）
 * @param   out_customer 取得結果を格納する出力先構造体（呼び出し元が確保したメモリを渡す）。
 *                       NULLの場合は書き込みを行わず CUSTOMER_RESULT_INVALID_ARGUMENT を返す。
 * @return  CUSTOMER_RESULT_OK ほか、上記CustomerResultのエラーコード
 *          （out_customerがNULLでない場合、out_customer->result にも同じ値が入る）
 */
CustomerResult customer_get(const char *customer_id, Customer *out_customer);

#endif /* CUSTOMER_H */
