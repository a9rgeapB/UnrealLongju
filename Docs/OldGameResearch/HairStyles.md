# Hair looks and quests

Hair appearance is authoritative, persistent, and independent of the body mesh. `AMT2PlayerState` saves a hair shape and an absolute expiry time. Observers receive the shape, not inventory contents. Classic hairstyle items (including client-proto hair costumes in the 73001–75619 range) consume one item per successful use, including repeated uses of the current look. They never swap a previous look back into the bag. A classic hairstyle already stored in wear slot 20 is discarded when a new look is applied. Separate wearable hair costumes still override the underlying hairstyle; removing those reveals that hairstyle again. The character-selection preview uses the same catalog and precedence.

## Legacy behavior

The client resolves `PART_HAIR` (3) through each race/sex `.msm` file's `HairData` entries. Each entry specifies a model and its replacement skin. Shape numbers are not item vnums.

- Cosmetic `ITEM_TYPE_HAIR` items use the player's job offset: Warrior 72000, Assassin 71250, Sura 70500, Shaman 69750. For example, Warrior item 73001 selects shape 1001. The original server did not consume repeated uses of the current look; this project consumes every successful use. Imported costume-form hairstyle items use their `HairShapeId`. Item restrictions still apply.
- `hair.quest` handles the bonus hair items starting at 74001, 74251, 74501, 74751 and their alternate-sex ranges. Its table controls the shape, apply type, value and duration. The source grants 30-day class bonuses or seven-day HP/SP/monster bonuses and consumes the item.
- Hair bonuses use affect 514. Expiry resets the look to normal and removes the bonus. Time spent offline counts toward expiry. The hairdresser and Halloween quest retain their source conditions and dialogue.

## Assets and runtime

`/Game/Logic/Items/DA_HairCatalog` contains imported `HairData` shape/mesh/texture mappings. The runtime loads the small catalog once, then asynchronously loads the selected visual assets through the equipment renderer. Existing imported hair meshes and textures are reused. The catalog is cooked under `/Game/Logic`.

Regenerate mappings after changing the authorized client dump or imported meshes:

```powershell
& 'F:/Engine2/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' F:/UnrealLongju/UnrealLongju.uproject -run=MT2ImportItems -HairCatalogOnly -nullrhi -unattended
```

The command validates mapped textures and uses only meshes present in imported Content. It does not import absent GR2 assets. Import missing meshes/textures through the existing asset importer first. `MT2 Character Appearance` project settings holds the catalog reference.

The quest runtime supports hair-part reads/writes, nil-aware wear reads, helper-based hair affects and flag assignments, and selecting a newly granted item for subsequent socket writes. `item.start_realtime_expire()` schedules inventory/equipment expiry only for protos declaring a real-time limit, matching the legacy binding. Deadlines survive database saves and inventory transfers. `say_show_item` currently renders the resolved item name in the text-based quest dialogue, rather than the legacy item-image layout.

The supplied hairdresser conversion branch contains contradictory empty-costume conditions. Those authored guards are preserved; successful import does not imply that this branch is reachable. The GM tester is guarded by `is_test_server()` as in the source.

## Validation checklist

Test cosmetic hair, timed bonus hair and the hairdresser/Halloween interactions on both sexes. Check consumption, same-look reuse, costume precedence, observer/late-join visibility, character selection, map transfer and reconnect. Expiry must remove both the look and bonus, including expiry while offline. Client/server builds must be updated together because the appearance payload and item-instance layout include hair/expiry data.
