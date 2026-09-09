# Windows Interception libsamplerate Smoke Status

Run in the Release `option_b_windows_iat_crt_harness` after mandatory IAT positive, negative, and cross-thread controls passed. `src_new(SRC_SINC_BEST_QUALITY, 1, ...)` was prepared outside the callback marker. A marked first `src_process` and a separately marked steady-state `src_process` both recorded zero events for the hooked CRT allocation, Windows heap, lock, wait/sleep, and file-I/O categories.

This is a smoke audit of the statically linked libsamplerate process route for this executable and configuration, not a proof for APIs that have no IAT route or other modules/configurations.
