#ifndef DB_LAYER_H
#define DB_LAYER_H

/*
 * DBアクセス層インターフェース。
 * 本番実装: src/db_layer.c (実DB接続予定)
 * テスト実装: test/mocks/mock_db_layer.c (リンク時に本実装と差し替え)
 * どちらか一方だけをビルド対象にリンクすることで差し替えを行う。
 */

/*
 * DB層の処理結果を表すエラーコード。
 * DBアクセスの結果は「成功したか」「なぜ失敗したか」を、この列挙型の値で呼び出し元に伝える。
 * C言語には例外機構がないため、戻り値でエラー種別を表現するのが一般的なやり方。
 */
typedef enum {
    DB_RESULT_OK = 0,             /* 正常終了 */
    DB_RESULT_NOT_FOUND,          /* 該当データなし */
    DB_RESULT_CONNECTION_ERROR,   /* DB接続断・未接続 */
    DB_RESULT_DB_ERROR,           /* DB側の想定外エラー（SQLエラー等） */
    DB_RESULT_DATA_CONFLICT       /* データ整合性異常（複数件ヒット等） */
} DbResult;

/*
 * DBから取得した顧客1件分のデータを保持する構造体。
 * 各文字列フィールドはC言語の文字列（末尾に'\0'が入る）として扱うため、
 * 実際に格納できる最大文字数 + 1バイトの配列サイズにしている。
 * 例: customer_id[24 + 1] は「24文字の顧客ID + 終端文字'\0'」の意味。
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
} DbCustomerRecord;

/**
 * @brief   DB接続を初期化する
 * @return  DB_RESULT_OK ほか、接続に失敗した場合はエラーコード
 */
DbResult db_layer_init(void);

/**
 * @brief   DB接続を終了する
 */
void db_layer_close(void);

/**
 * PERF: O(1) - DB1件検索のみ、ループ処理なし
 *
 * @brief   顧客IDをキーにDBから顧客情報を1件取得する
 * @param   customer_id  検索対象の顧客ID文字列
 * @param   out_record   取得結果を格納する出力先構造体（呼び出し元が確保したメモリを渡す）
 * @return  DB_RESULT_OK ほか、上記DbResultのエラーコード
 */
DbResult db_layer_find_customer(const char *customer_id, DbCustomerRecord *out_record);

#endif /* DB_LAYER_H */
