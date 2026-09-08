# Option B Finite VST3 Tail Complexity

The bounded probe contains 180 lines of host/runtime/scheduling/test code plus an 8-line deterministic fixture. These are fixture measurements, not production LOC estimates.

| Responsibility | Measured fixture surface | Architecture relevance |
| --- | --- | --- |
| Public VST3 host query and seconds-to-samples conversion | `HostedTail` | New bounded adapter responsibility |
| Framework-free policy and derived extent | `TailPolicy`, `TailExtent` | New bounded scheduling responsibility |
| Block-interior zero feed and finite process stop | `TailRuntime::render` | New bounded scheduling responsibility |
| Downstream multiply and linear summing | fixed fixture stages | Existing graph responsibilities exercised with Tail audio |
| Move/Delete/reconstruction | description mutation plus fresh runtime | Existing stopped-rebuild pattern exercised with Tail extent |
| Fixture, assertions, CLI, and instrumentation | remaining probe/fixture code | Not architecture LOC |

The evidence does not require a general scheduler, new PDC implementation, special Tail mixer, or new plugin-host lifecycle. It does require AudioNLE ownership of Tail policy, integer extent derivation, block-edge silence feed, and stopped rebuild integration.

Classification: **Small Tail extension** for this finite deterministic fixture. It does not cover unknown/infinite or dynamic Tail, stacked arbitrary processors, third-party plugin behavior, device callback scheduling, or live graph mutation.
