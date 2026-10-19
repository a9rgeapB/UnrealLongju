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
native network serializer, with named Blueprint-visible state structs and typed native accessors.

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
- [x] Add typed instance states for equipment/accessories, rods, picks, fish, skill books, blends
  and automatic recovery; isolate genuinely arbitrary quest socket storage in native
  adapters. Update inventory, transfer/stacking, world items, tooltips, quests and persistence.
- [x] Replace reference selectors in the item templates, refinement materials, fish rewards and
  supported family state. Keep source proto IDs in native import/legacy compatibility fields.
- [x] Migrate item assets without discarding designer-authored overrides. Resolve cross-item links
  after definitions exist; reject unresolved required links explicitly. Preserve old saves and
  validate Blueprint use sites rather than silently dropping pins or properties.
- [x] Validate reflected authoring data and all migrated items; build Editor and dedicated server;
  run the complete Metin2 automation suite; inspect diffs and refresh the local source graph.

## Validation focus

Saved sockets must retain their family-specific meanings; missing templates must not discard data.
Soft refinement cycles must not force circular Blueprint loads. Inventory snapshots must preserve
typed state across owner replication, stacking, drops, trade and persistence. Reparenting a native
family must not lose inherited authored values. Generic item details must not show tool/skill/buff
fields. Existing item Blueprint identities must continue to load after header moves.

## Current implementation

Every native item template owns matching `.h` and `.cpp` files. Shared types have focused headers;
`MT2ItemTypes.h` is an include facade. Common instance data owns one `FInstancedStruct` payload,
count, expiry and presentation state. Named family defaults live on their corresponding templates.
Reading absent state does not create an unrelated payload. Unsupported quest bonus edits reject
safely rather than reinterpreting another family's state.

Refinement links, materials, rod failure results, fish rewards, skill books, horse summon skills
and Metin conflict families use typed class/object references. Potentially cyclic item links are
soft references. Importing refreshes source-owned links and resolves partial-import targets against
the existing item registry. Legacy identities remain native for persistence and imported ranges.

Binary and default-text compatibility is implemented separately under `Items/Legacy`. Both common
instances and derived slots have custom-version serializers. Normalization retains opaque records
when definitions are unavailable. Lua socket access maps supported family fields explicitly, and
real-time quest expiry reads the same compatibility codec. JSON retains the legacy socket columns.
UE Engine binds `FInstancedStruct::NetSerializeScriptStructDelegate`, so owner replication uses
the existing Engine dependency.

All 5,743 item assets were compiled and migrated, with instance defaults compared against the
preservation snapshot before saving. Reference resolution reported zero missing links. Original
packages remain under `Saved/ItemAuthoringMigration/Assets`; the manifest must not be overwritten.
The mechanical split scripts are one-shot tools and must not be rerun.

Editor and dedicated-server Development builds passed. The complete Metin2 suite passed all
160 tests, including schema/layout checks, old/current archive round trips, default-text import,
Lua rod sockets, real-time quest expiry and partial reference reimport. A final Editor/server rebuild
passed; the Blueprint compilation check processed 8,758 assets with zero errors, warnings or failed
loads. The local source graph was refreshed and the moved symbols inspected; its AST parser still
reports partial extraction for some macro-heavy and third-party headers. Actual source and compiler
results remain authoritative. Packaged multiplayer behavior still needs a live gameplay smoke test;
an offline automation run is not evidence of that check.

## Remaining behavior work

Polymorph transformation and other unimplemented item systems remain tracked in `ItemBehavior.md`.
Their legacy quest scratch values are not exposed as generic authoring fields. This item-template
pass is not a claim that every unrelated reflected ID elsewhere in the project has been audited.
