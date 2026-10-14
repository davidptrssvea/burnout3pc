# Optional FSR 1 — 2026-10-14

FSR 1 is a separate Windows Settings choice, default off. Internal resolution
1x–4x and FSR sharpness 0–100% are selectable. Existing 2x / AF 8x configuration
and saves were preserved during installation. Select Upscaling = FSR 1 and
sharpness 50% to start. Restart the game after saving.

## Implementation

Official AMD EASU followed by RCAS, using full-precision GLSL and retained MIT
notices. No temporal reconstruction, motion vectors or DLSS. FXAA remains a
separate choice and, if enabled, runs before EASU. FSR targets the picture's
actual display rectangle; 3840x2160 with 4:3 means 2880x2160 plus black bars.
The selected aspect is preserved. If output is no larger than the input,
ordinary downscaling/passthrough is used. Resource or pipeline failure disables
FSR for that session and falls back to ordinary scaling.

## Verification

- Native host tests and final GPU display smoke pass.
- Smoke exercises FXAA + FSR, on/off switching, sharpness extremes, odd input
  sizes, output resizing when switching 4:3 to 16:9, and GPU readback of every
  solid-color pixel. Disabled mode retains the original picture.
- Windows Settings save/reload verifies new values and preservation of all
  unrelated configuration keys. Accessibility confirms English labels; the
  desktop capture helper returned access denied, so visual layout screenshots
  could not be obtained. GPU image readback was used for game-picture checks.
- Captured native game frame exported through the actual FSR display shaders
  at 1x and 2x, sharpness 50%. Both output 2880x2160. Inspected unscaled crops:
  2x FSR preserves more detail than 1x FSR and looks sharper than bilinear.
  The bilinear comparison image is an offline reference, not swapchain readback.
  These are still frames, not a temporal stability evaluation.

## Display-step benchmark

RTX 4070 Ti, 3840x2160 window, 4:3, vsync off; same captured game frame.
60 warmup frames, 600 timed presents per mode, three rounds. Median CPU wall
time per Present, including image upload, processing, queue waits and submit:

| Input | Standard | FSR 1 |
| --- | ---: | ---: |
| 640x448 (1x) | 0.2863 ms | 0.6337 ms |
| 1280x896 (2x) | 0.5839 ms | 0.9171 ms |

FSR adds about 0.33–0.35 ms in this display-path test. This is neither a
GPU-timestamp measurement nor game FPS, and this upload benchmark differs
from the game's GPU-image path. It shows FSR is inexpensive on this GPU but
does not itself save time at unchanged input resolution.

## Native fullscreen sequence

Each mode runs the same frame-indexed Silver Lake fixture with 4K fullscreen,
AF 8x, primary GPU rendering and real time from frame 17400. One run per mode,
no capture/sampling in the measured interval. FSR initialization messages
confirm the actual game display path, including the 2880x2160 output.
The six half-second start-camera bins are frames 17490–17670; driving is
17670–18030. These short runs are not a full-lap benchmark.

| Mode | Start picture production | Lowest start bin | Driving picture production |
| --- | ---: | ---: | ---: |
| 2x standard | 46.27/s | 30.2/s | 59.96/s |
| 2x FSR 1 | 45.01/s | 31.6/s | 59.94/s |
| 1x FSR 1 | 49.66/s | 33.1/s | 59.95/s |

Picture production is the game's VIF1-transfer counter, not unique presented
FPS. Presentation diagnostics may include repeated pictures and catch-up;
they must not be interpreted as more unique game frames. Driving presents
remain about 60/s. The start dip persists; these single runs do not establish
a reliable FPS improvement from FSR or lower resolution. Recommend trying 2x
+ FSR 1 for image quality, with Standard available for direct comparison.
1x FSR loses visible detail and is an optional performance tradeoff.

Timed race build SHA256: `18d09ac4d6fd84cebefe09607bd5cad04d3d32e32c8fc72a2938b2920bea4de2`.
Final installed game SHA256: `0e7657d630746f61488278ee87600dfcdc54fcb8a1ee91ccf418a27ea850f8fd`. The final build adds a
diagnostic readback guard for unsupported external images; the game rendering
and presentation path is unchanged. Host/display tests pass on that build.
Settings SHA256: `cdae3479634bc338285f99f87eee9ff7b78cf9cda91c76d9d6747719c1299c07`.
Previous executables are backed up locally; settings/save hashes are unchanged.
