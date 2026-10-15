# Mining

## Playing

Equip a pickaxe (subject to its imported level/race requirements), then click an ore vein. The character approaches it and performs 5–15 digging swings, two seconds each. A completed sequence resolves one ore roll, not one reward per swing. Click again, move, change equipment, take damage, become stunned, mount or lose the vein to interrupt without a reward or proficiency.

Successful ore goes directly into the inventory. Any remainder that cannot fit drops with 15 seconds of ownership. Pickaxe proficiency progresses independently of ore success and is saved with the item; its imported practice odds, proficiency cap and final grade are respected.

## Legacy rules and implementation

The original server's `game/src/mining.cpp` defines 18 veins: 20047–20059 produce raw ore 50601–50613; 30301–30305 produce 50614–50618. Success is 20% plus the mining skill (124) and pickaxe grade contributions. Quantity is weighted across nine ten-unit ranges, from 1–10 (20%) through 81–90 (1%). Veins expire independently after 7–15 minutes and follow existing world regeneration rules.

`UMT2MiningComponent` owns authoritative timing, cancellation and completion. Replicated sequence identity, vein, server start time and swing count drive local `general/dig` animations, including observers who become relevant during a sequence. There are no per-swing RPCs. `pc.mining()` uses this same authority path; native vein clicks enter it directly so they do not require a successfully imported quest or rotate the vein.

The legacy server enables auto-pickup. Direct inventory insertion avoids spawning and immediately destroying a temporary ground actor while retaining its observable reward/ownership outcome. The legacy implementation's redundant second quantity draw is not reproduced.

## Import and validation

`MT2ImportItems -PickaxesOnly` refreshes only pickaxe definitions, including practice odds, grade parsed from the legacy name and the existing `item/etc` meshes. Mining automation is under `Metin2.Mining`.

`MT2ImportMobs -OreVeinsOnly` imports the 18 ore-vein NPCs from their existing static meshes and race scripts, then rebuilds the VNUM registry. Veins use `AMT2OreVein`, a stationary NPC with static visuals and mesh-fitted interaction collision. They do not require skeletons or animation Blueprints. The targeted import leaves unrelated mobs, shops and loot tables unchanged. Spawn a vein with `/m 20047` (or another vein VNUM from the ranges above).

The mining tests cover sequence authority/cancellation, proficiency persistence and cap, partial-inventory delivery with matching pickup-quest events, local late-sequence animation reconstruction and all eight imported digging animation bindings. Packaged multiplayer visuals, real latency and concurrent-miner stress require a play test.

## Alchemist refining

Offer a stack of at least 100 raw ore to an alchemist using the inventory-to-NPC item interaction. Confirm the displayed fee and success rate. For non-diamond ore, choose a Spirit Stone (+0, +1 or +2) from the dialogue; the slot number identifies the exact inventory item. The picker shows eight stones per page. Changing pages or cancelling consumes nothing.

Each attempt consumes 100 raw ore and its Yang fee, including unsuccessful attempts. Diamond costs 10,000 Yang and needs no Spirit Stone. Other ores cost 3,000 Yang and consume one selected Spirit Stone. Success produces one refined ore (raw VNUM + 20). Full inventories must accommodate the result after consumption or the attempt is rejected without charge.

Alchemists 20060–20072 specialize in raw ore 50601–50613; alchemists 33009–33013 specialize in 50614–50618. Success is 60% for their specialization and 30% for another ore. Spawn an amber alchemist with `/m 20061` and test with raw amber 50602, Spirit Stone 28030 and at least 3,000 Yang.

Alchemists are independent: all players pay the same base fees, regardless of guild or empire. The imported `guild_building_melt` quest uses native refining nodes and the original localized dialogue. `MT2ImportQuests -AlchemistsOnly` refreshes this quest without rewriting other quests, shared tables or the active-quest manifest. Rebuild and package the changed quest asset for staged builds.

The inventory component owns the atomic exchange. Server-side dialogue revalidates the NPC, player, world, range, health, trading state and item snapshots before charging. Replies carry the identity of their dialogue page, so a repeated confirmation cannot select a material or answer a later conversation. Client and server must use matching rebuilt binaries. No per-frame work or new save format is introduced.

## Accessory sockets

Carry a refined diamond (50621) onto a bracelet, necklace, earring or belt and confirm to attempt opening one permanent socket. A valid attempt consumes one diamond with 50% success; accessories support at most three open sockets. Carry a compatible refined mineral onto an open jewelry socket to fill it. Both inventory and equipped targets are supported. The confirmation shows the selected accessory's exact additional stats, chance and duration; cancelling, incompatible items and full sockets consume nothing.

Insertion chances are 90%, 80% and 70% for grades one, two and three. Failure consumes one mineral but preserves existing filled sockets. Material matching follows the legacy server's numeric jewelry families, including extended jewelry and the necklace exceptions. Ordinary minerals cannot fill belts; belts require their separate subtype-29 filler. Rings do not support accessory sockets. Refined amber 50622 is a legacy special/map item, not a compatible socket material.

Filled grades increase each intrinsic template apply (except NONE and SKILL) by a total 10% / 20% / 40%, with minimum increases of +1 / +2 / +3. Random/rare attributes, Metin stones and raw armor defense are not amplified. These totals do not stack as 10+20+40: the third insertion adds only the difference between grades two and three. Mineral tooltips use short instructions rather than formulas or VNUM tables, while retaining named next-grade gains for compatible worn jewelry. Exact chances and duration remain in the target confirmation.

Accessory and Metin tooltips share the legacy socket-row renderer and imported silver/gold frame atlas. Jewelry shows one silver frame per opened socket: empty frames say "Empty socket", and filled frames contain the compatible mineral icon, its name and that socket's incremental bonuses. Remaining wear appears under the active mineral. These rows do not amplify or repeat the base-stat lines. The shared visual presentation does not change accessory grade/time storage or turn accessory minerals into ordinary Metin stones.

Grades three, two and one wear down after 6, 12 and 24 equipped hours respectively. Expiry removes one filled grade and starts the lower grade's full duration; open sockets remain. Wear pauses in inventory, ground items, trades and offline. One inventory-owned expiry timer handles all worn accessories, without Tick or per-second countdown replication. Hover refreshes the displayed countdown; it is not a continuously updated label.

Accessory saves reuse socket_0/current grade, socket_1/open sockets and socket_2/remaining seconds. Other item families retain their existing formats; no database migration is required. Fractional wear survives moves within a running game; the legacy whole-second save format rounds remaining wear down, losing less than one second at a save/reload boundary rather than granting extra time. Client/server binaries must be rebuilt together for the added confirmation RPC. Existing imported material metadata is sufficient; no item reimport is required.

Quick test: `/i 14000`, `/i 50621 20`, `/i 50623 20`. Bracelet 14000 uses mineral 50623. Open sockets with diamonds, then fill them; its +1 Attack Speed and +10 Maximum Stamina base applies gain +1/+1 at grade one and +3/+4 total at grade three. Socket automation is under `Metin2.Items.AccessorySockets`.

Automation covers material compatibility, roll boundaries and consumption, stale/repeated confirmations, actual equipped bonuses, wear/degradation, save/restore, paused drop/trade/death-drop state, and shared inventory/trade tooltip contents. Packaged multiplayer serialization, visual confirmation layout and real-network interactions still require a play test.

NPC pickaxe upgrades remain a separate implementation task. TODO: connect independent alchemists to empire ownership/fees and guild buildings.

TODO: Integrate alchemists with guild buildings and empire ownership once those systems support them, including owner-guild discounts, cross-empire fees, fee routing and building reconstruction/specialization changes. See `Docs/superpowers/specs/2026-10-15-independent-alchemists-design.md`.
