# CGENGOAP (customer_app)

顧客情報参照AP（C言語、参照専用のバックエンドAP）。

- プロジェクトのルール・読むべきドキュメントの索引: [CLAUDE.md](./CLAUDE.md)
- 要件定義: [requirements.md](./requirements.md)
- 設計: [design.md](./design.md) / [design_db_layer.md](./design_db_layer.md)

## ビルド/テスト

```sh
make              # build/main を生成
make test         # customer_get() のUT（13ケース）
make test_db_layer # db_layer.c（JSON方式）自体のUT（6ケース）
```

<!-- test: verify PR flow (feature branch) -->
