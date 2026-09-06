# Tracktion feasibility prototype

This directory is isolated from production AudioNLE source. Its Phase 1 target
only verifies that the pinned Tracktion Engine dependency can be compiled and
that an `Engine` can be constructed and destroyed in a headless process.

Use the repository build entry point:

```powershell
.\build.ps1
```

The pinned dependency revisions and the Phase 1 observations are recorded in
`benchmark-results/tracktion-feasibility/`.
