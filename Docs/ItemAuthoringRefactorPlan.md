# Item authoring and template organization

## Requirements

- User-facing references use item/mob classes or skill/data-asset object references, not numeric IDs.
- Genuine quantities retain numeric types with meaningful names, units and bounds.
- A template or instance state owns only data used by its family. No generic editable socket array.
- Each item template class has its own matching header and implementation file.
- Preserve item behavior, authored asset overrides, saves, quest socket semantics and server authority.

## Architecture

Keep stable reflected class names while moving their declarations to dedicated files under
`Source/Metin2/Items/Templates` (existing action templates stay under `Items/Use`). Headers include
their actual parent, never an umbrella containing every family. Shared visual/value types get
focused type headers. Runtime instance data holds common fields and one typed family-state payload,
not a field for every possible family. Use UE's reflected instanced-struct support, including its
native network serializer, with named state structs and typed Blueprint accessors.

Item class references are hard references where already loaded and soft class references for
potentially cyclic refinement chains. Skill references use `UMT2SkillDefinition`. Stable legacy
identities and unresolved external IDs are confined to native boundary/migration code; they are not
designer-facing reference selectors. Preserve unknown records instead of silently replacing IDs
with null references.

## Execution

- [x] Add failing reflection/layout tests for generic socket exposure, typed reference fields and
  template ownership; retain all existing behavior tests as regression coverage.
- [x] Split all combined template declarations/implementations and update direct includes. Preserve
  `/Script/Metin2` class identities and Blueprint parents. Split shared visual/family structs.
- [ ] Add typed instance states for equipment/accessories, rods, picks, fish, skill books, blends,
  polymorph and automatic recovery; isolate genuinely arbitrary quest socket storage in native
  adapters. Update inventory, transfer/stacking, world items, tooltips, quests and persistence.
- [ ] Replace item/mob/skill reference IDs in templates, instance states, loot/reward structs and
  associated settings. Keep source proto IDs only in internal import records/legacy codecs.
- [ ] Migrate item assets without discarding designer-authored overrides. Resolve cross-item links
  after definitions exist; reject unresolved required links explicitly. Preserve old saves and
  validate Blueprint use sites rather than silently dropping pins or properties.
- [ ] Validate reflected authoring data and all migrated items; build Editor and dedicated server;
  run the complete Metin2 automation suite; inspect diffs and refresh the local source graph.

## Validation focus

Saved sockets must retain their family-specific meanings; missing templates must not discard data.
Soft refinement cycles must not force circular Blueprint loads. Inventory snapshots must preserve
typed state across owner replication, stacking, drops, trade and persistence. Reparenting a native
family must not lose inherited authored values. Generic item details must not show tool/skill/buff
fields. Existing item Blueprint identities must continue to load after header moves.

## Progress

Initial audit: `MT2ItemTemplate.h` contains the family hierarchy; `FMT2ItemInstanceData` exposes
legacy sockets, skill IDs, equipment and auto-recovery fields for every item. Fish rewards,
refinement links and rod failure results are also exposed numeric references. Existing mount and
loot-crate templates already demonstrate typed references. UE 5.7 has reflected instanced structs
and a native network serialization hook.

File split: 42 combined template classes were separated; all native template classes now have their
own matching `.h` and `.cpp`. The two skill-training classes were also separated. Shared types were
split into eleven focused headers; `MT2ItemTypes.h` is an include facade, not a class/type monolith.
Editor compilation passed before the family-state field changes. Matching-header-first and explicit
visual-type includes were fixed. The new authoring tests reproduced both original violations.

Preservation snapshot: `Saved/ItemAuthoringMigration/Manifest.json` contains 5,743 item CDO records,
with original packages copied under `Saved/ItemAuthoringMigration/Assets`. Do not overwrite it or
rerun the mechanical split scripts. The snapshot commandlet currently exports only; its apply and
validation phases still need implementation. No item asset has been resaved by this refactor yet.

Current work: separate state structs exist in `Items/State`; common instance data now has one
`FInstancedStruct FamilyState`, common quantities and native typed accessors. Call sites still need
migration before the next build. `FMT2ItemSlot` now has a typed Template reference; its numeric Vnum
is native/serialization-only, no longer Blueprint-visible. Populate Template on inventory restore,
item generation and world-item construction. Bind the actual subclass, preserving vnum-range aliases.

Compatibility implementation required: a private, non-Blueprint legacy record in `Items/Legacy`
must retain old field names for binary/text/JSON decoding. Add custom-version serializers to both
`FMT2ItemInstanceData` and `FMT2ItemSlot` (slot serialization does not automatically invoke its base
struct serializer). Old persistent archives deserialize into the legacy record; modern ones serialize
the actual family payload. Apply class-aware normalization into named state when the template is
available; retain opaque records when it is unavailable. Legacy quest/socket access stays in the
codec. UE Engine binds `FInstancedStruct::NetSerializeScriptStructDelegate` in `UnrealEngine.cpp`,
so the existing Engine dependency supports owner replication without another runtime module.

Next mechanical steps: replace item `.Bonuses`, `.MetinSockets`, `.AccessorySockets` with explicit
read/edit accessors (skip state/legacy definitions and the accessor implementation), then migrate
rod/pick/fish uses off Metin scratch fields. Move auto-recovery, skill-book and blend logic to their
typed states. Do not create family state just to read a field on an unrelated item. Preserve equipment
bonuses when upgrading its state to Metin-equipment or accessory state. Editor-authored family defaults
must live on the corresponding templates, not in the common editable default-instance struct.
