# CPU bench results

Measured with the open-mpc platform's CPU bench (`tools/bench.c` of mpc-vst-plugins, run as its `tools/bench.sh` runs it; see its
`docs/BENCH.md`): 44100 Hz, 128-frame blocks (a 2902 µs budget), noise input, the plugin's thread CPU time per block, pinned at
normal priority (MPC's own audio always wins), with MPC running. Figures are percentages of one audio block on one core.
Verdict: PASS at p99 ≤ 15 % and worst block ≤ 50 %; WARN up to 35 % / 80 %; FAIL above.

## 2.2.0: MPC One (Gen2), MPC OS 3.10.1, 2026-11-14 (core 4)

| Stage | mean % | p99 % | max % |
|---|---|---|---|
| idle | 0.8 | 0.9 | 1.2 |
| audio in | 0.8 | 0.9 | 1.0 |
| Q-Link sweep (4 random parameters per block) | 1.7 | 3.1 | 3.4 |
| release tail | 1.2 | 1.4 | 1.7 |
| trigger fire (UTILITY's two RESET buttons) | 1.2 | 1.3 | 1.5 |

Worst p99 3.1 %, worst block 3.4 %, background threads 0.0 % of a core: **PASS**.

```
{"plugin":"ctrl_mixbus-2.2.0-aarch64.so","instrument":0,"open_ms":0.1,"p99_pct":3.1,"max_pct":3.4,"threads_pct":0.0,"verdict":"PASS","triggers":2,"trigger_max_pct":1.5}
```

The `.so` was the 2.2.0 release's Gen2 build (`build/aarch64/ctrl_mixbus.so`, as in `CTRL-MixBus-2.2.0-Gen2.zip`). MPC OS 3.10.1's
developer mode (Preferences > Enhancements) gives the SSH access the bench needs.
