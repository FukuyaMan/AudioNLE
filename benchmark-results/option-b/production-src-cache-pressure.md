# Production SRC Cache/Capacity Pressure

Prepared workload contract is fixed: <=2048 native frames and <=512 staged project frames. Skeleton capacity arithmetic rejects excess rather than allocating. Prepared density cases retained callback miss 0; stale generation is rejected before process and lifecycle work remains outside callback. This fixture does not dynamically grow cache. Full eviction/republication and long-form cache-pressure reliability remain deferred because density headroom is already insufficient.

## Long-form follow-up

Full eviction and later republication are now exercised with eight fixed 257-frame pages. 200 seeded hot/cold/adjacent/disjoint plans cause bounded eviction; old range is republished and ready before processing. Prepared callback misses remain 0; stale pages are rejected after generation churn. One shared worker/service model is assumed by the fixture; no worker-per-source expansion is introduced.
