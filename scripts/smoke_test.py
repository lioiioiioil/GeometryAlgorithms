"""Check that the bounded demonstration programs create readable PNG headers."""
import argparse
import os
from pathlib import Path
import struct
import subprocess
import tempfile


def check_png(path):
    with path.open('rb') as image:
        header = image.read(24)
    if len(header) != 24 or header[:8] != b'\x89PNG\r\n\x1a\n' or header[12:16] != b'IHDR':
        raise SystemExit(f'Invalid PNG header: {path.name}')
    width, height = struct.unpack('>II', header[16:24])
    if width == 0 or height == 0:
        raise SystemExit(f'Invalid PNG dimensions: {path.name}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=Path('build'))
    args = parser.parse_args()
    expected = {
        'curves_and_clipping': {
            'ClipLine_01_Inside.png', 'ClipLine_02_Outside.png',
            'ClipLine_03_Through_Horiz.png', 'ClipLine_04_Through_Vert.png',
            'ClipLine_05_Diagonal.png', 'ClipLine_06_Half_In.png',
            'ClipLine_07_Near_Edge.png', 'ClipLine_08_Tri_Through.png',
        },
        'polygon_union': {
            f'test_{case}_{stage}.png'
            for case in range(1, 18) for stage in ('input', 'output')
        },
    }
    for program, filenames in expected.items():
        suffix = '.exe' if os.name == 'nt' else ''
        executable = (args.build_dir / (program + suffix)).resolve()
        if not executable.is_file():
            parser.error(f'Executable not found: {executable}')
        with tempfile.TemporaryDirectory(prefix='geometry-smoke-') as directory:
            output = Path(directory) / 'output'
            output.mkdir()
            subprocess.run([str(executable)], cwd=directory, capture_output=True,
                           text=True, timeout=30, check=True)
            images = {path.name: path for path in output.glob('*.png')}
            if images.keys() != filenames:
                missing = sorted(filenames - images.keys())
                extra = sorted(images.keys() - filenames)
                raise SystemExit(f'{program}: missing={missing}, unexpected={extra}')
            for path in images.values():
                check_png(path)
        print(f'PASS {program}: {len(filenames)} PNG files')


if __name__ == '__main__':
    main()
