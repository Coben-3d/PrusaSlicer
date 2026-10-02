# Filaments locaux Prusa — MK4 et CORE One + INDX

Choisissez la couleur au chargement sur l’écran de l’imprimante, puis
**synchronisez matière et couleur dans PrusaSlicer** par PrusaLink local.
Pour INDX, la synchronisation porte sur les huit emplacements physiques.
Prusa Connect n’est pas nécessaire.

**Prototype communautaire expérimental de Coben-3d.** L’auteur a confirmé le
fonctionnement sur ses machines le 2 octobre 2026 ; le périmètre des contrôles
et les limites sont documentés. Ce fork n’est pas une version officielle Prusa.

- **[Téléchargements v0.1.0](https://github.com/Coben-3d/Prusa-Firmware-Buddy/releases/tag/local-filaments-v0.1.0)**
- **[Installer et utiliser](doc/LOCAL-FILAMENTS.md)**
- [Tests et limites](doc/LOCAL-FILAMENTS-VALIDATION.md)
- [Firmware MK4](https://github.com/Coben-3d/Prusa-Firmware-Buddy/tree/feature/mk4-loaded-filament-color) · [firmware CORE One INDX](https://github.com/Coben-3d/Prusa-Firmware-Buddy/tree/feature/coreone-indx-loaded-filament-colors)
- [PrusaSlicer modifié et ses sources](https://github.com/Coben-3d/PrusaSlicer/tree/feature/indx-local-filaments)

| Version | Périmètre |
|---|---|
| MK4 6.5.7-color+4 | Une bobine, sans MMU actif |
| CORE One INDX 6.9.1-color+1 | Huit têtes, profil HF0.4 de départ |
| PrusaSlicer 2.9.6+FilamentLocal-INDX | MK4 et INDX ; paquet Apple Silicon, macOS 26.2 minimum |

Le firmware personnalisé non signé demande la rupture irréversible de la
languette xBuddy `!`. Lire les [conditions Prusa](https://help.prusa3d.com/article/flashing-custom-firmware-core-one-l-core-one-mk4-s-mk3-9-s-mk3-5-s_814967)
et le guide avant installation. Les profils, versions et BBF doivent correspondre
à votre machine.

**English:** community prototype that stores a user-selected filament color on
the printer and syncs loaded material and color into PrusaSlicer over local
PrusaLink. Supports single-spool MK4 and CORE One INDX 8-tool. Downloaded macOS
binaries require Apple Silicon and macOS 26.2+. Custom unsigned firmware needs
the irreversible xBuddy appendix removal. Read the installation guide first.

---


![PrusaSlicer logo](/resources/icons/PrusaSlicer.png?raw=true)

# PrusaSlicer

You may want to check the [PrusaSlicer project page](https://www.prusa3d.com/prusaslicer/).
Prebuilt Windows, OSX and Linux binaries are available through the [git releases page](https://github.com/prusa3d/PrusaSlicer/releases) or from the [Prusa3D downloads page](https://www.prusa3d.com/drivers/). There are also [3rd party Linux builds available](https://github.com/prusa3d/PrusaSlicer/wiki/PrusaSlicer-on-Linux---binary-distributions).

PrusaSlicer takes 3D models (STL, OBJ, AMF) and converts them into G-code
instructions for FFF printers or PNG layers for mSLA 3D printers. It's
compatible with any modern printer based on the RepRap toolchain, including all
those based on the Marlin, Prusa, Sprinter and Repetier firmware. It also works
with Mach3, LinuxCNC and Machinekit controllers.

PrusaSlicer is based on [Slic3r](https://github.com/Slic3r/Slic3r) by Alessandro Ranellucci and the RepRap community.

See the [project homepage](https://www.prusa3d.com/slic3r-prusa-edition/) and
the [documentation directory](doc/) for more information.

### What language is it written in?

All user facing code is written in C++.
The slicing core is the `libslic3r` library, which can be built and used in a standalone way.
The command line interface is a thin wrapper over `libslic3r`.

### What are PrusaSlicer's main features?

Key features are:

* **multi-platform** (Linux/Mac/Win) and packaged as standalone-app with no dependencies required
* complete **command-line interface** to use it with no GUI
* multi-material **(multiple extruders)** object printing
* multiple G-code flavors supported (RepRap, Makerbot, Mach3, Machinekit etc.)
* ability to plate **multiple objects having distinct print settings**
* **multithread** processing
* **STL auto-repair** (tolerance for broken models)
* wide automated unit testing

Other major features are:

* combine infill every 'n' perimeters layer to speed up printing
* **3D preview** (including multi-material files)
* **multiple layer heights** in a single print
* **spiral vase** mode for bumpless vases
* fine-grained configuration of speed, acceleration, extrusion width
* several infill patterns including honeycomb, spirals, Hilbert curves
* support material, raft, brim, skirt
* **standby temperature** and automatic wiping for multi-extruder printing
* [customizable **G-code macros**](https://github.com/prusa3d/PrusaSlicer/wiki/PrusaSlicer-Macro-Language) and output filename with variable placeholders
* support for **post-processing scripts**
* **cooling logic** controlling fan speed and dynamic print speed

### Development

If you want to compile the source yourself, follow the instructions on one of
these documentation pages:
* [Linux](doc/How%20to%20build%20-%20Linux%20et%20al.md)
* [macOS](doc/How%20to%20build%20-%20Mac%20OS.md)
* [Windows](doc/How%20to%20build%20-%20Windows.md)

### Can I help?

Sure! You can do the following to find things that are available to help with:
* Add an [issue](https://github.com/prusa3d/PrusaSlicer/issues) to the github tracker if it isn't already present.
* Look at [issues labeled "volunteer needed"](https://github.com/prusa3d/PrusaSlicer/issues?utf8=%E2%9C%93&q=is%3Aopen+is%3Aissue+label%3A%22volunteer+needed%22)

### What's PrusaSlicer license?

PrusaSlicer is licensed under the _GNU Affero General Public License, version 3_.
The PrusaSlicer is originally based on Slic3r by Alessandro Ranellucci.

### How can I use PrusaSlicer from the command line?

Please refer to the [Command Line Interface](https://github.com/prusa3d/PrusaSlicer/wiki/Command-Line-Interface) wiki page.
