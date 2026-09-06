# AudioNLE 用語集

## 目的

本書は AudioNLE の製品・編集ドメインで用いる正式名称を定める。

`editing-model.md` の用語と意味を正とし、要件、UI、設計、コード設計で同じ概念に別の正式名称が与えられることを防ぐための参照資料とする。

本書は実装上の型名、外部連携方式、ファイル形式、API を定めない。

V1 は初期バージョンの対象範囲を表す。

将来候補として示す概念は、現時点で実装・提供を約束するものではなく、既存のドメイン境界を不必要に狭めないための拡張余地を表す。

---

# Project / Time

## Project

* **正式名称:** Project
* **定義:** Media、Timeline、Track、編集状態、Processing、Mixing、永続状態をまとめる編集単位。
* **関連:** Timeline、Media Bin、Track、Master Output。
* **混同しない:** Source Media。Project は外部素材そのものではない。

---

## Project Timeline Sample Rate

* **正式名称:** Project Timeline Sample Rate
* **定義:** Project が 1 つだけ持つ、Timeline の時間単位となる sample rate。新規 Project の既定値は 48 kHz。
* **関連:** Timeline Time Domain、Timeline Position、Timeline Duration。
* **混同しない:** MediaSource の native sample rate。後者は Source Time Domain の基準である。

---

## Timeline

* **正式名称:** Timeline
* **定義:** Track と Clip を配置する Project の正規時間軸。
* **関連:** Timeline Position、Timeline Duration、Ripple Range。
* **混同しない:** Source Time Domain。Timeline は単なる UI 表示ではなく、編集状態の正規時間軸である。

---

## Timeline Time Domain

* **正式名称:** Timeline Time Domain
* **定義:** Project Timeline Sample Rate に基づく整数 sample 軸。
* **関連:** Timeline、Timeline Position、Timeline Duration。
* **混同しない:** Source Time Domain。

---

## Timeline Position

* **正式名称:** Timeline Position
* **定義:** Timeline Time Domain における整数 sample の位置。
* **関連:** Timeline Start、Ripple Range、Relative Timeline Offset。
* **混同しない:** Source Position、Source Duration。

**例:**

48 kHz Project の Timeline Position `48000` は Project 開始から 1 秒を表す。

---

## Timeline Duration

* **正式名称:** Timeline Duration
* **定義:** Timeline Time Domain における整数 sample の長さ。
* **関連:** Timeline Position、Ripple Length。
* **混同しない:** Source Duration。

Clip の Timeline Range は原則として次の半開区間で表す。

```text
[Timeline Position, Timeline Position + Timeline Duration)
```

---

## Source Time Domain

* **正式名称:** Source Time Domain
* **定義:** MediaSource の native sample rate に基づく整数 sample 軸。
* **関連:** Source Position、Source Duration、Source End。
* **混同しない:** Timeline Time Domain。

両 domain の変換規則は必要だが、同一 domain として扱わない。

---

## Source Position

* **正式名称:** Source Position
* **定義:** Source Time Domain における整数 sample の位置。
* **関連:** Source Range、Trim、Source End。
* **混同しない:** Timeline Position、Timeline Duration。

---

## Source Duration

* **正式名称:** Source Duration
* **定義:** Source Time Domain における整数 sample の長さ。
* **関連:** Source Position、Source End、Trim。
* **混同しない:** Timeline Duration、Effect Tail。

**例:**

44.1 kHz Source の Source Duration `44100` は Source 上の 1 秒を表す。

---

## Source End

* **正式名称:** Source End
* **定義:** Clip が SourceReference から音声を取得する Source Range の終端。
* **関連:** Source Duration、Effect Tail、Processing End。
* **混同しない:** Processing End。

Source End 後にも Effect Tail が存在し得る。

---

## Processing End

* **正式名称:** Processing End
* **定義:** Clip Processing Output が終了する Timeline 上の終端。
* **関連:** Effect Tail、Clip Processing Output、Export。
* **混同しない:** Source End、Source Duration。

Effect Tail が存在する場合、Processing End は Source End に対応する Timeline Position より後になることがある。

---

# Source / Media

## Source Media

* **正式名称:** Source Media
* **定義:** Project の外部に存在する、編集前の音声内容・素材そのもの。
* **関連:** Media、MediaSource、Missing Media。
* **混同しない:** Media。

AudioNLE の編集操作は Source Media を変更しない。

---

## Media

* **正式名称:** Media
* **定義:** Source Media と、その source location、file size、optional hash、native sample rate、channel / stream 情報、利用可能な Source Range 等の参照・識別情報を表す Project 内の概念。
* **関連:** Media Bin、MediaSource、Missing Media。
* **混同しない:** Clip。

Media 自体は Timeline Position を持たず、同一 Media を複数 Clip から参照できる。

---

## MediaSource

* **正式名称:** MediaSource
* **定義:** 外部 Source Media を参照する SourceReference。
* **関連:** SourceReference、Media、Source Time Domain。
* **混同しない:** Media。

V1 で扱う唯一の SourceReference である。

MediaSource は Clip の入力 Source としての役割を持ち、Media は素材の参照・識別情報としての役割を持つ。

---

## SourceReference

* **正式名称:** SourceReference
* **定義:** Clip が参照する入力 Source を表す抽象的な概念境界。
* **関連:** Clip、MediaSource、Source Range。
* **混同しない:** Source Media。

SourceReference は外部ファイルそのものではない。

V1 では次のみを実装対象とする。

```text
SourceReference
└─ MediaSource
```

`SourceReference = MediaSource` は V1 の対象範囲を表すのであって、概念上の同義語ではない。

将来的に必要性が確認された場合、別の SourceReference を追加可能とする。

---

## Media Bin

* **正式名称:** Media Bin
* **定義:** Project に取り込まれた Media を Timeline 上の Clip から独立して管理する集合。
* **関連:** Media、Clip、Missing Media。
* **混同しない:** Timeline。

Clip を削除しても、明示的に除去されない限り Media Bin 内の Media は残る。

---

## Missing Media

* **正式名称:** Missing Media
* **定義:** Media の参照先となる Source Media が利用できない状態。
* **関連:** Media、Media Bin、Project Persistence。
* **混同しない:** Media の削除。

Missing Media でも Project、Clip、Group、Processing の状態は保持される。

---

## Source / Media の関係

```text
Clip
└─ SourceReference
   └─ MediaSource (V1)
      └─ Media
         └─ Source Media
```

この図は概念上の参照関係であり、所有権や実装上の lifetime を示さない。

---

# Track / Mixing

## Track

* **正式名称:** Track
* **定義:** Timeline または Mixing 上で音声を扱う論理的な処理単位。
* **関連:** Clip Track、Track Mix、Master Output。
* **混同しない:** Clip Track。

V1 では Clip Track のみを扱うが、Track 自体を「Clip を保持するもの」に限定しない。

将来的に必要性が確認された場合、Bus / Submix 等の Track role を追加可能とする。

---

## Clip Track

* **正式名称:** Clip Track
* **定義:** Clip が Timeline 上で所属し、各 Clip Processing Output を Track Mix にまとめる Track。
* **関連:** Clip、Track Mix、Track Gain、Track Pan、Track Processing Stack。
* **混同しない:** 一般名としての Audio Track。

V1 の唯一の Track role である。

---

## Track Mix

* **正式名称:** Track Mix
* **定義:** 同一 Clip Track の各 Clip Processing Output を原則として線形加算した結果。
* **関連:** Overlap、Track Processing Stack、Track Gain、Track Pan。
* **混同しない:** Master Output。

Track Mix 段階では即時 hard clipping を行わない。

---

## Track Gain

* **正式名称:** Track Gain
* **定義:** Track Processing Stack の後に適用する Track 単位の音量設定。
* **関連:** Clip Track、Track Mix、Track Processing Stack、Master Output。
* **混同しない:** Clip Processing Stack 内の Gain Processor。

---

## Track Pan

* **正式名称:** Track Pan
* **定義:** Track Processing Stack の後に適用する Track 単位の定位設定。
* **関連:** Clip Track、Track Mix、Track Processing Stack、Master Output。
* **混同しない:** Clip Processing Stack 内の Processor。

標準モデル:

```text
-1.0 = Left
 0.0 = Center
+1.0 = Right
```

---

## Master Output

* **正式名称:** Master Output
* **定義:** Track 出力を統合し、Master Processing Stack 後に Output / Export へ渡される最終結果。
* **関連:** Track Mix、Master Processing Stack、Export。
* **混同しない:** Track Mix。

---

# Clip / Editing

## Clip

* **正式名称:** Clip
* **定義:** SourceReference の Source Range を Timeline Range に対応付ける、Clip Track に属する非破壊編集オブジェクト。
* **関連:** SourceReference、Clip Track、Clip Processing Stack、Clip Group、Synchronization Member。
* **混同しない:** Media、Source Media。

---

## Clip Segment

* **正式名称:** Clip Segment
* **定義:** Split 等によって生じた Clip の時間的な断片を説明する際に用いる用語。
* **関連:** Clip、Split、Synchronization Member。
* **混同しない:** Synchronization Member。

Clip Segment は別種の domain object を意味するとは限らず、Split 後の Clip を同期関係の文脈で説明するための用語として使う。

---

## Selection

* **正式名称:** Selection
* **定義:** 一時的に選択された Clip または Timeline Range。
* **関連:** Edit Target、Group Operation、Ripple Scope。
* **混同しない:** Clip Group、Synchronization Group、Edit Target。

Selection は永続的な所属関係を意味しない。

---

## Edit Target

* **正式名称:** Edit Target
* **定義:** 現在の Editing Operation によって実際に変更される、解決済みの対象集合。
* **関連:** Selection、Clip Group、Synchronization Protection、Ripple Scope。
* **混同しない:** Selection。

通常の同期操作では 1 Clip の Selection に対して複数 Clip / Synchronization Member が Edit Target となり得る。

---

## Clip Group

* **正式名称:** Clip Group
* **定義:** 利用者が任意の複数 Clip をまとめて編集するための編集上の Group。
* **関連:** Group Operation、Selection、Synchronization Group。
* **混同しない:** Synchronization Group。

Clip Group は時間的な同期保護を意味しない。

---

## Group Operation

* **正式名称:** Group Operation
* **定義:** 指定した Clip Group の所属 Clip を Edit Target とする Editing Operation。
* **関連:** Clip Group、Edit Target、Synchronization Protection。
* **混同しない:** Synchronization Protection。

例:

* Group Move
* Group Delete

Synchronization Protection によって Edit Target がさらに拡張される場合がある。

---

## Move

* **正式名称:** Move
* **定義:** Clip または解決済み Edit Target の Timeline Position を変更する Editing Operation。
* **関連:** Multi-selection Move、Group Operation、Synchronization Bypass。
* **混同しない:** Ripple Delete。

Move 自体は削除範囲に応じて後続 Clip を自動移動しない。

---

## Split

* **正式名称:** Split
* **定義:** Clip 内部の Timeline Position で Clip を複数 Clip に分割する Editing Operation。
* **関連:** Clip Segment、Synchronization Member、Trim。
* **混同しない:** Trim。

分割後の Timeline Range と Source Range の union は分割前と一致する。

Source Media は変更しない。

---

## Trim

* **正式名称:** Trim
* **定義:** Clip の左端または右端を変更し、対応する Source Range を非破壊で変更する Editing Operation。
* **関連:** Trim Left、Trim Right、Source Range。
* **混同しない:** Delete。

Trim で除外した Source Range は、Source Media の有効範囲内であれば再拡張できる。

---

## Delete

* **正式名称:** Delete
* **定義:** Edit Target となった Clip を Timeline から除去する Editing Operation。
* **関連:** Ripple Delete、Media Bin。
* **混同しない:** Ripple Delete。

Delete は後続 Clip の Timeline Position を変更しない。

Delete は Media を削除しない。

Synchronization Bypass Delete は Synchronization Member を暗黙に削除しない。

---

## Ripple Delete

* **正式名称:** Ripple Delete
* **定義:** Ripple Range を削除し、Ripple Scope 内の後続対象を Ripple Length 分前方へ移動する Editing Operation。
* **関連:** Ripple Range、Ripple Scope、Ripple Length、Synchronization Protection。
* **混同しない:** Delete。

部分 overlap は概念上、

```text
Split
→ Delete
→ Move
```

と等価な状態遷移として扱える。

---

## Overlap

* **正式名称:** Overlap
* **定義:** 同一 Clip Track 上で複数 Clip の Timeline Range が重なる状態。
* **関連:** Track Mix、Crossfade、Fade In、Fade Out。
* **混同しない:** Crossfade。

Fade が設定されていない Overlap では各 Clip Processing Output を単純加算する。

---

## Crossfade

* **正式名称:** Crossfade
* **定義:** Overlap する先行 Clip の Fade Out と後続 Clip の Fade In の組合せによって表現される状態。
* **関連:** Overlap、Fade In、Fade Out。
* **混同しない:** Overlap。

専用 DSP object を必須としない。

片側の Fade しか存在しない状態は Crossfade ではない。

---

## Fade In

* **正式名称:** Fade In
* **定義:** Clip Processing Stack 上の Fade Processor として表現可能な、開始側に対する時間上の gain envelope。
* **関連:** Clip Processing Stack、Crossfade、Effect Tail。
* **混同しない:** Track Gain。

Fade curve の詳細は別途定義する。

---

## Fade Out

* **正式名称:** Fade Out
* **定義:** Clip Processing Stack 上の Fade Processor として表現可能な、終了側に対する時間上の gain envelope。
* **関連:** Clip Processing Stack、Crossfade、Effect Tail。
* **混同しない:** Track Gain。

Fade curve の詳細は別途定義する。

---

# Synchronization

## Synchronization Group

* **正式名称:** Synchronization Group
* **定義:** 複数 Synchronization Member 間の Timeline 上の時間関係を保護する Group。
* **関連:** Synchronization Member、Synchronization Protection、Relative Timeline Offset。
* **混同しない:** Clip Group。

---

## Synchronization Member

* **正式名称:** Synchronization Member
* **定義:** Synchronization Group 内で独立した相対時間関係を持つ、論理的な同期対象。
* **関連:** Synchronization Group、Clip Segment、Relative Timeline Offset。
* **混同しない:** Clip。

Synchronization Member は 0 個以上の Clip Segment と関連できる。

例として、次のような対象が Synchronization Member になり得る。

* microphone recording
* participant audio
* system audio
* camera audio
* imported stem
* その他、独立した同期対象

具体的な収録方法や source 種別に依存しない。

```text
Synchronization Group
└─ Synchronization Member
   └─ Clip Segment(s)
```

Split によって Clip Segment が増えても Synchronization Member 自体は増えない。

Synchronization Bypass Delete によってすべての Clip Segment が削除されても、Synchronization Member は暗黙に削除されない。

0 Clip Segment の Synchronization Member も有効状態として扱える。

---

## Synchronization Protection

* **正式名称:** Synchronization Protection
* **定義:** 通常の Editing Operation において Synchronization Group の Relative Timeline Offset を保つため、必要な Synchronization Member / Clip まで Edit Target を拡張する規則。
* **関連:** Edit Target、Synchronization Group、Relative Timeline Offset。
* **混同しない:** Clip Group。

Synchronization Protection は Group membership を作成・削除する操作ではない。

---

## Synchronization Bypass

* **正式名称:** Synchronization Bypass
* **定義:** 現在の Editing Operation に限って Synchronization Protection を一時的に適用せず、指定された Clip のみを Edit Target とする明示的な操作。
* **関連:** Synchronization Protection、Edit Target、Relative Timeline Offset。
* **混同しない:** Ungroup、Synchronization Member の削除。

Bypass は Synchronization Group / Member を破棄しない。

個別編集後の新しい相対関係は Project 状態として保存される。

---

## Relative Timeline Offset

* **正式名称:** Relative Timeline Offset
* **定義:** Synchronization Group 内の Synchronization Member 間で保持する Timeline Position の相対差。
* **関連:** Synchronization Protection、Synchronization Group Move、Ripple Delete。
* **混同しない:** 同一 Timeline Position。

同期している Synchronization Member が同じ開始位置を持つ必要はない。

通常の同期編集では Relative Timeline Offset を sample-accurate に維持する。

---

# Processing / Effects

## Processing Stack

* **正式名称:** Processing Stack
* **定義:** 音声処理を順序付き Processor 列として表す上位概念。
* **関連:** Clip Processing Stack、Track Processing Stack、Master Processing Stack、Processor。
* **混同しない:** Effect のみの並び。

Processor の順序は音声結果に意味を持つ。

---

## Clip Processing Stack

* **正式名称:** Clip Processing Stack
* **定義:** Clip の SourceReference と Track Mix の間に置かれる Processing Stack。
* **関連:** Clip、Processor、Clip Processing Output、Effect Tail。
* **混同しない:** Track Processing Stack、Master Processing Stack。

Clip Gain、Fade、Built-in Effect、VST3 Processor 等を含められる。

---

## Track Processing Stack

* **正式名称:** Track Processing Stack
* **定義:** Track Mix の後、Track Gain / Pan の前に適用される Processing Stack。
* **関連:** Track Mix、Processor、Track Gain、Track Pan。
* **混同しない:** Clip Processing Stack、Master Processing Stack。

---

## Master Processing Stack

* **正式名称:** Master Processing Stack
* **定義:** 各 Track の出力を統合した後、Master Output の前に適用される Processing Stack。
* **関連:** Master Output、Processor、Export。
* **混同しない:** Clip Processing Stack、Track Processing Stack。

---

## Processor

* **正式名称:** Processor
* **定義:** Processing Stack 上で音声またはその時間上の振る舞いを処理する要素。
* **関連:** Built-in Processor、Effect Processor。
* **混同しない:** Effect。

Processor は Effect より上位の概念であり、Gain / Fade 等の非 Effect 処理も含む。

---

## Built-in Processor

* **正式名称:** Built-in Processor
* **定義:** AudioNLE 自身が定義する Processor。
* **関連:** Processor、Effect Processor、Processing Stack。
* **混同しない:** Effect Processor。

例:

* Gain Processor
* Fade Processor
* Built-in EQ
* Built-in Compressor

すべての Built-in Processor が Effect Processor とは限らない。

---

## Effect Processor

* **正式名称:** Effect Processor
* **定義:** 音声 Effect を表す Processor。
* **関連:** Processor、VST3 Processor、Effect Tail。
* **混同しない:** Processor 全体。

Built-in Effect または external Effect を表現できる。

---

## VST3 Processor

* **正式名称:** VST3 Processor
* **定義:** V1 で扱う external Effect を表す Effect Processor。
* **関連:** Effect Processor、Processing Stack、Missing Plugin。
* **混同しない:** Processor 全体。

VST3 API や Plugin Host の具体実装方式を意味しない。

---

## Effect Tail

* **正式名称:** Effect Tail
* **定義:** stateful Processor が Source End 後にも生成する Clip Processing Output の区間。
* **関連:** Source End、Processing End、Tail Policy。
* **混同しない:** Source Duration の延長。

Effect Tail は Source Media の参照範囲を変更しない。

後段 Processor を通過し、後続 Clip と Track Mix 上で同時に加算され得る。

---

## Tail Policy

* **正式名称:** Tail Policy
* **定義:** Effect Tail の終了・打切りの扱いを定める独立した方針。
* **関連:** Effect Tail、Processing End、Export。
* **混同しない:** Source Duration。

概念上、少なくとも以下を表現可能とする。

```text
Reported
Manual(duration)
BoundedAutomatic(maxDuration)
CutAtSourceEnd
```

unknown / infinite Tail の既定 fallback の具体値は別途決定する。

---

## Clip Processing Output

* **正式名称:** Clip Processing Output
* **定義:** Clip の SourceReference を Clip Processing Stack に通した出力。
* **関連:** Clip Processing Stack、Effect Tail、Track Mix。
* **混同しない:** Source Media、Track Mix。

Source に対応する出力と Effect Tail の両方を含み得る。

---

# Ripple

## Ripple Range

* **正式名称:** Ripple Range
* **定義:** Ripple Operation によって削除または挿入される Timeline 上の半開時間範囲。
* **関連:** Ripple Length、Ripple Delete、Timeline Time Domain。
* **混同しない:** Ripple Scope。

```text
[Rstart, Rend)
```

として表す。

---

## Ripple Scope

* **正式名称:** Ripple Scope
* **定義:** Ripple Operation の操作対象として事前に解決済みの Track / Clip / Edit Target の集合。
* **関連:** Ripple Range、Edit Target、Synchronization Protection。
* **混同しない:** Selection。

Ripple Scope は UI の Selection そのものではない。

---

## Ripple Length

* **正式名称:** Ripple Length
* **定義:** Ripple Range の Timeline Duration。
* **関連:** Ripple Range、Ripple Delete。
* **混同しない:** Source Duration。

```text
Ripple Length = Rend - Rstart
```

Timeline Time Domain の整数 sample 数である。

---

# Naming Rules

* 同じ概念に複数の正式名称を作らない。
* 新しい用語を導入する前に、本書の既存用語との重複を確認する。
* 一般的な audio / NLE 用語を、通例と異なる意味で再定義しない。
* 異なる意味が必要な場合は修飾語を付ける。
* `Group` 単独では原則として使用しない。
* `Clip Group` または `Synchronization Group` と明示する。
* `Clip End` は正式用語として避ける。
* 必要に応じて `Source End` または `Processing End` を使う。
* `Time` 単独では Source / Timeline domain が不明なため避ける。
* `Position`、`Duration`、`Range` は必要に応じて domain を明示する。
* `Effect` と `Processor` を同義語として扱わない。
* `Processor` は上位概念であり、`Effect Processor` はその一部である。
* 全処理列は `Processing Stack` と呼ぶ。
* Clip / Track / Master の処理列はそれぞれ `Clip Processing Stack`、`Track Processing Stack`、`Master Processing Stack` と呼ぶ。
* UI 上の簡略表示が正式ドメイン用語と異なる場合、仕様・要件・設計では正式用語を使用する。
* V1 の具象 type と、将来拡張を可能にする概念境界を混同しない。
* 将来候補を正式機能として先行定義しすぎない。

---

# Deprecated / Avoided Terms

| 用語 | 扱い | 理由と代替 |
| ------------------------------- | ----------------- | ------------------------------------------------------------------------ |
| Group | 避ける | Clip Group と Synchronization Group を区別できない。 |
| Clip End | 避ける | Source End / Processing End のどちらか不明。 |
| Time | 避ける | Source Time Domain / Timeline Time Domain を区別できない。 |
| Effect Chain | 避ける | Gain / Fade を含む全処理列を表現できず Processing Stack と競合する。正式用語は Processing Stack。 |
| Track Processing / Effect Chain | 廃止 | Track Processing Stack に統一する。 |
| Master Processing / Master FX | 仕様文書では避ける | Master Processing Stack に統一する。UI で `Master FX` と表示することは可能。 |
| Source Clip | 避ける | Source Media、MediaSource、Clip のどれを意味するか曖昧。 |
| Audio Track | UI では許容、ドメインでは避ける | V1 の UI には自然だが、正式ドメイン用語は Clip Track。 |
| FX Chain | UI では許容、ドメインでは避ける | 正式な処理順は Processing Stack。 |
| Sync Group | UI では許容、仕様では避ける | 正式名称は Synchronization Group。 |

---

# Relationship Map

次の図は所有権や実装上の lifetime ではなく、概念間の関係を表す。

```text
Project
├─ Timeline
├─ Media Bin
│  └─ Media
│     └─ Source Media
│
├─ Track
│  └─ Clip Track (V1)
│     ├─ Clip
│     │  ├─ SourceReference
│     │  │  └─ MediaSource (V1)
│     │  ├─ Clip Processing Stack
│     │  ├─ Clip Group membership
│     │  └─ Synchronization Member association
│     │
│     ├─ Track Mix
│     ├─ Track Processing Stack
│     └─ Track Gain / Pan
│
├─ Synchronization Group
│  └─ Synchronization Member
│     └─ Clip Segment(s)
│
└─ Master
   ├─ Master Processing Stack
   └─ Master Output
```

概念上の主要 signal flow は次の通り。

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

---

# V1 と拡張境界

## SourceReference

V1:

```text
SourceReference
└─ MediaSource
```

将来的に必要性が確認された場合のみ Source type を追加する。

---

## Track

V1:

```text
Track
└─ Clip Track
```

将来的に必要性が確認された場合のみ Bus / Submix 等の Track role を追加する。

---

## Processor

V1:

```text
Processor
├─ Built-in Processor
└─ VST3 Processor
```

将来的に別 Plugin Format 等が必要になった場合は Processor type として追加可能とする。

V1 で未使用の形式を先行設計しない。

---

# 整合性レビュー

本書は `requirements.md` および `editing-model.md` で定義された以下の概念を維持する。

* Timeline の整数 sample model
* Source Time Domain と Timeline Time Domain の分離
* 非破壊 Clip
* SourceReference と MediaSource の分離
* Track と Clip Track の分離
* Track overlap と Track Mix の加算
* Clip Group と Synchronization Group の分離
* Synchronization Group と Synchronization Member の分離
* Selection と Edit Target の分離
* Synchronization Protection と Synchronization Bypass の分離
* Source End と Processing End の分離
* Processing Stack の順序性
* Effect Tail と Source Duration の分離
* Ripple Range と Ripple Scope の分離
* Processor と Effect Processor の分離
* V1 の具象 type と将来拡張境界の分離

正式な処理列の用語は `Processing Stack` に統一する。

以下を正式名称として使用する。

* Clip Processing Stack
* Track Processing Stack
* Master Processing Stack

`Effect Chain`、`Track Processing / Effect Chain`、`Master FX` 等は仕様上の正式名称として使用しない。

UI 表示上で `Effects`、`FX`、`Master FX` 等の短い名称を用いることは、本ドメイン用語の定義と矛盾しない。

---

# 仕様上の用語確認事項

以下は用語自体ではなく、今後の Design / ADR で決定する運用規則である。

* Source Time Domain / Timeline Time Domain 間の rounding policy。
* Project Timeline Sample Rate 変更可否。
* Ripple Scope の具体的 target resolution。
* Synchronization Group と Clip Group が交差した場合の Edit Target resolution。
* unknown / infinite Effect Tail に対する default Tail Policy。
* Processing Stack 内の Gain / Fade の default position。
* Processor Split 時の state 継承。
* Selection の persistence。
* Track role の将来拡張条件。

これらを決定する際も、本書で確定した正式用語の意味を変更しない。
