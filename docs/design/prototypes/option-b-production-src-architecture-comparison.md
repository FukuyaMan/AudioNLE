# Production SRC Architecture Comparison

Product requirements contain no explicit requirement for 96 kHz real-time conversion at 8/16 simultaneous views and no requirement for 16 simultaneous mixed-rate views. They require project-rate/source-rate correctness and realtime safety; those facts must not be inflated into a concurrency promise.

| Candidate | Quality thresholds | Timing/edit contract | RT audit | 8/16 performance | 96 kHz support | Integration complexity | Main risk |
| --- | --- | --- | --- | --- | --- | --- | --- |
| libsamplerate BEST | Existing prototype quality evidence; all-pair production rerun pending | `ValidatedV1` | Bounded residual blind spots | Fails supported-tier duration rows | Fails several rows | Moderate production SRC component | DSP cost / threshold failure |
| libsamplerate MEDIUM | Not reached | `V1InsufficientForMode` | Not audited | Not measured | Not measured | Moderate only after a new timing policy | Changing empirical physical range |
| libsamplerate FASTEST | Not reached | `V1InsufficientForMode` | Not audited | Not measured | Not measured | Moderate only after a new timing policy | Changing empirical physical range |
| r8brain-free-src | Not measured | Not adapted | Not audited | Not measured | Not measured | Large backend-specific subsystem | Internal double output/caches and callback ownership |
| Policy-constrained BEST | Same as BEST for retained rows | Existing V1 | Existing bounded audit | 44.1/48 evidence still unstable | Can be prepared/offline pending policy | Moderate + product policy | Product capability tradeoff |

Feasible policy options, none selected: P1 retains 44.1/48 real-time only at empirically demonstrated tiers and treats 96 kHz conversion as prepared/offline; P2 permits 96 kHz real-time only after a backend/mode meets the same 8-view thresholds; P3 makes 96 kHz proxy/pre-render a documented operational constraint while retaining import/edit correctness. None removes 96 kHz media support.

**Architecture recommendation: Run another backend comparator before decision.** This is a production architecture direction, not a final irreversible backend commitment. A dedicated production SRC ADR is not ready: no lower libsamplerate mode has a valid timing contract, r8brain lacks a bounded adapter comparator, and BEST fails supported-tier performance. Remaining decision evidence is a deliberately bounded r8brain adapter feasibility prototype or another backend with caller-owned fixed-buffer semantics, plus all-pair quality/real-media/release evidence for any candidate that passes performance.
