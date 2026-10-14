"""Build the customized port using the player's local NTSC-U disc image."""
from pathlib import Path
import argparse
import json
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / 'games' / 'burnout3'

def run(*args):
    subprocess.run([sys.executable, *map(str, args)], cwd=ROOT, check=True)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', type=Path, help='Your NTSC-U SLUS-21050 disc image')
    parser.add_argument('--skip-generation', action='store_true',
                        help='Reuse an existing local translation')
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('This fork currently builds on Windows with MSVC.')
    if not args.skip_generation:
        if args.iso is None or not args.iso.is_file():
            parser.error('--iso must name an existing NTSC-U disc image.')
        iso = args.iso.resolve()
        config_path = GAME / 'game.json'
        config = json.loads(config_path.read_text(encoding='utf-8'))
        config['iso'] = str(iso)
        # Local build configuration; do not commit the machine-specific path.
        config_path.write_text(json.dumps(config, indent=2) + '\n', encoding='utf-8')
        run('engine/tools/isotool.py', 'extract', iso, 'SLUS_210.50', '-o', GAME / 'extracted')
        run('engine/tools/analyze.py', '--game', 'games/burnout3')
        run('engine/tools/derive.py', '--game', 'games/burnout3', '--emit', '--csv')
        run('engine/tools/lift.py', '--game', 'games/burnout3', '--structure', '--emit', '--csv')
    elif not (GAME / 'src' / 'lifted').is_dir():
        parser.error('No local translation exists. Build with --iso first.')
    manifest = GAME / 'build' / 'port' / 'discovered.txt'
    manifest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(GAME / 'config' / 'runtime-functions.txt', manifest)
    os.environ.setdefault('CMAKE_BUILD_PARALLEL_LEVEL', '4')
    run('engine/tools/port.py', '--game', 'games/burnout3', 'build')
    compiler = Path(os.environ.get('WINDIR', r'C:\Windows')) / 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
    if not compiler.is_file():
        raise SystemExit('Game built, but .NET Framework C# compiler is missing for Settings.exe.')
    output = GAME / 'build' / 'port' / 'cmake' / 'Settings.exe'
    subprocess.run([str(compiler), '/nologo', '/target:winexe',
                    '/reference:System.Windows.Forms.dll', '/reference:System.Drawing.dll',
                    '/out:' + str(output), str(ROOT / 'tools' / 'Controls.cs')], cwd=ROOT, check=True)
    print('Built game and settings app:', output.parent)

if __name__ == '__main__':
    main()
