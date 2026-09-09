# Production SRC Direction Cost

Single-view diagnostic means at 256 project frames (us): 44.1->48 195.114; 48->44.1 227.965; 44.1->96 153.572; 96->44.1 367.645; 48->96 152.945; 96->48 277.233. The runner reports exact project request and `ceil(projectFrames / ratio)+4` native offer; no configuration used a 2048-frame process request.

Product-like 8/16-view means for the three controlled directions were 44.1->48: 810.940/1650.180; 44.1->96: 842.625/1648.750; 96->44.1: 1774.270/3540.110 us. Relative to the corresponding single-view 256 mean, these are approximately 4.16x/8.46x, 5.49x/10.74x, and 4.83x/9.63x. Scaling is broadly linear or better than linear; there is no measured superlinear locality degradation.

Direction cost tracks native-rate/DSP work. The fixture has no AudioNLE source/cache/output-staging/sum path in its measured section, so it cannot attribute the failure to a production wrapper. Content invariance was not separately rerun: the libsamplerate benchmark uses deterministic sinusoidal input and does not establish content-independent CPU cost.
