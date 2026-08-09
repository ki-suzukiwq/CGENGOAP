# ============================================================
# Makefile: `make` コマンドを実行したときのビルド手順を定義するファイル。
# 「変数名 := 値」で変数を定義し、「ターゲット: 依存ファイル」の下にタブでインデントした
# コマンドを書くと、「依存ファイルが更新されたらそのコマンドを実行する」というルールになる。
# ============================================================

# 使用するCコンパイラ。ここではclang（Xcode Command Line Toolsに含まれる）を指定。
CC := clang
# コンパイル時のオプション。
#   -Wall -Wextra : 警告を多めに出す（バグの早期発見のため）
#   -std=c11      : C11規格に準拠してコンパイルする
#   -g            : デバッグ情報を付与する（gdb/lldbでのデバッグを可能にする）
CFLAGS := -Wall -Wextra -std=c11 -g

# ディレクトリパスをまとめて変数化しておくことで、後の記述を短く・変更しやすくしている。
SRC_DIR := src
TEST_DIR := test
UNITY_DIR := $(TEST_DIR)/unity
MOCKS_DIR := $(TEST_DIR)/mocks
FIXTURES_DIR := $(TEST_DIR)/fixtures/db_layer
BUILD_DIR := build
# CJSON_DIR: db_layer.c（JSON方式暫定実装）がJSONパースに使うcJSONのベンダリング先。
#            test/unityと同様にソース直接配置（サブモジュールではない）。design_db_layer.md 6節参照。
CJSON_DIR := $(SRC_DIR)/vendor/cjson

# LDLIBS: リンク時に必要な外部ライブラリ。
#   -liconv : db_layer.c が文字コード変換(UTF-8→SJIS)に使うiconvのリンクに必要（macOSではリンク必須）。
LDLIBS := -liconv

# --- 通常ビルド (本番実装 db_layer.c を使用) ---
# APP_SRCS: 本番用実行ファイルをビルドするのに必要なソースファイル一覧。
#           db_layer.c（本物のDBアクセス層、JSON方式暫定実装）とcJSON本体を含む点がテストビルドとの違い。
APP_SRCS := $(SRC_DIR)/main.c $(SRC_DIR)/customer.c $(SRC_DIR)/db_layer.c $(CJSON_DIR)/cJSON.c
# APP_TARGET: 上記をビルドして出来上がる実行ファイルのパス。
APP_TARGET := $(BUILD_DIR)/main

# --- テストビルド (db_layer.c の代わりに mock_db_layer.c をリンク) ---
# TEST_SRCS: テスト実行ファイルをビルドするのに必要なソースファイル一覧。
#            db_layer.c の代わりに mock_db_layer.c を使うことで、
#            実DBに接続せずビジネスロジック(customer.c)だけをテストできる。
#            バックスラッシュ(\)は「行の続きがあります」という意味（Makefileの記法）。
TEST_SRCS := $(TEST_DIR)/test_customer.c \
             $(SRC_DIR)/customer.c \
             $(MOCKS_DIR)/mock_db_layer.c \
             $(UNITY_DIR)/unity.c
# TEST_TARGET: テストをビルドして出来上がる実行ファイルのパス。
TEST_TARGET := $(BUILD_DIR)/test_runner
# TEST_INCLUDES: コンパイラにヘッダファイル(.h)の検索先を教えるオプション（-I ディレクトリ）。
#                mock_db_layer.h や unity.h を見つけられるようにするために必要。
TEST_INCLUDES := -I$(SRC_DIR) -I$(UNITY_DIR) -I$(MOCKS_DIR)

# --- db_layer.c自体のテストビルド (JSON方式実装を直接テスト、mockは使わない) ---
# TEST_DB_LAYER_SRCS: db_layer.c（JSON方式）自体を検証するテスト実行ファイルに必要なソース一覧。
#                     db_layer_internal.h経由でdb_layer_load_from_path()を直接呼び出す。
#                     design_db_layer.md 7節参照。
TEST_DB_LAYER_SRCS := $(TEST_DIR)/test_db_layer.c \
                       $(SRC_DIR)/db_layer.c \
                       $(CJSON_DIR)/cJSON.c \
                       $(UNITY_DIR)/unity.c
TEST_DB_LAYER_TARGET := $(BUILD_DIR)/test_db_layer_runner
TEST_DB_LAYER_INCLUDES := -I$(SRC_DIR) -I$(UNITY_DIR)

# .PHONY: 「all」「test」「test_db_layer」「clean」は実際のファイル名ではなく、単なるコマンドの別名であることを
#         Makeに伝える宣言。これを書かないと、もし同名のファイル（例: "clean"）が存在した場合に
#         Makeがそれを「もう最新だから何もしなくてよい」と誤判定してしまうことがある。
.PHONY: all test test_db_layer clean

# `make` または `make all` を実行したときのデフォルトターゲット。
# 中身は無く、$(APP_TARGET)（=build/main）に依存しているだけ＝実質そのビルドを指す。
all: $(APP_TARGET)

# build/main の作り方（本番ビルドのルール）。
# 「| $(BUILD_DIR)」は「build/main を作る前に build ディレクトリが存在すること」という
# 順序だけの依存関係（BUILD_DIRの更新日時は見ない）。
$(APP_TARGET): $(APP_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(CJSON_DIR) $(APP_SRCS) -o $(APP_TARGET) $(LDLIBS)

# `make test` を実行したときのターゲット。
# まず build/test_runner をビルドし（下のルール）、その後に実際に実行する。
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# build/test_runner の作り方（テストビルドのルール）。
$(TEST_TARGET): $(TEST_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TEST_INCLUDES) $(TEST_SRCS) -o $(TEST_TARGET)

# `make test_db_layer` を実行したときのターゲット。
# db_layer.c（JSON方式実装）自体をmockなしで直接検証する。既存の `make test` とは独立。
test_db_layer: $(TEST_DB_LAYER_TARGET)
	./$(TEST_DB_LAYER_TARGET)

# build/test_db_layer_runner の作り方。
$(TEST_DB_LAYER_TARGET): $(TEST_DB_LAYER_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TEST_DB_LAYER_INCLUDES) -I$(CJSON_DIR) $(TEST_DB_LAYER_SRCS) -o $(TEST_DB_LAYER_TARGET) $(LDLIBS)

# build ディレクトリが無ければ作る（mkdir -p は既に存在してもエラーにならないオプション）。
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# `make clean` を実行したときのターゲット。build ディレクトリを丸ごと削除し、ビルド成果物を一掃する。
clean:
	rm -rf $(BUILD_DIR)
