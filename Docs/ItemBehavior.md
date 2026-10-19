# Item behavior and legacy parity

## Runtime model

Imported item Blueprints inherit from the native family classes in `MT2ItemTemplate.h`. A class
mapping alone does not establish working gameplay: right-click use, drag-to-apply, equipping,
per-instance generation, persistence and tooltips must all implement the corresponding behavior.
The server validates and performs inventory transactions. Clients request actions and display results.

Legacy `ITEM_TYPE_*` and `USE_*` declarations include unused entries. `AUTOUSE`, `MATERIAL`,
`SPECIAL`, `TOOL` and `LOTTERY` have no generic right-click operation in `CHARACTER::UseItemEx`;
their real behavior belongs to specific content, quests or another system. Assigning an invented
generic consume action would break parity.

## Coverage and dependencies

| Family or subtype group | Current behavior / work required |
| --- | --- |
| Weapons, armor, unique equipment, costumes, rings, belts, totems | Equipment/stat/visual foundation exists. Audit family-specific wear restrictions, expiry and belt inventory rules. |
| Metin stones, accessory materials | Native attachment, refinement and accessory wear systems exist. Audit detachment/cleaning and remaining drag-to-apply subtypes. |
| Recovery potions, speed potions, timed affects, auto recovery | Ability-up supports the nine legacy stat applies with refresh semantics. Continuous HP/SP recovery uses fixed per-second amounts for the full duration, including time spent at full resources. Cleansing removes poison, fire, stun, slow and configured skill debuffs. |
| Quest items and quest-specific special items | Quest item-use/give/socket dispatch exists. Check individual imported scripts rather than implementing a second competing handler. |
| Skill books, horse books, horse care, mount items, hair looks, mob lure | Dedicated runtime handlers exist. Verify item-specific conditions against legacy content. |
| Fishing rods, fish, pickaxes, raw/refined ores | Fishing/mining/refining/accessory foundations exist. Live fish have table-driven opening outcomes; live/dead fish can be given to a campfire to roast the entire stack. |
| Gift boxes, treasure boxes/keys | Native reward/crate/key transactions exist. Audit special non-item crate rewards and content groups. |
| Blend items | `blend.txt` supplies independent weighted magnitude/duration rolls. Instance sockets persist and appear in tooltips. Duplicate APPLY affects are rejected. Energy amplifies random equipment attributes, with separate eligible slots for energy and costume-attribute effects. |
| Polymorph balls and practice books | Need a transformation system: authoritative race/state, mob motion and attack data, equipment/skill/riding restrictions, duration, restoration, and observer/late-join presentation. Balls and books have different consumption, level and success rules. |
| Skill-forget items | Need legacy skill-level/point refund rules and item-selected skill state, integrated with the existing skill component. |
| Talisman/recall items | Need persistent destination sockets, delayed/cancellable travel and existing map-server transfer validation. |
| Binding/unbinding items | Need persistent binding state enforced by drop, trade, sell, refinement and equipment operations, including delayed unbinding. |
| Dragon Soul and special Dragon Soul | Need qualified character state, dedicated inventory and two equipment decks, activation/wear timers, generated attributes, table-driven refinement and UI. |
| Extract / time-charge items and Dragon Soul refining materials | Depend on the Dragon Soul system; implement after its inventory/state/transaction layer. |
| Recipe / create-stone / remaining special-use items | Need explicit template parameters and complete transactions; retain legacy no-op entries where the original server has no behavior. |
| Yang, secondary coins | Currency acquisition and shop/payment behavior; no arbitrary right-click conversion. |
| None, container, material, resource, special, tool, lottery, auto-use | Preserve family identity. Resolve actual per-item behavior through the content's supported system/quest; unused enum names are not missing consumable actions. |

## Implementation order

1. Complete small skill and targeted-item actions, including skill-forget and recipe transactions.
2. Implement transformation, item binding and recall systems, each with its dependent item actions.
3. Implement Dragon Soul inventory/decks, attributes/timers, refinement, extraction and charging.
4. Finish the equipment expiry/belt inventory audit and per-item quest/content checks. Keep
   unavailable/unused content explicit.

## Instance sockets and equipment amplification

Generic numeric instance data uses `LegacySockets` (at most three values). It does not encode
quest progress or blend parameters as installed Metin stones. Save JSON retains `socket_0` through
`socket_2`; equipment, accessories, rods/pickaxes, fish, skill books and auto recovery retain their
family-specific representations. Stack identity includes the numeric values; transferring or
restoring an item must not reroll an existing blend.
Restoring a valid blend with all three sockets zero initializes its first roll; restoring nonzero
socket data never rerolls it.
If a template or registry is unavailable, normalization preserves the existing socket representation
instead of inferring that the item is numeric and discarding its equipment data.

Energy sums normal/rare runtime item attributes by APPLY across the eligible worn slots, then
truncates `sum * percent / 100` once per APPLY. It does not amplify prototype applies, weapon base
damage or socket stones. Equipment changes and affect changes rebuild the bonuses. Eligible slots
are editable under Gameplay → Items → Attribute Amplification.

## Fish and campfires

The item importer reads `fishing.txt`, preserving its ASCII column separators despite CP949 names,
and ignores its spreadsheet totals footer. Localized display names still come from the item proto.
Opening a live fish rolls over 10,000 outcomes: 6,000 dead-fish outcomes, 2,000 fishbone outcomes,
and the remaining 2,000 distributed among the ten source use results. Empty/water-stone/treasure-map
results intentionally award nothing, as in the reference server. Dead fish cannot be opened.

Campfire item 27600 spawns the stationary NPC 12000 in front of the player on valid, dry ground.
The authoritative actor expires after 40 seconds by default. Placement distance, lifetime, trace
extents, collision and imported log mesh/fire effect are editable in Project Settings → Metin2
Fishing → Campfire. Visual assets load only on rendering instances, not on dedicated servers.
Giving a supported live/dead fish stack to a nearby campfire exchanges the whole stack for its
table-defined roasted item. Cooked fish use their normal consumable subtype.

Conversions plan output placement before committing inventory changes. Missing definitions,
insufficient space, death, remote interaction and trading reject the action without taking fish.
This deliberately avoids losing the input or partially delivering rewards when inventory is full.

`MT2ConfigureItemBehaviors` refreshes only blend, fish, campfire and affected consumable Blueprints;
`-Preview` validates/displays the selection without saving assets. It uses configured legacy source
paths; packaged runtime never reads those external tables.

## Sources and verification

Primary server sources: `game/src/char_item.cpp`, `item.cpp`, `char_affect.cpp`, `blend_item.cpp`,
`polymorph.cpp`, `dragon_soul.cpp`, `char_dragonsoul.cpp`, `fishing.cpp` and `common/enums.h`.
Tables: `share/locale/italy/blend.txt`, `dragon_soul_table.txt` and quest scripts.
Primary client sources: `source/UserInterface/InstanceBase*`, `PythonPlayer*`, item protocol handling
and `uitooltip.py`/inventory/Dragon Soul windows in the client scripts.

Reference text proto inspected for the audit contains 5,743 rows. It is research input, not the
runtime registry: imported/cooked item Blueprints and their actual generated data determine what can
be played. The audit must not equate the number of C++ family classes with functional item coverage.
