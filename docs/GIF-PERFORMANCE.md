# GIF packet performance check — 2026-10-14

Complete non-image GIF quadwords now go directly to the existing Qword parser.
Previously Words fed each one through four Word calls and a temporary assembly
buffer. Fragmented input retains the old assembly path; image bulk transfers
are unchanged. The fourth word's diagnostic source address is preserved.
No filtering, resolution, rendering rules or gameplay timing were changed.

## Validation

- Native host tests passed after the change, including existing image-format
  transfer and GPU consistency tests.
- New regression compares PACKED A+D, packed vertices, odd-length REGLIST
  padding, empty tags, and all stream chunk sizes 1–19 against scalar Word
  input. Register summaries, primitive counts and the full 4 MB of GS memory
  match, including rendered sprite pixels.
- Baseline and candidate completed the native scripted Silver Lake race
  sequence through frame 19400 with 2x internal resolution and AF 8x.
- Candidate race captures were visually inspected. Real-time gameplay evolves
  differently when fewer frames are missed; these are not claimed pixel-identical
  screenshots at the same numbered vertical blank.

## Measurements and limits

Windows, i5-13400F, RTX 4070 Ti. Both clean runs used the same configuration,
texture pack enabled (only one replacement badge), own-engine gain 200%,
fixed frame-indexed inputs, virtual boot followed by real time from frame 17600.
No sampling profiler, timeline recording, GPU profile output or screenshots
were enabled in the clean comparison. No window was shown: presentation and
fullscreen scaling costs are excluded.

For the same 28 complete one-second bins, frames 17700–19380:

| Native VIF1 transfer rate | Before | After |
| --- | ---: | ---: |
| Weighted mean | 57.98/s | 59.07/s |
| Lowest one-second bin | 50.0/s | 53.8/s |
| Bins at least 59/s | 18/28 | 22/28 |

The host reports vertical blanks separately from VIF1 transfers. For this game
the latter indicates native picture production, not measured displayed FPS.
This bounded scripted sequence is not a full-lap benchmark or a guarantee
of constant 60 FPS in every scene. Total boot-and-run wall time is not used
as the performance metric. Initial profiling and capture runs perturb timing
and are exploratory evidence only; disabling the tiny texture pack did not
establish a useful performance gain.

Installed executable SHA256:
`6375544275d9955b30c1cbe71ca869c35b35b6c2d646e0f444990207ba75e448`.
The previous executable is backed up. Installed settings and save-file hashes
were verified unchanged. No game data or private replay artifacts are included.
