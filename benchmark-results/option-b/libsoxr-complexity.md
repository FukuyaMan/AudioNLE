# libsoxr Integration Complexity

The attempted dependency required an old-CMake compatibility policy and failed its top-level-oriented module lookup as a subproject. Applying a vendor patch, global module shim, or copied dependency would be an additional build-integration decision, so it was not introduced in this timing experiment.

Classification: integration compatibility unmeasured; no bandlimited wrapper complexity classification is justified.
