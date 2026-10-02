# Prototype local : filament chargé sur MK4

Base : PrusaSlicer **2.9.6**, tag `version_2.9.6`, commit
`b028299c770b8380ee81c921a2867d522f288123`. Branche locale :
`feature/mk4-local-filament-colors`. Aucun fork Slicer publié ni PR ouverte.

Le firmware Buddy associé est le prototype `6.5.7-color+4`, commit
`95cfcf359236be6cb49e9e467fb50689fdcf3281`. Sa route est une extension du
prototype, absente du firmware officiel. Les tests automatisés utilisent une API factice sur localhost et un MK4 virtuel
Mini404. Un essai réel a ensuite été autorisé : Benjamin rapporte le démarrage,
le chargement violet et la synchronisation locale réussis. La persistance après
redémarrage reste à vérifier ; aucune réponse authentifiée réelle n’a été lue
directement par l’agent.

## Comportement

Le bouton **Synchroniser le filament (PrusaLink)** apparaît sous la ligne du
filament. Il est actif pour une imprimante FFF à une seule buse, sans extrudeurs
virtuels, avec une imprimante physique sélectionnée et `host_type=PrusaLink`.
L'adresse, la clé API ou les identifiants Digest et le certificat sont ceux de
cette connexion existante. Aucune nouvelle option de connexion, aucune
création d'identifiants et aucune dépendance à Prusa Connect.

Au clic, une requête GET authentifiée lit `api/v1/filaments`. Maximum :
3 secondes pour la connexion, 8 secondes au total, 4096 octets de réponse.
Aucune relance automatique ou redirection. Les traces HTTP sensibles sont
désactivées, y compris au niveau de journalisation debug. Le worker transmet
un résultat via une future ; un timer GUI le traite. Il n'accède jamais aux
fenêtres. La fermeture de la fenêtre ou un changement de sélection annule
la requête et empêche d'appliquer sa réponse.

Le seul contrat accepté pour ce prototype est :

```json
{"schema_version":1,"slots":[{"slot":0,"material":"PLA","color":"#000000","source":"user_declared"}]}
```

`color:null` signifie inconnue et conserve la couleur actuelle. `#000000`
est bien le noir. Les hexadécimaux sont normalisés en majuscules. Les versions
inconnues, slots multiples, indices autres que 0, clés répétées, types
incorrects et couleurs invalides sont refusés sans changement de couleur.
`material` peut être une chaîne ou null : c’est la matière déclarée par
l’opérateur, sans information de marque. Elle synchronise désormais la liste
Filament :

- un profil compatible déjà du même type est conservé, avec ses réglages édités ;
- sinon, un unique profil système **Generic <type>** compatible avec l’extrudeur
  et le profil d’impression est sélectionné ;
- si plusieurs choix restent possibles ou aucun profil générique n’est unique,
  une fenêtre propose les profils compatibles du type exact, sans deviner la marque ;
- une matière inconnue ou sans profil installé compatible conserve le profil
  actuel et affiche cette limite. La couleur peut toujours être synchronisée.

Le type est comparé exactement à `filament_type[0]` : aucun rapprochement entre
PLA et PLA+, aucun profil incompatible, invisible ou par défaut n’est choisi.
Le changement utilise le chemin normal de la liste Filament et ses confirmations
Save / Discard / Transfer / Cancel. Annuler la sélection ou le traitement d’un
profil édité conserve le profil et la couleur. Un changement accepté sélectionne
le **profil entier**, donc ses températures et autres paramètres d’impression ;
il ne remplace pas seulement l’étiquette du type de matière.

La partie couleur modifie uniquement `extruder_colour[0]`, par le même chemin
que le sélecteur de couleur existant : configuration éditée du preset
imprimante, rafraîchissement des listes et de la vue. Si le profil de filament
reste le même, ses températures et tous ses paramètres sont conservés. La
sélection d’un autre profil applique ses paramètres complets. La buse, le profil
d’impression et les fichiers de presets sauvegardés ne sont pas modifiés par
la synchronisation elle-même ; une sauvegarde de réglages édités reste une
action explicite du dialogue normal de Slicer. Une modification de couleur marque donc le
preset imprimante **édité** comme modifié ; aucun preset n'est sauvegardé
automatiquement. Elle peut être conservée dans le projet 3MF comme le choix
manuel existant. Ce chemin n'ajoute pas un Undo/Redo de réglages : la pile
Undo/Redo existante capture le modèle et ne restaure pas la configuration
éditée des presets.

Avant l'application, la sélection physique et sa configuration, la
configuration complète, le modèle, le nom du projet et les identifiants
d'objets sont comparés à leur état au clic. Une réponse tardive ne doit pas
écraser une couleur changée entre-temps ou un autre projet. Les erreurs
401/403, 404, connexion/délai et réponse invalide ont des messages distincts.

## Fichiers modifiés

| Composant | Fichiers | Rôle |
|---|---|---|
| Protocole | `src/slic3r/Utils/LoadedFilamentColor.{hpp,cpp}` | Parseur strict, noir/inconnu, schéma mono-bobine |
| Application | `LoadedFilamentColorConfig.cpp` | Couleur et résolution des profils de matière compatibles |
| Réseau | `LoadedFilamentColorRequest.{hpp,cpp}`, `OctoPrint.{hpp,cpp}` | GET local réutilisant `PrusaLink::set_auth` |
| HTTP | `Http.{hpp,cpp}` | Options de refus de redirection et traces sensibles ; valeurs par défaut existantes conservées |
| Interface | `GUI/Sidebar.{hpp,cpp}`, `GUI/Plater.cpp` | Bouton, statut, timer et annulation lors du reset de projet |
| Compilation | `CMakeLists.txt`, `src/slic3r/CMakeLists.txt`, `version.inc` | JSON partagé et identification `MK4-FilamentLocal` |
| Français | Catalogues PO/POT et MO français | Libellés couleur et matière ajoutés, traductions préexistantes conservées |
| Tests | `tests/slic3rutils/loaded_filament_color_*`, `test_loaded_filament_color_*.py` | Protocole, conservation des réglages, mocks HTTP et firmware virtuel |

## Validation de la synchronisation matière

La version `PrusaSlicer-2.9.6+MK4-FilamentLocal` est compilée en arm64 avec STEP
actif et les dépendances déjà présentes sur le SSD. Les nouveaux contrôles
passent :

| Contrôle | Résultat | Preuve dans `artifacts/` |
|---|---|---|
| CTest : protocole et choix de profil | 2 cibles, 0 échec ; configuration/matière 3 cas / 25 assertions | `slicer-material-ctest.log` |
| Interface wx native, événement réel et profils Prusa officiels | 13 tests, dont PETG→PLA, PLA personnalisé à 233 °C, matière ou couleur inconnue, aucun profil, annulation du choix ambigu et du dialogue de réglages édités, réponse tardive | `slicer-material-gui.log`, `.xml` |
| Client HTTP réel contre API factice locale | 16 tests, 0 échec | `slicer-material-http.log`, `.xml` |
| Catalogue français | 11 libellés ajoutés ; toutes les traductions du MO précédent conservées | `slicer-material-validation.json` |

Les tests de passage PETG→PLA vérifient que le profil édité entier est égal au
profil système `Generic PLA @PGIS`, et que le profil d’impression ne change pas.
Les tests de conservation comparent la configuration complète ; l’annulation
du dialogue normal de réglages édités est réellement exercée. Le firmware et
son API ne changent pas. L’essai réel de cette évolution matière reste distinct
du succès couleur déjà déclaré par Benjamin.

## Profils PrusaLink dans un dossier isolé

La fenêtre Imprimante physique n’offre PrusaLink que pour un profil rattaché
au constructeur Prusa Research. Copier uniquement les valeurs aplaties de
MK4IS perd cette association et la fenêtre bascule alors en OctoPrint, même si
`host_type = prusalink` était prérempli. Le dossier de test doit contenir
`vendor/PrusaResearch.ini` officiel et le profil utilisateur complet doit garder
`inherits = Original Prusa MK4 Input Shaper 0.4 nozzle`. Le chargement d’un
profil utilisateur applique ses valeurs sur les valeurs par défaut ; ce champ
d’héritage seul ne suffit pas à récupérer tous les paramètres du parent.

Cette correction a été vérifiée par un test natif ouvrant le vrai dialogue
Imprimante physique : association Prusa Research retrouvée et type PrusaLink
conservé. Preuve : `artifacts/hardware-trial/profile-diagnostic/RESULT.json`.

## Validation et reproductibilité

Le binaire natif arm64 de PrusaSlicer est compilé et lié avec STEP actif.
Les contrôles de la version couleur initiale suivants ont passé :

| Contrôle | Résultat | Preuve dans `artifacts/` |
|---|---|---|
| CTest C++ : protocole et conservation de la configuration | 2 cibles, 0 échec ; parseur 3 cas / 31 assertions, configuration 1 cas / 8 assertions | `slicer-color-ctest.log` |
| Client PrusaLink C++ réel contre API HTTP factice | 16 tests : API key, Digest, couleurs, erreurs, limites, redirection, délai, annulation | `slicer-color-http.log`, `.xml` |
| Bouton wxWidgets natif, événement réel | 5 tests : rouge, noir, inconnue, 404, réponse tardive après modification manuelle | `slicer-color-gui-final.log`, `.xml`, `slicer-gui-*.log` |
| Client C++ contre firmware MK4 virtuel en fonctionnement | 3 tests : noir, blanc, inconnue, déclaration EEPROM injectée | `slicer-color-firmware.log`, `.xml` |
| Import STEP réel et export STL par le binaire livré | Cube OCCT de 10 mm importé ; STL de 12 triangles exporté | `slicer-step-smoke.log`, `step-smoke-box.step`, `.stl` |

Les tests couleur initiaux comparent la configuration complète avant/après : seule
`extruder_colour` change pour une couleur connue ; aucun changement pour
inconnue ou erreur. Ils vérifient aussi que le fichier du preset imprimante
reste identique. Le test tardif attend que le serveur ait reçu la requête,
puis change la couleur en vert et vérifie qu'elle reste verte.

Le harnais utilise des dossiers de réglages jetables sur le SSD. Le chargement
des tokens du compte Prusa par le démarrage normal est bloqué par un interposeur
macOS de test (`deny_keychain.c`), sans modification du binaire de production.
Sa présence est vérifiée dans chaque essai. La mise à jour des presets et le
polling Connect sont désactivés ; le contrôle de version pointe sur localhost.
Les messages de trousseau bloqué, de version locale inaccessible et de pilote
3Dconnexion absent sont attendus dans ces journaux ; aucun pilote à installer.

Les premiers essais avaient un défaut de fermeture du harnais : `Destroy()`
contournait le nettoyage de `MainFrame::shutdown()`, puis une fermeture avant
l'initialisation du chemin temporaire déclenchait une exception amont. Le
harnais appelle désormais la mise à jour normale du plater et `Close()` ; les
cinq tests finaux se terminent avec un code 0. Le code de fermeture de
l'application n'a pas été modifié.

Aucun push ou publication Slicer. Le commit local et les SHA256 des binaires
sont consignés dans `artifacts/slicer-color-validation.json`, hors dépôt.

Les sources, téléchargements, dépendances et compilations restent sur le SSD
`/Volumes/JAUNE - SAVE BEN/PrusaDev`. Un alias temporaire sans espaces,
`/tmp/prusadev-native-20261001`, pointe vers ce dossier. `TMPDIR` est aussi un
alias du dossier `tmp` sur le SSD. Aucun paquet système n'est installé.

Outils utilisés : Apple Clang 17/Command Line Tools, CMake 3.28.3 local,
Ninja existant, SDK macOS 26.2, architecture arm64 et cible macOS 26.2.
Les dépendances proviennent des URL et SHA256 déclarés par la base PrusaSlicer.
Le build final utilise `SLIC3R_ENABLE_FORMAT_STEP=ON` et inclut la dépendance
OCCT **7.6.1** définie par le dépôt, archive officielle vérifiée par SHA256
`b7cf65430d6f099adc9df1749473235de7941120b5b5dd356067d12d0909b1d3`.
Le wrapper OCCT et son raccordement macOS restent ceux de la base officielle.
Le lien `load_step_internal` est résolu par cette bibliothèque réelle ; aucun
stub ni désactivation de STEP dans le résultat final.

La copie de compilation MPFR nécessite un ajustement de `acinclude.m4` :
préserver l'alias sans espaces au lieu de le résoudre vers le nom du volume
contenant des espaces. Cet ajustement ne modifie pas le dépôt PrusaSlicer.
CMake reçoit explicitement le wx-config installé à
`build/slicer-deps/prefix/lib/wx/config/osx_cocoa-unicode-static-3.2`.

Les commandes détaillées et résultats sont conservés dans les journaux
`artifacts/slicer-*.log` du dossier PrusaDev. Les cibles de test sont :

```sh
cmake --build "$SLICER_BUILD" --target PrusaSlicer loaded_filament_color_tests loaded_filament_color_config_tests loaded_filament_color_client --parallel 2
ctest --test-dir "$SLICER_BUILD" -R '^loaded_filament_color_(config_)?tests$' --output-on-failure
SLICER_COLOR_CLIENT="$SLICER_BUILD/tests/slic3rutils/loaded_filament_color_client" python -m pytest tests/slic3rutils/test_loaded_filament_color_http.py
```

Pour l'interopérabilité virtuelle, définir également `BUDDY_SOURCE_PATH`,
mettre `tests/slic3rutils` dans `PYTHONPATH`, charger le plugin
`-p buddy_fixture_bridge`, puis lancer
`test_loaded_filament_color_firmware.py` avec `--firmware` et `--simulator`
pointant vers les artefacts locaux Buddy et le runner Mini404 compatible.
Reprendre les variables de caches/OCR documentées dans le dépôt Buddy.

Pour les tests natifs, compiler `deny_keychain.c` en bibliothèque dynamique
macOS et vérifier son interposition avant le lancement ; construire le harnais
avec `python tests/slic3rutils/build_color_gui_harness.py "$SLICER_BUILD"`.
Définir `SLICER_COLOR_GUI` et `SLICER_DENY_KEYCHAIN_DYLIB`, puis exécuter
`test_loaded_filament_color_gui.py`. Le test refuse de fonctionner sans cet
interposeur ou hors d'un dossier temporaire dédié. Il teste le vrai Sidebar et
les bibliothèques du build final, en remplaçant uniquement le point d'entrée
par le harnais.

Le catalogue français de la base comporte déjà une entrée plurielle invalide
pour `msgfmt --check`. Le catalogue MO du prototype est comparé à celui du tag :
aucune traduction existante changée, 12 nouvelles traductions ajoutées. Cette
anomalie préexistante n'est pas corrigée dans ce prototype.

## Limites

Le binaire de test vise ce Mac arm64 avec macOS 26.2 ; Windows, Linux et
les autres versions de macOS n'ont pas été compilés ni validés ici. Le
round-trip 3MF et Undo/Redo n'ont pas de test de bout en bout dans cette série.

La route exige le firmware personnalisé associé. Sur un firmware officiel,
le client reçoit normalement 404 et conserve la couleur. Un serveur local
qui respecte ce contrat permet de tester le bouton sans flasher une machine ;
ce n'est pas une lecture de couleur depuis un MK4 officiel.

Les tests firmware virtuels injectent une déclaration dans l'EEPROM virtuelle.
Le chargement GUI complet jusqu'à la confirmation réussie reste non validé :
le Mini404 retenu se bloque pendant Parking. Aucun garde-fou firmware n'est
contourné. Il faudra valider ce parcours, le capteur actif, M600 et les cas de
runout avant toute expérimentation sur une machine réelle. Aucun flash,
changement matériel, accès à une imprimante réelle ou secret réel dans cette
validation.

L'extension INDX à huit têtes reste hors de ce prototype. Elle demande une
identité stable par bobine/tête, une correspondance explicite avec les
extrudeurs Slicer et une politique pour les slots inconnus ou absents. Le
parseur actuel refuse volontairement plusieurs slots.
