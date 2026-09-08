# Option B Mixed-Rate SRC Corrective Complexity

Classification: **Tiny AudioNLE-owned ZOH timing primitive**.

The corrective component is one fixture-local class with two integer factor fields and two integer runtime fields. Its sole responsibility is deterministic Source identity selection and reset at a Timeline sample. The worker, fixed native page cache, ClipRuntimeView, SourceNode, and graph harness remain separate fixture components.

It adds no filtering, interpolation beyond held selection, history buffer, dynamically allocated process state, locks, wait path, DSP subsystem, JUCE modification, or alternate timing authority. The process path reads a selected native token into caller-owned scalar output and advances four fixed-size integers.

JUCE floating ZOH is not authoritative for Source-to-Timeline sample identity in this prototype. The correction does not assess or replace JUCE's general SRC quality.
