# Filaments locaux Prusa — firmware + PrusaSlicer personnalisé

Choisissez la couleur sur l’écran de l’imprimante, puis synchronisez
**matière et couleur dans PrusaSlicer** via **PrusaLink local**.
Sur INDX : palette de 60 nuances et correction directe par extrudeur.

**Il faut télécharger notre version complète de PrusaSlicer et installer le
firmware correspondant à sa machine. Ce n’est pas un plugin/add-on de l’officiel.**
Les fonctions habituelles restent disponibles, y compris Prusa Connect facultatif.
La synchronisation des filaments utilise PrusaLink local, sans dépendance au cloud.

**Vous consultez ici : Slicer commun MK4 / INDX.** Prototype communautaire expérimental de
Coben-3d, sans validation officielle Prusa.

## Choisir sa machine

| Machine | Firmware et fonctions | Guide | Branche sources |
|---|---|---|---|
| MK4, une bobine, sans MMU | 6.5.7-color+4 ; couleurs nommées au chargement | [MK4](doc/INSTALL-MK4.md) | [MK4](https://github.com/Coben-3d/Prusa-Firmware-Buddy/tree/feature/mk4-loaded-filament-color) |
| CORE One / CORE One+ équipée d’INDX 8 têtes | 6.9.1-color+3 ; 60 nuances et correction sans recharge | [INDX](doc/INSTALL-COREONE-INDX.md) | [CORE One INDX](https://github.com/Coben-3d/Prusa-Firmware-Buddy/tree/feature/coreone-indx-loaded-filament-colors) |
| CORE One normale V1, sans INDX | Portage en pause ; aucun BBF fourni | — | — |

Deux branches firmware distinctes dans le même dépôt, un [Slicer commun](https://github.com/Coben-3d/PrusaSlicer/tree/feature/indx-local-filaments)
compatible avec les deux. La palette INDX n’est pas encore portée sur MK4.

- **[Téléchargements v0.2.1](https://github.com/Coben-3d/Prusa-Firmware-Buddy/releases/tag/local-filaments-v0.2.1)** : BBF de sa machine + ZIP Slicer.
- **[Guide complet](doc/LOCAL-FILAMENTS.md)** : installation, lanceurs/Dock,
  PrusaLink et Connect, profils, dépannage et retour officiel.
- [Tests et limites](doc/LOCAL-FILAMENTS-VALIDATION.md).
- [Sources PrusaSlicer personnalisées](https://github.com/Coben-3d/PrusaSlicer/tree/feature/indx-local-filaments). Les sources GitHub ne sont pas le binaire.

Binaire Slicer : **Apple Silicon et macOS 26.2 minimum**. Windows, Linux et Mac
Intel : sources disponibles, aucun binaire ou validation fourni ici.
Les BBF non signés nécessitent la rupture **irréversible** de la languette
xBuddy `!` : lire [les conditions Prusa](https://help.prusa3d.com/article/flashing-custom-firmware-core-one-l-core-one-mk4-s-mk3-9-s-mk3-5-s_814967)
et le guide. Les BBF de machines différentes ne sont pas interchangeables.

**English:** community custom firmware plus a full customized PrusaSlicer
download, not a plugin. Local material/color sync over PrusaLink. Separate MK4
and CORE One INDX firmware branches, one shared Slicer, optional Prusa Connect.
macOS binary requires Apple Silicon and macOS 26.2+. Standard CORE One without
INDX is not supported yet. Read the installation guide first.

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
