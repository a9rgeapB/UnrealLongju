# Metin2 automation tests

`Metin2.Dungeons.DevilTowerMessages` checks entry/late-entry objectives, duplicate-admission suppression, per-participant completion, outsider exclusion and completed-stage arrival. `Metin2.UI.NotificationRouting` also verifies that multiline dungeon messages use chat history rather than the info/loot history.

`MT2HeldAttackTests.cpp` covers melee through overlap-only dungeon volumes versus solid walls, held-attack fallback without animation callbacks, retries across an activation blocker, and preservation/release of keyboard intent during automatic target changes. Packaged input, stun/knockback recovery and network prediction still require gameplay validation.

`MT2DungeonTests.cpp` covers `Metin2.Dungeons`: shared membership, idempotent starts/registration, doors, countdowns, entitlements, same-world advance, empty-only resets, teardown, living-objective despawn failure, spawn rejection and `pc.in_dungeon()`. Tower tests cover opening transitions surviving a disconnect, fake-stone purge versus real-stone completion, five seal/key transactions, fake/remote key rejection, map replay protection, blacksmith eligibility and final late-join rewards. Fixtures do not validate a full packaged multi-client tower run or cross-process exit. See [shared dungeon stages](../../../Docs/Dungeons.md).

All first-party Unreal automation test implementations live in this directory.
Fishing gameplay and persistence coverage is in `MT2FishingTests.cpp`, compiled
by the runtime module. See [fishing](../../../Docs/Fishing.md).

| Files | Compiling module |
| --- | --- |
| `MT2DamageFeedbackTests.cpp`, `MT2DuelTests.cpp`, `MT2MapAttributeTests.cpp`, `MT2PathSettingsTests.cpp`, `MT2ProfilingPortsTests.cpp`, `MT2QuestPlayerApiTests.cpp` | `Metin2` runtime module |
| `MT2AreaPaintTests.h`, `MT2PIETravelTests.h` | `MT2UEEditor`, through `Private/MT2TestRegistration.cpp` |
| `MT2QuestImporterTests.h` | `MT2UEEditor`, included at the end of `Private/Importers/MT2QuestImporter.cpp` |

The editor test headers contain implementations, not public APIs. Include each
from its designated editor translation unit only. Keeping registration there
avoids adding editor dependencies to the runtime module, and lets importer tests
exercise existing private parser helpers without exposing them or duplicating
production code. Implementations are guarded by `WITH_DEV_AUTOMATION_TESTS`.

Build `UnrealLongjuEditor` and run the `Metin2` group through Session Frontend's
Automation tab. Save work before running PIE integration tests: they load the
configured editor startup map and start/stop PIE. GPU/PIE tests require a rendering
editor; NullRHI does not validate those paths. See
[PIE map travel](../../../Docs/PIETravel.md) and
[area painting](../../../Docs/AreaPainting.md) for fixture details.

Duel coverage includes `Metin2.Combat.Duels.Lifecycle`, `.DeathPenalties`, and
`.ButtonLabels`. See [duels](../../../Docs/Duels.md) for scope and limitations.

`Metin2.Config.FrameProPort` covers the startup game-port offset, independent
voice-port overrides, engine URL defaults, explicit profiler overrides, and
TCP port bounds.

See [water](../../../Docs/Water.md) for the retained map-bake and read-only verification workflow.

Mob scheduling/movement fixtures are in `MT2LegacyMobSchedulingTests.cpp` and `MT2MobMoveSegmentTests.cpp`; knockback coverage is in `MT2KnockbackTests.cpp`. Run `Metin2.World` and `Metin2.Combat` for these groups.

`MT2FakePlayersTests.cpp` covers the Development load-test harness under `Metin2.Server.FakePlayers`: authorization, quotas, staggered spawns, equipment, ephemeral identities, targeting, grid registration and cleanup. See [server load testing](../../../Docs/ServerLoadTesting.md) for command syntax and benchmark limitations.
