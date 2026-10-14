"""Package the Xbox fork using an explicit allowlist, without personal data."""
from pathlib import Path
import argparse
import hashlib
import re
import zipfile

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--settings', type=Path, required=True)
    parser.add_argument('--notices', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    for path in (args.exe, args.settings, args.notices):
        if not path.is_file():
            parser.error(f'Missing input: {path}')
    exe = args.exe.read_bytes()
    for path in (str(Path.home()), str(ROOT)):
        for form in (path, path.replace('\\', '/')):
            for encoding in ('utf-8', 'utf-16le'):
                if form.lower().encode(encoding) in exe.lower():
                    raise SystemExit('Executable contains a machine-specific path.')
    if re.search(rb'[A-Za-z]:\\Users\\[^\\\x00]{1,64}\\', exe):
        raise SystemExit('Executable contains a user-directory path.')
    prefix = args.output.name.removesuffix('-windows-x64.zip')
    config = '''# Xbox fork defaults; choose your own disc image on first launch.
aa=off
anisotropic=8x
aspect=4:3
blur=original
fps_counter=on
frame_pacing=on
fullscreen=on
map.circle=b
map.cross=a+rt
map.down=down
map.l1=lb
map.l2=x
map.l3=l3
map.left=left
map.r1=a
map.r2=rb
map.r3=r3
map.right=right
map.select=view
map.square=lt
map.start=menu
map.triangle=y
map.up=up
own_engine_volume=100
resolution=2x
upscaling=off
fsr_sharpness=50
rumble_strength=100
texture_pack=on
trigger_threshold=10
vsync=off
vu_fast=on
xbox_prompts=on
'''
    assets = {
        'burnout3.exe': exe,
        'Settings.exe': args.settings.read_bytes(),
        'THIRD-PARTY-NOTICES.txt': args.notices.read_bytes(),
        'LICENSE.txt': (ROOT / 'LICENSE').read_bytes(),
        'FSR-LICENSE.txt': (ROOT / 'engine/gpu/shaders/fsr/NOTICES.txt').read_bytes(),
        'ENGINE-LICENSE.txt': (ROOT / 'engine/LICENSE').read_bytes(),
        'README.txt': (ROOT / 'games/burnout3/port/release/README-Xbox.txt').read_bytes(),
        'pc_settings.ini': config.encode(),
        'Play Burnout 3.cmd': b'@echo off\r\ncd /d "%~dp0"\r\nstart "" "burnout3.exe"\r\n',
        'Settings.cmd': b'@echo off\r\ncd /d "%~dp0"\r\nstart "" "Settings.exe"\r\n',
        'texture pack/README.txt': b'Optional: place your own PCSX2 SLUS-21050 texture replacements here. No pack is included.\r\n',
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, contents in assets.items():
            archive.writestr(prefix + '/' + name, contents)
    with zipfile.ZipFile(args.output) as archive:
        if archive.testzip() is not None or len(archive.namelist()) != len(assets):
            raise SystemExit('Archive verification failed.')
        assert archive.read(prefix + '/burnout3.exe') == exe
        assert b'disc=' not in archive.read(prefix + '/pc_settings.ini')
    checksum = hashlib.sha256(args.output.read_bytes()).hexdigest()
    args.output.with_suffix('.zip.sha256').write_text(
        f'{checksum}  {args.output.name}\n', encoding='utf-8')
    print('Verified package:', args.output)
    print('Executable SHA256:', hashlib.sha256(exe).hexdigest())
    print('Package SHA256:', checksum)

if __name__ == '__main__':
    main()
