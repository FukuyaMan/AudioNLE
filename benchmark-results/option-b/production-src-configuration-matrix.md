# Production SRC Configuration Matrix

| Native -> Project | Reduced P/Q | Phase classes | v1 preroll status | Planning / long positions |
| --- | --- | ---: | --- | --- |
| 44100 -> 48000 | 160/147 | 160 | ValidatedV1 | PASS |
| 48000 -> 44100 | 147/160 | 147 | ValidatedV1 | PASS |
| 44100 -> 96000 | 320/147 | 320 | UnvalidatedConfiguration | PASS; runtime policy validation deferred |
| 96000 -> 44100 | 147/320 | 147 | UnvalidatedConfiguration | PASS; runtime policy validation deferred |
| 48000 -> 96000 | 2/1 | 2 | UnvalidatedConfiguration | PASS; runtime policy validation deferred |
| 96000 -> 48000 | 1/2 | 1 | UnvalidatedConfiguration | PASS; runtime policy validation deferred |

Every row derives `physicalStart=floor(max(0,floor(T0*Q/P)-1024)/Q)*Q`, integer `outputDiscard`, source-start clamp, and 1/3/6/12-hour integer coordinates. Planning does not render full duration and does not infer v1 validity from arithmetic alone.

## Coverage gate disposition

Only 44.1 <-> 48 rows have runtime phase/edit/cache evidence. The 96-kHz matrix remains explicit `UnvalidatedConfiguration`; arithmetic coverage does not upgrade policy validity. Gate result: **Production SRC configuration coverage Inconclusive**.

## 96 kHz comparator update

The dedicated fixed-mode comparator now validates V1 for 320/147, 147/320, 2/1 and 1/2 with exhaustive phase classes and compact runtime evidence. All common 44.1/48/96 rows are `ValidatedV1`; no policy generalisation beyond this matrix is implied.
