# Execution Plan: Remove Rejected Tracktion Prototype Implementation

## Objective

Align current `main` with ADR 0001 by removing the rejected Tracktion Engine
executable prototype, its submodule, build integration, and tests while
retaining the decision's Markdown evidence and conclusions.

## Requirements

* Keep ADR 0001, comparison rationale, benchmark evidence, and useful
  historical design conclusions.
* Remove Tracktion-only CMake targets, source trees, submodule metadata, and
  `build.ps1` bootstrap paths.
* Do not remove Option B code or JUCE utilities that current Option B targets
  require.
* State in retained history documents that removed executable prototype code is
  recoverable from Git history.

## Classification

The tracked Tracktion-only implementation consists of
`prototype/tracktion-feasibility/`, `prototype/tracktion-selective-reuse/`,
and their submodule pointer. Option B has no include, link, CMake, or source
dependency on either tree; it obtains direct JUCE independently. The Markdown
ADR/design/benchmark/plan records are retained because they explain the
Candidate A versus B decision and record bounded conclusions.

## Invariants and edge cases

Default CMake configures direct JUCE and Option B without a Tracktion checkout.
The standard CTest registry remains the current 35 Option B tests. Removing
the historical executable code must not modify sample authority, source
runtime, SRC policy, or Option B fixtures.

## Test strategy

Confirm no tracked submodule or Tracktion build path remains, configure/build
through `build.ps1`, run full default CTest, and verify 35/35 passing tests.
Review retained historical documents for an explicit Git-history recovery note.

## Completion criteria

No Tracktion dependency, submodule, option, target, executable test, or source
implementation remains in current `main`; decision evidence remains readable;
and the default Option B build/test gate is green.

## Outcome

Removed the Tracktion submodule and both Tracktion-only prototype trees, along
with their root CMake wiring and build-script bootstrap path. No Option B code
depended on those trees. ADR 0001, comparison/result documents, benchmarks, and
historical plans remain; the retained documents state that the executable
implementation is recoverable from Git history. `build.ps1` completed a clean
default configuration/build and its full CTest gate passed 35/35 Option B tests
on 2026-09-10. `ctest -N` registered exactly those 35 tests and no Tracktion
test.
