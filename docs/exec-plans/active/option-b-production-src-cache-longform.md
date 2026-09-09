# Execution Plan: Option B Production SRC Cache-Pressure + Long-Form Reliability

## Purpose

Exercise validated 44.1->48 source-service/SRC planning under sparse long-form positions, fixed-page pressure, seeks, generations and reconstruction without retaining decoded duration PCM.

## Fixed constraints

Eight 257-frame pages (8224 bytes per source), <=2048 native / <=512 project frames, v1 preroll, prepared-only callback and the documented 32-view upper-density constraint remain unchanged.

## Result

The sparse long-form fixture passes exact planning through 12 hours, eviction/republication, seeded seeks, generation rejection, bounded capacity/failure semantics and reconstruction without duration-scaled PCM. The parent production integration plan remains active.
