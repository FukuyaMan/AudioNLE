# AudioNLE の製品要件

## 目的と読み方

本書は AudioNLE 初期バージョンの製品要件を MoSCoW 方式で定義する。

AudioNLE は、長尺・マルチトラック音声、とりわけ spoken-word コンテンツを、audio-only のタイムライン上で非破壊編集し、ミックスして完成音声として出力するための Nonlinear Editor である。

各要件 ID は一意とする。

* `Must` は初期バージョンの成立に必須とする。
* `Should` は優先度が高いが、Must 完了後に判断可能なものとする。
* `Could` は余力がある場合、または将来の発展候補とする。
* `Won't` は初期バージョンで意図的に扱わない範囲とする。

Must の Acceptance Criteria は、特記しない限り自動テスト、検証用テストデータ、または再現可能な手順によって判定できるものとする。

「高速」「軽量」「安定」「直感的」といった表現だけを完了条件として使用しない。

---

# Must

## Media / Import

### MEDIA-001 — 単一音声ファイルの取り込み

利用者は、対応する一般的な単一音声ファイルを Project に取り込み、Media Bin から Timeline の Track へ Clip として配置できなければならない。

Acceptance Criteria

* 取り込んだ素材は Media Bin に 1 件の Media として表示される。
* Media Bin の素材を Timeline に配置すると、その Media を参照する Clip が作成される。
* 同一 Media から複数の Clip を作成できる。
* Clip の Timeline 上の開始位置と使用範囲は、Project の時間モデルに従って保存される。

---

### MEDIA-002 — 複数音声ストリームを持つコンテナの取り込み

利用者は、複数の音声ストリームを持つ対応コンテナを取り込み、含まれる各音声ストリームを個別の編集対象として利用できなければならない。

対象例には MP4、MKV、MOV 等を含むが、特定の録音ソフトや生成元には依存しない。

Acceptance Criteria

* テスト用コンテナ内の検出可能な各音声ストリームを識別できる。
* 利用者は取り込む音声ストリームを選択できる。
* 選択した各音声ストリームを個別の Media または同等の編集可能な単位として扱える。
* 複数ストリームを Timeline へ同期配置した場合、コンテナ内で与えられた相対時刻関係が保持される。
* 映像ストリームが存在しても、映像編集機能を要求しない。

---

### MEDIA-003 — 同期関係を持つ複数音声ファイルの取り込み

利用者は、外部から与えられた同期情報または利用者が指定した相対開始位置に基づき、複数音声ファイルを同期関係を保った状態で Timeline に配置できなければならない。

AudioNLE 自身による自動同期検出は、この要件には含めない。

Acceptance Criteria

* 複数 Media を一括配置する際、指定された各素材の開始 offset が保持される。
* 配置された Clip を Synchronization Group として関連付けられる。
* Synchronization Group への所属は Project Save / Load 後も保持される。
* 同期情報の自動推定が存在しなくても本要件を満たせる。

---

### MEDIA-004 — Media Bin

Project 内へ取り込まれた Media は、Timeline 上の Clip とは独立して Media Bin で管理されなければならない。

Acceptance Criteria

* 同一 Media を複数 Clip から参照できる。
* Clip を削除しても、Media Bin から明示的に除去されない限り Media は残る。
* Media Bin から Timeline への再配置によって、新しい Clip を作成できる。
* Media の識別情報と元ファイル参照情報を保持できる。

---

### MEDIA-005 — Source Media の不変性

編集操作および Project の保存は、Source Media の内容を変更してはならない。

Acceptance Criteria

* Split、Trim、Move、Delete、Ripple Delete、Fade、Processing Stack の変更、Effect の適用前後で Source Media の内容が変更されない。
* テスト用 Source Media について、編集前後の cryptographic hash が一致する。
* Project 保存処理は Source Media を書き換えない。
* Project は Source Media の完全な PCM コピーを編集データとして保存しない。

---

## Timeline / Time Model

### TIME-001 — Project Timeline Sample Rate

各 Project は 1 つの Timeline Sample Rate を持たなければならない。

新規 Project のデフォルト値は 48 kHz とする。

Acceptance Criteria

* 新規 Project の既定 Timeline Sample Rate は 48,000 Hz である。
* Project は Timeline Sample Rate を保存する。
* Project Save / Load 後に Timeline Sample Rate が一致する。
* Timeline 上の正規位置表現は Project Timeline Sample Rate に基づく整数 sample 位置である。
* 異なる native sample rate の Source Media を同一 Project で扱える。

---

### TIME-002 — Source Time Domain と Timeline Time Domain の分離

Source Media の sample domain と Project Timeline の sample domain は区別されなければならない。

Acceptance Criteria

* Clip は Timeline 上の位置・長さと Source Media 内の使用位置・使用範囲を区別して保持できる。
* 44.1 kHz 等、Project Timeline Sample Rate と異なる Media を配置しても、Timeline 上の時間関係を保持できる。
* Source Media の native sample rate が Project Sample Rate と異なることによって、Clip の論理的な Timeline 位置が変化しない。

---

### TIME-003 — Timeline 表示形式

Timeline 上の標準的な時刻表示は `hh:mm:ss.mmm` とする。

内部表現は表示形式に依存してはならない。

Acceptance Criteria

* Timeline sample position を `hh:mm:ss.mmm` 形式で表示できる。
* 表示値から内部 sample position を直接保持する設計ではない。
* Project Sample Rate に基づき表示値を再計算できる。

---

## Timeline Editing

### EDIT-001 — Clip Move

利用者は選択した Clip を Timeline 上の任意の許可された Track と開始 sample 位置へ移動できなければならない。

Acceptance Criteria

* Snapping が無効な場合、移動後の Clip 開始位置は指定された整数 Timeline sample 位置と一致する。
* Move により Clip の参照 Media と Source 使用範囲は変化しない。
* Track Lock 等により移動不可の場合、状態変更は発生しない。
* Move は Undo / Redo できる。

---

### EDIT-002 — Split

利用者は Clip 内の指定 Timeline sample 位置で Clip を分割できなければならない。

Acceptance Criteria

* 分割点が Clip 内部にある場合、元 Clip は同じ Media を参照する 2 Clip に置き換わる。
* 分割後 2 Clip の Timeline 上の連続範囲は分割前と一致する。
* 分割後 2 Clip の Source 使用範囲を連結すると分割前の Source 使用範囲と一致する。
* 分割点は整数 Timeline sample 位置と一致する。
* Clip 開始位置または終了位置そのものへの Split は無効操作として扱える。
* Split は Undo / Redo できる。

---

### EDIT-003 — 非破壊 Trim Left / Right

利用者は Clip の左右端をドラッグ等の直接操作によって非破壊 Trim できなければならない。

Acceptance Criteria

* Left Trim は Clip の Timeline 開始位置と Source 使用開始位置を対応して変更し、Timeline 上の右端を維持する。
* Right Trim は Clip の Timeline 終了位置と Source 使用終了位置を対応して変更し、Timeline 上の開始位置を維持する。
* Source Media の有効範囲を超える Trim は成立しない。
* Clip 長が 0 以下となる Trim は成立しない。
* 一度短縮した Clip は、Source Media の有効範囲内で再び端を外側へ伸ばすことにより、以前除外した Source 範囲を復元できる。
* Trim は Source Media を変更しない。
* Trim は Undo / Redo できる。

---

### EDIT-004 — Delete

利用者は選択 Clip を Timeline から削除できなければならない。

Acceptance Criteria

* Delete は対象 Clip を Timeline から除去する。
* Delete によって同一 Track または他 Track の後続 Clip の Timeline 位置は自動的に変更されない。
* Delete は Media Bin 内の Media を削除しない。
* Delete は Undo / Redo できる。

---

### EDIT-005 — Ripple Delete

利用者は指定した Timeline 範囲を削除し、その後続要素を前方へ詰める Ripple Delete を実行できなければならない。

デフォルトでは、現在選択されている編集対象範囲に対して適用する。

Synchronization Group に属する Clip が影響を受ける場合は、その同期関係を保って追従させる。

Acceptance Criteria

* Ripple Delete の対象 Timeline 範囲が操作前に決定される。
* 削除範囲後方の、scope 内にある Clip は削除範囲長と同じ Timeline sample 数だけ前方へ移動する。
* scope 外の Clip は移動しない。
* 同じ Synchronization Group に属し、Ripple 対象となる Clip の相対 Timeline offset は操作前後で変化しない。
* Ripple Delete の適用 scope は決定論的である。
* Ripple Delete は Undo / Redo できる。

---

### EDIT-006 — 複数選択

利用者は複数 Clip を同時に選択し、対応する編集操作を適用できなければならない。

Acceptance Criteria

* 複数 Track にまたがる Clip を選択できる。
* 選択状態は視覚的または同等に識別できる。
* 対応する操作では、選択された複数 Clip を対象として Move、Delete 等を実行できる。
* 選択状態そのものと Clip Group / Synchronization Group は別概念である。

---

### EDIT-007 — Clip Group / Ungroup

利用者は任意の複数 Clip を編集上の Clip Group としてまとめ、また解除できなければならない。

Clip Group は Synchronization Group とは別概念とする。

Acceptance Criteria

* 任意の複数 Clip を 1 つの Clip Group に所属させられる。
* Clip Group に対する対応操作では、所属 Clip をまとめて扱える。
* Ungroup 後、各 Clip の Timeline 位置、Source 使用範囲、Media 参照は Ungroup 前と一致する。
* Clip Group を解除しても Synchronization Group の関係は自動的に破棄されない。
* Group / Ungroup は Undo / Redo できる。

---

### EDIT-008 — Snapping

利用者は Snapping を有効または無効にできなければならない。

Acceptance Criteria

* Snapping 無効時、Move、Trim、Split の操作位置は指定 Timeline sample 位置になる。
* Snapping 有効時、定義済みスナップ対象と許容範囲内にある操作位置は対象 sample 位置へ決定論的に補正される。
* Snapping の対象と優先順位は別途仕様化する。

---

### EDIT-009 — Track Lock

利用者は Track を Lock できなければならない。

Acceptance Criteria

* Lock 中 Track では Clip の作成、Move、Trim、Split、Delete、Ripple Delete が実行されない。
* Lock 中でも再生、表示、Mixer 状態の確認は可能である。
* Lock 状態は Project Save / Load 後も復元される。

---

### EDIT-010 — Clip Fade

利用者は Clip に Fade In / Fade Out を設定できなければならない。

Acceptance Criteria

* Fade の開始・終了位置は Timeline sample position として保存できる。
* Fade は非破壊である。
* Fade 設定は Processing Stack 内で定義された順序に従って適用される。
* Fade の変更は Undo / Redo できる。
* Fade 設定は Save / Load 後に復元される。

---

### EDIT-011 — Clip Overlap

同一 Track 上で複数 Clip が時間的に重なることを許可しなければならない。

Acceptance Criteria

* 同一 Track 上に重複 Timeline 範囲を持つ複数 Clip を配置できる。
* 重なった Clip はそれぞれ独立して編集できる。
* 重複部分では各 Clip の処理済み出力が Track Mix へ送られる。

---

### EDIT-012 — Crossfade 表現

Crossfade は独立した専用 DSP オブジェクトを必須とせず、原則として重なった Clip の Fade Out と Fade In の組み合わせとして表現する。

Acceptance Criteria

* Clip A の Fade Out と Clip B の Fade In を重複範囲に設定して Crossfade を構成できる。
* Crossfade 相当の状態は各 Clip の Fade 情報として保存・復元できる。
* Fade の一方のみを変更可能である。
* Crossfade 表現は Source Media を変更しない。

---

## Synchronization

### SYNC-001 — Synchronization Group

時間関係を保護するための Synchronization Group を提供しなければならない。

Synchronization Group は Clip Group とは別概念とする。

Acceptance Criteria

* 複数 Clip を Synchronization Group に所属させられる。
* Synchronization Group は相対的な Timeline 時間関係を表現できる。
* Synchronization Group への所属は Save / Load 後に復元される。
* Clip Group への追加・削除によって Synchronization Group は暗黙に変更されない。

---

### SYNC-002 — Sample-accurate な同期保持

Synchronization Group に対する同期編集では、所属 Clip 間の相対 Timeline sample offset を保持しなければならない。

Acceptance Criteria

* Group Move 前後で、同期対象 Clip 間の開始 sample position 差が一致する。
* Group Split は対応する Clip を同じ Timeline sample position で分割する。
* Ripple Delete の対象となった同期 Clip 間の相対 offset は操作前後で一致する。
* 同期保持は整数 Timeline sample position に基づく。

---

### SYNC-003 — 一時的な同期保護バイパス

利用者は Synchronization Group 自体を破棄せず、一時的に同期保護をバイパスして個別 Clip を編集できなければならない。

Acceptance Criteria

* 明示的な個別編集操作では対象 Clip のみを Move、Trim、Split、Delete 等の対象にできる。
* 個別編集後も Synchronization Group の所属情報そのものは保持できる。
* 個別編集による Timeline offset の変更は保存される。
* 個別編集操作は Undo / Redo できる。

---

### SYNC-004 — 同期操作の予測可能性

同期編集または個別編集によって影響を受ける Clip を、利用者が操作前に判断できなければならない。

Acceptance Criteria

* 選択状態または編集対象表示から、操作対象 Clip を識別できる。
* Synchronization Group 全体を編集する状態と、一時的な個別編集状態を区別できる。
* 同期を変更する可能性のある操作が暗黙に実行されない。

---

## Processing / Effects

### PROC-001 — Clip Processing Stack

各 Clip は順序付きの Processing Stack を持てなければならない。

Processing Stack は Source と Track Mix の間に位置する。

Acceptance Criteria

* Clip Processing Stack に複数 Processor を追加できる。
* Processor は Stack 上の順序に従って処理される。
* 利用者は対応する Processor の順序を変更できる。
* Stack の追加、削除、並べ替えは Undo / Redo できる。
* Stack 構成は Save / Load 後に同じ順序で復元される。

---

### PROC-002 — Built-in Clip Processor

Gain、Fade 等、Clip 固有の基本処理は Processing Stack 上の Processor として表現可能でなければならない。

Acceptance Criteria

* Clip Gain を Processing Stack に含められる。
* Clip Fade を Processing Stack に含められる。
* 対応する Built-in Processor と Effect Processor を順序変更可能な設計とする。
* Source 自体と Track Mix は Clip Processing Stack の並べ替え対象ではない。

---

### PROC-003 — Processing Stack の順序依存性

Processor の並び順によって処理結果が変化することを許可し、その順序を Project 状態として保持しなければならない。

Acceptance Criteria

* `Gain → Effect` と `Effect → Gain` を異なる Stack として表現できる。
* 異なる順序で異なる出力結果を生成できる。
* 並び順は Save / Load 後に一致する。

---

### FX-001 — Track FX / Master FX

利用者は Track および Master Output に対して Effect を順序付きで設定できなければならない。

Acceptance Criteria

* Track ごとに独立した Effect Chain または Processing Stack を持てる。
* Master Output に Effect Chain を持てる。
* Effect の追加、削除、並べ替えができる。
* Effect 構成と順序は Save / Load 後に復元される。
* Effect の追加、削除、並べ替えは Undo / Redo できる。

---

### FX-002 — VST3 Hosting

初期バージョンは VST3 プラグインを Effect Processor として利用できなければならない。

Acceptance Criteria

* 利用可能な VST3 を Clip、Track、Master の対応 Processing Stack へ追加できる。
* VST3 を削除できる。
* プラグインを識別する情報を Project に保存できる。
* 再読込時に同じプラグインを復元できる。

---

### FX-003 — VST3 Parameter State

VST3 の Parameter / Plugin State を Project に保存し、再読込時に復元できなければならない。

初期バージョンでは Parameter Automation を Must としない。

Acceptance Criteria

* VST3 の復元可能な State を Project に保存できる。
* Save / Load 後に Plugin State が復元される。
* Plugin State の保存失敗によって Project 全体の保存内容が失われない。

---

### FX-004 — Plugin Latency Compensation

正しく latency を報告する対応 Plugin によって、他 Track と比較した信号の Timeline 上の時刻関係が意図せず変化してはならない。

Acceptance Criteria

* 既知の latency を報告するテスト Effect を有効にした場合、補償後の基準信号との整列誤差は 0 Timeline sample である。
* Plugin の latency 変更後に補償状態を更新できる。
* Plugin の bypass / enable 状態変更後に補償状態を更新できる。
* 誤った latency 情報を報告する外部 Plugin の動作までは保証対象としない。

---

### FX-005 — Processing Tail

Processing Stack 内の Processor が Source 終了後にも出力を生成する場合、その Tail を Clip の Source 終了位置で不必要に切断してはならない。

Acceptance Criteria

* Source 終了後も必要な期間、Processing Stack に無音入力を与えて処理を継続できる。
* Tail は Tail 生成 Processor より後段の Processor を通過する。
* Tail は同一 Track 上の後続 Clip と同時に再生できる。
* Tail は Offline Export に反映される。
* 有限 Tail 長を正しく報告する Effect について、その情報を利用できる。
* Tail 長を自動決定できない Effect について、定義済み fallback policy に従う。
* Clip を Move した場合、その Clip に由来する Tail も新しい Timeline 位置へ追従する。
* Clip を Delete した場合、その Clip に由来する Tail も削除される。

---

### FX-006 — Missing Plugin 耐性

Project 内で使用している VST3 が利用できない場合でも、Project 全体の読み込みが失敗してはならない。

Acceptance Criteria

* 不足 Plugin が存在しても Project の他の Track、Clip、Media、編集情報を読み込める。
* 不足している Plugin を利用者が識別できる。
* 不足 Plugin に関する保存済み State を可能な限り保持し、後の再利用に備えられる。
* Missing Plugin が原因で Project 自体が破損扱いにならない。

---

## Mixer

### MIX-001 — Track Mix

同一 Track 上で同時刻に存在する複数 Clip の処理済み出力は、原則として加算して Track Mix を生成しなければならない。

Acceptance Criteria

* 重複 Clip の出力は線形加算される。
* Track Mix 段階で 0 dBFS を超える内部値を即座に hard clip しない。
* 内部 mixing は floating-point audio domain で扱える。
* 後段の Track FX および Track Gain / Pan に加算結果を渡せる。

---

### MIX-002 — Track Gain / Pan

利用者は Mixer から各 Track の Gain と Pan を調整できなければならない。

初期バージョンでは Gain / Pan Automation を Must としない。

Acceptance Criteria

* Track ごとに Gain の現在値を確認・変更できる。
* Track ごとに Pan の現在値を確認・変更できる。
* Pan の標準モデルは `-1.0 = Left`, `0.0 = Center`, `+1.0 = Right` とする。
* 値は Save / Load 後に復元される。
* 設定変更は他 Track の値を変更しない。
* Gain / Pan の変更は Undo / Redo できる。

---

### MIX-003 — 標準信号処理順序

Track Mixing 以降の標準処理順序は、以下の概念モデルを満たさなければならない。

`Source → Clip Processing Stack → Track Mix → Track FX → Track Gain / Pan → Master FX → Output`

Acceptance Criteria

* Clip Processing Stack は Track Mix より前に処理される。
* Track FX は Track Mix の後に処理される。
* Track Gain / Pan は Track FX の後に処理される。
* Master FX は各 Track の結果を統合した後に適用される。

---

## Project

### PROJECT-001 — Save / Load

利用者は Project を 1 つの論理的な Project File として保存し、同じ編集状態を再読込できなければならない。

内部形式は別途設計する。

Acceptance Criteria

* 利用者から見て 1 Project として保存・選択できる。
* Track、Clip、Clip Group、Synchronization Group、Media 参照、Processing Stack、Mixer、Fade、Plugin State を保存・復元できる。
* Save → Load 後の Clip Timeline 位置、Timeline 長、Source 使用範囲は保存前と一致する。
* Project Timeline Sample Rate が復元される。
* Project File へ Source Media を無条件に埋め込まない。

---

### PROJECT-002 — Undo / Redo

主要編集操作は Undo / Redo できなければならない。

Acceptance Criteria

少なくとも以下を Undo / Redo できる。

* Clip Move
* Trim
* Split
* Delete
* Ripple Delete
* Clip Group / Ungroup
* Synchronization Group の変更
* 個別同期バイパス編集
* Fade 変更
* Processor / Effect の追加
* Processor / Effect の削除
* Processor / Effect の並べ替え
* Track Gain / Pan の変更

Undo 後に Redo を行った場合、対象 Project 状態は編集直後と等価な状態へ戻る。

Undo / Redo は Source Media を変更しない。

---

### PROJECT-003 — Media Reference

Project は Source Media への参照を保存しなければならない。

Media の再識別には少なくとも path と file size を利用でき、optional hash を保持可能な構造とする。

Acceptance Criteria

* Media ごとに参照 path を保存できる。
* Media ごとに file size を保存できる。
* 必要に応じて hash 等の追加識別情報を保存できる。
* 相対 path を使用可能な場合に保持できる設計を許容する。
* path 変更のみを理由に Media 内容自体を Project へ複製しない。

---

### PROJECT-004 — Missing Media

Source Media が欠落している場合でも Project を可能な範囲で読み込めなければならない。

Acceptance Criteria

* Missing Media が存在しても Project 自体を開ける。
* 欠落した Media を識別できる。
* Missing Media を参照する Clip を識別できる。
* 他の正常な Media、Track、Clip、Effect、Project 情報は失われない。
* Missing Media を含む状態でも Project を再保存できる。

---

## Playback

### PLAY-001 — Timeline Playback

利用者は Timeline の指定位置から音声を再生・停止できなければならない。

Acceptance Criteria

* 再生開始位置は Project Timeline 上の位置に対応する。
* 再生中は Timeline 上の Clip、Processing Stack、Track Mix、Track FX、Gain / Pan、Master FX が反映される。
* 再生停止後に再び再生できる。
* Seek 後に新しい位置から再生できる。

---

### PLAY-002 — 即応性の高い Preview

Podcast / long-form editing において操作ストレスを抑えるため、再生開始および Seek 後の Preview は可能な限り即応的でなければならない。

超低遅延のライブ演奏用途は対象としないが、編集操作に対する Preview latency は重要な性能要件として扱う。

Acceptance Criteria

* 再生開始 latency を性能計測可能な指標として定義する。
* Seek 後の音声再開 latency を性能計測可能な指標として定義する。
* 具体的な目標値は性能計画で確定する。
* 長尺 Media であることのみを理由に Source 全体の事前 decode 完了を再生開始条件としない。

---

## Performance / Resource Use

### PERF-001 — 長尺・複数 Track・多数 Clip を通常ケースとして扱う

数時間規模の Media、複数 Track、多数 Clip を含む Project を通常の編集対象として扱わなければならない。

Acceptance Criteria

* 代表 Benchmark Project を性能計画で定義する。
* Benchmark Project に対して Open、Timeline 表示、Move、Trim、Split、Scroll、Zoom、Playback を実行できる。
* 操作のたびに Source Media 全体を再 decode しない。
* Source Media 全体の PCM 展開を Project 利用の必須条件としない。
* 応答時間・メモリ使用量の具体的目標は別途性能計画で確定する。

---

### PERF-002 — Waveform 表示

Timeline は長尺 Media の Waveform を表示し、Scroll / Zoom に応じて表示内容を更新できなければならない。

Acceptance Criteria

* 表示中 Timeline 範囲に対応する Waveform を描画できる。
* Zoom 変更後に適切な Waveform 表示解像度へ更新できる。
* Waveform 表示のために Source Media 全体の PCM を常時 RAM に保持しない。
* Waveform 表示データを再利用可能な構造を許容する。
* Waveform cache の具体形式は設計段階で決定する。

---

### PERF-003 — Source Media の不要コピー禁止

Source Media を Project 内へ不要に複製してはならない。

Acceptance Criteria

* Media Import により Source Media の完全コピーを Project データとして自動生成しない。
* Clip の Split、Trim、Move、複製等によって Source PCM ファイルを複製しない。
* 多数 Clip を作成しても、Source Media 本体のコピー数は増加しない。

---

### PERF-004 — 無条件 RAM 展開の禁止

長尺 Source Media 全体を無条件に RAM へ展開してはならない。

Acceptance Criteria

* Project Open 時に全 Media の完全 PCM buffer を必須生成しない。
* Waveform 初回表示時に長尺 Media 全体の完全 PCM buffer を常駐させない。
* Playback が必要範囲を段階的に取得可能な設計を許容する。

---

## Export

### EXPORT-001 — WAV Export

利用者は Master Output を WAV として書き出せなければならない。

Acceptance Criteria

* 出力先を指定できる。
* 書き出し後、読み取り可能な WAV ファイルが生成される。
* Timeline、Clip Processing、Tail、Track Mix、Track FX、Gain / Pan、Master FX が反映される。

---

### EXPORT-002 — MP3 Export

利用者は Master Output を MP3 として書き出せなければならない。

Acceptance Criteria

* 出力先を指定できる。
* bitrate を選択できる。
* 書き出し後、読み取り可能な MP3 ファイルが生成される。
* Timeline 上の完成音声処理結果が反映される。

---

### EXPORT-003 — AAC (M4A) Export

利用者は Master Output を AAC 音声を含む M4A として書き出せなければならない。

Acceptance Criteria

* 出力先を指定できる。
* bitrate を選択できる。
* 書き出し後、読み取り可能な M4A ファイルが生成される。
* Timeline 上の完成音声処理結果が反映される。

---

### EXPORT-004 — Sample Rate / Channel Configuration

利用者は Export 時の Sample Rate と Channel Configuration を選択できなければならない。

初期バージョンの Channel Configuration は Mono / Stereo とする。

Acceptance Criteria

* Export 時に Sample Rate を選択できる。
* Mono または Stereo を選択できる。
* 出力ファイルの Sample Rate は選択値と一致する。
* 出力ファイルの Channel Configuration は選択値と一致する。
* 出力形式の制約により変換が必要な場合、その変換内容を利用者に明示できる。
* 利用者に知らせず暗黙に異なる設定へ変更しない。

---

## Real-time Audio Safety

### RT-001 — Audio Processing と非リアルタイム処理の分離

Audio Processing 経路は、UI、Project 保存、Waveform 生成、Plugin Scan 等の非リアルタイム処理と責務を分離しなければならない。

Acceptance Criteria

* Audio Processing と UI / File Access の責務境界を設計上識別できる。
* Audio Processing 経路内で Project 保存を行わない。
* Audio Processing 経路内で Waveform 生成を行わない。
* Audio Processing 経路内で Plugin Scan を行わない。
* Audio Processing 経路内でネットワーク処理を行わない。

---

### RT-002 — Blocking Operation の回避

リアルタイム Audio Processing 経路は、予測不能な長時間 blocking operation に依存してはならない。

Acceptance Criteria

* 長時間保持される mutex wait を Audio callback の通常処理経路に置かない。
* File I/O を Audio callback の通常処理経路で直接行わない。
* 不要な dynamic allocation を Audio callback の定常処理経路で繰り返さない構造を優先する。
* 詳細な Threading Model は実装前に設計文書で定義する。

---

# Should

## MEDIA-101 — Media Metadata 表示

Media Bin で Media の基本情報を確認できる。

例:

* duration
* native sample rate
* channel count
* codec
* container
* audio stream information

---

## PROJECT-101 — Media Relink

Missing Media の参照先を利用者が再指定できる。

再リンク時には path、file size、optional hash 等を利用できる。

---

## PROJECT-102 — Autosave / Crash Recovery

Project の定期 Autosave または異常終了後の Recovery を提供する。

通常の Project File を破損させない方式を優先する。

---

## EDIT-101 — 編集状態の視覚的区別

以下を Timeline 上で視覚的に区別できる。

* Selection
* Clip Group
* Synchronization Group
* Track Lock
* Snapping
* 一時的な同期保護 bypass 状態

---

## EDIT-102 — 複数 Clip の一括編集

選択した複数 Clip をまとめて Move、Trim、Delete 等できる。

---

## MIX-101 — Mute / Solo

Track の Mute / Solo を Mixer から操作できる。

---

## FX-101 — VST3 Parameter Automation

VST3 Parameter を Timeline 上で Automation できる。

初期 Must では静的 Parameter State のみを要求する。

---

## MIX-102 — Track Gain / Pan Automation

Track Gain / Pan を Timeline 上で Automation できる。

---

## FX-102 — Effect Tail 手動設定

自動 Tail 長が適切でない場合、利用者が Clip Processing Tail の長さまたは policy を手動指定できる。

想定 policy 例:

* Automatic
* Manual Duration
* Cut at Clip End

---

## FX-103 — Plugin Crash Isolation

VST3 の異常終了が AudioNLE 本体全体の異常終了に直結しにくい Plugin isolation / sandboxing を提供する。

---

## FX-104 — Built-in Basic Effects

外部 VST3 がなくても基本的な spoken-word 処理を行える Built-in Effect を提供する。

候補:

* EQ
* Compressor
* Limiter

必要に応じて De-esser 等を検討する。

---

## EXPORT-101 — Opus Export

Master Output を Opus として書き出せる。

---

## EXPORT-102 — Export Range

Project 全体または指定 Timeline 範囲のみを書き出せる。

---

# Could

## MEDIA-201 — 映像参照表示

動画コンテナ内の音声を編集する際、同期・内容確認のための映像 Preview を表示できる。

映像編集そのものは行わない。

---

## MEDIA-202 — 自動同期支援

複数音声ファイルについて、metadata、timecode、波形等を利用した同期候補を提示する。

同期結果は利用者が確認可能であることを前提とする。

---

## EDIT-201 — Marker / Comment

Timeline 上に Marker や Comment を配置できる。

---

## PERF-201 — Waveform Cache 事前生成

長尺 Project 向けに Waveform Cache を明示的に事前生成できる。

---

## FX-201 — Processing Stack Preset

spoken-word 向け Processing Stack の Preset を保存・適用できる。

---

## EXPORT-201 — Export Preset

Sample Rate、Channel、bitrate 等の Export 設定を Preset として保存できる。

---

## CTRL-201 — 単純な外部コントローラ

再生、停止、Seek 等を補助する単純な外部コントローラに対応できる。

---

# Won't — 初期バージョンで扱わない

## NOGO-001

MIDI 編集、ピアノロール、ソフトウェア音源、シンセサイザー、楽譜、MIDI CC、Step Sequencer。

## NOGO-002

テンポ、拍、拍子、小節を主軸とする音楽制作および演奏・打ち込み向けワークフロー。

## NOGO-003

高度なライブ演奏・ライブ録音、低遅延演奏監視、演奏向け Punch In、複雑な Cue 系統。

## NOGO-004

映像編集、映像カット、合成、テロップ、映像エフェクト、カラーグレーディング、映像書き出し。

## NOGO-005

高度な外部機器制御や汎用制作環境統合。

## NOGO-006

高度な Surround および Dolby Atmos 等の immersive audio workflow。

## NOGO-007

クラウド共同編集およびオンライン Project 同期。

## NOGO-008

AI 文字起こし、AI 編集、自動話者分離。

## NOGO-009

高度なノイズ除去、残響除去、音声修復アルゴリズムの AudioNLE 本体への内蔵。

外部 VST3 等による処理はこの非目標に含まない。

## NOGO-010

録音機能。

## NOGO-011

macOS / Linux の正式サポート。

設計上の移植性は不必要に損なわない。

## NOGO-012

VST3 以外の Plugin Format。

---

# 要件間の依存関係

* MEDIA-001〜005 は Timeline 編集および Project 永続化の基礎となる。
* TIME-001〜003 は EDIT、SYNC、Playback、Export の時間表現の基礎となる。
* Clip Group と Synchronization Group は独立概念として扱う。
* EDIT-007 は編集上の任意 Group を扱う。
* SYNC-001〜004 は時間関係保護のための Synchronization Group を扱う。
* PROC-001〜003 は Clip FX、Gain、Fade 等の順序付き処理モデルの基礎となる。
* FX-002、FX-003 は VST3 利用と保存の基礎となる。
* FX-004 は Plugin Latency Compensation を扱う。
* FX-005 は stateful Effect の Tail 処理を扱う。
* MIX-001 は重なった Clip を Track 上で加算する。
* MIX-003 は Clip Processing から Master Output までの標準処理順を定義する。
* PROJECT-001 は全主要 Project 状態の永続化を担う。
* PROJECT-002 は主要編集操作の Undo / Redo を横断的に要求する。
* PLAY-001 / PLAY-002 は長尺音声編集時の Preview UX の基礎となる。
* PERF-001〜004 と RT-001〜002 は各主要機能に対する横断的制約である。
* EXPORT-001〜004 は Timeline、Processing、Mixer の最終結果を出力する。

---

# 未決定事項

以下は Architecture / Editing Model / Performance Plan 等を確定する前に別途決定する。

* 対応する入力 codec、container、bit depth、channel 数の詳細。
* Project Timeline Sample Rate を新規 Project 作成後に変更可能とするか。
* Source sample domain と Timeline sample domain 間の端数・rounding policy。
* Benchmark Project の具体的な duration、Track 数、Clip 数。
* Playback start latency の具体目標値。
* Seek latency の具体目標値。
* Scroll / Zoom の応答時間目標。
* 最大または推奨メモリ使用量。
* Snapping の対象、許容範囲、優先順位。
* Ripple Delete の詳細 scope 決定ルール。
* 複数選択範囲と Ripple 対象範囲が異なる場合の semantics。
* Fade curve の種類。
* Clip が複数重複した場合の UI 表現。
* Processing Stack における Built-in Gain / Fade の初期配置位置。
* Built-in Processor を完全に任意位置へ移動可能とするか、一部制約を設けるか。
* Effect Tail が infinite / unknown の場合の Automatic fallback。
* VST3 Scan の方式。
* VST3 UI の表示方式。
* Plugin isolation の方式。
* Project File の内部形式。
* Project File の versioning / migration policy。
* Media Relink 時の hash の種類と生成タイミング。
* Autosave の周期と保存方式。
* WAV の bit depth 選択。
* MP3 / AAC の bitrate 候補。
* AAC (M4A) Export に使用する codec / encoder の詳細。
* Export Sample Rate の候補値。
* Undo History の上限、保存範囲、Project 再起動後の扱い。
* Built-in EQ / Compressor / Limiter の具体仕様。
* Project 内部 audio processing precision を 32-bit float とするか 64-bit float とするか。

---

# Vision / Non-goals との整合性レビュー

本書の Must 要件は、長尺・マルチトラック音声を動画編集ソフトに近い Timeline 操作で非破壊編集し、Mixer と Processing Stack を通して完成音声へ仕上げるという AudioNLE の Vision を具体化している。

Media の入力形式は編集の入口として扱い、特定の録音ソフトやコンテナ形式を製品ドメインの中心に置いていない。

Clip Group は任意の編集単位をまとめる概念、Synchronization Group は時間関係を保護する概念として明確に分離する。

Clip FX、Track FX、Master FX、および順序変更可能な Processing Stack は、音楽制作 DAW の汎用性を再現するものではなく、spoken-word および長尺音声を完成品質へ仕上げるための処理として位置付ける。

VST3 Parameter Automation、Track Gain / Pan Automation、Plugin Crash Isolation、Built-in Basic Effects、Autosave 等は重要ではあるが、初期編集モデル成立の必須条件とはせず Should とする。

映像は入力コンテナや将来的な参照表示としてのみ扱い、映像編集機能には拡張しない。

MIDI、ソフト音源、テンポ・小節ベースの作曲機能、ライブ演奏用途、クラウド共同編集、高度な AI 機能、録音、イマーシブ音声等を初期スコープから明示的に除外し、AudioNLE の中心価値である軽快で予測可能な audio-only NLE の実現を優先する。
