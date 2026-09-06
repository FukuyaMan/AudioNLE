# Tracktion Engine Feasibility Prototype 計画

## 1. 役割

本書は、Tracktion Engine を AudioNLE の audio engine、playback、plugin、offline render 基盤の候補として進めてよいかを落とすための feasibility prototype 計画である。採用計画でも採用決定でもない。

Tracktion が AudioNLE の domain independence、整数 Timeline sample、長尺 streaming、Processing Stack、PDC、Effect Tail、offline render、headless test を合理的な adapter で満たせないなら、`Reject` を正当な結論とする。GPLv3 互換性は本 prototype の不採用理由にしない。ただし Tracktion、JUCE、bundled dependency の正確なライセンス、再配布条件、NOTICE は実測結果とは独立して確認対象に残す。

## 2. 目的・Scope・Non-goals

### 目的

* AudioNLE Domain state を authoritative に保ったまま、Tracktion runtime を構築・破棄・再構築できるかを測る。
* Timeline / Source domain の分離、sample-accurate PDC、Tail、overlap、offline render、long-form seek を検証する。
* runtime 編集の更新経路と、headless integration test の現実性を測る。

### Scope

headless harness 上で、1 Project Timeline Sample Rate、WAV source、1〜数 Clip Track、Clip placement / overlap、simple Gain / Fade、Track / Master processing、seek、playback、offline render、runtime reconstruction を扱う。VST3 test plugin が使える場合は使用し、使えない段階では deterministic test processor を先行して使う。

### Non-goals

* production GUI、Media Bin、Inspector、Mixer GUI、waveform UI、shortcut system。
* MP4 / MKV / MOV、FFmpeg、complete import、waveform cache、Project file format、autosave、crash recovery。
* MP3 / AAC export、plugin sandbox、complete VST3 scan UX、macOS / Linux support。
* full Synchronization editing、full Ripple UI、production optimization、production architecture の決定。

## 3. Prototype Boundary

```text
Test / Prototype Driver
↓
AudioNLE Minimal Domain State
↓
Tracktion Adapter
↓
Tracktion Engine / JUCE runtime
```

Minimal Domain State は framework-free とし、少なくとも Project Timeline Sample Rate、Track / Clip / Media identity、Timeline start / duration samples、Source start / duration samples、Processor identity / order / parameter state、Track Gain / Pan を表す。Tracktion / JUCE 型、runtime pointer、Tracktion serialization は含めない。

Synchronization Group / Member、Clip Group、Ripple は full implementation しない。ただし adapter がそれらの存在、Tracktion の grouping、または Tracktion edit semantics を必要としないことを確認する。これらは Domain で解決済みの Clip / Processor 状態として adapter に渡せばよい。

## 4. 仮説

| ID | 仮説 | 合否の中心 |
| --- | --- | --- |
| H1 | Domain Independence: Domain を正とし、Tracktion object graph を Project state / persistence にしない。 | runtime を破棄しても Domain state が残る。 |
| H2 | Runtime Reconstruction: Domain から Track、Clip placement、Source Range、overlap、Clip / Track / Master processing、Gain / Pan、processor state、playback positionを再構築できる。 | rebuild 後の signal / timing が等価。 |
| H3 | Integer Sample Timeline Compatibility: integer Timeline sample が Tracktion 時刻表現に侵食されない。 | round-trip で Domain の sample 値が不変。 |
| H4 | Long-form Suitability: 全 PCM 常駐なしに数時間・多数 Clip の playback / seek / update が可能。 | 全 decode / 巨大 RAM を要求しない。 |
| H5 | Processing Stack Mapping: Clip → Track Mix → Track → Gain/Pan → Master の順序を表せる。 | 順序 fixture が期待通り異なる。 |
| H6 | Plugin Delay Compensation: 正しい latency report がある VST3 の PDC を sample-accurate に扱える。 | alignment error = 0 timeline samples。 |
| H7 | Effect Tail: Source End 後の Tail、downstream 処理、overlap、offline render を扱える。 | Tail が Processing End まで残る。 |
| H8 | Dynamic Editing: Domain → Adapter の一方向更新で edit を安全・予測可能に反映できる。 | runtime edit の観測値を記録。 |
| H9 | Offline Render Equivalence: offline / playback が PDC、Tail、order、Gain / Pan で意味的に一致。 | deterministic signal の比較。 |
| H10 | Headless Testability: GUI / physical device なしに CI 向け integration test を実行できる。 | harness が automated で再実行可能。 |

## 5. State Ownership Test（H1 / H2）

### 手順

1. framework-free Domain state を fixture として生成する。
2. adapter が Domain state から Tracktion runtime を構築する。
3. 指定位置から offline render および可能なら test playback を実行し、結果と timing observation を記録する。
4. runtime を完全に破棄する。
5. 同一 Domain state から新しい runtime を構築する。
6. 同じ render / timing test を実行し、結果を比較する。

### Pass criteria

* Domain state に Tracktion / JUCE object、runtime ID、runtime pointer、Tracktion serialization が含まれない。
* rebuild 前後で Clip の Timeline / Source sample state、processor order、Gain / Pan、playback position が同一である。
* deterministic fixture の render 結果、PDC alignment、Processing End が等価である。
* Synchronization Group / Member、Clip Group、Ripple の状態を runtime に保存・逆同期しなくても reconstruction が成立する。

rebuild 後に runtime object graph の永続化または手作業での reverse synchronization が必要なら H1 / H2 は Fail とする。

## 6. Fixture 設計

| Fixture | 内容 | 用途 |
| --- | --- | --- |
| F-001 Impulse | single-sample impulse、一定値、silence。 | PDC、Tail、overlap、hard clipping の観測。 |
| F-002 Order | Gain 前後で結果が異なる deterministic Effect。 | Processing Stack order / reorder。 |
| F-003 Tail | 1 秒 source + 2 秒の deterministic delay / reverb-like tail。 | Source End、Processing End、downstream Fade、Export。 |
| F-004 PDC | 同じ event を Path A と Path B に送り、B が 1024 sample latency を報告。 | 0 sample alignment、bypass、reorder。 |
| F-005 Rate | 44.1 kHz WAV source と 48 kHz Project。 | Source / Timeline domain の非混同、round-trip。 |
| F-006 Long-form | 48 kHz、3 時間、8 Track、500+ Clip、overlap あり。 | construction、memory、seek、update、throughput。 |
| F-007 Missing | 欠落 source / plugin load failure を模擬。 | Domain state が runtime failure で破壊されないか。 |

F-006 は巨大 PCM を repository に置かない。sparse / generated source、短い deterministic source の繰返し参照、または test 実行時生成を使う。各 fixture は source rate、channel、duration、期待 render、期待 Timeline / Source sample state を文書化する。

## 7. Test Scenarios

### T-001 Integer sample round-trip（H3）

48 kHz Project の多数の Timeline Position / Duration（0、1、48000、任意の素数的な値、長尺終端近傍）を Domain に生成する。`Domain integer sample → Tracktion runtime representation → adapter observation` を通しても、Domain の正規値を runtime の秒 / floating-point 値で上書きしないことを確認する。

F-005 で 44.1 kHz Source / 48 kHz Project を含める。SRC quality は対象外であり、Source / Timeline 両 range と同一 Domain fixture からの再構築で整数 Timeline state が変化しないことを判定する。

* **Pass:** すべての Domain Timeline Position / Duration が bit-for-bit 同一。
* **Hard fail:** Tracktion time を Domain の正規保存値にしないと成立しない、または round-trip により整数 sample が変化する。

### T-002 Long-form construction / seek（H4）

F-006 に対し、runtime construction time、process memory、playback start latency、project start / middle / near end / rapid repeated seek の resume latency、edit update latency、offline throughput を記録する。decode / streaming のスレッド、allocation spike、blocking I/O の痕跡も観測する。

絶対閾値は未決定のため baseline を目的とする。ただし playback 前に Project または source 全体の decode を必要とする、source duration に比例する巨大 PCM resident を常時要求する、seek ごとに全 decode / 全 runtime rebuild を要求する、Clip 数増加で construction が極端に悪化する場合は rejection signal とする。

### T-003 Clip overlap / Track Mix（H5）

同一 Clip Track に F-001 の 2 Clip を overlap させる。期待出力は各 Clip Processing Output の線形和である。3 Clip overlap も追加し、0 dBFS を越える内部値が Track Mix で即時 hard clipping されないことを offline output または測定点で確認する。

* **Pass:** 各 sample が期待する加算値と一致し、hard clipping が観測されない。

### T-004 Processing Stack order / reorder（H5 / H8）

F-002 で `Source → Gain → Effect` と `Source → Effect → Gain` を Domain の Processor order として構築する。出力が異なること、および Domain order が runtime の処理順に反映されることを確認する。停止中と playback 中の Processor reorder を実行し、更新方式、audio safety、glitch、latency、determinism を記録する。

* **Pass:** 期待する 2 出力が区別され、reorder 後の output が新しい Domain order と一致する。
* **Conditional pass:** playback 中の更新に制約があるが、明示的で安全な update point により domain-first を保てる。
* **Fail:** runtime order を Domain へ逆同期しないと order を維持できない。

### T-005 PDC（H6 / H9）

F-004 の Path A（latency なし）と Path B（reported latency = 1024 samples）を同じ Timeline event に配置する。PDC 後の alignment error を Timeline sample で測る。plugin bypass、latency change、processor reorder、offline render でも繰返す。

* **Pass:** plugin が正しい latency を report する前提で、すべての指定ケースの `alignment error = 0 timeline samples`。
* **Hard fail:** PDC が未実装、非決定的、または合理的な adapter 補完なしに 0 sample を達成できない。

### T-006 Effect Tail（H7 / H9）

F-003 を使い、Source duration = 1 秒、Tail duration = 2 秒とする。Source End 後に silence input でも Tail が出ること、Tail が downstream Processor を通ること、後続 Clip と overlap して Track Mix で加算されること、offline render に Processing End まで含まれることを確認する。

次の順序も比較する。

```text
Reverb → Fade
Fade → Reverb
```

* **Pass:** Source End で Tail が切断されず、順序に応じた結果が出て、後続 Clip との加算と offline render に反映される。
* **Hard fail:** clip lifetime または render range が Tail を Source End で強制切断し、回避に大規模 fork / patch が必要。

### T-007 Runtime editing（H8）

停止中と playback 中に Clip Move、Trim、Split、Delete、overlap creation、Processor reorder、Gain change を実行する。各操作について `Domain state change → adapter update → runtime playback state` の一方向だけを許す。

結果表に、full rebuild / Track-level update / Clip-level update の別、update point、audio thread safety、glitch risk、latency、deterministic behavior、runtime から Domain への逆同期の有無を記録する。

* **Fail:** 編集ごとに長尺 Project 全体の重い rebuild が必要、または runtime を編集して Domain へ逆同期しないと状態を保てない。
* **Conditional pass:** full rebuild が停止中のみで十分高速、または playback 中に安全な明示的制約がある。

### T-008 Headless and missing state（H10）

GUI、plugin editor、physical audio device なしで T-001〜006 を automation する。F-007 の missing source、missing plugin、plugin load / processing failure を注入し、runtime failure 後も Domain state、Clip identity、processor identity / state、Missing state が保持されることを確認する。

* **Pass:** CI で再現可能な command / fixture / expected result を定義でき、failure が Domain state を破壊しない。
* **Fail:** GUI または物理 device が必須、または runtime failure が Project state を破壊する。

## 8. Runtime Rebuild Strategy の比較

| 方式 | prototype で測ること | 合否への影響 |
| --- | --- | --- |
| full rebuild | F-006 の construction、停止中 / playback 中の可否、resource release。 | reconstruction の基線。長尺 update ごとに重いなら不適。 |
| Track / Clip incremental update | update の局所性、PDC / Tail / order の再計算、glitch。 | 有利だが domain-first を壊さないこと。 |
| immutable playback snapshot 的再構築 | publish latency、旧 runtime の退役、安全な切替。 | RT safety に有利かを観察。 |
| hybrid | operation ごとの update strategy。 | V1 で合理的な複雑さかを比較。 |

production architecture は決めない。目的は Domain authoritative のまま現実的な更新経路が少なくとも一つ存在するかを判定することにある。

## 9. Tracktion-specific Object Leakage Check

prototype 完了時に Domain / Application layer を点検し、次を禁止する。

* Tracktion Edit を Project model または persistence として保存すること。
* Tracktion Clip ID を AudioNLE Clip identity として直接使うこと。
* Tracktion Track object を Domain Track として扱うこと。
* Tracktion time type を Domain Timeline Position / Duration として保存すること。
* plugin runtime instance を Project state として保存すること。
* Tracktion serialization を AudioNLE Project persistence とすること。

Tracktion / JUCE 型は Adapter の内部に閉じる。発見された漏出は H1 の Fail または、局所化可能性を根拠にした Conditional Pass として記録する。

## 10. 観測・計測方法

| 領域 | 必須 observation |
| --- | --- |
| timing | Domain Timeline sample、runtime mapping、PDC alignment error、Processing End。 |
| resource | construction / update / seek / render の時間、peak / steady memory、allocation spike。 |
| streaming | source 全 decode の有無、seek 時の read / decode 範囲、blocking I/O。 |
| signal | sample 比較、加算値、Tail 範囲、Processor order の差、Gain / Pan。 |
| safety | audio path の file I/O、blocking、scan、waveform、serialization、unbounded allocation の有無。 |
| integration | headless execution、fixture 再現性、dependency revision、OS / compiler / audio configuration。 |
| failure | missing source / plugin、load failure、processing failure、Domain state の保持。 |

測定の絶対値が未決定の領域では、計測機器、fixture、繰返し回数、中央値 / 最大値、環境を結果へ残す。数値を得る前に「高速」と結論付けない。

## 11. Pass / Conditional Pass / Fail と Rejection Rule

各仮説と各 test scenario を `Pass`、`Conditional Pass`、`Fail`、`Inconclusive` のいずれかで記録する。

* **Pass:** 明示した criterion を満たし、重大 workaround が不要。
* **Conditional Pass:** 制約、限定 adapter、追加 prototype、または明示的な V1 usage rule があれば成立する。
* **Fail:** criterion を満たせず、合理的な adapter では補えない。
* **Inconclusive:** fixture / environment / observation が不十分で、限定した再試験が必要。

H1 Domain Independence、H3 integer sample model、H6 PDC、H7 Effect Tail の Fail は原則 hard rejection signal とする。H4 long-form、H8 runtime editing、H10 headless test の Fail も、V1 の中心要件を満たせない程度なら Reject とする。他項目は重大度、workaround の局所性、V1 implementation cost を評価して `Proceed with Constraints` または `Inconclusive` を選べる。

次も rejection signal とする。

* Tracktion internal semantics を AudioNLE の Synchronization Group / Member または Ripple model に強制的に合わせる必要がある。
* 長期保守する Tracktion fork / patch が前提となる。
* adapter が実質的に custom audio engine と同等の複雑さになる。
* source 全 decode、巨大 PCM resident、全 Project rebuild が長尺 workflow の前提となる。

## 12. Expected Prototype Artifacts

本タスクでは作成しないが、後続の prototype 実装では次の成果物を残す。

```text
prototype/                    # headless driver と adapter 実験
tests/                        # automated integration scenarios
fixtures/                     # signal / source fixture 定義
benchmark-results/            # raw measurements と環境情報
docs/design/prototypes/
  tracktion-feasibility-results.md
```

成果物には reproducible test instructions、exact dependency revisions、license / NOTICE 確認表、fixture descriptions、Pass / Fail table、benchmark measurements、known limitations、architecture observations、Tracktion-specific workarounds を含める。実際のコード、ディレクトリ、依存導入は本計画の範囲外である。

## 13. `tracktion-feasibility-results.md` の想定形式

後続の結果文書は少なくとも次の順序で記録する。

1. Environment（OS、CPU、RAM、audio configuration、実行日時）
2. Dependency revisions（Tracktion、JUCE、plugin、その他の revision と license / NOTICE 確認）
3. Test matrix（fixture、scenario、期待値、実行方法）
4. Pass / Conditional Pass / Fail / Inconclusive
5. Performance measurements
6. PDC results
7. Tail results
8. Runtime editing results
9. Domain leakage review
10. Required workarounds
11. Risks and known limitations
12. Recommendation
13. ADR readiness

各 Fail / Conditional Pass には再現手順、観測値、影響する requirements、代替 path を付ける。

## 14. Decision Rule

prototype 完了後の結論は次のいずれかに限定する。

| 結論 | 条件 |
| --- | --- |
| Proceed | 全 hard criterion が Pass。残る問題は通常の実装・設計作業の範囲。Tracktion を ADR 採用候補として進める。 |
| Proceed with Constraints | hard fail はなく、adapter / usage boundary / V1 operation 制約を ADR 候補として明文化すれば成立する。 |
| Reject | hard rejection signal、または long-form / runtime edit / headless test の重大 Fail がある。Tracktion を primary audio engine にしない。 |
| Inconclusive | 限定 fixture、環境、plugin、測定が不足。追加の小さな prototype を定義する。 |

`Proceed` は既定結論ではない。Fail を性能 tuning や「将来対応」で覆い隠さない。

## 15. Alternative Path

`Reject` の場合は `technical-options.md` の Option B 系、すなわち AudioNLE Domain + custom scheduling + 選択した JUCE audio / plugin primitives + media adapter を再評価する。本書はその architecture を設計しない。Reject の理由を、domain boundary、PDC、Tail、long-form streaming、runtime editing、headless test のどこにあるかへ分解して次の比較へ渡す。

## 16. 自己レビュー

* Tracktion の採用を前提にせず、Reject と Inconclusive を正式な結論に含めた。
* Domain を authoritative とし、Tracktion object graph の persistence と Domain への漏出を禁止した。
* integer Timeline sample、Source / Timeline domain 分離、Processor order、PDC の 0 sample criterion、Tail の Processing End を検証対象にした。
* Synchronization Group / Member と Ripple は Tracktion grouping / timeline edit に委譲せず、adapter がその full implementation を要求しないことを確認対象にした。
* 長尺 fixture、全 PCM 非常駐、seek、runtime editing、offline render、headless test を含めた。
* production GUI、complete import / export、full sync / Ripple、platform support、最適化を scope から除外した。
* ライセンスと NOTICE は確認対象に残し、GPLv3 互換性だけを rejection reason にしていない。
