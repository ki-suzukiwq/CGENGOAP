---
description: customer_appの全テスト（make test + make test_db_layer）を実行し結果を報告する
allowed-tools: Bash(make test), Bash(make test_db_layer)
---

customer_appプロジェクトで以下を順に実行する。

1. `make test` を実行する（`customer_get()`のUT、13ケース想定）
2. `make test_db_layer` を実行する（`db_layer.c`のJSON方式実装のUT、6ケース想定）

各コマンドの実行後、Tests/Failures/Ignoredの件数を簡潔に報告する。
失敗したテストがあれば、失敗したテスト関数名と、原因調査に必要な最小限の情報（該当ソースの行番号など）を示す。
テストが全て成功した場合は結果件数のみ簡潔に報告し、余計な説明は加えない。
