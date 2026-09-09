# Execution Plan: Production Build Scope and SRC Cache Repair

## Objective

Make the standard configure/build/CTest gate represent ADR 0001's accepted
Option B architecture. Keep historical Tracktion feasibility prototypes
available through an explicit opt-in, then repair the active libsamplerate
timing-contract fixture's 320/147 prepared-cache priming defect.

## Requirements

* `AUDIONLE_BUILD_TRACKTION_PROTOTYPES` defaults to `OFF`.
* Default configuration requires no Tracktion checkout, library, target, or
  Tracktion CTest registration.
* `ON` restores both historical Tracktion prototype subtrees and their tests.
* Option B continues to use direct JUCE hosting without consuming Tracktion's
  bundled JUCE target.
* The 320/147 cache case retains exact required-range coverage and all
  callback-safety assertions.

## Components and implementation

* Root CMake establishes the architecture-scoped option and independently
  acquires the pinned direct JUCE dependency required by Option B.
* `build.ps1` exposes the option and initializes the Tracktion submodule only
  for opt-in builds.
* Windows CI runs only the default gate.
* ADR 0001, source-runtime design, and the CI plan record the production versus
  historical boundary.
* The synthetic timing fixture sizes its modulo page table from the exact
  primed range rather than silently overwriting a required page.

## Invariants and edge cases

Default targets retain integer sample authority, source/cache callback safety,
and all Option B assertions. The cache must cover both endpoints of the
physical range for every supported P/Q matrix; no assertion, matrix row, or
production behavior is weakened. Opt-in Tracktion support retains the pinned
submodule and existing historical target names.

## Test strategy

Configure/build and run full CTest with defaults; verify no CTest name starts
with `tracktion_`. Configure/build with the option enabled and run the
Tracktion-labelled CTest subset where practical. Review diffs and CMake cache
entries to prove the default configuration did not initialize or reference the
Tracktion source tree.

## Completion criteria

The default Option B build and full CTest are green; Tracktion remains an
explicit, buildable historical opt-in; documentation and CI describe the same
boundary; and the 320/147 prepared-cache row passes with callback assertions
intact.

## Outcome

Default `build.ps1` configured direct JUCE without Tracktion and completed
35/35 Option B CTests. The 320/147 invalidation range needs nine 257-frame
pages; expanding the fixture table from eight to nine removes the modulo
overwrite while retaining every requested sample and callback assertion. The
96 kHz downsample quality diagnostic also now uses its existing 6000-frame
window from a valid settled offset.

At the time of this plan, `build.ps1 -BuildTracktionPrototypes -SkipTests`
built the retained Tracktion targets, and opt-in CTest registered 16
`tracktion_*` tests. Its known `tracktion_feasibility_phase_e_pdc` discrepancy
remained reproducible; the opt-in subset otherwise passed 15/16. A later
accepted-architecture cleanup removed that rejected executable prototype from
current `main`; its evidence remains and its harness is recoverable from Git
history.
