# Production SRC Boundary Policy

Source start: clamp to 0, no negative read, retain logical coordinate. Source end: `end_of_input` is physical-only; output clipping fixes final exposed Timeline end and settling is not Effect Tail. Missing non-start history: explicit preparation failure, never silently shortened preroll. Capacity excess: pre-callback bounded failure.
