# r8brain global-cache and realtime audit

Source inspection shows `CDSPResampler` acquires global FIR/FFT/filter-bank caches while constructing its processing chain.  Cache lookup/build is synchronized (`R8BSYNC`) and can allocate, while `process` walks the already-created processor chain and internal fixed buffers.  Construction, cache warmup, clear/reconstruction, and destruction therefore belong to the control thread.

No Windows-IAT process smoke was run: the formal capacity gate already rejects four common directions before a valid fixed callback adapter exists.  This document makes no claim that r8brain callback processing has passed the IAT contract.  The required no-allocation/no-lock callback proof remains absent.
