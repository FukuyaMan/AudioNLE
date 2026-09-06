# AudioNLE 技術選択肢

## 1. 目的と前提

本書は技術の採用決定ではない。長尺・マルチトラック spoken-word を扱う audio-only NLE の要件を満たすための選択肢、実装負担、ロックイン、検証事項を比較し、後続 ADR の材料にする。

`C++20+`、CMake、JUCE、Tracktion Engine、FFmpeg、VST3、Windows 11 x64 first はいずれも候補であり、決定ではない。特に、Synchronization Group / Member、Ripple Range / Scope、整数 Timeline sample、Source / Timeline domain の分離、Processing Stack、Tail Policy は AudioNLE のドメインであって、候補ライブラリのオブジェクトや時刻型へ委譲しない。

評価は V1 の Must（非破壊編集、サンプル精度、VST3、PDC、Tail、WAV / MP3 / AAC 出力、長尺 import / waveform / export）を主軸とし、動画編集・音楽 DAW・録音・クラウド編集を対象外とする。

## 2. 評価軸

| 軸 | AudioNLE での意味 |
| --- | --- |
| 要件適合性 | 編集、同期、Processing、Export の Must を実現しやすいか。 |
| 長尺適性 | 全 PCM 常駐なしで seek、streaming、waveform、offline render を扱えるか。 |
| 時間モデル | Project の整数 Timeline sample と Source domain を正確に保持できるか。 |
| リアルタイム安全性 | audio path から I/O、scan、保存、波形生成、無制限割当を隔離しやすいか。 |
| プラグイン | VST3 load / state / editor / PDC / tail / missing plugin を扱えるか。 |
| 可搬性 | Windows 11 を優先しながら macOS / Linux の余地を損なわないか。 |
| ドメイン分離 | framework object graph を Project の正とせず、headless domain test を保てるか。 |
| 保守・配布 | API 安定性、ライセンス、依存サイズ、更新・デバッグ・OSS 配布への影響。 |
| Codex 開発適性 | 文書・fixture・headless test を使った小さな検証単位が作れるか。 |

## 3. GUI / Application Framework

### 比較

| 候補 | Timeline / waveform UI | Windows-first / 可搬性 | Plugin editor・accessibility | 主な負担・ロックイン |
| --- | --- | --- | --- | --- |
| JUCE | custom drawing と入力処理を同一 framework で扱える。audio / plugin との結合が近い。 | Windows / macOS / Linux を狙える。 | plugin editor hosting の足場があるが、native 外観・accessibility は要検証。 | JUCE UI、audio、plugin の一体依存。AGPLv3 / commercial の選択を要確認。 |
| Qt (Widgets / Quick) | GPU を含む custom rendering、docking、panel、shortcut、high-DPI、accessibility の選択肢が厚い。 | Windows 11、macOS、Linux を公式対応範囲に含む。 | plugin editor のネイティブ埋込みは別途検証・adapter が必要。 | Qt の UI / event model、QML 採用時は追加言語・debugging 面の負担。LGPLv3 / GPLv3 / commercial を精査。 |
| Win32 / Windows App SDK | Windows の入力、windowing、accessibility と親和。低層で timeline を最適化可能。 | Windows に限定される。 | Windows plugin editor には有利な場合があるが、個別対処が増える。 | UI / docking / high-DPI / rendering / shortcut を自前統合。将来移植のコストが高い。 |
| wxWidgets 等の汎用 C++ GUI | 基本的な desktop UI は可能。 | 複数 OS の選択肢。 | audio / plugin integration の足場は薄い。 | 高密度 timeline の描画・DPI・docking・plugin editor の実績を別途確認。 |
| 最小独自 UI layer | domain に最適化した Timeline interaction を作れる。 | 描画 backend に依存。 | すべて自前。 | V1 で UI 基盤開発が主目的を圧迫するため高リスク。 |

JUCE は一つの依存で audio application と plugin host の足場を得られるが、「音声アプリだから JUCE」とは結論付けない。Qt は Timeline / Mixer / Media Bin / Inspector の一般デスクトップ UI に強い一方、audio engine と plugin hosting は別途選ぶ。Qt の現行ドキュメントは Windows 11 x64、macOS、主要 Linux を支持対象に挙げ、Qt Quick を新規 UI の推奨技術としている一方、ライセンス形態とモジュールごとの差異があるため配布方式を先に確認する必要がある。[Qt platforms](https://doc.qt.io/qt-6.10/supported-platforms.html) [Qt licensing](https://doc.qt.io/qt-6/licensing.html)

**V1 の判断材料:** waveform の LOD 描画、数千 Clip の hit test、drag / trim / split、high-DPI、keyboard shortcut、Mixer、dock / panel layout、plugin editor 埋込み、screen reader を小規模 prototype で測る。UI framework は domain の Selection / Edit Target / Ripple Scope を所有しない。

## 4. Audio Engine

| 方向性 | 提供し得るもの | AudioNLE が保持すべきもの | リスク |
| --- | --- | --- | --- |
| Tracktion Engine | transport、timeline playback、clip / track / effect の土台、plugin hosting、offline render に近い機能。 | Project、Clip、Source Range、Synchronization Group / Member、Ripple、Processing Stack の正規状態。 | engine の Edit / Clip semantics へ domain が引かれる。長尺、PDC、Tail、dynamic edit の実挙動を prototype で確認。 |
| JUCE primitives + AudioNLE scheduling | device / buffer / audio primitive、plugin / processor 関連の土台。 | timeline scheduling、streaming、overlap、PDC、tail、offline render、transport、すべての editing semantics。 | V1 実装量・リアルタイム境界・plugin host が大きい。domain 適合度は高い。 |
| 独自 audio engine | ドメインに完全に合わせた sample scheduling、tail、PDC を設計できる。 | 上記と同じ。 | playback / seek / stream / PDC / VST3 / offline render を一から検証するため最も高リスク。 |
| 他 OSS engine | 特定の playback / graph 機能を再利用できる可能性。 | domain の正規状態と独自の同期・ripple。 | 長尺 NLE、VST3 host、PDC、Windows の実績が揃う候補は個別評価が必要。追加候補を先行導入しない。 |

Tracktion Engine は JUCE module として供給され、README は C++20 を要求し複数 desktop OS を掲げる。これは開発速度に有利だが、full DAW / sequencer にも使える一般性は AudioNLE の独自モデルと同一ではない。[Tracktion Engine README](https://github.com/Tracktion/tracktion_engine)

**評価:** Tracktion を選んでも domain model を Tracktion 型へ直結しなければ、独自 Synchronization Group / Member や Ripple の前後状態を domain が管理し、adapter が engine playback state を構成できる。代償は二重状態の同期であり、adapter の更新時点、再生中編集、plugin state / tail / latency の反映規則が高リスクになる。JUCE primitives / custom scheduling は domain 適合性と置換性が高いが、V1 completion までの実装量が大きい。

## 5. Plugin Hosting

| 候補 | 利点 | 不足・検証事項 | Processor boundary |
| --- | --- | --- | --- |
| Tracktion Engine に委譲 | scan、load、editor、render の既存機能を利用できる可能性。 | PDC、tail reporting、missing plugin、bypass、crash、offline / RT thread の実挙動を確認。 | plugin identity / parameter state は domain、live instance は adapter。 |
| JUCE の hosting 機構を利用 | VST3 host の既存抽象を利用し、UI と合わせやすい。 | scan / sandbox / crash isolation / PDC / tail を製品要件として検証・補完。 | 同上。 |
| Steinberg SDK を直接扱う | VST3 の動作と host policy を最も細かく制御できる。 | scan、load / unload、state、editor、PDC、tail、thread safety、クラッシュ耐性を自前で担う。 | 最も明確だが V1 実装量最大。 |

V1 は VST3 のみであり、他形式を抽象化のために先行実装しない。VST3 SDK は現行ポータル上で MIT ライセンスとされ、host の配布も可能と説明されているが、host 自体の信頼性・plugin crash・PDC はライセンス問題ではなく製品検証問題である。[VST3 licensing](https://steinbergmedia.github.io/vst3_dev_portal/pages/FAQ/Licensing.html)

PDC は Processor が報告する latency と、Track / Master 経路の補償を engine adapter が扱う領域である。domain は Processor identity、順序、parameter state、bypass、missing state を authoritative に持つ。Tail も Processor / Tail Policy の domain state と adapter の実測・報告結果を分ける。Dummy latency plugin と dummy tail plugin を使う headless offline render test が採否条件になる。

## 6. Media Import / Decode

| 候補 | Container / multi-stream / codec | 長尺 seek / partial decode | 配布・可搬性 | 適合評価 |
| --- | --- | --- | --- | --- |
| FFmpeg / libav* | MP4 / MKV / MOV、複数 audio stream、WAV / MP3 / AAC / Opus、metadata を広く扱える。 | demux / decode を worker に隔離しやすいが streaming cache は自前設計。 | LGPL 構成・外部 codec・特許・再配布の精査が必須。複数 OS。 | 最も広い import 対応候補。 |
| JUCE 対応 codec のみ | 単純な audio file には有効。container / multi-stream の範囲は不足し得る。 | audio app と統合しやすい。 | JUCE のライセンス・対応 codec の確認が必要。 | MEDIA-002 を安定して満たせるか不確実。 |
| Windows Media Foundation | Windows Source Reader / Sink Writer で OS codec を利用可能。 | playback 向け pipeline と raw sample access の選択肢がある。 | Windows lock-in、OS / codec availability 差。 | Windows-only fallback / supplement として有力。cross-platform core には弱い。 |
| libsndfile 等 | WAV 等の sampled audio file に有用。 | simple streaming I/O に向く。 | format 範囲が狭い。 | WAV 専用の補助候補で、container / multi-stream の主解ではない。 |

FFmpeg は既定で LGPLv2.1+ だが GPL / nonfree の optional component や外部 codec の選択で条件が変わる。配布前に構成、dynamic linking、ライセンス表示、ソース提供、特許リスクを専門的に確認する必要がある。[FFmpeg legal](https://ffmpeg.org/legal.html) Media Foundation の Source Reader は source / decoder から raw media data を得る用途に位置付けられ、Sink Writer は encode を扱うため Windows-first prototype には使えるが、クロスプラットフォームの基盤にはならない。[Source Reader](https://learn.microsoft.com/en-us/windows/win32/medfound/source-reader) [Sink Writer](https://learn.microsoft.com/en-us/windows/win32/medfound/sink-writer)

**推奨する境界:** domain は `Media` / `MediaSource` / Source Range だけを保持し、decode adapter が stream enumeration、metadata、seek、partial decode を提供する。Media の source location は domain にあり、decoder context や FFmpeg / OS 型は保持しない。decode、waveform、playback は別 worker / cache を通じる。

## 7. Export / Encoding

| 候補 | WAV / MP3 / AAC / Opus | offline render・設定 | 主なリスク |
| --- | --- | --- | --- |
| FFmpeg / libavcodec | V1 Must と Should の広い候補。 | bitrate、sample rate、channel、metadata を統合しやすい。 | import と同じ licensing / redistribution / deterministic build の管理。 |
| OS codec API | Windows の配布負担を減らす可能性。 | platform encoder の有無・結果差を検証。 | AAC / MP3 の一貫性と macOS / Linux 移植性が弱い。 |
| standalone codec libraries | WAV と各 codec を最小依存で組む。 | encoder ごとの明示的 control。 | AAC を含むと依存・ライセンス・integration が増え、複数 library のテストが必要。 |
| JUCE の範囲 | WAV 等の基本範囲では候補。 | audio engine と統合可能。 | AAC / M4A、MP3、Opus の Must / Should を一貫して満たせるか要確認。 |

Export は real-time path ではなく offline worker で行う。Project Timeline Sample Rate と出力 sample rate の変換、Mono / Stereo、bitrate、metadata、Tail を含む Processing End を明示的に input とする。Golden render は codec ごとの bit-exact を前提にせず、PCM intermediate の比較、container metadata、許容誤差を分ける。

## 8. Sample Rate Conversion

| 候補 | quality / streaming | latency / CPU | license・API | 評価 |
| --- | --- | --- | --- | --- |
| engine 内蔵 resampler | engine と一体で使える。 | 実装・品質・offline / real-time 再現性を個別検証。 | engine lock-in。 | engine 選定後の候補。 |
| JUCE resampler | JUCE 構成では依存追加なし。 | 品質・境界処理・offline / RT 差を fixture で確認。 | JUCE license に従う。 | JUCE 構成の低摩擦候補。 |
| libsamplerate | 専用 SRC、BSD-2-Clause。 | streaming と state の管理が必要。 | API は比較的限定的。 | 低ライセンス摩擦の有力候補。 |
| soxr | quality 設定の選択肢、PCM SRC。 | 高品質 real-time 設定では latency が増え得る。 | LGPL-2.1。 | offline / high quality には有力、preview latency は実測必須。 |

SRC は Source / Timeline domain を混ぜない。domain は両 range を整数で持ち、adapter が exact ratio と一貫した rounding policy を適用する。libsamplerate は BSD-2-Clause、soxr は LGPL-2.1 であり、soxr の README は高品質 real-time 設定の latency 例を示す。[libsamplerate](https://github.com/libsndfile/libsamplerate) [soxr](https://github.com/chirlu/soxr)

## 9. Waveform / Peak Cache

Source Media 全 PCM を RAM に常駐させる案は除外する。

| 方針 | 長尺 / Scroll / Zoom | startup / storage | invalidation・移植性 |
| --- | --- | --- | --- |
| decode 時 multiresolution peak 生成 | 即時表示に有利。 | import 待ちと初期 I/O が増える。 | Media identity と source modification を結び付ける。 |
| lazy generation | 初回 import が軽い。 | 初見範囲の表示待ち。 | visible range の優先順位が必要。 |
| background generation | 編集を妨げにくい。 | worker / cancellation / progress が必要。 | 最も現実的な基本方針候補。 |
| chunked binary cache | 数時間素材の LOD と部分更新に適する。 | cache schema を管理。 | hash / size / native rate / stream ID で invalidation。 |
| memory mapped cache | 大きい cache の RAM 圧を抑え得る。 | OS 差と file lifecycle。 | platform adapter に隔離する。 |
| project-local cache | Project 移動と結び付きやすい。 | 同じ Media を複数 Project で重複。 | portable project bundle と相性。 |
| application-global cache | 再利用に有利。 | cleanup / relocation / privacy。 | path だけに依存しない識別が必要。 |

有力な比較対象は「background + lazy priority + chunked multiresolution binary cache」である。これは決定ではない。cache は Source Media の派生物であり Project の正規編集状態ではない。source relocation / modification は path、file size、optional hash、stream ID と整合して判定し、欠落時に Project を破損させない。peak cache unit test、relocation fixture、数時間 benchmark が必要である。

## 10. Project Serialization

| 候補 | 長所 | 短所・適合性 |
| --- | --- | --- |
| JSON 単体 | readable、diffable、fixture / migration test が容易。 | 大きい plugin state blob、atomic save、corruption 耐性を補う必要。 |
| JSON + sidecar cache | 正規編集状態と waveform / transient cache を分離できる。 | 複数ファイルの atomicity、移動時の管理。 |
| ZIP container + JSON | 利用者には 1 logical file、metadata / binary state を同梱可能。 | partial update、corruption recovery、diffability が低下。 |
| SQLite | atomic transaction、large blob、autosave / recovery に強い。 | human readability、merge / diff、schema migration、dependencyが重い。 |
| custom binary | サイズ・速度を最適化可能。 | migration・debug・fixture・corruption 対策を自前で負う。 |

V1 では `JSON + sidecar cache` が editing state と再生成可能 cache の分離に最も素直である一方、「1 つの論理的 Project File」という要求との UI 上の見せ方、plugin state blob の扱いを prototype で確認する。ZIP + JSON は一ファイル配布には有利だが、autosave / crash recovery の設計が先に必要になる。どの形式でも Source Media は無条件に埋め込まない。

## 11. Undo / Redo

| 方式 | 適合性 | 主な弱点 |
| --- | --- | --- |
| Command-based | Move / Trim / Split / Ripple の差分が明確で、domain test に向く。 | plugin parameter blob、複雑な target expansion、長い操作列の inverse を慎重に定義。 |
| immutable state / snapshot | 操作前後の等価性、Undo / Redo、テストが単純。 | 大規模 Project / plugin state のメモリ・コピー負担。 |
| hybrid command + selective snapshot | 通常編集は差分、plugin state / 複雑操作は限定 snapshot にできる。 | command と snapshot の境界・整合性が増える。 |
| event sourcing 的方式 | 監査・再生に強い。 | migration、plugin state、履歴肥大、V1 には過剰。 |

有力な比較対象は command-based を基礎に、不可逆または大きい plugin state だけを必要最小限の before / after state で補う hybrid である。決定には、Synchronization Member を含む Split / bypass Delete、Ripple Scope 解決、Processor reorder、Missing state を対象に property-based round-trip を行う。Save / Load は Undo history と独立し、Source Media を変更しない。

## 12. Threading Model

| 責務 | 実行経路 | audio thread への禁止 |
| --- | --- | --- |
| UI | Selection、gesture、表示、command 発行。 | UI 操作自体を audio thread で行わない。 |
| real-time audio | 現在の playback / processing snapshot を消費。 | file / network I/O、blocking mutex、scan、waveform、serialization、unbounded allocation。 |
| decode / streaming worker | partial decode、seek read-ahead、cache。 | audio callback で同期 read しない。 |
| waveform worker | peak cache の生成・更新。 | audio path から起動・待機しない。 |
| save / autosave worker | serialization、atomic write、recovery。 | audio path から保存しない。 |
| plugin scan worker | discovery / validation。 | audio path から scan しない。 |
| offline export worker | non-real-time render / encode。 | device playback と同じ deadline を課さない。 |

候補は message passing、immutable playback snapshot、bounded queue、read-copy-update 的な公開であり、具体実装は未決定である。重要なのは domain command が Project state を確定し、adapter が安全な時点で playback snapshot を更新すること。plugin が host の deadline を破る、plugin UI が不安定、decode が遅い事態を domain lock に波及させない。

## 13. Domain Model Boundary

**authoritative state は AudioNLE Domain が持つ。**

| Domain が正とする state | 外部依存側が持つ runtime state |
| --- | --- |
| Project Timeline Sample Rate、Media identity / location、Clip Timeline / Source Range、Track association | decoder context、read-ahead buffer、waveform cache handle。 |
| Clip Group、Synchronization Group、Synchronization Member、Relative Timeline Offset、Ripple Range / Scope の解決結果 | engine clip / edit / transport object。 |
| Processing Stack order、Processor identity / parameter state、Tail Policy、Track Gain / Pan、Missing Media / Plugin state | live plugin instance、reported latency / tail、audio device state。 |

望ましい境界は次の通りである。

```text
UI
↓
Application / Editing Commands
↓
AudioNLE Domain Model
↓
Engine / Media / Plugin Adapters
↓
Selected frameworks and platform services
```

この境界は adapter の変換コストと duplicate state の同期問題を生むが、以下を可能にする。

* Move、Split、Trim、Ripple、Group、Undo / Redo、Save / Load を audio device / plugin host なしで test できる。
* Project serialization が framework object graph の保存にならない。
* Tracktion / JUCE / decoder / plugin host の交換範囲を局所化できる。
* Timeline 時刻と Source 時刻が framework の floating-point time 表現に無条件で従属しない。

adapter は domain state を勝手に再定義してはならない。特に engine の clip group と AudioNLE の Clip Group / Synchronization Group を同一視しない。runtime が返す latency、tail、decode error は adapter status として domain state へ明示的に反映する。

## 14. Windows-first と Portability

Windows 固有 API の価値が高い領域は audio device、file dialog、window / plugin editor embedding、Media Foundation fallback、installer / code signing、performance profiling である。これらは platform / adapter layer に閉じ込める。

初期から可搬性を保つべき境界は、domain model、Project serialization、Source / Timeline time model、decode / encode interface、audio engine interface、waveform cache format、plugin Processor boundary、headless test である。V1 で macOS / Linux の正式対応を目的に全 UI や build を抽象化する必要はない。Windows-first の具体コードを domain に漏らさないことが十分な投資となる。

## 15. Build / Dependency Management

| 選択肢 | 再現性 / CI | Windows 開発・OSS | 注意点 |
| --- | --- | --- | --- |
| CMake | ecosystem / IDE / CI との接続が広い。 | Windows toolchain と相性が良い。 | generator・toolchain・option の pin が必要。 |
| git submodule | source と revision が明示的。 | offline clone 後に有利。 | update UX、nested dependency、contributor friction。 |
| FetchContent | CMake と統合しやすい。 | 初回取得が簡単。 | network 非依存 build、hash / revision pin、transitive update を管理。 |
| vcpkg | Windows package experience と binary cache。 | contributor の導入が比較的容易。 | registry / triplet / version pin と source build 再現性。 |
| Conan | profile・binary package の選択肢。 | 複数環境で有用。 | tool の運用・lockfile を学習する負担。 |
| vendoring | 最も再現可能。 | network 不要。 | repository size、security update、license tracking。 |

候補は「CMake + 明示 pin + 依存ごとの取得方針」であり、CMake や package manager 自体の採用決定ではない。依存のライセンス、source revision、binary provenance、offline build、upgrade test を manifest / policy で追跡できることを重視する。

## 16. Testing Strategy への影響

| テスト | 必要な技術的性質 |
| --- | --- |
| pure domain / property-based edit | framework-free domain。整数 sample、Source / Timeline range、Group / Member を直接構築できる。 |
| Undo / Redo、Save / Load | deterministic serialization と state equality。plugin / media 欠落 fixture を使える。 |
| split / trim / sync / ripple | engine なしで target resolution と offset invariant を検証できる。 |
| dummy latency / tail plugin | plugin adapter を offline render で起動でき、PDC と Processing End を測れる。 |
| media fixture | container の multi-stream、codec、metadata、seek、missing media を固定 fixture で確認できる。 |
| long-form benchmark | streaming / waveform cache / scroll / zoom / seek を代表 project で計測できる。 |
| real-time safety | audio path が I/O / scan / serialization / waveform work を呼ばないことを計測・静的点検できる。 |

Tracktion-heavy 構成は integration test 比率が上がる。custom scheduling は domain test が強いが audio integration test の量が増える。いずれでも GUI を起動しない headless domain test と offline render test を release gate にできることが必須である。

## 17. 構成案

### Option A — Framework-heavy

* **構成:** JUCE UI / primitives + Tracktion Engine + FFmpeg 系 media adapter。
* **利点:** V1 の transport、render、plugin の足場を最も早く得られる可能性。Windows / macOS / Linux の方向性を保ちやすい。
* **欠点:** JUCE / Tracktion semantics と lifecycle への依存が強く、domain / adapter 分離に規律が必要。
* **implementation cost:** 低〜中。**maintenance cost:** 中。**risk:** engine adapter、PDC / Tail / long-form 実挙動。
* **AudioNLE 適合度:** 高（prototype 合格が条件）。**置換可能性:** 中。**V1 completion:** 高め。

### Option B — Middle ground

* **構成:** JUCE または Qt UI + AudioNLE domain / custom scheduling + JUCE hosting または direct VST3 adapter + FFmpeg 系 media adapter。
* **利点:** Clip、Synchronization Member、Ripple、Source / Timeline domain をそのまま playback scheduling に反映しやすい。domain test が強い。
* **欠点:** PDC、Tail、seek、dynamic edit、offline render の実装と検証を多く負う。
* **implementation cost:** 高。**maintenance cost:** 中〜高。**risk:** V1 の実装量。
* **AudioNLE 適合度:** 非常に高。**置換可能性:** 高。**V1 completion:** 中。

### Option C — Windows-first composition

* **構成:** native Windows UI + Windows Media Foundation + custom engine / plugin adapter。
* **利点:** Windows の device / windowing / codec と密に統合できる。
* **欠点:** cross-platform path が弱く、timeline / plugin / render の大部分を自前実装する。codec availability の環境差。
* **implementation cost:** 高。**maintenance cost:** 高。**risk:** UI 基盤と engine の二重開発。
* **AudioNLE 適合度:** 中。**置換可能性:** 低〜中。**V1 completion:** 低〜中。

### Option D — Qt-centered custom core

* **構成:** Qt UI + custom domain / scheduling + FFmpeg 系 media adapter + selected VST3 hosting adapter。
* **利点:** Mixer / panel / accessibility / high-DPI と custom Timeline UI を audio engine と分離できる。
* **欠点:** plugin editor embedding と audio engine を別途完成させる必要がある。Qt license / deployment を精査。
* **implementation cost:** 高。**maintenance cost:** 中〜高。**risk:** V1 の UI と audio の統合面。
* **AudioNLE 適合度:** 高。**置換可能性:** 高。**V1 completion:** 中。

## 18. Decision Matrix

5 は相対的に有利、1 は不利を示す。点数は採用根拠ではなく、prototype とライセンス調査の優先順位付けである。

| 案 | 開発速度 | feature fit | 長尺適性 | plugin | 可搬性 | testability | 保守 | 依存リスク | architecture cleanliness |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| A Framework-heavy | 5 | 4 | 3 | 4 | 4 | 3 | 3 | 2 | 3 |
| B Middle ground | 2 | 5 | 4 | 3 | 4 | 5 | 3 | 3 | 5 |
| C Windows-first | 2 | 3 | 3 | 2 | 1 | 3 | 2 | 4 | 3 |
| D Qt-centered | 3 | 4 | 4 | 3 | 5 | 5 | 3 | 3 | 5 |

Option A は Tracktion の実挙動が requirements を満たすなら V1 最短だが、engine lock-in が最大の論点である。Option B / D は domain の清潔さと testability で優位だが、PDC / Tail / dynamic playback を自前で背負う。Option C は Windows-first を過度に platform lock-in へ拡大しやすい。

## 19. Provisional Recommendation

現時点で最も有力なのは **Option A を prototype で検証し、domain-first adapter 方針を必須条件にする** 構成である。これは ADR や採用決定ではない。

理由は、V1 の難所が UI よりも、VST3、PDC、Tail、offline render、transport、長尺 playback の同時成立にあり、Tracktion / JUCE の成熟した経路を評価する価値が高いためである。最大の欠点は Tracktion / JUCE への lock-in と、AudioNLE の Clip / Sync / Ripple semantics を engine object に二重化することにある。

隔離すべき箇所は engine playback、plugin instance / editor、media decode / encode、device I/O、cache、platform windowing である。最も強い lock-in risk は Tracktion Engine の edit / plugin lifecycle であり、次が JUCE の license と UI / plugin abstractions、FFmpeg の配布構成である。prototype が sample-accurate sync、PDC、Tail、long-form seek、offline render、headless test を満たせない場合、Option B へ移る判断材料とする。

## 20. 未決定事項と必要な prototype

* Tracktion Engine が数時間・多数 Clip の Timeline で満たす操作応答、メモリ、seek の実測。
* Tracktion adapter が domain state を正として保つ際の更新コストと、再生中編集時の挙動。
* Tracktion / JUCE / direct VST3 の PDC、Tail reporting、bypass、missing plugin、plugin crash の実挙動。
* FFmpeg の必要 codec を含む再配布構成、LGPL / GPL / third-party encoder / patent の適法性。法務確認が必要。
* GUI 候補での数千 Clip の timeline rendering、high-DPI、hit test、plugin editor embedding、accessibility。
* background + chunked peak cache の容量、生成時間、source relocation / modification invalidation。
* Project serialization の JSON + sidecar、ZIP container、SQLite に対する large plugin state、atomic save、recovery の prototype。
* libsamplerate、soxr、engine SRC の quality、stream boundary、preview latency、offline determinism。
* Selection / Edit Target / Ripple Scope の UI resolution と domain command の境界。

## 21. ADR 候補

後続で判断する場合、少なくとも次を独立した ADR 候補とする。

1. Domain model independence from audio engine / framework。
2. GUI / application framework。
3. Audio engine / timeline playback strategy。
4. VST3 hosting strategy と Processor adapter boundary。
5. Media decode / container strategy。
6. Export / encoding strategy。
7. Sample rate conversion strategy と rounding policy。
8. Waveform / peak cache strategy。
9. Project serialization / autosave / recovery strategy。
10. Undo / Redo strategy。
11. Windows-first platform policy と portability boundary。
12. Build / dependency provenance policy。

## 22. 自己レビュー

* JUCE、Tracktion Engine、FFmpeg はいずれも候補として比較し、採用前提とはしていない。
* requirements の multiple stream import、sample-accurate editing、PDC、Tail、offline export、長尺 waveform / streaming、Missing state、Undo / Redo を評価の軸にした。
* 一般 DAW 論ではなく、Synchronization Group / Member、Ripple Range / Scope、Source / Timeline domain、spoken-word long-form の具体的な境界を扱った。
* engine / framework が domain state を所有しないこと、pure domain test と headless offline test を維持することを明記した。
* unknown な性能・PDC / Tail・ライセンス・cache・serialization は未決定事項と prototype に残した。
* 将来拡張のためだけの generic graph、複数 plugin format、cross-platform UI abstraction、Bus Track 実装は推奨していない。

## 参考一次情報

* [JUCE README / license](https://github.com/juce-framework/JUCE/blob/master/LICENSE.md)
* [Tracktion Engine README](https://github.com/Tracktion/tracktion_engine)
* [Qt supported platforms](https://doc.qt.io/qt-6.10/supported-platforms.html)
* [Qt licensing](https://doc.qt.io/qt-6/licensing.html)
* [FFmpeg legal](https://ffmpeg.org/legal.html)
* [VST3 licensing](https://steinbergmedia.github.io/vst3_dev_portal/pages/FAQ/Licensing.html)
* [Microsoft Media Foundation Source Reader](https://learn.microsoft.com/en-us/windows/win32/medfound/source-reader)
* [libsamplerate](https://github.com/libsndfile/libsamplerate)
* [soxr](https://github.com/chirlu/soxr)
