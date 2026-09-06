# AGENTS.md

## 1. このリポジトリについて

このリポジトリでは、長尺音声の編集に特化した軽量な Audio-only Nonlinear Editor を開発する。

主な用途は以下とする。

* Podcast 編集
* マルチトラック音声、複数音声ストリームを含む動画・音声コンテナ、および同期済み複数音声ファイルの編集
* インタビュー編集
* YouTube / 配信アーカイブ向け音声編集
* ナレーション編集
* BGM・効果音を含む spoken-word コンテンツ制作

このアプリケーションは一般的な音楽制作 DAW を目指さない。

最重要目標は、数時間規模のマルチトラック音声を、動画編集ソフトに近い操作感で軽快かつ安全に編集できることである。

---

## 2. 開発上の最重要原則

以下は、明示的な設計変更が承認されない限り維持すること。

### 非破壊編集

元のメディアファイルを編集操作によって変更してはならない。

Split、Trim、Move、Ripple Delete などは、プロジェクト内の Clip 情報のみを変更する。

### サンプル精度

タイムライン上の内部時刻表現は原則として整数サンプル位置を使用する。

編集操作によって意図しないサンプル単位のずれを発生させてはならない。

### マルチトラック同期

同期グループに属する Clip をグループ編集した場合、Clip 間の相対的な時間位置を維持する。

Split、Move、Trim、Ripple Delete などによって、意図しないトラック間同期ずれを発生させてはならない。

### 長尺素材を前提とする

数時間規模の素材を通常ケースとして扱う。

素材全体を無条件に RAM へ展開する設計は禁止する。

波形描画やメディアアクセスでは、キャッシュ、ストリーミング、LOD 等を利用できる構造を優先する。

### リアルタイムオーディオ安全性

オーディオ処理スレッドでは、可能な限り以下を避ける。

* 動的メモリアロケーション
* ファイル I/O
* ネットワーク I/O
* UI 操作
* 長時間保持される mutex
* 処理時間が予測できない操作

リアルタイムオーディオスレッドと UI スレッドの責務を明確に分離する。

---

## 3. 製品思想

操作感は DAW よりも動画編集ソフトの NLE を参考にする。

特に以下を重視する。

* タイムライン中心の操作
* Clip の直接操作
* Clip 端のドラッグによる非破壊 Trim
* Clip の自由な移動
* Split
* Ripple Delete
* 複数 Clip の選択
* Group / Ungroup
* Track Lock
* Snapping
* Fade / Crossfade
* Media Bin
* Inspector
* Mixer

UI を音楽制作 DAW の慣習へ不必要に寄せない。

---

## 4. 想定する主要概念

少なくとも以下を独立したドメイン概念として扱う。

* Project
* Media
* Track
* Clip
* Clip Group
* Timeline
* Selection
* Effect
* Effect Chain
* Mixer
* Master Output

これらの意味と編集規則については以下を正とする。

* `docs/product/editing-model.md`
* `docs/product/requirements.md`

---

## 5. 非目標

少なくとも初期バージョンでは、以下を目的としない。

* MIDI 編集
* ピアノロール
* ソフトウェア音源
* シンセサイザー
* 楽譜編集
* MIDI CC 編集
* Step Sequencer
* テンポベースの音楽制作
* 高度なライブ演奏機能
* 映像編集
* カラーグレーディング
* 映像エフェクト
* 一般的な DAW 全機能の再実装

「将来使えそう」という理由だけで機能を追加しない。

---

## 6. 想定技術

現時点の候補は以下。

* C++20 以降
* CMake
* JUCE
* Tracktion Engine
* FFmpeg
* VST3

主対象 OS:

* Windows 11 x64

設計上無理がない範囲では macOS / Linux への移植性を維持する。

ただし初期開発、動作確認、リリースでは Windows を優先する。

技術選定はまだ最終決定ではない。

重要な技術選定を変更または確定するときは ADR を作成すること。

---

## 7. アーキテクチャ原則

### ドメインモデルを音声エンジンから分離する

Project、Track、Clip、Group などの製品上のモデルを、Tracktion Engine や JUCE 等の外部ライブラリの型へ直接依存させない方向を優先する。

概念上は以下の構造を目指す。

```text
UI
 ↓
Application / Editing Commands
 ↓
Domain Model
 ↓
Audio Engine Adapter
 ↓
Tracktion Engine / JUCE / FFmpeg
```

外部ライブラリを将来交換できる境界を維持する。

### UI とドメインロジックを分離する

Split、Trim、Move、Ripple Delete 等の編集操作を GUI イベントへ直接実装してはならない。

編集操作は UI なしでテストできる構造にする。

### 保存形式とランタイム状態を分離する

プロジェクト保存・読込で必要な永続データと、一時キャッシュや UI 状態を明確に区別する。

---

## 8. 文書をコードより優先する場面

以下に該当する変更では、実装前に関連文書を読むこと。

* 新しい主要機能
* ドメインモデルの変更
* 外部依存関係の追加
* プロジェクト保存形式の変更
* オーディオエンジン構造の変更
* タイムライン編集 semantics の変更
* threading model の変更

関連文書が不足している場合は、実装前に文書を更新する。

---

## 9. 文書構造

主要な製品文書:

* `docs/product/vision.md`
* `docs/product/requirements.md`
* `docs/product/non-goals.md`
* `docs/product/editing-model.md`

全体アーキテクチャ:

* `ARCHITECTURE.md`

技術設計:

* `docs/design/`

Architecture Decision Record:

* `docs/decisions/`

実行計画:

* `docs/exec-plans/active/`
* `docs/exec-plans/completed/`

`AGENTS.md` 自体を巨大な仕様書にしない。

詳細な仕様は上記文書へ置き、AGENTS.md はルールと案内図として維持する。

---

## 10. ADR

重要な設計判断は `docs/decisions/` に ADR として残す。

例:

```text
0001-domain-model-independent-from-audio-engine.md
0002-use-juce.md
0003-use-tracktion-engine.md
0004-use-integer-sample-time.md
```

ADR には最低限以下を記載する。

* Context
* Decision
* Alternatives
* Consequences

決定済み ADR と矛盾する変更を黙って行わない。

必要なら新しい ADR で決定を置き換える。

---

## 11. Execution Plan

非自明な機能を実装するときは、先に `docs/exec-plans/active/` に Execution Plan を作成する。

最低限以下を記載する。

* 目的
* 対象要件
* 変更予定コンポーネント
* 実装方針
* 維持すべき invariant
* edge cases
* テスト戦略
* 変更予定ファイル
* リスク
* 完了条件

実装終了後、必要に応じて `completed/` へ移動する。

小さな typo 修正や明らかな局所修正では Execution Plan は不要。

---

## 12. テスト方針

テスト可能性を設計上の要件として扱う。

特に以下の編集操作には自動テストを用意する。

* Clip Move
* Trim Left
* Trim Right
* Split
* Delete
* Ripple Delete
* Group
* Ungroup
* Group Move
* Group Split
* Undo
* Redo
* Save / Load round trip

重要な invariant はテストによって検証する。

例:

* Split 前後で素材参照が変化しない
* Split 後の Clip 長合計が元 Clip と一致する
* Group 編集で相対 sample offset が変化しない
* Undo → Redo で編集後と同じ状態へ戻る
* Save → Load で sample position が完全一致する

GUI の手動確認だけを正常性の根拠にしない。

---

## 13. 変更時の基本手順

非自明なタスクでは以下の順序を基本とする。

1. 関連する AGENTS.md を読む。
2. `docs/product/` の関連仕様を読む。
3. `ARCHITECTURE.md` と関連設計文書を読む。
4. 必要なら Execution Plan を作成する。
5. 既存テストを確認する。
6. 最小の変更として実装する。
7. テストを追加または更新する。
8. ビルドする。
9. 自動テストを実行する。
10. 差分を自己レビューする。
11. 仕様・設計を変更した場合は文書も更新する。

テストを実行していない状態で「完了」と扱わない。

実行できなかった検証がある場合は、その理由を明示する。

---

## 14. Codex の行動指針

### 勝手にスコープを拡大しない

依頼された機能を実現するために必要でないリファクタリングや機能追加を行わない。

### 不明点を推測で仕様化しない

既存の製品仕様・ADRから結論を導けない重要事項は、実装前に文書上の問題として明示する。

### 既存コードを先に読む

新しい仕組みを作る前に、同じ問題を解決する既存コード、ヘルパー、テストがないか確認する。

### 大規模変更を一度に行わない

変更をレビュー可能な単位に分割する。

### warning を無視しない

自身の変更によって新しい compiler warning や static analysis warning を追加しない。

### コメントで悪い設計を隠さない

複雑なコードを長いコメントで説明するより、構造そのものを単純にする。

---

## 15. レビュー

重要な変更では実装とは独立した視点でレビューする。

レビューでは少なくとも以下を確認する。

* requirements への適合
* sample accuracy
* source media の不変性
* ownership / lifetime
* thread safety
* real-time audio safety
* undo / redo の完全性
* serialization
* 大規模 audio buffer の不要なコピー
* integer overflow
* edge cases
* test coverage

レビュー結果は severity 順に報告する。

---

## 16. 完了条件

機能は「コードを書いた」ときではなく、以下を満たしたときに完了とする。

* 要件を満たしている
* ビルド可能
* 関連テストが成功する
* invariant が維持されている
* 不要なスコープ拡張がない
* 必要な文書が更新されている
* 未検証事項が明示されている

速度よりも、将来変更可能で検証可能な構造を優先する。

---

## 17. ビルドとテスト

Windows 11 x64 における標準のビルド入口は、リポジトリルートの `build.ps1` とする。

Codex は、特別な理由がない限り独自に `cmake`、`ninja`、`msbuild` 等を直接組み合わせず、以下を使用すること。

```powershell
.\build.ps1
```

`build.ps1` は、Visual Studio Build Tools の開発環境初期化、CMake configure、Ninja build、および標準的な検証処理を担当する。

ビルド方法、コンパイラ設定、依存関係、環境初期化方法を変更する必要がある場合は、Codex 固有の回避策を追加するのではなく、可能な限り `build.ps1` を更新し、人間・Codex・CI が同じビルド入口を使用できる状態を維持する。

テストが `build.ps1` に統合されている場合は、その結果を正とする。

別途テスト用スクリプトまたはテストコマンドが定義されている場合は、ビルド成功後にそれを実行する。

ビルドまたは必要なテストを実行できなかった場合は、完了したものとして扱わず、完了報告で理由と未検証事項を明示する。

自身の変更によって新しい compiler warning、linker warning、static analysis warning を発生させてはならない。
