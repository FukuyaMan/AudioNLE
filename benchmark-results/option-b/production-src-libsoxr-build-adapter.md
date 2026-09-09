# libsoxr 0.1.3 build-adapter evidence

Date: 2026-09-09.  Scope: comparator only; this is not a production dependency decision.

## Pin and licence

The requested revision `6e80f8b597c92afd3edf7e714a3b8d7b4534213d` is the annotated upstream `0.1.3` tag.  It peels to source commit `945b592b70470e29f917f4de89b4281fbbd540c0` (`update NEWS, versions`).  Source is fetched from `https://github.com/chirlu/soxr.git` by that tag.

Pinned `LICENCE` says GNU LGPL, version 2.1 or later.  It requires retaining the LGPL licence/copyright notice and providing the licence/corresponding-source obligations applicable to redistribution.  `pffft-wrap.c`, selected by this build, carries its own embedded PFFFT licence and must be retained/reviewed for any distribution.  This is not a permissive-library result.

## Adapter

`option_b_soxr_static` is an AudioNLE-local STATIC target.  It directly compiles the upstream core (`soxr.c`, data I/O, filters, constant/variable rate engines, scalar FFT paths, and PFFFT SIMD paths); it does not call `add_subdirectory` on libsoxr and does not run its package/module lookup, install, examples, tests, CLI, OpenMP, or shared-library setup.

`soxr-config.h` is generated into the comparator build directory from upstream `soxr-config.h.in`.  Values are checked against the 0.1.3 MSVC configuration: `WITH_PFFFT`, CR32/CR32S/CR64/CR64S/VR32, high-precision clock, and standard headers are enabled; FFmpeg is disabled; developer tracing is disabled.  The target defines `SOXR_LIB`, `_USE_MATH_DEFINES`, and `_CRT_SECURE_NO_WARNINGS`.  No upstream source is patched.

On Windows x64/MSVC, the upstream normal runtime dispatch selects scalar or SIMD rate engines.  x64 selects SSE/SSE2-capable CR32S normally; CR64S performs its upstream AVX/OS state check.  No `/arch:AVX*` or Ryzen-specific compile flag was added, so the binary remains representative of the intended Windows x64 baseline.

## Build proof

`./build.ps1` configured, compiled, and linked `option_b_soxr_static` and `option_b_libsoxr_comparator`.  The executable creates a float32 interleaved one-channel VHQ instance, processes deterministic input, verifies finite output, flushes and destroys it.  Runtime reports `libsoxr-0.1.3` and exits zero.

The prior integration issue was top-level-oriented libsoxr CMake/module setup, not a DSP or runtime failure.  The local adapter removes that integration path.
