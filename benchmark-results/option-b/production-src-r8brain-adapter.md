# r8brain bounded adapter

Pinned `r8brain-free-src` revision `9e73d2dd59fd5b95108fdb4f590083e35758b45f` builds as a header-only MSVC C++20 comparator from `https://github.com/avaneev/r8brain-free-src.git`; upstream `LICENSE` is MIT.

The adapter constructs `CDSPResampler` on the control thread and owns fixed 2048-frame float input, 2048-frame double input staging (16,384 bytes/view), and 512-frame float exposed output (2,048 bytes/view).  r8brain owns intermediate double output buffers.  The diagnostic object is deliberately constructed with an 8192 input maximum solely to query all formal requirements; it is not a production capacity promotion.  It uses `getInputRequiredForOutput(512)`, `getLatency`, `getLatencyFrac`, `process`, and `clear`; all are physical runtime information only.  Timeline/source samples and exact P/Q remain AudioNLE authority.

The adapter compiles and runs, but it is not admitted to callback/timing comparison because its own public requirement exceeds the unchanged public input capacity in four of six common directions.  No public capacity was silently enlarged.
