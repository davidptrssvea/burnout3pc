# Burnout 3 PC — Xbox controller edition

Personal Windows fork of [Daniel Gomes Vieira's Burnout 3 PC port](https://github.com/danielgomesvieira2000/burnout3-pc), with Xbox One controller, graphics, performance and audio improvements.

Requires your own **NTSC-U SLUS-21050** Burnout 3: Takedown disc image. Game images, extracted assets, generated game code, BIOS files, texture packs and personal saves are excluded. Unofficial fan project, unaffiliated with EA or Criterion.

Current `main` restores edition 1's muted pause-preview workaround. The audible fault in experimental edition 2 remains under investigation.

## Changes

- Xbox One USB button remapping, alternative bindings (`a+rt` means either button), vibration strength and trigger threshold.
- Runtime-generated Xbox prompts following the selected mapping.
- English Windows settings app, FXAA and anisotropic filtering at 2x/4x/8x/16x.
- FPS display on **F10**, replacing the upstream F10 HUD-editor binding.
- More efficient anisotropic filtering, draw bounds and VU calculations. Performance depends on hardware and scene.
- Own-car engine audio fixes, including SPU2 capture/PCM routing and audio instruction translation.
- **OWN ENGINE** volume in pause → Audio, 0–200%, saved separately from Effects Volume.
- Experimental pause Audio interpolation correction: native ramps settle at the sound driver's requested level, but user playtesting still reports badly distorted preview audio. Edition 2 re-enables that preview; **use edition 1's muted-preview workaround** while investigation continues.

DualSense adaptive triggers and steering-wheel force feedback have not been added.

## Build

Windows 10/11 x64, Python 3.10+, Pillow, Visual Studio 2022 C++ Build Tools (including CMake and Ninja), an AVX2 CPU and a Vulkan 1.3 driver are required. CMake downloads third-party build dependencies.

```powershell
git clone https://github.com/davidptrssvea/burnout3pc.git
cd burnout3pc
python -m pip install Pillow
python tools/build.py --iso "D:\Games\Burnout 3 - Takedown.iso"
```

The first build extracts the executable and generates the game translation from your disc locally. Generated files stay ignored by Git. The modified engine is ordinary source in `engine/`, with no submodule needed. An address manifest supplies the discovered runtime functions required for audio.

Outputs: `games/burnout3/build/port/cmake/burnout3.exe` and `Settings.exe`. Keep both together. Choose the same disc image when the game asks for one. For subsequent builds using an existing local translation:

```powershell
python tools/build.py --skip-generation
```

The upstream VU recording workflow in [the original README](docs/UPSTREAM-README.md) can improve performance. VU programs must be recorded locally; no game-derived program manifest is distributed.

## Settings

Run `Settings.exe` with the game closed. Original and Modern: RT / LT provide starting profiles. Save writes `pc_settings.ini` beside the executables and preserves unrelated settings. Modern: RT/A accelerates and confirms, LT brakes, A boosts, Menu pauses, Y goes back. `a+rt` means A **or** RT, not a chord.

Resolution, fullscreen and aspect ratio remain in the game's PC Settings. Choose settings appropriate to your display; 4:3 preserves the original presentation. AF 4x/8x is a useful starting point on the tested hardware.

In pause Audio, adjust OWN ENGINE, confirm to save or cancel to restore the entry value. 100% is the original volume; louder values may clip.

## Validation and attribution

[Validation notes](docs/VALIDATION.md) cover previous tests and current packaging checks. A fresh full build from a clean checkout has not yet been completed for this packaging step. This remains an alpha.

Original MIT notices are retained in [LICENSE](LICENSE) and [engine/LICENSE](engine/LICENSE). See [provenance](docs/PROVENANCE.md) for upstream revisions. Upstream documents its use of Claude; this fork was developed with Codex and playtested by David. These licences grant no rights over the commercial game.
