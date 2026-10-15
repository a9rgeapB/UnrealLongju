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

Ore refining, NPC pickaxe upgrades, accessory socket insertion and accessory bonus/degradation rules are separate systems, not part of this mining slice.
