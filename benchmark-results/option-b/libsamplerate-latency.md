# libsamplerate Latency Evidence (Partial)

The backend has no delay value used by this experiment. Initial process counts for a 4096-frame impulse are recorded in `libsamplerate-timing-contract.md`; they are not interpreted as edit latency. Continuous-reference/fresh-state preroll measurement remains required.

## D0--D9 diagnostic

For deterministic broadband/multitone input, 44.1->48 kHz starts T4096 and T4256 (same rational phase, separated by one 160-sample phase period), and 48->44.1 kHz T4096, the first 4096 exposed output samples are bit-identical to the continuous reference after phase-lattice input planning.

For 44.1->48 at T4096, the requested logical Source coordinate is 3763. A nominal 1024-frame preroll snaps the physical Source start to 2646, giving 1117 actual native history and output trim 1216. For 48->44.1 at T4096, logical Source is 4458; physical start 3360 gives 1098 history and trim 1009. All reported windows through 2048 samples have max and RMS error 0; the complete 4096 comparison also has max/RMS 0.

The required model is not `round((logicalSource - physicalStart) * ratio)`. It is `outputDiscard = T0 - physicalStart * ratio`, after choosing physical start on the rational output-grid lattice (147 native samples for 44.1->48, 160 for 48->44.1). A +/-4 diagnostic trim search chooses shift 0. Exact copied physical input equals the corresponding continuous input, `end_of_input` remains false, and one large call equals 128-frame calls.

Classification: **Wrapper input/output accounting defect identified**. This resolves the earlier 0.00918/0.00472 residual for the tested interior cases; it is not yet LS5--LS8 PASS because the full start/content/source-boundary/reset sweep remains.

Full sweep result: the fixed 1024-frame nominal history plus phase-lattice rounding passes all tested phase/content/start cases. The actual history may be up to 1024 plus `Q-1` frames due to rounding down. This is an empirical configuration-specific bound, not a public libsamplerate delay contract.
# LS13--LS19 end and boundary observations

Logical Clip end is an authoritative wrapper exposure limit; it does not automatically expose sinc settling. At physical MediaSource end the final `src_process` call receives `end_of_input=true`; generated settling is retained only as physical backend output and is clipped to the requested logical Timeline range. No unexplained extra or missing exposed Timeline sample was observed in the exercised cases. This is distinct from AudioNLE Effect Tail semantics.

## LS20 impulse observations

At fixed converter configuration, impulse peak positions were output frame 4354 (44.1->48) and 3675 (48->44), with total output counts 17414 and 14699. The response is physical runtime/filter behavior only: it is not persisted latency, edit authority, or Effect Tail. The established 1024-native-frame plus phase-rounding preroll remains the edit-boundary policy; these observations do not derive a new public delay contract.
