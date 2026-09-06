# AudioNLE の編集モデル

## 1. 目的

本書は AudioNLE の編集挙動を、実装技術から独立した製品ドメインとして定義する。

対象は、長尺・マルチトラック音声、とりわけ spoken-word 音声を Timeline 上で非破壊編集し、Processing、Mixing、Export まで行うために必要な編集モデルである。

本書でいう「変更」は Project 状態の変更であり、Source Media 自体を変更することではない。

本書では UI の具体的な操作方法、C++ 型、ファイル形式、処理アルゴリズム、Plugin API、Audio Engine 実装を定義しない。

初期バージョンで必要な概念は必要最小限に保つ一方、将来的な追加によって中心モデルそのものを破壊しないよう、拡張境界を明確にする。

---

# 2. 基本原則

* Project は 1 つの Project Timeline Sample Rate を持つ。
* Timeline 上の正規位置と長さは、Project Timeline Sample Rate に基づく整数 sample で表す。
* Source Media の時間軸と Timeline の時間軸は別 domain として扱う。
* Clip は SourceReference の一部を Timeline 上へ対応付ける非破壊編集オブジェクトである。
* V1 の SourceReference は MediaSource のみとする。
* Clip Group は任意の一括編集単位である。
* Synchronization Group は時間的な関係を保護する単位である。
* Clip Group と Synchronization Group は別概念である。
* Synchronization Group 内では、Clip そのものではなく論理的な Synchronization Member を同期対象の基本単位とする。
* Selection と実際の Edit Target は別概念である。
* 同一 Clip Track 上で Clip overlap を許可する。
* overlap した Clip の処理済み出力は原則として加算する。
* Processing Stack の順序は音声結果に意味を持つ。
* Effect Tail は Source の参照範囲を延長しない。
* 通常の同期編集では相対 Timeline offset を sample-accurate に維持する。
* 同期保護 bypass は Synchronization Group または Synchronization Member を暗黙に破棄しない。
* Editing Command の作用対象と、UI がその作用対象をどう決定するかは分離して考える。
* V1 の実装範囲を必要最小限に保ちつつ、将来の Source、Track role、Processor 等の追加を妨げない。

---

# 3. 用語

| 用語 | 定義 |
| ---------------------------- | --------------------------------------------------------------------------- |
| Project | Media、Track、Timeline、編集状態、Processing、Mixing、永続状態を持つ編集単位。 |
| Project Timeline Sample Rate | Project 固有の Timeline 時間単位。新規 Project の既定値は 48 kHz。 |
| Timeline | Track と Clip を配置する、Project Timeline Sample Rate に基づく整数 sample 軸。 |
| SourceReference | Clip が参照する入力 source を表す抽象的な概念境界。V1 では MediaSource のみを扱う。 |
| MediaSource | 外部 Source Media を参照する SourceReference。 |
| Media | 外部に存在する Source Media と、その識別・参照情報。編集結果ではない。 |
| Source Time Domain | Source Media の native sample rate に基づく整数 sample 軸。 |
| Timeline Time Domain | Project Timeline Sample Rate に基づく整数 sample 軸。 |
| Track | Timeline または Mixing 上で音声を扱う論理的な処理単位。V1 では Clip Track のみを扱う。 |
| Clip Track | Timeline 上に Clip が所属し、Clip Processing Output を Track Mix にまとめる Track。 |
| Clip | SourceReference の範囲を Timeline 上の範囲へ対応付ける非破壊編集オブジェクト。 |
| Selection | 一時的に選択された Clip または Timeline range。Group membership ではない。 |
| Edit Target | 現在の Editing Operation によって実際に変更される対象集合。Selection と一致しない場合がある。 |
| Clip Group | 利用者が任意の複数 Clip をまとめて編集するための Group。 |
| Synchronization Group | 複数 Synchronization Member の時間関係を保護する Group。 |
| Synchronization Member | 1 つの論理的な同期 source / recording participant を表し、0 個以上の Clip segment を持ち得る単位。 |
| Processing Stack | Clip ごとの順序付き Processor 列。SourceReference と Track Mix の間に置かれる。 |
| Processor | 音声またはその時間上の振る舞いを処理する Processing Stack の要素。 |
| Built-in Processor | 製品が定義する Gain、Fade、内蔵 Effect 等の Processor。 |
| Effect Processor | Built-in Effect または external Effect を表す Processor。 |
| Track Mix | 同一 Clip Track の各 Clip Processing Output を加算した結果。 |
| Master Output | Track の出力を統合し、Master Processing 後に出力・Export される結果。 |
| Media Bin | Project に取り込まれた Media を Timeline 上の Clip から独立して管理する集合。 |
| Source End | Clip が参照する Source 範囲の終端。 |
| Processing End | Clip Processing Output が終了する Timeline 上の終端。Tail があれば Source End 対応位置より後になる。 |
| Effect Tail | Source End 後にも stateful Processor が生成する Processing Output。 |
| Ripple Range | Ripple 操作によって除去または挿入される Timeline range。 |
| Ripple Scope | Ripple 操作の対象となる、事前に解決された Track / Clip / Edit Target 集合。 |

「Clip End」は Source End と Processing End のどちらを指すか曖昧になるため、本書では原則として使用しない。

---

# 4. Time Model

## 4.1 Project Timeline Sample Rate

Project は 1 つの Project Timeline Sample Rate を持つ。

新規 Project の既定値は 48 kHz とする。

Timeline position と Timeline duration は、Project Timeline Sample Rate に基づく整数 sample として表す。

例:

```text
Project Timeline Sample Rate = 48000 Hz

timeline position 48000
= Project start から 1 second
```

Timeline 上の編集結果の正規位置は整数 sample 値で判定する。

---

## 4.2 Source Time Domain

各 MediaSource は native sample rate を持つ。

Source position と Source duration は、その MediaSource 固有の Source Time Domain における整数 sample とする。

例えば 44.1 kHz MediaSource では、

```text
44100 source samples = 1 second
```

である。

---

## 4.3 Timeline Time Domain と Source Time Domain

Clip は次の 2 つの範囲を同時に持つ。

```text
Timeline Range
[TimelineStart, TimelineStart + TimelineDuration)

Source Range
[SourceStart, SourceStart + SourceDuration)
```

両者は別 domain の値であり、同じ整数値を持つ必要はない。

例えば 44.1 kHz MediaSource を 48 kHz Project に配置した場合でも、Source Time Domain と Timeline Time Domain は独立して保持する。

---

## 4.4 Sample Rate 変換の基本方針

Source Time Domain と Timeline Time Domain の変換では、floating-point seconds を正規状態として使用しない。

概念上、sample rate 間の対応は整数比またはそれと同等の exact ratio として扱えることを前提とする。

例:

```text
44100 Hz source
→ 48000 Hz timeline

ratio = 48000 / 44100
      = 160 / 147
```

ただし具体的な内部表現や resampling algorithm は本書では定義しない。

変換に伴う rounding policy は次の原則を満たさなければならない。

* Media placement
* Split
* Trim
* Source End mapping
* Save / Load
* Playback
* Export

で一貫した規則を使用する。

また、不要な

```text
Source → Timeline → Source → Timeline
```

の繰り返し変換によって rounding error を累積させない。

具体的な rounding rule は未決定である。

---

# 5. SourceReference と Media

## 5.1 SourceReference

Clip は SourceReference を参照する。

SourceReference は将来の source type 追加を妨げないための概念境界であり、V1 で扱う具象 source は MediaSource のみとする。

V1:

```text
SourceReference
└─ MediaSource
```

将来的には、必要性が確認された場合に限り、例えば以下のような source type を追加可能とする。

```text
SourceReference
├─ MediaSource
├─ CompoundSource
├─ GeneratedSource
└─ SequenceSource
```

これら将来 source は V1 の実装対象ではない。

---

## 5.2 Media

Media は外部 Source Media とその参照・識別情報を表す。

単一音声ファイル、コンテナ内の個別 audio stream、複数ファイルの各 source は必要に応じて別 Media として Media Bin に登録される。

Media は少なくとも概念上、以下を持つ。

* source location
* file size
* optional hash
* native sample rate
* channel information
* available Source Range
* stream identification information

同一 Media は複数 Clip から参照できる。

Media Bin に Media が存在しても Timeline 上に Clip が存在する必要はない。

Missing Media は参照先が利用できない状態であり、Project の Clip、Group、Processing、Timeline 情報を削除する理由にはならない。

---

# 6. Track

## 6.1 Track の基本モデル

Track は Timeline または Mixing 上で音声を扱う論理的な処理単位である。

すべての Track が必ず Clip を持つとは定義しない。

V1 では Clip Track のみを扱う。

```text
Track
└─ Clip Track   // V1
```

将来的に必要性が確認された場合には、例えば Bus / Submix 等の別 Track role を追加可能とする。

```text
Track
├─ Clip Track
└─ Bus Track    // future
```

Bus Track は V1 の実装対象ではない。

---

## 6.2 Clip Track

Clip Track は Timeline 上で Clip が所属する編集レーンである。

Clip Track 上では複数 Clip の時間的 overlap を許可する。

各 Clip の Processing Output は Track Mix へ入力される。

3 Clip 以上が同時に overlap した場合も同じ規則で加算する。

Clip Track は少なくとも以下の状態を持てる。

* Lock
* Gain
* Pan
* Track Processing / Effect Chain

Pan の標準モデルは以下とする。

```text
-1.0 = Left
 0.0 = Center
+1.0 = Right
```

Gain / Pan Automation は V1 の Must ではない。

---

## 6.3 Track Lock

Lock 中の Clip Track は、通常の編集操作による Clip の作成、Move、Trim、Split、Delete、Ripple Delete の対象にならない。

Lock 中でも表示、Playback、Mixer state の確認は可能である。

Lock によって Edit Target から除外される規則は、Editing Operation の target resolution 時に適用される。

---

## 6.4 Track Mix

同一 Clip Track 上の Processing Output は原則として線形加算する。

例:

```text
Clip A output = 0.8
Clip B output = 0.8

Track Mix = 1.6
```

Track Mix 段階では 0 dBFS 超過を理由に即時 hard clipping しない。

内部 audio precision の具体形式は本書では定義しない。

---

# 7. Clip

Clip は 1 つの Clip Track に所属し、1 つの SourceReference の範囲を非破壊で参照する。

V1 では SourceReference は MediaSource である。

Clip は概念上、少なくとも以下を持つ。

* SourceReference
* Timeline start
* Timeline duration
* Source start
* Source duration
* Track association
* Clip Group membership
* Synchronization Member association
* Processing Stack

Clip の Split、Trim、Move、Delete は Source Media 自体を変更しない。

---

## 7.1 Trim

Trim は Clip が使用する Source Range と Timeline Range を変更する。

Trim によって除外された Source Range は破壊されない。

MediaSource の有効範囲内であれば、Clip 端を再び外側へ伸ばすことで以前除外した Source Range を復元できる。

---

## 7.2 Split

Split は 1 Clip を、同一 SourceReference を参照する複数 Clip segment へ分割する。

同一 sample rate の単純例:

```text
Clip A

timelineStart    = 48000
timelineDuration = 96000

sourceStart      = 0
sourceDuration   = 96000

Split at timeline sample 96000

→ A-left
  timeline [48000, 96000)
  source   [0, 48000)

→ A-right
  timeline [96000, 144000)
  source   [48000, 96000)
```

異なる sample rate 間の mapping は Time Model の変換 policy に従う。

---

# 8. Selection と Edit Target

Selection は UI / 編集操作の一時状態であり、選択中の Clip または Timeline range を表す。

Selection は以下と独立する。

* Clip Group
* Synchronization Group
* Synchronization Member
* Track Lock

Selection と Edit Target は同じとは限らない。

例えば Synchronization Group に属する 1 Clip のみを選択した状態で通常 Move を行った場合、

```text
Selection
= Clip A

Edit Target
= Synchronization Group の対象 Member / Clip
```

となり得る。

一方、同期保護 bypass 操作では、

```text
Selection
= Clip A

Edit Target
= Clip A
```

となる。

Editing Operation は最終的に解決された Edit Target に対して作用する。

UI の Selection から Edit Target をどう解決するかは、操作種別、Clip Group、Synchronization Group、Track Lock、Ripple Scope 等によって決定される。

Selection の Save / Load は未決定である。

---

# 9. Clip Group

Clip Group は利用者が任意の複数 Clip を一括編集するために作成する編集上の Group である。

Clip Group は Synchronization Group とは別概念である。

Group 作成は指定 Clip を Clip Group に所属させる。

Group への追加・削除は Clip Group membership のみを変更する。

Ungroup は Clip Group を解消する。

---

## 9.1 Clip Group Operation

Group Move の Edit Target は Clip Group の全所属 Clip である。

Group Delete の Edit Target も Clip Group の全所属 Clip である。

Selection が Clip Group の一部のみであっても、明示的な Group Operation では Edit Target は Group 全体へ拡張される。

通常の Selection Operation と Group Operation の違いは、操作実行前に判別可能でなければならない。

---

## 9.2 Clip Group と Synchronization Group

1 Clip は Clip Group と Synchronization Group の両方に関係できる。

Clip Group の作成、変更、削除、Ungroup は Synchronization Group または Synchronization Member を暗黙に変更しない。

Clip Group Operation と Synchronization protection が同時に作用する場合、Edit Target は両方の規則を満たすよう解決される。

複数 Group が交差する場合の target expansion の詳細規則は別途決定する。

---

# 10. Synchronization Model

## 10.1 Synchronization Group

Synchronization Group は複数 Synchronization Member 間の Timeline 上の時間関係を保護する。

「同期している」は「すべての Clip の Timeline start が同一」を意味しない。

Synchronization Group は Member 間の相対 Timeline offset を保持できる。

例:

```text
Synchronization Group

Member A offset = 0
Member B offset = +2400 samples
Member C offset = -480 samples
```

---

## 10.2 Synchronization Member

Synchronization Member は、Synchronization Group 内の 1 つの論理的な同期 source を表す。

例えば、

```text
Host microphone
Guest microphone
System audio
```

のような単位に相当し得る。

Synchronization Member は 0 個以上の Clip segment と関連できる。

例:

```text
Synchronization Group
├─ Member A
│  ├─ Clip A1
│  └─ Clip A2
├─ Member B
│  └─ Clip B1
└─ Member C
   └─ Clip C1
```

Split によって Clip segment が増えても Synchronization Member 自体は増えない。

Delete によって一時的に Clip segment がなくなっても Synchronization Member を保持可能とする。

これにより、Clip segment の存在と論理的な同期 source の存在を分離する。

---

## 10.3 通常の同期 Move

通常の Synchronization Group Move では、対象となる Synchronization Member に属する Clip segment を同一 Timeline delta だけ移動する。

任意の 2 Synchronization Member の相対 Timeline offset は操作前後で同じである。

---

## 10.4 Synchronization Group Split

Synchronization Group Split は、指定 Timeline sample position を内部に含む Clip segment のみを分割する。

分割位置を含まない Clip segment は変更しない。

例:

```text
Member A
[--------------------]

Member B
        [-------------]

split @ x
     ^
```

結果:

```text
Member A
[----][---------------]

Member B
        [-------------]
```

Member B の Clip は変更しない。

Split 後の左右 Clip は元と同じ Synchronization Member に所属する。

Synchronization Group membership と Member identity は維持される。

---

## 10.5 同期保護 bypass

Synchronization protection bypass は、Group や Member identity を破棄せず、今回の Editing Operation に限って同期保護を適用しない明示的操作である。

bypass 対象として少なくとも以下を許可する。

* Move
* Trim
* Split
* Delete

---

## 10.6 bypass Move

bypass Move は対象 Clip segment のみを移動する。

Synchronization Group と Synchronization Member は維持する。

結果として相対 offset が変更された場合、その新しい関係を Project 状態として保存する。

---

## 10.7 bypass Trim

bypass Trim は対象 Clip segment のみを Trim する。

Synchronization Member は維持する。

他 Member の Clip position は変更しない。

---

## 10.8 bypass Split

bypass Split は対象 Clip segment のみを分割する。

分割後の左右 Clip は、元 Clip と同じ Synchronization Member に所属する。

例:

```text
Before

Member A
└─ Clip A

After bypass Split

Member A
├─ Clip A-left
└─ Clip A-right
```

Synchronization Group は維持する。

---

## 10.9 bypass Delete

bypass Delete は対象 Clip segment のみを削除する。

削除された Clip が属していた Synchronization Member は自動的に削除しない。

残る Member と Group は維持する。

Member が一時的に 0 Clip segment になっても有効状態として扱える。

Member 自体を削除する操作は bypass Delete とは別の明示的操作とする。

---

# 11. Editing Operations

各 Editing Operation は少なくとも以下を定義する。

* input state
* Edit Target
* operation parameters
* valid condition
* resulting state
* preserved invariants

無効操作は Project 状態を変更しない。

---

## 11.1 Move

Move は対象 Clip の Timeline position を変更する。

保持するもの:

* SourceReference
* Source Range
* Processing Stack
* Clip Group membership
* Synchronization Member association

通常の同期 Move では対象同期 Clip に同一 delta を適用する。

---

## 11.2 Split

Split は Clip 内部の Timeline position に対して行う。

分割後:

* 同一 SourceReference を参照する
* Timeline Range の union は元 Clip と一致する
* Source Range の union は元 Clip と一致する
* Processing Stack の継承規則に従う
* Clip Group membership の継承規則に従う
* Synchronization Member は元 Clip と同じ Member を引き継ぐ

Processor state の Split 時継承詳細は別途定義する。

---

## 11.3 Trim Left

Trim Left は Timeline 左端と Source Range の開始側を対応して変更する。

Timeline 右端は維持する。

外側への再拡張を許可する。

---

## 11.4 Trim Right

Trim Right は Timeline 右端と Source Range の終了側を対応して変更する。

Timeline 左端は維持する。

外側への再拡張を許可する。

---

## 11.5 Delete

Delete は対象 Clip を Timeline から除去する。

Delete は後続 Clip を移動しない。

Media、SourceReference、他 Clip は変更しない。

---

## 11.6 Multi-selection Move

複数 Selection に対する Move は、Selection から Edit Target を解決した後、対象へ同一 delta を適用する。

Clip Group、Synchronization Group、Track Lock の規則によって Edit Target が Selection より広くなる場合がある。

---

## 11.7 Group Move / Delete

Clip Group Operation は Group 全所属 Clip を Edit Target とする。

Synchronization protection が適用される場合、Edit Target はさらに拡張され得る。

---

# 12. Ripple Editing

## 12.1 基本モデル

Ripple Editing は、

```text
Ripple Range
+
Ripple Scope
```

の組み合わせとして定義する。

Ripple Delete 自体は、UI の Selection、Track selection、Group state 等を直接参照しない。

UI / Application layer は Ripple Operation 実行前に Ripple Range と Ripple Scope を解決する。

---

## 12.2 Ripple Range

Ripple Range は Timeline 上の半開区間である。

```text
[Rstart, Rend)
```

長さ:

```text
RippleLength = Rend - Rstart
```

---

## 12.3 Ripple Scope

Ripple Scope は Ripple Operation の対象となる解決済み集合である。

Scope は例えば以下から導出され得る。

* selected Clip Track
* selected Track set
* selected Edit Target
* explicit Track set

ただし UI 上の導出方法は本書では決定しない。

---

## 12.4 Ripple Delete の基本意味

Ripple Delete は概念上、

1. Ripple Range と交差する対象 Clip を必要に応じて境界で分割する
2. Ripple Range 内の部分を削除する
3. Ripple Range より後方の Scope 内 Clip を RippleLength 分前方へ移動する

操作とみなす。

例:

```text
Before

A: [------------------------]
          [ Ripple Range ]

After conceptual split

A: [left][delete][right]

After Ripple Delete

A: [left][right]
```

これにより、部分 overlap Clip を特殊ケースとして扱わず、Split + Delete + Move と等価な状態遷移として定義できる。

---

## 12.5 完全に後方にある Clip

Ripple Scope 内で、

```text
Clip.start >= Rend
```

となる Clip は原則として、

```text
-RippleLength
```

だけ移動する。

Scope 外 Clip は移動しない。

---

## 12.6 Synchronization Group と Ripple

通常の Ripple Delete は Synchronization Group の相対 Timeline relation を破壊してはならない。

Ripple Scope 内の 1 Clip だけを移動すると Synchronization Member 間の関係が壊れる場合、通常 Ripple Operation では必要な同期対象まで Edit Target を拡張する。

同期関係を意図的に変更したい場合は明示的 bypass Operation を使用する。

Synchronization Group が Ripple Scope 境界をまたぐ場合の target resolution 詳細は未決定である。

---

## 12.7 Clip Group と Ripple

Clip Group 自体は時間同期保護を意味しない。

そのため Clip Group membership だけを理由に必ず Ripple 対象を拡張するとは限らない。

Clip Group と Ripple Scope の組み合わせにおける target resolution は別途定義する。

---

# 13. Clip Overlap / Crossfade

Overlap は同一 Clip Track 上で複数 Clip の Timeline Range が重なる状態である。

Overlap 自体は Crossfade ではない。

Fade が存在しない overlap は単純加算となる。

---

## 13.1 Crossfade

Crossfade は専用 DSP Object を必須とせず、

* 先行 Clip の Fade Out
* 後続 Clip の Fade In

の組み合わせとして表現する。

Fade In または Fade Out のどちらか一方だけを設定することもできる。

3 Clip 以上が overlap している場合も、それぞれ独立した Processing Output を Track Mix で加算する。

---

# 14. Processing Stack

Clip は SourceReference と Track Mix の間に順序付き Processing Stack を持つ。

Processor には少なくとも次を表現できる。

* Gain
* Fade
* Built-in Effect
* external Effect

V1 の external Effect は VST3 とする。

---

## 14.1 Stack Order

Processing Stack の順序は意味を持つ。

例:

```text
Source
→ Gain
→ Reverb
→ Fade
→ Track Mix
```

と、

```text
Source
→ Gain
→ Fade
→ Reverb
→ Track Mix
```

は異なる Processing Stack であり、異なる audio output を生成し得る。

SourceReference 自体と Track Mix は Processing Stack 内の並べ替え対象ではない。

---

## 14.2 Gain / Fade

Gain と Fade は Built-in Processor として Processing Stack 上に存在可能とする。

Fade Processor は、自身より前段から渡された Processing Output に対する Timeline 上の gain envelope として作用する。

したがって、

```text
Reverb
→ Fade
```

の場合、Reverb Tail も Fade Processor の対象になり得る。

一方、

```text
Fade
→ Reverb
```

の場合、Fade 後の入力を Reverb が処理するため、その Reverb Tail は Fade 終了後も残り得る。

これにより Processing Stack の並び順そのものが Tail behavior を制御できる。

---

## 14.3 Processor Extension Boundary

Processor は V1 の具体実装だけに閉じない概念境界とする。

V1:

```text
Processor
├─ Built-in Processor
└─ VST3 Processor
```

将来的な Plugin Format 等は、必要性が確認された場合に Processor type として追加可能とする。

ただし V1 では未使用形式を先行設計・実装しない。

---

# 15. Effect Tail

## 15.1 Source End と Processing End

Source End は Clip が SourceReference から音声を取得する終端である。

Processing End は Processing Stack の出力が終了する終端である。

stateful Processor が Source End 後にも出力を生成する場合、その区間を Effect Tail とする。

```text
Source
████████████████|

Processing Output
████████████████|▒▒▒▒▒▒▒▒▒
```

Effect Tail は Source Duration の延長ではない。

---

## 15.2 Tail Processing

Source End 後、必要な期間 Processing Stack へ silence input を与えて処理を継続可能とする。

Tail は Tail-generating Processor より後段の Processor を通過する。

例:

```text
Source End
↓
silence
→ Delay
→ EQ
→ Compressor
→ Track Mix
```

Delay の internal state が生成する Tail は EQ と Compressor を通過する。

---

## 15.3 Tail と後続 Clip

Effect Tail は同一 Track 上の後続 Clip と同時に存在できる。

```text
Clip A source
████████|

Clip A tail
        |▒▒▒▒▒▒

Clip B
        █████████
```

Track Mix:

```text
Clip A tail
+
Clip B output
```

として加算する。

---

## 15.4 Tail と Clip Operation

Clip Move は、その Clip 由来の Processing Output と Tail の Timeline 上の位置を同じ delta だけ移動する。

Clip Delete は、その Clip に由来する Tail も削除する。

Split 後の Tail state の具体的な継承・再計算規則は Processor semantics に依存するため別途設計する。

Export は定義された Processing End まで Tail を反映する。

---

## 15.5 Tail Policy

Tail behavior は TailPolicy として独立して扱えるモデルを採る。

概念上、少なくとも以下を表現可能とする。

```text
TailPolicy

Reported
Manual(duration)
BoundedAutomatic(maxDuration)
CutAtSourceEnd
```

デフォルト動作は概念的に、

```text
finite reported tail
→ Reported tail を使用

unknown / infinite
→ BoundedAutomatic fallback
```

とする。

fallback duration の具体値は domain model に固定しない。

Application / Project default または user setting として設定可能な構造を許容する。

Manual Tail と CutAtSourceEnd は将来的な user control として扱える。

---

# 16. Signal Flow / Mixing

標準概念モデルは次の通りとする。

```text
SourceReference
↓
Clip Processing Stack
↓
Track Mix
↓
Track Processing / FX
↓
Track Gain / Pan
↓
Master Processing / FX
↓
Master Output
```

同一 Clip Track の overlap は、

```text
Clip A Processing Output ─┐
                          ├─ Sum → Track Mix
Clip B Processing Output ─┘
```

として処理する。

Track Mix 段階で即時 hard clipping は行わない。

---

# 17. Persistence

以下は原則として Project の永続状態とする。

* Project Timeline Sample Rate
* Media reference
* Media identification information
* Missing Media state
* Track structure
* Track role
* Track Lock
* Track Gain
* Track Pan
* Track Processing state
* Clip Track association
* Clip Timeline Range
* Clip Source Range
* SourceReference
* Clip Group membership
* Synchronization Group
* Synchronization Member
* Synchronization Member と Clip segment の関係
* Synchronization relative offset
* Clip Processing Stack
* Processor order
* Processor state
* Missing Plugin state
* Fade state
* Tail Policy

Project File は Source Media を無条件に埋め込まない。

Missing Media または Missing Plugin が存在しても、利用可能な Project state を失わず読み込み・再保存できる。

Selection の永続化は未決定である。

具体的な Project File format は本書では定義しない。

---

# 18. Undo / Redo

Undo / Redo は Project state transition を戻し、再適用する。

最低限、以下を対象とする。

* Move
* Trim
* Split
* Delete
* Ripple Delete
* Clip Group
* Ungroup
* Synchronization Group membership change
* Synchronization Member association change
* synchronization bypass edit
* Fade change
* Processor add
* Processor remove
* Processor reorder
* Track Gain
* Track Pan

Undo 後は、影響範囲の domain state が操作直前と等価でなければならない。

対象には少なくとも以下を含む。

* Clip identity
* Timeline Range
* Source Range
* Track association
* Clip Group membership
* Synchronization Group
* Synchronization Member
* Processor order
* Processor state
* Fade state
* Tail Policy
* Missing state

Redo 後は操作直後と等価な状態へ戻る。

Undo / Redo は Source Media を変更しない。

Selection を Undo / Redo に含めるかは未決定である。

---

# 19. Invariants

以下を AudioNLE editing model の invariant とする。

* Source Media は編集によって変更されない。
* Project Save / Load は Source Media を変更しない。
* Undo / Redo は Source Media を変更しない。
* Timeline の正規 position / duration は Project Timeline Sample Rate に基づく整数 sample である。
* Source Time Domain と Timeline Time Domain は別 domain である。
* Clip は SourceReference を参照する。
* V1 の SourceReference は MediaSource である。
* Trim は Source Range を破壊しない。
* Media 有効範囲内であれば Trim 後の Source Range を再拡張できる。
* Clip Group と Synchronization Group は別概念である。
* Synchronization Group と Synchronization Member を区別する。
* Split により Synchronization Member 自体は増えない。
* bypass Split 後の Clip segment は元 Synchronization Member を引き継ぐ。
* bypass Delete は Synchronization Member を暗黙に削除しない。
* 通常の Synchronization Group editing は相対 Timeline offset を sample-accurate に保持する。
* synchronization bypass は Group / Member membership を暗黙に削除しない。
* Selection と Edit Target は別概念である。
* Processing Stack の順序は意味を持つ。
* Effect Tail は Source Duration と別概念である。
* Effect Tail は Source End で不必要に切断されない。
* Effect Tail は後段 Processor を通過できる。
* Clip overlap は許可される。
* overlap した Processing Output は原則として加算される。
* Track Mix は即時 hard clipping を行わない。
* Lock 中の Clip Track の Clip は通常 Editing Operation の Edit Target にならない。
* invalid Editing Operation は Project state を変更しない。
* Ripple Range と Ripple Scope は別概念である。
* Ripple Operation の domain logic は UI selection resolution と分離する。
* V1 の Track type は Clip Track のみだが、Track domain は Clip 保持だけに固定しない。
* V1 に存在しない将来機能を理由として不要な具象 type を先行実装しない。

---

# 20. 拡張境界

AudioNLE は V1 の実装範囲を必要最小限に保つ。

一方で、以下の domain boundary は将来の追加を妨げないものとする。

## Source

V1:

```text
SourceReference
└─ MediaSource
```

将来必要に応じて Source type を追加可能とする。

---

## Track

V1:

```text
Track
└─ Clip Track
```

将来必要に応じて Bus / Submix 等を追加可能とする。

---

## Processor

V1:

```text
Processor
├─ Built-in Processor
└─ VST3 Processor
```

将来必要に応じて別 Processor type を追加可能とする。

---

拡張境界を設けることは、将来機能を V1 で実装することを意味しない。

V1 では実際に必要な具象 type のみを実装する。

---

# 21. 未決定事項

以下は本 editing model では確定しない。

* Source Time Domain と Timeline Time Domain 間の具体的 rounding rule。
* resampling algorithm。
* Project Timeline Sample Rate を Project 作成後に変更可能とするか。
* Project Sample Rate 変更時の既存 Clip position / duration の扱い。
* Snapping target。
* Snapping tolerance。
* Snapping priority。
* Ripple Scope の UI 上の既定決定方法。
* Ripple Scope 境界をまたぐ Synchronization Group の target resolution。
* Clip Group と Ripple Scope が交差した場合の target resolution。
* 複数 Clip Group / Synchronization Group の Edit Target expansion conflict。
* Fade curve。
* Gain / Fade Processor の default Stack position。
* Built-in Processor の並べ替え制約。
* Split 時の Processing Stack / stateful Processor state の継承詳細。
* unknown / infinite Effect Tail 用 BoundedAutomatic の default duration。
* TailPolicy を Project default / application default / per-Clip のどこまで設定可能にするか。
* Selection の Save / Load。
* Selection の Undo / Redo。
* overlap Clip の UI 表現。
* internal floating-point precision。
* Master Output での final clipping / limiting policy。
* Track role の具体的な将来拡張。
* SourceReference の具体的な将来 type。
* Project File format。
* Project schema versioning / migration。

---

# 22. Requirements との整合性レビュー

本モデルは requirements.md の Media、Time、Timeline Editing、Synchronization、Processing、Mixer、Project、Playback、Export 要件を、実装方式から独立した domain state と state transition として具体化している。

Source Media の不変性、整数 Timeline sample position、非破壊 Trim、Track overlap、Track Mix の加算、Processing Stack の順序、Effect Tail、Undo / Redo は requirements.md と一致する。

Clip Group は任意の一括編集 Group、Synchronization Group は時間関係を保護する Group として分離している。

さらに Synchronization Member を導入し、

```text
Synchronization Group
└─ Synchronization Member
   └─ Clip segment(s)
```

という関係にすることで、Split / Delete 後にも論理的な同期 source を維持できる。

Selection と Edit Target を区別することで、通常編集、Clip Group operation、Synchronization protection、bypass operation、Ripple operation の target resolution を UI 選択状態から分離している。

Ripple Delete は `Ripple Range + Ripple Scope` として定義し、部分 overlap Clip を Split / Delete / Move と等価な状態遷移として扱うことで、将来の Ripple Insert、Extract、Insert Edit 等の NLE 操作へ拡張可能な基礎を保つ。

SourceReference を導入し、V1 では MediaSource のみを実装対象とすることで、現在の実装を単純に保ちながら、将来的な Compound / Generated Source 等によって Clip model を全面変更する必要を減らす。

Track は V1 では Clip Track のみを扱うが、Track 自体を「Clip を保持するもの」に限定しないことで、将来的な Bus / Submix 等の追加余地を残す。

Processing Stack は Built-in Processor と VST3 Processor のみを V1 対象とし、未使用 Plugin Format や generic node graph を先行設計しない。

Effect Tail は Source Duration から独立し、TailPolicy を独立した概念として扱うことで finite、unknown、infinite、manual、cut 等の将来的な処理方針を domain model の破壊なしに追加可能とする。

本モデルは V1 の実装スコープを広げることではなく、V1 を必要最小限に保ったまま将来の変更による中心 domain model の作り直しを避けることを目的とする。

---

# 23. 仕様上の確認事項

実装前に、少なくとも以下は ADR または Design 文書で確定する必要がある。

1. Source / Timeline Sample Domain 間の rounding rule。
2. Ripple Scope の具体的な target resolution。
3. unknown / infinite Effect Tail に対する default TailPolicy。
4. Split 時に stateful Processing Stack をどう継承・再初期化するか。
5. Project schema の versioning / migration 方針。

これらを確定する際も、本書で定義した以下の境界を崩してはならない。

* Source Time Domain と Timeline Time Domain の分離
* Clip Group と Synchronization Group の分離
* Synchronization Group と Synchronization Member の分離
* Selection と Edit Target の分離
* SourceReference と Clip の分離
* Track role と Clip Track の分離
* Processing Stack と具体 Plugin Format の分離
* Source Duration と Effect Tail の分離
* Ripple Range と Ripple Scope の分離
