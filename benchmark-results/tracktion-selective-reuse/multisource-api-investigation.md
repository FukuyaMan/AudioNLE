# Tracktion Selective Reuse — Multi-source API / Ownership Investigation

Execution date: 2026-09-07 (Asia/Tokyo)

## Selected fixture model: A — one runtime per MediaSource

Each `MediaRuntime` owns one JUCE `Thread`, one worker-local `AudioFormatReader`, four fixed 128-frame mono PCM pages (2,048 bytes), and one latest-request slot. Each `ClipNode` is only a transient view with a Domain Clip ID, MediaSource ID, integer Source range, and integer Timeline range. It owns no reader, cache, worker, or request queue.

The existing pinned-source inspection remains applicable: public Tracktion `Node` and `SimpleNodePlayer` provide processing/graph traversal, and public `SummingNode` provides graph mixing. Public JUCE `Thread`, `WaitableEvent`, and `AudioFormatReader` provide worker lifecycle, wakeup, and worker-only WAV reads. No `Edit`, `WaveAudioClip`, `WaveNodeRealTime`, private API, patch, fork, or high-level source scheduler is used.

| Model | Result |
| --- | --- |
| A: per MediaSource | Selected: bounded and smallest fixture extension. Workers/caches scale with unique media, so same-file views share both. |
| B: per Clip | Rejected: would turn 32 views of one file into 32 workers/readers/caches and duplicate identical/overlapping reads. |
| C: shared arbitration service | Deferred: it needs fixed queueing, priority/fairness, cancellation and page-pool ownership beyond this bounded gate. |

The Model A latest-request slot has capacity one per media. A newer request overwrites the unconsumed request; no dynamic queue exists. This proves neither production prefetch policy nor fairness under competing active views. That is a later arbitration gate.
