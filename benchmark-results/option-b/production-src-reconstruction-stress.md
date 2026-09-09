# Production SRC Reconstruction Stress

200 deterministic seeded nonzero seek plans include near, far forward/backward and return positions. Each republished prepared range reconstructs the same plan. Generation N -> N+1 rejects stale page/prepared state before callback, then current preparation progresses. Repeated destroy/reconstruct has no hidden state accumulation in the sparse fixture.
