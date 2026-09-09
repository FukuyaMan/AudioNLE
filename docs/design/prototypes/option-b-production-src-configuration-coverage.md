# Production SRC Configuration Coverage

The configuration record/fingerprint includes backend pin, `SRC_SINC_BEST_QUALITY`, native/project rates, reduced P/Q, preroll policy version, boundary policy version and capacity policy. A change to any field invalidates prepared state.

| Rate pair | P/Q | Preroll policy | Phase coverage | Edit regression | Capacity | Runtime status |
| --- | --- | --- | --- | --- | --- | --- |
| 44100 -> 48000 | 160/147 | V1, 1024 + phase rounding | all 160 | PASS | 2048/512 | ValidatedV1 |
| 48000 -> 44100 | 147/160 | V1, 1024 + phase rounding | all 147 | PASS | 2048/512 | ValidatedV1 |
| 44100 -> 96000 | 320/147 | V1 planning only | 320 arithmetic only | not run | explicit | UnvalidatedConfiguration |
| 96000 -> 44100 | 147/320 | V1 planning only | 147 arithmetic only | not run | explicit | UnvalidatedConfiguration |
| 48000 -> 96000 | 2/1 | V1 planning only | 2 arithmetic only | not run | explicit | UnvalidatedConfiguration |
| 96000 -> 48000 | 1/2 | V1 planning only | 1 arithmetic only | not run | explicit | UnvalidatedConfiguration |

The existing validated rows cover broadband, marker, multitonal, spoken-like and music-like content; nonzero/source starts, seek/trim/split/reconstruction, source end, prepared cache, 128/256/512/irregular partition and callback counters. None of that is inferred for 96 kHz. The exact 96-kHz v1 outcome, capacities, performance sanity and spectral sanity require a dedicated comparator before the status changes.

ADR readiness is narrowed but unchanged: common configuration coverage is incomplete; deep realtime proof, release/licensing and explicit production quality/performance thresholds also remain blockers.
