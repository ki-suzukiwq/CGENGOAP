#include <stdio.h>
#include "db_layer.h"
#include "customer.h"

/*
 * このAPのエントリポイント（プログラム起動時に最初に実行される関数）。
 * 現時点ではDB接続の初期化・終了を呼び出すだけの最小限の雛形。
 * 本来はここで customer_get() を呼び出して顧客情報を取得し、結果を出力する想定。
 */
int main(void) {
    db_layer_init();
    printf("customer_app started\n");
    db_layer_close();
    return 0;
}
