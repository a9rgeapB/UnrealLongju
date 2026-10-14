# Offline character play

The **UnrealLongju Game target** runs a local authoritative game in one process. It does not require a gateway, coordinator, or dedicated map server. The separate **UnrealLongjuClient** and **UnrealLongjuServer** targets retain the online flow.

## Launch

On Windows, run `Scripts/BuildOfflineDevelopment.bat` to build, cook, and stage the offline Game target. Then run `Scripts/StartOfflineDevelopment.bat`. Distribute the complete `Saved/StagedOffline/Windows` directory, not just its executable. `UE_ROOT` can override the build script's default engine root, `F:/Engine2`.

For Shipping, use `Scripts/BuildOfflineShipping.bat` followed by `Scripts/StartOfflineShipping.bat`. Its separate output is `Saved/StagedOfflineShipping/Windows`; distribute that complete directory. Debug files are not staged, and the launcher does not force a log window. Shipping and Development share the local offline profile and saves. The first Shipping compilation may take substantially longer if the engine's Shipping binaries are not already built.

Offline Shipping Game builds do not require a patcher token or the online autopatcher. Launch directly from the packaged `UnrealLongju.exe` or use `Scripts/StartOfflineShipping.bat`. The separate online Shipping Client target still requires an authorized patcher launch.

The build script requests iterative cooking and deployment. Unchanged cooked assets can be reused; changed assets, dependencies, or cook settings may still require recooking. Do not delete the cook cache when testing incremental builds.

The gateway startup map opens character selection directly. The existing four slots and character creation controls are reused. Creation commits a local character; Start enters the character's locally cooked map and restores its saved state before enabling normal saves.

Offline sessions disable the gameplay voice relay and microphone capture. They do not open a gameplay server connection.

All characters in the local offline profile have admin permissions when playing offline, including characters created previously. No separate admin setup is required, and no online admin registry entries are created. Online permissions remain unchanged; fake-player bots retain their non-admin status.

Editor Play Standalone through the gateway map uses this flow when no online connection or server role was requested. Direct gameplay-map PIE remains available for development without character selection. Do not supply a server address or `-connect` when testing offline play.

## Local data

Offline data lives in `UnrealLongju/Offline/offline.sqlite` beneath Unreal's per-user settings directory (normally `%LOCALAPPDATA%/UnrealLongju/Offline` on Windows). It uses an implicit local profile, with no username/password. It is separate from coordinator storage and cannot import or export online characters.

Player state uses the existing persistence format: appearance, attributes, progression, skills, inventory/equipment, quest state, and world position. Local SQLite commits are synchronous. Autosave and normal exit/return save paths reuse player persistence; an unsuccessful character restore is never saved over the existing record.

Disconnect returns to local character selection after saving, rather than connecting to a network gateway. The menu Exit button saves before quitting. World teardown and GameInstance shutdown provide final-save fallbacks; force-killing the process can still lose changes since the last committed save.

Missing maps or unavailable save storage produce errors instead of silently creating an unsaved/default character.

## Scope

This is the offline character-selection, initial map-entry, and persistence foundation. It is not a full audit of multiplayer-only features, every quest/dungeon, or subsequent gameplay map transfers. Offline files can be modified by players and are not trusted by the online server.

## Packaged smoke test

After staging, launch without online arguments, create a character, enter its starting town, collect an item, and return using Disconnect. Select the character again, then use Exit and relaunch. Check that appearance, inventory, progression, and position persist. Repeat with a second character to verify slot separation. This visual packaged check complements automation; compiling the Game executable alone does not produce a playable cooked package.

## Particle cook validation

Run the editor commandlet `-run=MT2RepairParticleOwnership` against the project to audit Cascade module ownership under `/Game/ymir_work`. It returns failure if invalid modules remain. Add `-Repair` to relocate invalid modules using the importer's normalization and save only affected assets; textures, curves, emitter settings, and object references are retained. Original asset files are backed up under `Saved/ParticleOwnershipBackups` before modification. Close the interactive editor before repairing. Loading a damaged asset may trigger an engine error on the repair pass; always rerun the audit in a fresh process and require zero invalid assets and exit code zero before cooking.
