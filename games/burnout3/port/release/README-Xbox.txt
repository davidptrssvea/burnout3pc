Burnout 3 PC - Xbox Controller Edition
v0.2.0-xbox.3 (2026-10-14), Windows x64, prerelease

This is the latest locally playtested custom build based on upstream
0.2.0-alpha. The executable's internal upstream version may still show 0.2.0.
Unofficial fan project, unaffiliated with Electronic Arts or Criterion.

REQUIREMENTS
Windows 10/11 x64, AVX2 CPU, Vulkan 1.3 graphics driver, .NET Framework 4.x
for Settings.exe, and your own NTSC-U SLUS-21050 Burnout 3: Takedown ISO.
The compiled game translation is included; game textures, music, movies,
disc images, BIOS files, texture packs and personal saves are not included.

START
Extract the whole folder somewhere writable. Run Play Burnout 3.cmd or
burnout3.exe, and select your own ISO when asked. Run Settings.cmd or
Settings.exe with the game closed to adjust controls, vibration, AA, AF,
FPS display, performance mode, internal resolution and optional FSR 1.
Settings are saved beside the executables.

DEFAULTS
Modern Xbox One USB mapping: RT/A accelerates and confirms, LT brakes,
A boosts, Menu pauses, Y goes back. a+rt means either A or RT, not a chord.
8x AF, AA off, FPS on, performance mode on, fullscreen, 4:3 aspect.
Internal resolution defaults to 2x; FSR 1 is off, with sharpness set to 50%.
4K display output does not guarantee native internal 3840x2160 rendering.
Change these options in the game's PC Settings or Settings.exe as appropriate.

FEATURES
Remappable Xbox controls and vibration, generated prompts following your
bindings, FXAA, AF 2x/4x/8x/16x, optional FSR 1 (EASU + RCAS), F10 FPS display,
optimized AF, GIF packet handling and GPU buffer conversions,
and restored own-car engine audio. In pause -> Audio, OWN ENGINE adjusts
0-200% independently. 100% is the original volume and the release default.
Confirm saves; cancel restores the entry value. High gains may clip.

The distorted automatic Effects Volume preview is suppressed only in
pause Audio. The volume control, race effects and front-end preview remain.
The underlying preview distortion has not been repaired.
This remains an alpha; performance varies by machine and scene.
DualSense adaptive triggers and steering-wheel force feedback are not added.

UPDATING AN EXISTING INSTALLATION
Close the game and make a backup. Replace burnout3.exe and Settings.exe,
and optionally use the included English launchers. Keep your existing
pc_settings.ini, memcard folder and texture pack. Do not overwrite your
existing configuration with the release's default pc_settings.ini unless
you want to reset it. If your old launcher opens Inställningar.exe, replace
that file with the new Settings.exe renamed to Inställningar.exe, or use
the included Settings.cmd launcher.

SOURCE AND LICENCES
https://github.com/davidptrssvea/burnout3pc
Based on Daniel Gomes Vieira's MIT-licensed burnout3-pc and ps2-port-engine.
Upstream was developed with Claude; custom changes with Codex and David's
playtesting. Retained licences: LICENSE.txt, ENGINE-LICENSE.txt and
THIRD-PARTY-NOTICES.txt. No licence over the commercial game is granted.

Optional FSR 1:
Settings.exe: Upscaling = FSR 1, Internal Resolution = 2x, FSR Sharpness = 50%.
Standard is the default. Lower resolution can improve performance at a loss of detail;
FSR alone does not increase FPS. Aspect ratio and fullscreen remain in PC Settings.
AMD shader notices are included in FSR-LICENSE.txt.
