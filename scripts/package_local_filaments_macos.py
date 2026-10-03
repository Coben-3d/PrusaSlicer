#!/usr/bin/env python3
"""Package the prototype from explicit source/build inputs, never user settings."""
import argparse
import configparser
import hashlib
import json
from pathlib import Path
import plistlib
import re
import shutil
import subprocess
import tempfile
import zipfile

MACOS_OPENING = '''# Ouverture du prototype sur macOS

Le binaire fourni est arm64, exige macOS 26.2 minimum et possède une signature
ad hoc, sans certificat Developer ID ni notarisation Apple. macOS peut donc
bloquer un téléchargement provenant d’Internet.

Consultez la procédure Apple pour ouvrir une application d’un développeur non
identifié : https://support.apple.com/fr-fr/102445. Après une tentative
d’ouverture, elle décrit Réglages Système → Confidentialité et sécurité →
Ouvrir quand même, puis la confirmation pour l’application concernée.

Le paquet fournit les sources, les versions et les empreintes SHA-256.
Cette distribution ne modifie pas les protections globales de macOS.
Les deux lanceurs ouvrent des réglages séparés. La connexion Prusa Connect
reste facultative et utilise le trousseau macOS normal lorsque vous vous connectez.
Ouvrez « PrusaSlicer Filaments MK4.app » ou « PrusaSlicer Filaments INDX.app »
dans le dossier extrait. Le raccourci de PrusaSlicer officiel n’ouvre pas cette
version personnalisée et ne contient pas son bouton de synchronisation.
'''


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--release-version', default='0.2.0')
    parser.add_argument('--profiles', type=Path, required=True)
    parser.add_argument('--dependency-license-root', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert re.fullmatch(r'\d+\.\d+\.\d+', args.release_version)
    assert not args.output.exists(), 'Never replace an existing public archive'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    vendor = configparser.ConfigParser(interpolation=None, strict=False)
    vendor.optionxform = str
    vendor.read(args.profiles / '2.5.10.ini')
    assert vendor.has_section('printer:Prusa CORE One INDX 8T HF0.4 nozzle')

    def flatten(name):
        section = vendor['printer:' + name]
        result = {}
        for parent in section.get('inherits', '').split(';'):
            if parent.strip():
                result.update(flatten(parent.strip()))
        result.update((k, v) for k, v in section.items() if k != 'inherits')
        return result

    specifications = [
        ('MK4', 'MK4IS', '0.4', 'Original Prusa MK4 Input Shaper 0.4 nozzle',
         'MK4 - filaments locaux', '0.20mm SPEED @MK4IS 0.4', 'Generic PLA @PGIS',
         'Generic PETG @PGIS', 1),
        ('INDX', 'COREONE_INDX8T', 'HF0.4', 'Prusa CORE One INDX 8T HF0.4 nozzle',
         'CORE One INDX - filaments locaux', '0.20mm Balanced ★ @COREONEINDX HF0.4',
         'Generic PLA @COREONEINDX HF0.4', 'Generic PETG @COREONEINDX HF0.4', 8),
    ]
    with tempfile.TemporaryDirectory(prefix='local-filaments-public-', dir=args.output.parent) as temp:
        package = Path(temp) / ('PrusaSlicer-Local-Filaments-v' + args.release_version)
        app = package / 'slicer'
        (app / 'MacOS').mkdir(parents=True)
        shutil.copyfile(args.binary, app / 'MacOS/PrusaSlicer')
        (app / 'MacOS/PrusaSlicer').chmod(0o755)
        shutil.copytree(args.source / 'resources', app / 'Resources',
                        ignore=shutil.ignore_patterns('.DS_Store', '._*'))
        shutil.copyfile(args.source / 'LICENSE', package / 'LICENSE-PrusaSlicer-AGPLv3')
        guide = (args.source / 'doc/LOCAL-FILAMENTS.md').read_text()
        (package / 'GUIDE-FR.md').write_text(guide.replace('(LOCAL-FILAMENTS-VALIDATION.md)', '(VALIDATION.md)'))
        shutil.copyfile(args.source / 'doc/LOCAL-FILAMENTS-VALIDATION.md', package / 'VALIDATION.md')
        for guide in ['INSTALL-MK4.md', 'INSTALL-COREONE-INDX.md']:
            text = (args.source / 'doc' / guide).read_text()
            text = text.replace('(LOCAL-FILAMENTS.md)', '(GUIDE-FR.md)')
            text = text.replace('(LOCAL-FILAMENTS-VALIDATION.md)', '(VALIDATION.md)')
            (package / guide).write_text(text)
        (package / 'MACOS-OUVERTURE.md').write_text(MACOS_OPENING)
        for subtree in ['bundled_deps']:
            for path in sorted((args.source / subtree).rglob('*')):
                if path.is_file() and (path.name.upper().startswith(('LICENSE', 'COPYING', 'NOTICE'))):
                    destination = package / 'licenses' / path.relative_to(args.source)
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(path, destination)

        if args.dependency_license_root:
            for directory in sorted(args.dependency_license_root.glob('dep_*-prefix/src/dep_*')):
                if not directory.is_dir():
                    continue
                candidates = list(directory.iterdir())
                if directory.name == 'dep_wxWidgets':
                    candidates += list((directory / 'docs').glob('*'))
                for path in candidates:
                    name = path.name.upper()
                    if path.is_file() and (name.startswith(('LICENSE', 'LICENCE', 'COPYING', 'NOTICE', 'COPYRIGHT'))
                                           or name == 'OCCT_LGPL_EXCEPTION.TXT'):
                        destination = package / 'licenses/dependencies' / directory.name / path.name
                        destination.parent.mkdir(parents=True, exist_ok=True)
                        shutil.copyfile(path, destination)

        for short, model, variant, parent, name, print_name, pla, petg, count in specifications:
            settings = package / ('settings-' + short.lower())
            (settings / 'printer').mkdir(parents=True)
            assets = settings / 'vendor/PrusaResearch'
            assets.mkdir(parents=True)
            shutil.copyfile(args.profiles / '2.5.10.ini', settings / 'vendor/PrusaResearch.ini')
            for key in ['bed_model', 'bed_texture', 'thumbnail']:
                filename = vendor['printer_model:' + model][key]
                source = args.profiles / filename
                if not source.exists():
                    source = args.source / 'resources/profiles/PrusaResearch' / filename
                shutil.copyfile(source, assets / filename)
            values = flatten(parent)
            assert len(values['nozzle_diameter'].split(',')) == count
            assert values['single_extruder_multi_material'] == '0'
            for key in ['print_host', 'printhost_apikey', 'printhost_user', 'printhost_password']:
                assert not values.get(key, ''), 'Source profile must contain no credentials or address'
                values.pop(key, None)
            values.update(inherits=parent, printer_settings_id=name, host_type='prusalink')
            (settings / 'printer' / (name + '.ini')).write_text(
                ''.join(k + ' = ' + v + '\n' for k, v in values.items()))
            text = ('version = 2.9.6\ntranslation_language = fr_FR\npreset_update = 0\n'
                    'notify_release = none\nauth_login_dialog_confirmed = 1\nshow_hints = 0\n'
                    'connect_polling = 1\nsingle_instance = 1\nno_defaults = 0\n'
                    '[presets]\nprinter = ' + name + '\nprint = ' + print_name + '\n')
            for index in range(count):
                text += 'filament' + ('_' + str(index) if index else '') + ' = ' + pla + '\n'
            text += ('[vendor:PrusaResearch]\nmodel:' + model + ' = ' + variant + '\n'
                     '[filaments]\n' + pla + ' = 1\n' + petg + ' = 1\n')
            (settings / 'PrusaSlicer.ini').write_text(text)
            assert not (settings / 'physical_printer').exists()
            launcher = package / ('Lancer-PrusaSlicer-' + short + '.command')
            launcher.write_text(
                '#!/bin/zsh\nset -eu\ntask_package_dir=${0:A:h}\n'
                'unset DYLD_INSERT_LIBRARIES\n'
                'exec "$task_package_dir/slicer/MacOS/PrusaSlicer" '
                '--datadir "$task_package_dir/settings-' + short.lower() + '" --single-instance\n')
            launcher.chmod(0o755)
            subprocess.run(['zsh', '-n', str(launcher)], check=True)
            bundle = package / ('PrusaSlicer Filaments ' + short + '.app')
            (bundle / 'Contents/MacOS').mkdir(parents=True)
            (bundle / 'Contents/Resources').mkdir()
            wrapper = bundle / 'Contents/MacOS/PrusaSlicer-Filaments'
            wrapper.write_text(
                '#!/bin/zsh\nset -eu\ntask_package_dir=${0:A:h:h:h:h}\n'
                'unset DYLD_INSERT_LIBRARIES\n'
                'exec "$task_package_dir/slicer/MacOS/PrusaSlicer" '
                '--datadir "$task_package_dir/settings-' + short.lower() + '" --single-instance\n')
            wrapper.chmod(0o755)
            subprocess.run(['zsh', '-n', str(wrapper)], check=True)
            shutil.copyfile(args.source / 'resources/icons/PrusaSlicer.icns',
                            bundle / 'Contents/Resources/PrusaSlicer.icns')
            (bundle / 'Contents/Info.plist').write_bytes(plistlib.dumps({
                'CFBundleIdentifier': 'org.coben3d.prusaslicer.filaments.' + short.lower(),
                'CFBundleName': 'PrusaSlicer Filaments ' + short,
                'CFBundleDisplayName': 'PrusaSlicer Filaments ' + short,
                'CFBundleExecutable': wrapper.name,
                'CFBundleIconFile': 'PrusaSlicer.icns',
                'CFBundlePackageType': 'APPL',
                'CFBundleShortVersionString': '2.9.6',
                'CFBundleVersion': args.release_version,
                'LSMinimumSystemVersion': '26.2',
                'NSHighResolutionCapable': True,
            }))

        manifest = {
            'version': 'PrusaSlicer-2.9.6+FilamentLocal-INDX',
            'package_release': args.release_version,
            'platform': 'macOS', 'architecture': 'arm64', 'minimum_macos': '26.2',
            'code_commit': 'b05b5ae4bba0372a7c69138ab48a6a093741a46e',
            'source_url': 'https://github.com/Coben-3d/PrusaSlicer/tree/feature/indx-local-filaments',
            'binary_sha256': digest(app / 'MacOS/PrusaSlicer'),
            'prusa_research_version': '2.5.10',
            'profiles_commit': '65c5c8f1e1c3836f306119c49d717759cbc368db',
            'personal_settings_included': False, 'credentials_included': False,
            'keychain_blocked_by_launchers': False,
            'connect_polling_enabled': True,
        }
        (package / 'package-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
        (package / 'licenses/README.md').write_text(
            '# Licences et sources\n\n'
            'PrusaSlicer est distribué sous AGPLv3. Les ressources et dépendances\n'
            'conservent leurs notices et licences propres, présentes dans les ressources\n'
            'et les copies de licences jointes. Les sources correspondantes du projet\n'
            'sont accessibles depuis package-manifest.json et dans le fork GitHub.\n'
            'Les dépendances de compilation sont déclarées dans le répertoire deps\n'
            'du dépôt source, avec leurs versions et leurs URL de téléchargement.\n')
        files = sorted(p for p in package.rglob('*') if p.is_file())
        with zipfile.ZipFile(args.output, 'x', zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
            for path in files:
                archive.write(path, path.relative_to(package.parent))
        with zipfile.ZipFile(args.output) as archive:
            assert archive.testzip() is None
            assert not any('/physical_printer/' in name or '/__MACOSX/' in name
                           or Path(name).name.startswith('._') for name in archive.namelist())
            binary_name = package.name + '/slicer/MacOS/PrusaSlicer'
            assert hashlib.sha256(archive.read(binary_name)).hexdigest() == manifest['binary_sha256']
        print(json.dumps({'filename': args.output.name, 'bytes': args.output.stat().st_size,
                          'sha256': digest(args.output), 'file_count': len(files)}, indent=2))


if __name__ == '__main__':
    main()
