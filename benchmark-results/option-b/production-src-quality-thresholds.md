# Production SRC Quality Thresholds

| Requirement | Threshold | Current evidence | Status |
| --- | --- | --- | --- |
| Passband error/ripple | <=0.10 / <=0.05 dB | 44.1<->48 <=0.02878 / 0.00123 dB | PASS; 96 production rerun needed |
| Image/alias rejection | >=60 dB | image 65.12 dB; alias 163.63 dB | PASS with image margin constraint |
| DC/numerical | <=0.10 dB, no drift/NaN/Inf | stable, finite | PASS; production-duration rerun needed |
| Reconstruction | max/RMS <=2e-5 | PASS | PASS |
| Determinism | same build/config exact | PASS | PASS |
| Fixture/rate coverage | all supported pairs + real media | generated coverage; real media absent | Needs rerun |
