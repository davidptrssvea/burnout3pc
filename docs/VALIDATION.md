# Validation notes

Previous development checks on Windows with i5-13400F and RTX 4070 Ti:

- Native host tests cover PCM gain, audio capture/IRQs, dynamic prompts and draw bounds.
- Translator tests cover PMULTH and MTSAB/MTSAH immediates. Audio DSP was compared against an interpreter with randomized inputs.
- AF/race playtesting showed better frame times; other hardware has not been benchmarked.
- Native pause Audio engine volume was checked for save, reopening, cancellation and startup persistence.
- Final pause-preview test confirmed the native pause flag, suppression and volume persistence. Race PCM before opening and after resuming was byte-identical to baseline. Preview clipped samples fell from approximately 1.7% to zero by suppressing the demonstration.
- Front-end preview is preserved by the pause-context guard; its playback was not separately retested during the final suppression check.

For packaging, the settings app is compiled and the build helper is syntax/CLI checked. A fresh full regeneration/build from a clean checkout has not yet been completed. VU recordings and game translations are deliberately omitted and must be generated locally.

A pre-existing intermittent native host-test process failure (`0xC0000409`, no output) passed on retry. Underlying pause-preview distortion remains unresolved; only that demonstration is suppressed.

## Pause-preview interpolation correction (Xbox edition 2)

**Follow-up:** user listening confirms that the preview remains badly distorted.
The measurements below establish improved interpolation, not restored sound
quality. Edition 1 is recommended while the audio data path is investigated.

The suppression above describes edition 1. Edition 2 restores the native
preview functions and supplies the RWA driver's requested sweep endpoints while
pause Audio is open. Its ~33 ms polling interval otherwise lets ~10 ms ramps
overshoot and reverse repeatedly. This corrects that oscillation; it does not
make the entire IOP scheduler cycle-accurate.

Native host tests cover upward/downward/signed endpoints, register readback and
clearing a stale endpoint. A full race/pause/Audio/volume/resume replay passed.
Before-menu and resumed-race PCM were identical to baseline, as was the complete
front-end Audio replay. Preview voice levels settled at the native targets:
20690/20804, then 19514/19624 at the lowered Effects Volume, then 20690/20804.
The 303–310 s preview remained audible (RMS 11433.50); final clipped samples
fell from 647 to 13 of 672000. Small remaining loud transients are not claimed
fixed. Other tracks and output modes have not been exhaustively playtested.

## Complete GIF quadwords

[Performance and regression results](GIF-PERFORMANCE.md) document the new packet
assembly fast path, a clean native before/after race check, and its limits.

## Independent pause-preview observation

David reports the same objectionable in-race Audio demonstration in PCSX2
with the game. This weakens the earlier attribution to this port. The pause
preview remains muted; no new mixer correction is claimed.

## Race-start buffer conversion

[Start-sequence measurements and graphics equivalence](START-PERFORMANCE.md)
cover the buffer-addressing and prebuilt transfer-pipeline optimization.

## Optional FSR 1

[FSR display tests, image checks and bounded race comparison](FSR-TEST.md).
