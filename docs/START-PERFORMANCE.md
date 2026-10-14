# Race-start buffer conversion — 2026-10-14

The start-camera sequence at 2x internal resolution and AF 8x puts much more
load on GPU buffer conversion than the following drive. An exploratory GPU
profile over the transition attributed approximately 47% of GPU time to
swizzle operations and 45% to drawing; it did not establish shader compilation
as the cause of the dip.

## Change

- GS block and column table indexing in shared swizzle/depth-load addressing
  is replaced with equivalent bit permutations, including the depth XOR and
  the distinct CT16S layout.
- 32 optional color-image transfer pipelines are prepared when the renderer
  starts: four color formats, both directions and scales 1–4. Vulkan
  specialization constants let the driver remove unused direction, format,
  scale and depth branches. Parameters and synchronization are unchanged.
- Unsupported combinations and unsuccessful optional pipeline creation retain
  the generic path. Specialization handles are released by the renderer.
- The internal resolution, AF level, rendering rules, audio and gameplay
  timing are unchanged. FSR/DLSS were not added at David's preference.

## Verification

Native host tests pass. Existing GPU comparisons cover all eight color/depth
formats against scalar GS memory. New tests render on scaled color targets
at 2x, 3x and 4x, compare their complete 4 MB memory result to the scalar GS,
then check every native pixel after reloading randomized memory. These exercise
both directions of each common scaled color-format specialization.

A native in-race capture was replayed on both the previous and modified GPU
renderers at 2x. The full-resolution and 1x bitmap outputs and full GS memory
are byte-identical. Capture files contain game data and remain local.

## Clean start-sequence comparison

Same scripted Silver Lake sequence, i5-13400F / RTX 4070 Ti, 2x, AF 8x, same
configuration and frame-indexed inputs. Virtual boot switches to real time
from frame 17400. No sampling/timeline/GPU-profile output or capture is active
in the measured interval. The candidate capture occurs later, at frame 17850.

Six half-second bins, frames 17490–17670:

| Internal picture production | Previous | Modified |
| --- | ---: | ---: |
| Weighted mean | 45.28/s | 51.92/s |
| Lowest half-second bin | 34.0/s | 37.9/s |

The VIF1-transfer counter indicates native picture production for this game,
not measured displayed FPS. These are headless renderer runs, excluding
fullscreen presentation costs. This short transition is not a full-lap test,
and a remaining start dip is visible. The measured mean improvement is about
15%; no guarantee of locked 60 FPS is made. Total run time includes boot and,
for the candidate, capture; it is not used as the performance metric.

The installed exe is backed up and settings/save-file hashes are unchanged.
Installed executable SHA256:
`20081babb7e618e22e170ef1d968e7eecb2c6f1458b05f1515f8c2ede23aafce`.
