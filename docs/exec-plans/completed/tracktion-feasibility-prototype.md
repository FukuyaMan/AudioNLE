# Execution Plan: Tracktion Feasibility Prototype

> Historical plan. ADR 0001 selected Option B and rejected Tracktion as the production backend. The executable prototype was removed from current `main`; this plan and its evidence are retained, and the implementation is recoverable from Git history.

## 1. Objective

`docs/design/prototypes/tracktion-feasibility.md` の試験を、再現可能かつ段階的に実装・記録する。本計画の目的は Tracktion Engine の採用ではなく、AudioNLE の primary audio engine 候補として不適格なら早期に `Reject` できる証拠を得ることである。

最終結論は `Proceed`、`Proceed with Constraints`、`Reject`、`Inconclusive` のいずれでもよい。実装結果を得るまで ADR は作成しない。

## 2. Inputs / References

* `AGENTS.md` — 非破壊編集、整数 sample、同期、長尺、real-time safety、domain separation。
* `docs/product/requirements.md` — TIME、EDIT、SYNC、PROC / FX、MIX、PLAY、PERF、PROJECT の Must。
* `docs/product/editing-model.md` — Domain state、Processing End、Tail Policy、Ripple、Synchronization Member の意味。
* `docs/product/terminology.md` — 正式用語。特に Processing Stack、Clip Processing Output、Track Mix、Master Output を用いる。
* `docs/design/technical-options.md` — Tracktion は prototype 合格を条件とする候補であること。
* `docs/design/prototypes/tracktion-feasibility.md` — 仮説、fixture、hard rejection rule、結果文書形式。

## 3. Scope

prototype 専用の headless runtime / test harness を、production AudioNLE application と分離して作る。対象は次に限る。

* 1 Project Timeline Sample Rate と framework-free な整数 Timeline sample state。
* WAV source、1〜数 Clip Track、Clip placement、overlap、seek。
* Clip / Track / Master Processing Stack、Track Gain / Pan。
* deterministic processor、必要になった段階で VST3 test plugin。
* runtime reconstruction、offline render、headless test、長尺・dense edit 計測。

## 4. Non-goals

* production GUI、Media Bin、Inspector、Mixer UI、waveform UI、shortcut。
* production domain API、Project file format、autosave、crash recovery。
* FFmpeg integration、MP4 / MKV / MOV、MP3 / AAC export、waveform cache。
* complete plugin scan UX、sandbox、full synchronization editing、full Ripple UI。
* macOS / Linux support、production performance optimization、production architecture の決定。

本 prototype 専用 workaround を production architecture の既定にしない。

## 5. Assumptions

* V1 の Project Timeline Sample Rate の既定は 48 kHz。Timeline Position / Duration は整数 sample が正規値である。
* Source Time Domain と Timeline Time Domain は別であり、44.1 kHz source / 48 kHz Project を扱う。SRC quality は本計画の評価対象外である。
* AudioNLE Domain state は authoritative で、Tracktion runtime は派生状態である。
* Tracktion / JUCE / VST3 の型、runtime pointer、runtime ID、serialization blob は Minimal Domain State に入れない。
* GPLv3 互換性は rejection reason にしない。ただし各依存の正確な license、NOTICE、再配布条件は記録・確認する。
* performance の絶対閾値が未決定の項目は baseline を取る。ただし全 PCM 常駐、全 decode、極端な rebuild のような構造的 rejection signal は判定する。

## 6. Prototype Architecture Boundary

```text
Test / Prototype Driver
↓
AudioNLE Minimal Domain State
↓
Tracktion Adapter
↓
Tracktion Engine / JUCE runtime
```

### Minimal Domain State

概念上、次だけを表す最小 representation を最初に作る。

```text
ProjectTimelineSampleRate
TrackId
ClipId
MediaId
TimelineStartSamples
TimelineDurationSamples
SourceStartSamples
SourceDurationSamples
ProcessorId
ProcessorOrder
ProcessorParameters
TrackGain
TrackPan
```

これは production model の最終 API ではない。次を含めない。

* Tracktion / JUCE object または time type。
* runtime pointer、Tracktion Edit / Track / Clip、Tracktion serialization。
* plugin runtime instance。

### Project / Transport State の分離

Project / Editing State は Clip position、Source range、Track association、Processor order、Gain / Pan を持つ。Transient Transport State は playhead position、playing / stopped、optional loop state のみを持つ。Transport State は Project persistence の authoritative state と同一視しない。

### Formal Processing Flow

prototype の正式 signal flow は次で統一する。

```text
SourceReference
↓
Clip Processing Stack
↓
Clip Processing Output
↓
Track Mix
↓
Track Processing Stack
↓
Track Gain / Pan
↓
Master Processing Stack
↓
Master Output
```

`Effect Chain`、`Track FX chain`、`Master FX chain` を prototype の正式概念として使わない。

### 一方向更新

変更経路は `Domain state change → Adapter update → runtime` だけを基準とする。runtime の編集結果を Domain へ逆同期する構造は採らない。Synchronization Group / Member、Clip Group、Ripple は adapter の入力へ含めず、将来 Domain が解決した Clip / Processor state を与えられることだけを確認する。

## 7. Dependency Plan

### Phase 0 — Discovery and pinning

いかなる dependency も導入せず、次を調査して implementation log に固定する。

* Tracktion Engine の exact tag / commit と license。
* Tracktion が要求する JUCE exact revision、C++ standard、MSVC / Windows 11 x64 要件。
* bundled dependency の license、NOTICE / attribution、再配布上の注記。
* VST3 test plugin を使う場合の SDK / test plugin license と必要 dependency。
* headless build の可否と、dependency retrieval の候補（submodule、FetchContent、vendoring 等）。

floating `main`、unpinned dependency、未記録の binary artifact は使用しない。Phase 0 の出力は後続 results の Dependency revisions / license record に転記できる形式にする。法的結論は出さず、不明点を明記する。

### Phase 1 — Build bootstrap

Phase 0 の pin が承認・記録された後、prototype 専用 build boundary で最小 headless executable / test executable を build する。

* **Pass:** clean checkout から同じ手順で build できる、exact revision が出力される、headless executable が physical audio device なしで起動・終了する。
* **Stop:** headless bootstrap が合理的な範囲で再現できない場合、H10 の early investigation として記録する。

本 Phase では GUI application、audio device、plugin scan、編集機能を追加しない。

## 8. Directory / Artifact Plan

後続 implementation では production source と混ぜず、次の配置を第一候補とする。既存 repository structure が判明した時点で同じ分離原則を満たす最小限の調整をしてよい。

```text
prototype/
  tracktion-feasibility/
    src/                 # adapter と headless driver
    tests/               # scenario assertions
    fixtures/            # 小さい source と fixture definition
    README.md            # configure / build / test / benchmark 手順

benchmark-results/
  tracktion-feasibility/ # raw measurement と environment

docs/design/prototypes/
  tracktion-feasibility-results.md
```

この Execution Plan は上記ディレクトリを作成しない。prototype-specific include、dependency、build target は `prototype/tracktion-feasibility/` の外へ漏らさない。production code への変更は別タスク・別レビューとする。

## 9. Fixture Plan

fixture は deterministic、small、生成可能、source rate と期待結果が文書化されていることを条件とする。

| ID | Fixture | 用途 | 期待 |
| --- | --- | --- | --- |
| F-001 | Impulse | PDC、timeline timing、overlap、clipping。 | sample position を一意に観測できる。 |
| F-002 | Constant | sum、Gain、Pan、Mix。 | 線形加算と level を検証できる。 |
| F-003 | Order-sensitive Processor | Gain 前後で確実に異なる処理。 | Processor order / reorder の差が観測できる。 |
| F-004 | Latency Processor | fixed 1024 sample latency を report。 | PDC alignment を測れる。 |
| F-005 | Tail Processor | 1 秒 source、finite 2 秒 Tail。 | Source End / Processing End / Tail Move を測れる。 |
| F-006 | Sample-rate WAV | 44.1 kHz source、48 kHz Project。 | domain 分離と integer round-trip。 |
| F-007 | Long source generated state | 3 hours、8 Track、50 Clip。 | duration / seek / memory scaling。 |
| F-008 | Dense edit generated state | 30 minutes、8 Track、5000 Clip。 | Clip count / rebuild / scheduling scaling。 |
| F-009 | Failure fixture | missing source、missing plugin / load failure。 | Domain state の保持。 |

VST3 test plugin は Phase E 以降の追加候補であり、初期の PDC / Tail test を plugin scan に依存させない。repository に巨大 PCM を置かず、F-007 / F-008 は生成または小素材の繰返し参照で作る。

## 10. Implementation Phases

各 Phase は前 Phase の Pass を前提にする。Fail / hard stop の場合は、原因特定の限定再試験を除き後続の無関係な Phase を開始しない。

### Phase A — Framework boundary and reconstruction

* **実装:** framework-free Minimal Domain State、Transient Transport State、最小 Tracktion Adapter、runtime construct / destroy / reconstruct。
* **試験:** H1 / H2。Domain fixture → runtime → offline render / timing observation → runtime destroy → same fixture から rebuild → 再比較。
* **Pass:** Domain に framework type / runtime identity がなく、rebuild 前後で Timeline / Source sample、processor order、Gain / Pan、Transport setup、deterministic output が等価。
* **Hard stop:** Tracktion object graph、Tracktion serialization、runtime identity、reverse synchronization が必要。

### Phase B — Integer Timeline Sample

* **実装:** Domain integer sample と runtime mapping / observation の最小経路。
* **試験:** H3。48 kHz Project の 0、1、48000、任意位置、長尺近傍を含み、F-006 の 44.1 kHz source / 48 kHz Project を追加。
* **Pass:** Domain Timeline Position / Duration は mapping / reconstruction 前後で bit-for-bit 不変。
* **Hard stop:** runtime seconds / floating representation を Domain の正規値にする必要がある、または integer state が変化する。

### Phase C — Basic Signal Flow

* **実装:** 1 Clip、複数 Clip、overlap、Clip / Track / Master Processing Stack、Track Gain / Pan の adapter mapping。
* **試験:** F-001 / F-002 で Track Mix を比較。2 Clip と 3 Clip overlap を含める。
* **Pass:** Clip Processing Output の線形加算が期待 sample と一致し、Track Mix で即時 hard clipping を観測しない。
* **Stop:** 正式 signal flow を表せない場合、H5 Fail として Phase D 以降を停止する。

### Phase D — Processing Stack order

* **実装:** F-003 の Processor mapping と reorder update。
* **試験:** `Gain → deterministic Effect` と `Effect → Gain`、停止中 / playback 中の reorder。
* **Pass:** Domain ProcessorOrder が runtime order に反映され、期待どおり異なる出力を得る。runtime から Domain への逆同期を要しない。
* **記録:** update strategy、full / track / clip-level update、latency、glitch、audio-thread blocking、determinism。

### Phase E — PDC

* **実装:** F-004 の fixed 1024 sample Latency Processor。必要時のみ deterministic VST3 test plugin を追加。
* **試験:** Clip / Track / Master Processing Stack の latency を区別し、bypass、latency change、reorder、offline render を測る。
* **Pass:** 正しい latency report を前提に全ケースで `alignment error = 0 timeline samples`。
* **Hard stop:** 合理的な adapter で 0 sample を達成できない、または非決定的。

### Phase F — Effect Tail

* **実装:** F-005 と Tail Policy を観測できる adapter path。
* **試験:** Source End、silence input 後の Tail、downstream processing、later Clip overlap、offline render、`Reverb → Fade` と `Fade → Reverb`、Clip Move。
* **Tail Move case:** source output start = 48000、Processing End = 192000 を `+48000` Move した後、source output position、Tail position、Processing End がすべて `+48000` されること。Tail が orphan runtime state として残らないこと。
* **Pass:** expected sample range の Tail が Processing End まで存在し、downstream / overlap / offline output に反映される。
* **Hard stop:** Source End で Tail が強制切断され、Tracktion fork / major patch が必要。

### Phase G — Runtime editing

* **実装:** Domain → Adapter の一方向 update で Move、Trim、Split、Delete、overlap creation、Processor reorder、Gain change。
* **試験:** 各 operation を Stopped、Paused または equivalent state、Playing に分ける。
* **記録:** operation、runtime update strategy、full rebuild required、track rebuild required、clip-level update possible、update latency、glitch、audio-thread blocking、reverse synchronization required。
* **判定:** Stopped で高速な full rebuild は即 Fail ではない。Playing 中の full rebuild は実測値と UX 制約から Conditional Pass / Fail を判定する。長尺 Project 全体の重い rebuild が編集ごとの必須条件なら rejection signal。

### Phase H — Long-form

* **Long source:** F-007（3 hours、8 Track、50 Clip）で source duration scaling、seek、memory、streaming を測る。
* **Dense edit:** F-008（30 minutes、8 Track、5000 Clip）で Clip count scaling、construction、update cost、scheduling overhead を測る。
* **測定:** construction time、steady / peak memory、playback start、seek resume、repeated seek、stopped / playing edit update、offline throughput。
**Rejection signal:** 全 PCM resident、playback 前全 decode、seek ごとの全 decode、測定で裏付けられた極端に重い full rebuild、非実用的な Clip count scaling。

### Phase I — Headless / failure injection

* **実装:** command-line automation、F-009 failure injection、runtime destroy / reconstruct。
* **試験:** GUI、plugin editor、physical audio device なしに Phase A〜F の重要 test を実行。missing source / plugin、load failure、processing failure の後も Domain state を保持する。
* **Pass:** CI-style に再現可能で、runtime failure が Clip / Processor identity、Missing state、Domain state を破壊しない。

## 11. Test Matrix

| Test | 仮説 | Phase | Fixture | Pass criterion | Stop impact |
| --- | --- | --- | --- | --- | --- |
| T-001 reconstruction | H1, H2 | A | F-001, F-002 | rebuild 前後の state / render / timing 等価。 | Hard stop candidate |
| T-002 sample round-trip | H3 | B | F-006 | integer Timeline sample が不変。 | Hard stop candidate |
| T-003 overlap / mix | H5 | C | F-001, F-002 | linear sum、no immediate clip。 | H5 Fail |
| T-004 stack order | H5, H8 | D | F-003 | order 差と reorder を反映。 | Conditional / Fail |
| T-005 PDC | H6, H9 | E | F-004 | 0 timeline sample error。 | Hard stop candidate |
| T-006 tail | H7, H9 | F | F-005 | expected Tail range、move、overlap、render。 | Hard stop candidate |
| T-007 editing | H8 | G | F-001–005 | Domain-first update の可否。 | Conditional / Reject signal |
| T-008 long source | H4 | H | F-007 | baseline と streaming / seek evidence。 | Reject signal |
| T-009 dense edit | H4, H8 | H | F-008 | Clip scaling evidence。 | Reject signal |
| T-010 headless / failure | H10 | I | F-009 | automated run と Domain state 保存。 | Reject signal |

## 12. Measurement Plan

### Signal and timing

* PDC は Timeline sample 単位で比較する。reported latency = 1024 の expected result は alignment error = 0 timeline samples。
* Tail は Source End、first Tail sample、last non-zero Tail sample、Processing End、reported Tail、observed Tail、offline render end、downstream output、later Clip overlap を記録する。
* overlap は expected linear sample sum、Processor order は expected distinct output と比較する。

### Performance and safety

各 benchmark で environment、fixture、iteration、中央値、最大値を残す。測定値は runtime construction time、steady / peak memory、playback start latency、seek resume latency、repeated seek、stopped / playing update latency、offline render throughput、Clip count scaling を含む。

audio path に file I/O、network I/O、blocking mutex、plugin scan、waveform generation、project serialization、unbounded allocation が入らないことを点検・記録する。性能閾値が未決定なら baseline と構造的 rejection signal を記録し、「高速」だけで判定しない。

## 13. Pass / Fail Recording

各 test は次の形式で raw result と要約を残す。

```text
Test ID:
Hypothesis:
Environment:
Fixture:
Expected:
Observed:
Measurement:
Status: Pass | Conditional Pass | Fail | Inconclusive
Workaround:
Requirement impact:
Notes:
```

raw result は `benchmark-results/tracktion-feasibility/` に、要約は `docs/design/prototypes/tracktion-feasibility-results.md` に残す。Pass は workaround なし、Conditional Pass は明示的な V1 制約または局所 adapter、Fail は合理的な adapter で補えない不適合、Inconclusive は限定再試験が必要な場合とする。

## 14. Stop Conditions

次は early stop の hard stop candidate である。

1. H1: Tracktion Edit / runtime identity / serialization を authoritative Project state にしないと成立しない。
2. H3: integer Timeline sample model を保てない。
3. H6: 合理的な adapter で PDC 0 sample alignment を達成できない。
4. H7: Tail が Source End で切断され、回避に Tracktion fork / major patch が必要。

hard stop 到達時は後続の性能最適化や GUI を実施せず、原因を限定する再試験だけを行い、結果文書と `Reject` / `Inconclusive` の判断へ進む。H4、H8、H10 の重大 Fail も長尺 V1 を成立させない場合は Reject とする。

## 15. Risks

| Risk | 緩和策 / 記録 |
| --- | --- |
| Tracktion API / runtime semantics の誤読 | exact revision、最小 fixture、observation、source / documentation reference を残す。 |
| prototype が production application 化する | Phase scope と Non-goals を守り、UI / import / persistence を追加しない。 |
| plugin scan が PDC / Tail test を止める | deterministic processor を先行し、VST3 plugin は後段で追加。 |
| 長尺 fixture が repository を肥大化 | generated / sparse / repeated-source state を使用。 |
| performance 数値を環境差で誤判定 | environment、iteration、raw data、baseline、構造的 signal を併記。 |
| dependency が production に漏れる | prototype-only build boundary と leakage review。 |
| legal detail の未確認 | revision / license / NOTICE record を残し、結論を保留する。 |

## 16. Expected Outputs

implementation 完了時には、次を残す。

* prototype-only headless driver / tests / deterministic fixture definitions。
* exact dependency revisions、retrieval method、license / NOTICE record。
* reproduce 手順（configure、build、test、benchmark）。
* raw benchmark / signal measurement。
* framework leakage review と Tracktion-specific workaround 一覧。
* `docs/design/prototypes/tracktion-feasibility-results.md`。
* Proceed / Proceed with Constraints / Reject / Inconclusive を選べる evidence。

## 17. Completion Criteria

後続 implementation が完了とみなせるのは、次を満たすときである。

* required test matrix が実行され、各行に Pass / Conditional Pass / Fail / Inconclusive がある。
* exact dependency revisions、benchmark environment、raw measurements が保存されている。
* H1、H3、H6、H7 と long-form / headless の結論が明示されている。
* Domain leakage review が完了し、Tracktion workaround が列挙されている。
* results 文書が作成され、結論と requirement impact を説明している。
* production source へ prototype dependency / workaround が混入していない。

## 18. ADR Handoff

本計画の完了は ADR の自動作成を意味しない。まず results 文書をレビューする。

ADR 候補として進められるのは、結果が `Proceed` または `Proceed with Constraints` であり、hard criterion に未解決 Fail がなく、Domain boundary が維持され、Tracktion fork が必須でなく、adapter complexity が許容範囲で、long-form workflow に重大障害がない場合だけである。

`Reject` なら Audio Engine ADR を Tracktion 採用として作成しない。`technical-options.md` の Option B 系を、Reject の根拠（domain boundary、PDC、Tail、streaming、runtime editing、headless test）に絞って再評価する。

## 19. Cleanup / Separation Policy

prototype が Reject または不要になった場合、production へ移植する前提の refactor を行わない。prototype code、dependency、fixture、benchmark artifact は production build / distribution から分離する。結果文書と再現手順は残し、不要な実行 artifact の保管・削除方針は結果レビュー時に決める。

## 20. 自己レビュー

* この計画は implementation、dependency 導入、CMake / source / fixture 作成を行っていない。
* Tracktion を採用前提にせず、hard stop と Reject / Inconclusive を定義した。
* Minimal Domain State は framework-free で、Project State と Transport State を分離した。
* 正式用語として Processing Stack、Clip Processing Output、Track Mix、Master Output を用いた。
* Domain → Adapter → Runtime の一方向性、runtime object leakage の禁止を明記した。
* long source と dense Clip を分け、Tail Move、Stopped / Paused / Playing update、PDC の sample 測定、Tail の sample range 測定を含めた。
* headless / automated test、pin した dependency revision、results 文書後の ADR handoff を定義した。
