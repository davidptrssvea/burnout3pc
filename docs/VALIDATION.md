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
