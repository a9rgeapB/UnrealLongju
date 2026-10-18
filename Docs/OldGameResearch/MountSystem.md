# Mount System

## Scope

This records the original client/server behaviour and the current Unreal implementation, including
which presentation details and multiplayer cases still need validation.

## Original game behaviour

### Server authority

The original server owns horse level, health, stamina, and riding state in `THorseInfo`. Riding is
refused while dead, polymorphed, out of horse health/stamina, or in restricted content. Starting a
ride despawns the separately summoned horse and puts its race vnum on the player update packet.
Dismounting clears that vnum and normally summons the horse beside its owner again.

Horse progression has 30 levels and three grades: basic (1-10), armed (11-20), and military
(21-30). `horse_rider.cpp` defines minimum player level, horse race, health, stamina, primary stats,
damage, and defense per level. Basic horses cannot attack. Armed horses attack but do not use horse
skills. Military horses can use horse skills.

Stamina decreases by one every six minutes while riding and regenerates by one every twelve minutes
while dismounted. Horse health decreases on a long real-time schedule and catches up after login.
Zero stamina dismounts the rider; zero health kills/despawns the horse. Feed/revive are quest and
item driven.

### Client presentation and movement

The client receives `dwMountVnum` in actor replication. It creates a second visual actor for the
mount, but the player remains the network actor. Position, rotation, speed, and motion are copied to
the mount. Rider and mount actions start together. This is presentation coupling, not two independent
authoritative actors.

Mounted rider modes are weapon-specific: `horse`, `horse_onehand_sword`, `horse_twohand_sword`,
`horse_dualhand_sword`, `horse_bow`, `horse_fan`, and `horse_bell`. The mount plays corresponding
wait/walk/run/attack/death actions. The old client also uses a slower mounted rotation rate, raises
nameplates, changes some weapon attachment rules, and emits horse dust while moving.

Horse-only skills are blocked while dismounted. Ordinary non-horse skills are blocked while mounted.
Some special mount vnums override attack/skill capability.

## UE architecture

`UMT2MountComponent` lives on `AMT2PlayerCharacter` and owns replicated mounted state. The player
remains the possessed `ACharacter` and sole replicated movement source. A local skeletal mesh
component renders the mount beneath the rider. This preserves CharacterMovement prediction, avoids
duplicate correction traffic, and matches the old game's effective network model.

`UMT2MountDefinition` is a data asset containing identity, mesh, animation class, mount/rider relative
transforms, movement multiplier, and combat permissions. Runtime code has no mount asset paths or
vnum switches. Imported horses and modern mounts can share the component.

The movement multiplier belongs to `UMT2MovementSpeedComponent`, so equipment/status refreshes do
not erase it. Equipping a weapon while mounted selects the existing `horse_*` rider animation set.
Mounting, dismounting, and death are server-authoritative and replicated to all clients.

## Current implementation

- Replicated mount/dismount state and authority-only Blueprint entry points. A client cannot submit
  an arbitrary definition; the mount item handler resolves it through server-owned item/template data.
- Data-driven mount definition asset.
- Mount skeletal mesh and animation class.
- Configurable rider/mount transforms and movement multiplier.
- Weapon-specific mounted rider animation sets.
- Attack and horse-skill capability queries.
- Forced dismount on death; dedicated servers skip visual creation.
- Mount item base class with consumable/permanent policy and optional timed expiry.
- PlayerState-owned horse progression persists level, health, stamina, health-decay deadline and
  stamina recovery timestamp in the existing character JSON snapshot. Riding remains transient.
- Horse progression uses the legacy 30-level health/stamina/stat table, grade boundaries, rider
  minimum level, six-minute riding drain, twelve-minute dismounted/offline recovery and three-day
  health decay. Empty stamina or death forces a dismount.
- Legacy horse level/grade/health/stamina/dead/riding/summoned quest reads use the authoritative
  progression and mount state. `horse.advance`, `horse.set_level`, `horse.feed` and `horse.revive`
  are available as server-side quest actions.
- Vnums 50054-50056 import as feed items and 50057-50059 as revive items; invalid use does not
  consume the item. The action and mount are resolved on the server.
- Ordinary horse grade controls combat: basic cannot attack, armed can attack, and military can
  use horse skills. Horse-level primary-stat minimums apply only while mounted and are recalculated
  on mount state changes.
- Item importer parses `ride.quest` and `ride_upgradable.quest`, reparents matching item Blueprints,
  and imports ride duration, minimum level, consume policy, and mount VNUM.
- Item import creates reusable `/Game/Logic/Mounts/DA_Mount_<VNUM>` assets from the matching mob
  Blueprint registered in `/Game/Logic/DA_MT2VnumRegistry`.

## Remaining work

1. Import/generate dedicated mount animation Blueprints where mob animation graphs do not expose
   the complete wait/walk/run/attack/death set.
2. Add the horse status UI and a complete summon/dismiss presentation loop (called-horse data is
   currently represented by a validated mount definition, not a separate authoritative actor).
3. Synchronize one-shot mount actions with rider attack, damage, skill, and death actions.
4. Add dust, summon, mount, and dismount effects/sounds.
5. Add capsule/camera/nameplate adjustments and restricted-map policies.
6. Expand multiplayer regression coverage for replication, death, travel, and equipment changes.

## Original source references

- Server: `game/src/char_horse.cpp`, `horse_rider.h/.cpp`, `char_item.cpp`, `questlua_horse.cpp`.
- Client: `UserInterface/InstanceBase.cpp`, `InstanceBaseMovement.cpp`, `InstanceBaseBattle.cpp`,
  `PythonPlayerSkill.cpp`, and `NetworkActorManager.cpp`.
- Motion: `GameLib/ActorInstance.cpp`, `ActorInstanceMotion.cpp`, `ActorInstanceAttach.cpp`,
  `RaceData.cpp`, and `RaceMotionData.h`.
