"""Build a native, persistent macOS parent for the unchanged Slicer executable."""
import os
from pathlib import Path
import plistlib
import re
import shutil
import subprocess


def build_launcher(package, short, icon, release_version, build_directory):
    identifier = 'org.coben3d.prusaslicer.filaments.' + short.lower()
    name = 'PrusaSlicer Filaments ' + short
    bundle = package / (name + '.app')
    macos = bundle / 'Contents/MacOS'
    resources = bundle / 'Contents/Resources'
    macos.mkdir(parents=True)
    resources.mkdir()
    build_directory.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment.update(TMPDIR=str(build_directory),
                       CLANG_MODULE_CACHE_PATH=str(build_directory / 'module-cache'))
    executable = macos / 'PrusaSlicer-Filaments'
    source = Path(__file__).with_name('local_filaments_launcher.m')
    subprocess.run([
        'xcrun', 'clang', '-arch', 'arm64', '-mmacosx-version-min=26.2',
        '-fobjc-arc', '-Wall', '-Wextra', '-Werror', '-framework', 'Cocoa',
        '-DPRUSA_LAUNCHER_IDENTIFIER="' + identifier + '"',
        str(source), '-o', str(executable),
    ], check=True, env=environment)
    shutil.copyfile(icon, resources / 'PrusaSlicer.icns')
    (bundle / 'Contents/Info.plist').write_bytes(plistlib.dumps({
        'CFBundleIdentifier': identifier,
        'CFBundleName': name,
        'CFBundleDisplayName': name,
        'CFBundleExecutable': executable.name,
        'CFBundleIconFile': 'PrusaSlicer.icns',
        'CFBundlePackageType': 'APPL',
        'CFBundleShortVersionString': '2.9.6',
        'CFBundleVersion': release_version,
        'LSMinimumSystemVersion': '26.2',
        'LSUIElement': True,
        'NSPrincipalClass': 'NSApplication',
        'NSHighResolutionCapable': True,
        'NSLocalNetworkUsageDescription': 'Lire la matière et la couleur des filaments chargés sur votre imprimante Prusa via PrusaLink sur le réseau local.',
        'PrusaSlicerDataDirectory': 'settings-' + short.lower(),
    }))
    subprocess.run(['/usr/bin/codesign', '--force', '--sign', '-', '--identifier', identifier, str(bundle)],
                   check=True, capture_output=True)
    subprocess.run(['/usr/bin/codesign', '--verify', '--strict', str(bundle)],
                   check=True, capture_output=True)
    load_commands = subprocess.check_output(['/usr/bin/otool', '-l', str(executable)], text=True)
    uuid = re.search(r'\buuid\s+([0-9A-F-]+)', load_commands).group(1)
    return {'bundle': bundle.name, 'identifier': identifier, 'uuid': uuid, 'code_signature': 'ad hoc'}
