# Tracktion Selective Reuse — Runtime-edit M4–M8 Evidence

Execution date: 2026-09-07 (Asia/Tokyo)

## M4 Move — Pass

Clip A moved from Timeline `[20000, 20129)` to `[22000, 22129)` while retaining Source `[0, 129)` and MediaSource ID 0. After stopped-state view rebuild, old output was zero, new marker was exact, and shared Clip B (Source 128 at Timeline 21000) was unchanged.

## M5 Trim / re-expand — Pass

Original: Source `[0,257)` -> Timeline `[30000,30257)`. Left trim 128 produced `[128,257)` -> `[30128,30257)`; Timeline 30127 was zero and 30128 exact. Right trim produced Source `[0,129)` -> Timeline `[30000,30129)`; 30128 was exact and 30129 zero. Re-expanding to the original range restored Source 256 at Timeline 30256. No source media mutation occurred.

## M6 Split — Pass

Original `[40000,40257)` / `[0,257)` split at 40128 into Clip IDs 61 `[40000,40128)` / `[0,128)` and 62 `[40128,40257)` / `[128,257)`, both MediaSource ID 0. Unions equal the original; boundary and final samples were exact, with no duplicate or loss.

## M7 Delete / lifetime — Pass

Clip A's worker request was held pending, A was deleted in Domain, then the hold was released before rebuilt observation. A never reappeared; shared Clip B remained exact and the shared worker/cache remained usable. After final consumer deletion, the bounded worker destructor completed. Domain stores no runtime identity.

## M8 rapid invalidation — rebuild control pass; partial update deferred

With an old A request held pending, Domain edits executed `move A -> trim B -> delete C -> move A`. Domain revision became 5; Clip runtime-view revisions were A=3, B=2, C=2. The retained media runtime processed released work, but rebuilt views emitted only the final Domain state: old A and C locations were zero; final A and trimmed B mappings were exact.

Partial in-place graph mutation was not implemented: proving it would require an ownership/lifecycle design for a persistent low-level Tracktion graph and affected-node replacement. No general scheduler, dependency mutator, high-level scheduler, private API, or patch was introduced. The stopped-state rebuild is the correctness reference.

All M4–M8 cases: maximum timing error 0 Timeline samples; stale output 0; callback reader/file, wait/block/spin, allocation/growth, and synchronous fallback 0. M9 is ready as a separate arbitration follow-up; M10/M11 were not executed.
