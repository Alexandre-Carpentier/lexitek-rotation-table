# Migration : CMakeSettings.json ? CMakePresets.json

## ? Migration Réussie

Votre `CMakeSettings.json` a été migré vers `CMakePresets.json`.

### Fichiers modifiés

- **Créé** : `CMakePresets.json`
- **Original** : `CMakeSettings.json` (peut être supprimé)

### Validation

```bash
cmake --list-presets
# Output:
# Available configure presets:
#   "x64-Debug"   - x64 Debug
#   "x64-Release" - x64 Release
```

? Les deux presets sont correctement reconnus par CMake.

## Différences dans la migration

### CMakeSettings.json (Ancien)
```json
{
  "configurations": [
    {
      "name": "x64-Debug",
      "generator": "Ninja",
      "configurationType": "Debug",
      "inheritEnvironments": [ "msvc_x64_x64" ],
      "buildRoot": "${projectDir}\\out\\build\\${name}",
      "installRoot": "${projectDir}\\out\\install\\${name}"
    }
  ]
}
```

### CMakePresets.json (Nouveau)
```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "x64-Debug",
      "displayName": "x64 Debug",
      "generator": "Ninja Multi-Config",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      },
      "binaryDir": "${sourceDir}/out/build/${presetName}",
      "installDir": "${sourceDir}/out/install/${presetName}"
    }
  ],
  "buildPresets": [ ... ],
  "testPresets": [ ... ]
}
```

## Points importants

1. **Standard officiel CMake 3.21+** ?
   - Votre CMake 3.31.6 est compatible
   - Standard de l'industrie

2. **Visual Studio 2022 natif** ?
   - Reconnaît automatiquement CMakePresets.json
   - Interface graphique intégrée pour sélectionner les presets
   - Pas besoin de CMakeSettings.json

3. **Portable** ?
   - Fonctionne sur Visual Studio, VS Code, CLion, command-line
   - Un seul fichier de configuration pour tous

4. **Multi-config avec Ninja** ??
   - Utilisé `Ninja Multi-Config` pour supporter Debug et Release
   - Compatible avec CMake 3.20+

## Utilisation dans Visual Studio 2022

### Interface graphique
1. Ouvrir le projet dans Visual Studio 2022
2. Cliquer sur la liste déroulante de configuration 
3. Sélectionner "x64-Debug" ou "x64-Release"
4. Build > Build Solution

### Ligne de commande
```bash
cmake --preset x64-Debug
cmake --build --preset x64-Debug
ctest --preset x64-Debug
```

## Nettoyage optionnel

Vous pouvez supprimer le `CMakeSettings.json` car il n'est plus utilisé :

```bash
rm CMakeSettings.json
```

Visual Studio utilisera automatiquement `CMakePresets.json` maintenant.

## Dépannage

Si Visual Studio 2022 n'affiche pas les presets :
1. Fermer et rouvrir le dossier du projet
2. Vérifier que le `CMakePresets.json` est à la racine du projet
3. Exécuter `cmake --list-presets` pour valider la syntaxe

## Références

- [CMake Presets Documentation](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)
- [Visual Studio CMake Support](https://docs.microsoft.com/en-us/cpp/build/cmake-presets-vs)
