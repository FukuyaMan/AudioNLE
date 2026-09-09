# Execution Plan: Option B Production SRC Integration Planning

## Purpose

Define the bounded component boundary and evidence programme needed before the selected libsamplerate prototype can become a production AudioNLE SRC commitment. This is a planning/design gate only: no production component, dependency change, runtime change, or ADR is authorised.

## Established baseline

The prototype selected static libsamplerate 0.2.2, `SRC_SINC_BEST_QUALITY`, caller-buffer streaming, for 44.1 <-> 48 kHz. Integer Project Timeline positions, integer native Source positions, and exact rational mapping remain authoritative. Backend phase/history/output count remains physical runtime state only.

Carry forward without upgrade: 1024 native-frame plus phase-rounding preroll is empirical; AudioNLE callback contract is proven; libsamplerate internal realtime safety is partially unproven.

## Planned outputs

1. Specify the production-oriented boundary from `ClipRuntimeView` through range planning, cache, runtime, staging and SourceNode.
2. Assign ownership for all logical coordinates, physical planning, lifecycle, invalidation, buffers and telemetry.
3. Record explicit evidence gaps and bounded future acceptance gates.
4. Define a representative sample-rate, realtime-audit and performance matrix.
5. Propose an engine-internal API that does not leak libsamplerate concepts into Domain types.
6. Define conditions for a future dedicated SRC backend ADR.

## Non-negotiable invariants

* Logical clip boundaries and persisted data use integer Domain coordinates only.
* Preroll, filter history, delay and flush are wrapper-owned physical concerns.
* Callback processing performs only prepared bounded work; lifecycle, allocation and cache preparation stay off callback.
* No opaque backend state is persisted or used as timing authority.
* This plan does not reopen completed prototype or backend-selection gates.

## Completion criteria

Complete when the companion design document defines the boundary, responsibilities, evidence gaps, future matrices, API shape, ADR criteria and revisit rules, and recommends one bounded next implementation gate. It must make no production-safety or production-quality claim.

## Files

* `docs/design/prototypes/option-b-production-src-integration-planning.md`
* `docs/exec-plans/active/option-b-production-src-integration-planning.md`

## Risks and stop conditions

Do not turn prototype measurements into production guarantees. Stop for a decision before implementation if physical SRC state would need to enter Domain authority, if cache service cannot remain bounded/callback-safe, or if the empirical preroll policy cannot be explicitly versioned and tested per configuration.

## Release / threshold update

Release licensing and project-local quality/performance thresholds are now defined. Remaining ADR blockers are deeper realtime proof and threshold evidence reruns (production-duration performance, all-pair production spectral/real-media fixtures, and release package verification). No production ADR is created by this update.
