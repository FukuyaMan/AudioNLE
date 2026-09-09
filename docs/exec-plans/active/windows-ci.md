# Execution Plan: Windows CI

## Objective

Run the repository's complete Windows x64 configure, build, and CTest gate on
GitHub Actions, outside the command runner's local execution time limit.

## Requirements

Use repository-local vcpkg pinned by `vcpkg-configuration.json`, acquire the
manifest FFmpeg dependency without a machine-global FFmpeg installation, build
with the existing `build.ps1` entry point, and expose every CTest failure in the
Actions log.

## Components and implementation

* `.github/workflows/windows-ci.yml`: `windows-2022`, checkout, a 120-minute
  job timeout, vcpkg binary-cache restore, build, and full CTest steps.
* `build.ps1`: discover installed Visual Studio through `vswhere`, initialize
  its x64 environment, align repository-local vcpkg to the manifest baseline,
  bootstrap that revision, then configure and build with Ninja and the vcpkg
  toolchain.
* The workflow invokes `build.ps1 -SkipTests` and deliberately runs the same
  full `ctest --output-on-failure` command as a separate final step so failures
  remain visible in the Actions log.

## Invariants

The vcpkg baseline remains the only vcpkg revision authority. FFmpeg remains
the manifest-selected MSVC/x64 dependency; no global FFmpeg discovery or
product-code workaround is introduced. Caches contain only vcpkg binary
archives, whose ABI validation still occurs during manifest installation; build
directories and installed trees are not restored.

## Edge cases and risks

Initial cache-miss runs can compile FFmpeg and take materially longer than
ordinary builds. Network availability for pinned vcpkg and existing
FetchContent dependencies remains required. Historical Tracktion prototype
tests remain enabled; if they fail, the CTest step reports their actual names
and output rather than masking them.

## Validation strategy and completion criteria

Validate PowerShell/YAML syntax, invoke the standard local build entry point,
run full CTest where the available local runner permits it, and review the
workflow paths/arguments. Completion requires a Windows hosted workflow that
checks out, bootstraps pinned local vcpkg, installs the manifest transitively
during CMake configure, builds, and reports full CTest failure output.

## Local validation observation

The standard `build.ps1 -SkipTests` configure/build succeeded locally with the
manifest-installed `ffmpeg[avcodec,avformat,swresample]:x64-windows@9.0.1#1`.
The full local CTest run completed with 49/51 passing. It recorded the existing
historical Tracktion failure `tracktion_feasibility_phase_e_pdc`: `baseline
expected sample 1024, observed 1026`. It also recorded
`option_b_libsamplerate_timing_contract`: `OB-LSR FAIL prepared cache`.
Neither test was disabled or changed. The dedicated CI CTest step will retain
both test names and their emitted failure text in the Actions log if the hosted
run reproduces them.
