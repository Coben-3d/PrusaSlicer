# Synchronisation locale dans PrusaSlicer

Voir [le guide d’installation](LOCAL-FILAMENTS.md) et
[le portage INDX](COREONE-INDX-local-filaments.md).
Le binaire publié 2.9.6+FilamentLocal-INDX accepte MK4 v1 et INDX v2.

`LoadedFilamentColor.cpp` valide le protocole ;
`LoadedFilamentColorConfig.cpp` résout les profils compatibles et les couleurs ;
`LoadedFilamentColorRequest.cpp` réutilise l’authentification de PrusaLink ;
`Sidebar.cpp` possède l’état asynchrone, le bouton et le dialogue de profils.
Le corps HTTP est limité à 4 096 octets ; les redirections sont refusées.
La synchronisation ne sauvegarde pas automatiquement les presets édités.

Conserver le catalogue PrusaResearch et le champ `inherits` du profil utilisateur
pour que la fenêtre Imprimante physique propose PrusaLink. Un profil personnalisé
doit aussi contenir ses valeurs complètes ; le chargement du preset utilisateur
ne repose pas uniquement sur `inherits` pour récupérer tous les paramètres.

Pour compiler, suivre les guides de ce dépôt pour macOS, Linux ou Windows.
Les cibles utiles sont `PrusaSlicer`, `loaded_filament_color_tests`,
`loaded_filament_color_config_tests` et `loaded_filament_color_client`.
Les scénarios GUI et HTTP se trouvent dans `tests/slic3rutils`.

La distribution macOS est générée par `scripts/package_local_filaments_macos.py`.
Elle copie uniquement le binaire, les ressources de sources, les licences,
le blocage du trousseau et le catalogue officiel fourni. Elle construit des
réglages neufs sans consulter les réglages personnels.
