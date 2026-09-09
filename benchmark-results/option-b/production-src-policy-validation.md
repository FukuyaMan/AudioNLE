# Production SRC Policy Validation

`PrerollPolicyV1` is validated only for 160/147 and 147/160 with `SRC_SINC_BEST_QUALITY`. The 320/147, 147/320, 2/1 and 1/2 rate pairs retain explicit V1 planning metadata but are not runtime policy evidence. They must fail reconstruction/load as `UnvalidatedConfiguration`; no ZOH, alternate mode, nearest-rate or enlarged-preroll fallback exists.
