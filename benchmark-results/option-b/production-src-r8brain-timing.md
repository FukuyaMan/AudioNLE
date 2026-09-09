# r8brain timing and capacity gate

For 512 project frames, r8brain's formal `getInputRequiredForOutput` reports:

| source -> project | required native frames | current 2048 capacity |
| --- | ---: | --- |
| 44.1 -> 48 | 2246 | FAIL (+198) |
| 48 -> 44.1 | 2309 | FAIL (+261) |
| 44.1 -> 96 | 2012 | PASS |
| 96 -> 44.1 | 4617 | FAIL (+2569) |
| 48 -> 96 | 2028 | PASS |
| 96 -> 48 | 4566 | FAIL (+2518) |

This is an API-derived capacity gate, not guessed preroll.  The current runtime cannot give a common-six-rate guarantee without a material public capacity/queue contract change.  Arbitrary-start reconstruction, phase/edit matrix, source service, and Source-end handling are not run after this mandatory capacity failure.
