# libsoxr Timing-Contract Result

Classification: **libsoxr Inconclusive; run libsamplerate comparator**.

Pinned source requested: tag `0.1.3`, revision `6e80f8b597c92afd3edf7e714a3b8d7b4534213d`; static, VHQ, linear phase, one runtime thread, OpenMP off. Configure failed before target compilation because upstream subproject module lookup uses the parent `CMAKE_SOURCE_DIR` and cannot find `SetSystemProcessor`.

No `soxr_create`, `soxr_process`, `soxr_clear`, or `soxr_delete` call executed. Thus no callback/process safety, delay, preroll, quality, or lifecycle conclusion is available.
