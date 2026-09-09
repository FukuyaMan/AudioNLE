# r8brain-free-src Comparator Feasibility

Inspected upstream revision `9e73d2dd59fd5b95108fdb4f590083e35758b45f` (HEAD resolved during this investigation) in an external temporary checkout; this is not a vendored or production dependency. Repository `LICENSE` is MIT. `CDSPResampler` is header-only C++ and is MSVC-compatible in principle; its constructor takes source/destination rates and a maximum input length, exposes `getInputRequiredForOutput`, `getLatency`, `getLatencyFrac`, `getMaxOutLen`, and `clear`.

The same API documents that construction allocates intermediate buffers, `process` accepts/returns `double*`, and returned output points to an internally owned buffer. The header also warns of global static FFT/filter caches. This conflicts with the current fixed float caller-output staging / no callback allocation model unless a bounded conversion, ownership, warmup, and output-consumption adapter is designed and verified first.

No r8brain runtime comparator was added. Its required adapter is a **Large backend-specific subsystem** for this prototype, not a bounded comparator drop-in: it needs a new physical planning contract, fixed double conversion buffers, output queue/consumption semantics, lifecycle warmup, and a full realtime audit. Adding it without these is not valid comparison evidence. This is a concrete integration blocker, not a DSP-quality rejection.
