# Legacy Server and Client System Catalog

This is the source-oriented map of the systems implemented by the legacy Metin2 server and client. It is intended as a starting point for later ports: use the named files to follow a system into its packet handlers, data tables, quest bindings, and client presentation. It describes the checked-out legacy source, not every live server configuration or every feature ever shipped by every Metin2 version.

## Source scope

The main code areas are:

| Area | Source root | What it owns |
| --- | --- | --- |
| Shared wire/data definitions | `server_src/common/`, mirrored client definitions under `client_src/source/common/` | Packet headers, character/item/mob tables, enums, and shared record layouts. These definitions are the compatibility boundary between client, game process, and DB process. |
| Authoritative game process | `server_src/game/src/` | Connections after authentication, live characters/items, maps, combat, quests, social state, events, and game-to-game routing. |
| Database process | `server_src/db/src/` | SQL access, account/player/item/guild/marriage data, caches, ID ranges, and the DB/game peer protocol. |
| Client runtime | `client_src/source/` | Networking, local player and remote actor models, rendering, audio, input, and Python bindings. |
| Client UI and content | `client/pack/` | Packed `root`, `uiscript`, locale, map, model, effect, and audio archives. Many user-facing screens and quest UI scripts are inside these archives rather than standalone source files in `client_src/source/`. |
| Server data and quests | `server_src/share/` | Server configuration, locale tables, NPC/mob/item records, shops/refines, regen groups, and Lua quest scripts. |

The game server and DB server are separate processes. A game process owns its live map population and communicates with the DB process and peer game processes. The client progresses through login, character selection, loading, and in-game network phases; it presents server-authored state and sends player actions. The exact deployment topology, enabled systems, and table contents are configuration-dependent.

This guide groups systems by responsibility instead of repeating the same class in every call path. File references are relative to the source roots above. The graph reports are useful navigation aids, but the source files and shared packet/table definitions remain authoritative.

## Process, connection, and persistence

| System | Legacy behavior and ownership | Main source entry points |
| --- | --- | --- |
| Game-process lifecycle | Initializes managers and data, runs the network/event loop, periodically updates users and flushes saves, and tears down live entities. Runtime configuration controls ports, channel/core identity, map assignments, and peer connectivity. | `game/src/main.cpp`, `game/src/config.cpp`, `game/src/config.h` |
| Client connection and phases | A descriptor moves through handshake/authentication, login, select, and game phases. The server validates phase-specific packet headers and dispatches requests to phase handlers. | `game/src/desc.cpp`, `game/src/desc_manager.cpp`, `game/src/input.cpp`, `game/src/input_auth.cpp`, `game/src/input_login.cpp`, `game/src/input_main.cpp` |
| Login and character entry | Account login keys and player selection are validated before the character table is loaded; entering the world restores character state, attaches the session, creates visibility, and emits initial state. Logout/disconnect paths save and detach the character. | `game/src/login_data.cpp`, `game/src/input_login.cpp`, `game/src/char.cpp`; client: `UserInterface/PythonNetworkStreamPhaseLogin.cpp`, `PythonNetworkStreamPhaseSelect.cpp`, `PythonNetworkStreamPhaseLoading.cpp` |
| Database service | The DB process owns SQL connections and mediates account, player, item, guild, and other persistent records. The game process sends typed DB packets; DB replies hydrate caches or acknowledge saves. | `db/src/Main.cpp`, `DBManager.cpp`, `ClientManager.cpp`, `ClientManagerLogin.cpp`, `ClientManagerPlayer.cpp`, `ClientManagerGuild.cpp`, `common/packet_headers.h` |
| Save caches and item IDs | Player/item writes are buffered and cached to reduce SQL traffic. Item IDs are allocated from reserved ranges, and cache flush/expiry behavior is part of persistence correctness. | `db/src/Cache.cpp`, `db/src/ItemIDRangeManager.cpp`, `db/src/ClientManager.cpp`; game: `game/src/item_manager_idrange.cpp`, `char.cpp` |
| Game-to-game routing | Peer descriptors carry cross-process operations such as character transfer/warp, chat, guild/party updates, and online presence. A map transfer is coordinated between source and destination processes and updates the session’s game connection. | `game/src/p2p.cpp`, `p2p.h`, `desc_p2p.cpp`, `input_p2p.cpp`, `map_location.cpp`, `common/packet_headers.h` |
| Packet and trust boundary | Shared packed packet layouts define client↔game and game↔DB/P2P traffic. Client packets request actions; the server checks phase, actor, range, item state, and permissions before applying them. The client also performs local packet integrity/hack checks, which are not a substitute for server validation. | shared: `common/packet_headers.h`, `common/tables.h`; game: `game/src/packet.h`, `input_main.cpp`, `input_db.cpp`; client: `UserInterface/PythonNetworkStream.*`, `UserInterface/PythonNetworkStreamPhase*.cpp` |
| Account/security integrations | Login has optional regional billing, passpod/matrix-card, country blocking, teen-time, and anti-cheat integrations. Availability depends on compile-time options and server configuration; these are service integrations rather than core world simulation. | `game/src/passpod.*`, `matrix_card.*`, `block_country.*`, `input_teen.*`, `XTrapManager.*`, `db/src/BlockCountry.*`, `db/src/LoginData.*`, `teen/src/` |

## World, characters, and movement

| System | Legacy behavior and ownership | Main source entry points |
| --- | --- | --- |
| Character entity | `CHARACTER` is the central live entity for player characters and non-player actors. It combines entity visibility, finite-state behavior, horse riding, stats, inventory access, and player/mob-specific data. PC, monster, stone, door, building, warp, and other kinds share the entity base while following different state/update paths. | `game/src/char.h`, `char.cpp`, `entity.h`, `entity.cpp`, `FSM.h`, `char_manager.cpp` |
| Spatial map and visibility | Maps are split into sector-tree cells. Entering/leaving a cell updates the observer’s entity view; only nearby actors/items are sent to a client. Movement, spawn, destroy, and map transfer all affect visibility membership. | `game/src/sectree.h`, `sectree.cpp`, `sectree_manager.h`, `sectree_manager.cpp`, `entity_view.cpp`; client: `UserInterface/PythonCharacterManager.cpp`, `InstanceBase.*` |
| Character movement and synchronization | The client sends movement/state intent with destination, facing, function, and timing. The server validates/synchronizes positions, checks collision/map restrictions, and broadcasts movement packets; the client interpolates actor motion and selects the corresponding motion mode. | game: `char.cpp`, `char_state.cpp`, `entity.cpp`; client: `UserInterface/PythonPlayerInput*.cpp`, `InstanceBaseMovement.cpp`, `PythonNetworkStreamPhaseGameActor.cpp` |
| Mob definitions, AI, and combat | Mob prototypes provide race, stats, AI/race flags, attack ranges, skills, resistances, and rewards. Server FSM/AI chooses idle, pursuit, attack, flee, berserk, or special behavior. Damage and death are resolved on the server, including hate/attacker attribution and reward distribution. | `game/src/mob_manager.*`, `char_state.cpp`, `char_battle.cpp`, `battle.cpp`, `char.cpp`; data: `share/locale/*/mob_proto`, regen files |
| Spawn and regeneration | Text regen/group data describes NPCs, mobs, stones, groups, patrols, and timing. Spawn managers load the definitions, create entities, and schedule replacement after deaths or despawns. | `game/src/regen.*`, `group_text_parse_tree.*`, `mob_manager.*`; data under `share/data/` and `share/locale/` |
| Static NPCs, stones, doors, and buildings | NPC interaction, metin-stone lifecycle, doors, destructible buildings, and warp/goto objects share character/entity infrastructure but have specialized state and interaction rules. Stones can spawn guards and drop rewards; doors/buildings participate in map or guild-war logic. | `char_state.cpp`, `char_battle.cpp`, `building.*`, `castle.*`, `target.*`, `cmd_general.cpp`; quest bindings: `questlua_npc.cpp`, `questlua_building.cpp` |
| Map collision and terrain queries | The game server loads map attributes and spatial data for height, walkability, safe-zone flags, and object/block restrictions. The client independently loads visual terrain and environment assets. These are related data domains, not a single shared runtime object. | server: `sectree_manager.*`, `map_location.*`; client: `GameLib/AreaTerrain.*`, `PythonBackground.*`, `PRTerrainLib/` |
| Character progression and stats | Level/experience, base points, derived battle points, HP/SP/stamina, skill points, empire, alignment, play time, and character appearance/equipment are held in character state. Equipment, affects, skills, mounts, and special transformations contribute to calculated points. | `game/src/char.cpp`, `char_battle.cpp`, `char_resist.cpp`, `char_affect.cpp`, `char_skill.cpp`, `char_change_empire.cpp`; shared records: `common/tables.h` |
| Death, penalties, and respawn | Death is server-resolved and can invoke experience loss, item/yang penalties, PK/alignment effects, affect cleanup, party revival, and respawn choices. Safe-zone, war-map, and duel context alter portions of the rules. | `game/src/char.cpp`, `char_battle.cpp`, `char_affect.cpp`, `pvp.cpp`, `input_main.cpp`, `start_position.*`; client: `PythonNetworkStreamPhaseGame.cpp`, packed death/respawn UI |
| Mounts and horses | Horse state, summon/feed/ride progression, stamina, horse skills, and movement/attack mode are server-owned. Horse data and item-driven summon behavior integrate with the character and item systems. | `game/src/char_horse.cpp`, `horse_rider.*`, `horsename_manager.*`, quest bindings `questlua_horse.cpp`; client: `InstanceBaseTransform.cpp`, `PythonPlayerSkill.cpp`, packed horse UI |
| Pets | The server’s pet system manages summon items, associated pet actors, follow AI, and lifecycle. Quest bindings expose pet operations; client actor and UI behavior is driven by packets and packed scripts/assets. | `game/src/PetSystem.*`, `char.cpp`, `questlua_pet.cpp`; client: `InstanceBase.*`, packed `root` and `uiscript` archives |
| Polymorph and appearance | Polymorph changes the actor race/model and can alter combat stats based on the transformation item and skill. Character parts and weapon/body state are synchronized so clients resolve the correct race motion and visuals. | `game/src/polymorph.*`, `char.cpp`, `char_item.cpp`; client: `InstanceBaseTransform.cpp`, `InstanceBaseMotion.cpp`, `GameLib/` race data |

## Combat, skills, and status

| System | Legacy behavior and ownership | Main source entry points |
| --- | --- | --- |
| Basic combat and damage | The server validates attackability, map/PvP rules, range and attack timing, computes hit/damage/defense/resists, and applies knockback/death/reward effects. Weapon, class, level, affect, and target race all influence results. | `game/src/char_battle.cpp`, `battle.cpp`, `char.cpp`, `pvp.cpp`, `char_resist.cpp` |
| Class skills | Skill tables and per-character levels drive skill costs, cooldowns, target rules, splash/hit limits, effects, and master/grand-master behavior. Skill-specific effects may apply points/affects or invoke special attacks. | `game/src/skill.cpp`, `skill.h`, `skill_power.cpp`, `char_skill.cpp`, `char_battle.cpp`; data: skill tables; client: `PythonSkill.*`, `PythonPlayerSkill.cpp` |
| Affects and timed effects | Affects carry an apply type/value, duration, flags, and source. The server schedules expiry/tick behavior, updates points, persists the applicable subset, and sends add/update/remove packets. Potions, skills, item bonuses, mounts, and quests use this layer. | `game/src/affect.*`, `char_affect.cpp`, `affect_flag.h`; client: `PythonNetworkStreamPhaseGame.cpp`, `InstanceBaseEffect.cpp`, `PythonPlayer.cpp` |
| Party skills and shared rewards | Party membership is server-managed, with leader/role/state, invite/accept/remove, distribution modes, party buffs/skills, map membership, and party-aware experience/reward logic. | `game/src/party.*`, `char_battle.cpp`, `questlua_party.cpp`; client: `PythonNetworkStreamPhaseGame.cpp`, packed party windows |
| PvP, duels, and arenas | PvP rules consider empire, guild, party, alignment, map, and duel state. Arena and battle-arena managers run separate match/observer flows; three-way-war maps add event-specific faction rules. | `game/src/pvp.*`, `arena.*`, `BattleArena.*`, `threeway_war.*`, `war_map.*`; client: `PythonNetworkStreamPhaseGame.cpp`, packed target/duel/arena UI |

## Items, economy, and crafting

| System | Legacy behavior and ownership | Main source entry points |
| --- | --- | --- |
| Item definitions and instances | `item_proto` defines item family/subtype, anti/use flags, wear flags, size, values, limits, applies, sockets, refine target, and addon. Runtime `CItem` stores mutable count, owner, attributes, sockets, and lifecycle state. | `game/src/item.h`, `item.cpp`, `item_manager.*`, `item_attribute.*`, `item_addon.*`; shared: `common/tables.h`; data: locale `item_proto` and item lists |
| Inventory and equipment | Server inventory slots are the authority for item ownership and equipment. Move, split/stack, use, equip, unequip, drop, pickup, and item-on-item actions validate slot/type/ownership and update points/appearance. Client grid UI displays server state and submits slot operations. | `game/src/char_item.cpp`, `item.cpp`, `item_manager.cpp`, `common/tables.h`; client: `PythonItem.*`, `PythonNetworkStreamPhaseGameItem.cpp`, `PythonSlotWindow.cpp`, packed inventory UI |
| Item attributes and sockets | Random attributes, sockets, stone insertion/removal, item bonuses, blend values, and special progression item families are calculated by server item logic. Socket interpretation depends on item family; never infer that every socket is a Metin socket. | `game/src/item_attribute.cpp`, `item_addon.cpp`, `char_item.cpp`, `char_dragonsoul.cpp`, `DragonSoul.*`; client: `PythonItem.*`, packed item tooltip/inventory UI |
| Refinement and upgrades | Refine recipes define materials, fees, success chances and result items. Server consumes inputs and performs the roll; special over-+9 refine paths have separate logic. Upgrade NPCs and quest dialogue commonly call the same server item operations. | `game/src/refine.*`, `over9refine.*`, `char_item.cpp`, `questlua_item.cpp`; data: refine tables/locale; client: packed refine window scripts |
| Shops and commerce | NPC shops use server-loaded shop tables; buy/sell checks stock, prices, counts, inventory space, and limits. Extended shops support additional currency/layout behaviors. | `game/src/shop.*`, `shopEx.*`, `shop_manager.*`, `char_item.cpp`; client: `PythonShop.*`, packed shop UI |
| Player trade | A trade session locks two characters into an exchange state, mirrors offered items/money, and commits only after both accept and the server revalidates inventories and capacity. Cancellation and disconnect paths release state. | `game/src/exchange.*`, `char_item.cpp`; client: `PythonExchange.*`, `PythonNetworkStream.h`, packed exchange UI |
| Safebox and mall | Safebox is account-linked remote storage with separate money/item movement and save paths. Mall delivery is an account item-award queue that transfers items into inventory. | `game/src/safebox.*`, `char_item.cpp`; DB: `ClientManagerPlayer.cpp`, `ItemAwardManager.*`; client: `PythonSafeBox.*`, packed safebox/mall UI |
| Cube recipes and item manufacture | Cube combines input items using recipe tables and a success/result rule. Server validates the offered ingredient set and performs the exchange. Quest and NPC dialogue can initiate the flow. | `game/src/cube.*`, `char_item.cpp`, `questlua_item.cpp`; client: packed cube UI |
| Ground drops and pickup | Drops are world entities with ownership/protection timers. Server validates pickup range, owner/group eligibility, and inventory capacity; successful pickup transfers the item and dispatches quest pickup events. | `game/src/item.cpp`, `item_manager.cpp`, `char_item.cpp`, `entity_view.cpp`; client: `PythonNetworkStreamPhaseGameItem.cpp`, packed target/drop UI |
| Auction | Auction manager and DB peer logic hold listings, bids, sales/wishes, and account/item settlement. This is separate from NPC shops and direct player trade. | game: `game/src/auction_manager.*`, `auction_packet.h`; DB: `db/src/AuctionManager.*`, `common/auction_table.h` |
| Dragon Soul alchemy | Dragon Soul items use a separate grade/step/strength family, inventory pages, sockets/attributes, refining, set bonuses, and character-point integration rather than ordinary Metin-stone rules. | `game/src/DragonSoul.*`, `dragon_soul_table.*`, `char_dragonsoul.cpp`; client: `PythonItem.*`, packed Dragon Soul UI |
| Mining and fishing | Mining is a timed interaction with an ore vein and pickaxe proficiency/reward logic; fishing uses rod/bait, timed cast/catch, minigame result, and reward selection. Both have dedicated packets and quest hooks. | `game/src/mining.*`, `fishing.*`, `char.cpp`, `questlua_pc.cpp`; client: `PythonNetworkStreamPhaseGame.cpp`, `InstanceBaseMotion.cpp`, packed fishing UI |

See the focused legacy behavior notes for [items and inventory](ItemsAndInventory.md), [Metin stones](MetinStones.md), [item tooltips](ItemTooltips.md), [mining and accessory sockets](Mining.md), and the [fishing guide](../Fishing.md).

## Quests, dungeons, and timed world content

| System | Legacy behavior and ownership | Main source entry points |
| --- | --- | --- |
| Lua quest runtime | Quests are Lua scripts organized as quests/states/triggers. The quest manager loads/compiles scripts, associates per-player quest state/flags, dispatches events, and suspends/resumes dialogue for select/input/confirm. Quest calls execute through registered native Lua function tables. | `game/src/questmanager.*`, `questpc.*`, `questnpc.*`, `questevent.*`, `questlua.cpp`, all `questlua_*.cpp`; scripts in `share/locale/*/quest/` |
| Quest events and timers | Login/logout, NPC click, item use/pickup, kill, level, target, timer, and map/attribute events can activate state triggers. Player timers, named server timers, and loop timers are distinct; a quest may also coordinate party/map state. | `game/src/questmanager.cpp`, `questevent.*`, `event.*`, `questlua_global.cpp`, `questlua_pc.cpp`, `questlua_item.cpp`, `questlua_target.cpp` |
| Dungeon instances | `CDungeonManager` creates and tracks runtime dungeon objects associated with dungeon map indices. Quest code drives entry/exit, party membership, map transitions, regen groups, timers, stages, and completion. Dragon Lair has dedicated native support; Devil Tower and other scripted dungeons are primarily quest-driven. Legacy map instances are separate runtime map indices, which is distinct from UnrealLongju’s current same-map/shared-room direction. | `game/src/dungeon.*`, `DragonLair.*`, `questlua_dungeon.cpp`, `questlua_dragonlair.cpp`, `questlua_forked.cpp`; scripts/data in `share/locale/*/quest/` |
| World regeneration and scheduled events | The event queue schedules delayed callbacks for character actions, mob respawn, affects, items, timers, and event managers. Global events include OX quiz, arena/battle arena, three-way war, seasonal events, and speed-server progression. | `game/src/event.*`, `event_queue.*`, `regen.*`, `OXEvent.*`, `arena.*`, `BattleArena.*`, `threeway_war.*`, `SpeedServer.*`, `xmas_event.cpp` |
| Mining/fishing quest integration | `pc.mining`, `pc.fishing`, item/NPC callbacks, and quest event notifications expose native timed activities to Lua; scripts can present localized dialogue and rewards while the game process owns the actual activity state. | `questlua_pc.cpp`, `questlua_item.cpp`, `questmanager.cpp`, `mining.cpp`, `fishing.cpp` |

See [quest system](QuestSystem.md) and [quest porting status](QuestPortingStatus.md) for the current Unreal subset and importer limitations. The legacy catalog records available mechanisms; it does not imply every quest script or binding has been ported.

## Social, guild, territory, and chat systems

| System | Legacy behavior and ownership | Main source entry points |
| --- | --- | --- |
| Guild membership and progression | Guild records, ranks/permissions, invitations, membership, experience, skills, comments, marks, and member contributions are coordinated between game and DB managers. Guild skills and membership changes fan out to online members and peer processes. | game: `game/src/guild.*`, `guild_manager.*`; DB: `db/src/GuildManager.*`, `ClientManagerGuild.cpp`; client: `PythonGuild.*`, packed guild UI |
| Guild wars and buildings | Guild war declarations/matches use war state and dedicated war maps; territory/building systems manage guild structures, guards, doors, and castle/land operations. | `game/src/guild_war.cpp`, `war_map.*`, `building.*`, `castle.*`, `questlua_building.cpp`, `questlua_guild.cpp` |
| Party | Server invites and tracks leader, members, roles, distribution, map membership and party skills. It is used by quests/dungeons and reward distribution as well as UI grouping. | `game/src/party.*`, `questlua_party.cpp`; client: party handlers in `PythonNetworkStreamPhaseGame.cpp`, packed party UI |
| Messenger and friends | Friend lists, requests, online presence, and private messages have manager/database support; presence is propagated across game processes. Guild and messenger relationships are distinct. | `game/src/messenger_manager.*`, `input_main.cpp`, `input_p2p.cpp`; DB: `ClientManager.cpp`; client: `PythonMessenger.*`, packed messenger UI |
| Chat and commands | Normal chat, whispers, party/guild/empire/shout channels, command parsing, flood/insult checks, GM commands, and localized server messages are routed by the game process and client chat layer. | `game/src/input_main.cpp`, `cmd.cpp`, `cmd_general.cpp`, `cmd_gm.cpp`, `char.cpp`, `messenger_manager.cpp`; client: `PythonChat.*`, `PythonNetworkStreamPhaseGame.cpp` |
| Marriage and wedding | Marriage records and spouse state are stored through DB support; wedding map/session behavior, rings, love points, and quest functions build on character/map/quest systems. | game: `game/src/marriage.*`, `wedding.*`, `questlua_marriage.cpp`; DB: `db/src/Marriage.*`; client: packed wedding/relationship UI |
| Empire, monarch, and privileges | Empire identity affects map rules, chat, PvP, and faction interactions. Monarch state, empire privileges, temporary boosts, and administrative privilege tables are managed separately from ordinary character stats. | `game/src/char_change_empire.cpp`, `monarch.*`, `priv_manager.*`, `questlua_monarch.cpp`; DB: `db/src/Monarch.*`, `PrivManager.*` |
| Safe zones and map restrictions | Map attributes and server checks restrict PK, movement, or interactions in marked cells; war maps and special maps provide their own policy. | `game/src/char_battle.cpp`, `input_main.cpp`, `sectree_manager.*`, `war_map.*`; data: map attribute files |

See focused notes for [guild timers and socket quest storage](GuildAndTimers.md), [messenger](MessengerSystem.md), [safe zones](SafeZones.md), [duels](../Duels.md), and [admin/GM](AdminGMSystem.md).

## Client runtime and presentation

| Client subsystem | Responsibility | Main source entry points |
| --- | --- | --- |
| Application and Python host | Creates the game application/window/device, initializes Python and engine singletons, loads locale/config, and transitions between phase windows. Python UI scripts call native modules registered from C++. | `UserInterface/PythonApplication.*`, `PythonApplicationProcedure.cpp`, `PythonApplicationModule.cpp`, `EterPythonLib/PythonWindowManager.*`, `ScriptLib/` |
| Network stream and phase handlers | Performs handshake, login, character selection, loading, in-game packet dispatch, and offline phase handling. Exposes send helpers for movement, combat, item operations, quests, party, guild, trade, storage, and chat. | `UserInterface/PythonNetworkStream.*`, `PythonNetworkStreamPhase*.cpp`, `PythonNetworkStreamCommand.cpp` |
| Local player/controller model | Converts keyboard/mouse input to movement and action packets; manages attack, skill use, target selection, pickup, quickslots, and local action state. | `UserInterface/PythonPlayer.*`, `PythonPlayerInput*.cpp`, `PythonPlayerSkill.cpp`, `PythonPlayerEventHandler.*` |
| Remote actor and character instances | Maintains visible actors, interpolates network movement, resolves race/parts/equipment, selects locomotion/attack/hit/death motions, and attaches effects/sounds. | `UserInterface/InstanceBase.*`, `InstanceBaseMovement.cpp`, `InstanceBaseMotion.cpp`, `InstanceBaseBattle.cpp`, `InstanceBaseEffect.cpp`, `InstanceBaseTransform.cpp`, `PythonCharacterManager.*` |
| UI windows and widgets | Python scripts define most game screens. Native `EterPythonLib` controls provide windows, buttons, text, images, slot grids, drag/drop, and event callbacks. Packed scripts cover login/select, inventory, equipment, skills, quests, chat, party/guild, shops/trade/storage, target board, map/minimap, fishing and special events. | native: `source/EterPythonLib/PythonWindow.*`, `PythonSlotWindow.*`, `PythonGridSlotWindow.*`; gameplay bindings under `source/UserInterface/Python*.cpp`; content in `client/pack/root.epk`, `uiscript.epk`, locale archives |
| Item and character presentation | Resolves item names/icons and item slot behavior; character actor parts use race-specific body/hair/weapon mappings; tooltips combine proto values, applies, random attributes, sockets, affects, and localized strings. | `UserInterface/PythonItem.*`, `InstanceBaseTransform.cpp`, `GameLib/ItemManager.cpp`, packed `root`/locale/item archives |
| Outdoor/indoor maps | Loads terrain, texturesets, area/attribute data, static objects, buildings, water, trees, and environment visuals; manages map camera, picking and minimap coordinates. | `UserInterface/PythonBackground.*`, `PythonMiniMap.*`, `GameLib/AreaTerrain.*`, `Area.*`, `PRTerrainLib/`, `SpeedTreeLib/` |
| Rendering and skeletal animation | Direct3D-era render/device abstraction, Granny skeletal models and motion, mesh/material rendering, LOD, shadow/light systems, and actor attachments. This pipeline is separate from the authoritative game simulation. | `EterLib/`, `EterGrnLib/`, `EterImageLib/`, `GameLib/`, `SpeedTreeLib/`, `UserInterface/InstanceBase*` |
| Effects, projectiles, and audio | Client effect instances render particle/mesh effects; flying objects and hit/skill events create projectile visuals. Miles-backed sound handles music, ambient loops, positional effects and motion-linked sounds. | `EffectLib/`, `UserInterface/PythonEffectModule.cpp`, `PythonFlyModule.cpp`, `InstanceBaseEffect.cpp`, `PythonPlayerEventHandler.*`, `MilesLib/`, `UserInterface/PythonApplication*` |
| Input, camera, and environment | Application handles keyboard/mouse, camera movement and picking; background/map systems provide environment and ambient state. Local UI state can intercept inputs before gameplay actions. | `PythonApplicationEvent.cpp`, `PythonApplicationCamera.cpp`, `PythonApplicationCursor.cpp`, `PythonPlayerInput*.cpp`, `PythonBackground.*` |
| Locale and packaged resources | Locale files provide item/mob/skill names, UI strings and empire text conversion. EterPack mounts indexed/encrypted archives containing scripts, models, maps, textures, and sound. | `EterLocale/`, `EterPack/`, `EterBase/MappedFile.*`, `UserInterface/PythonApplication.cpp`; archive index: `client/pack/Index` |

The client is not authoritative for item grants, combat results, quest progress, or persistence. Its prediction/interpolation and UI state are presentation mechanisms around server packets.

## Data-to-runtime path

The usual trace for a feature is:

1. Find its shared record and enum in `server_src/common/tables.h` or `client_src/source/common/`.
2. Find the server loader/manager that turns locale tables or map files into runtime definitions (`item_manager`, `mob_manager`, `skill`, shop/refine loaders, or regen parsing).
3. Follow the game packet input handler into the authoritative action (`input_main.cpp`, `char_item.cpp`, `char_battle.cpp`, quest binding, or a dedicated manager).
4. Follow emitted `common/packet_headers.h` and `game/src/packet.h` data to the client phase handler and actor/UI model.
5. Check `db/src/` if the action persists, and check `share/locale/*/quest/` if NPC dialogue or event progression is script-driven.

For map behavior, also trace map attribute/regen inputs and the `SECTREE` visibility path. For effects and animation, trace the client race/motion/effect assets and packed scripts separately from the server event that requests them.

## Current UnrealLongju porting map

The Unreal implementation is a selective port, not a drop-in rehosting of these legacy classes. Use the current [project status](../ProjectStatus.md), [architecture](../Architecture.md), and the focused guides linked above for exact ownership and implementation constraints.

The current project documents substantial Unreal implementations for character/combat foundations, item inventory/equipment and commerce, mobs/loot/movement, skills/effects, quests, mining, fishing, guild core, messenger, map gameplay attributes, and a Devil Tower dungeon slice. These do not establish complete legacy parity.

The current project status explicitly lists remaining work around full dungeon/instance coverage, cube, safebox/mall, pets, marriage, guild wars/buildings, and complete horse backends. Quest translation and end-to-end quest fidelity remain incomplete. Auction, monarch, OX/arena/three-way-war, Dragon Soul, and other legacy subsystems must be audited individually before assigning a port status; their presence in this catalog is not evidence that an Unreal backend exists.

## Focused notes

These guides contain deeper rules already researched for specific systems:

- [Items, item_proto, inventory, and equipment](ItemsAndInventory.md)
- [Item tooltip composition](ItemTooltips.md)
- [Metin stones](MetinStones.md)
- [Affects](AffectSystem.md) and [potions](PotionsAndAffects.md)
- [Skills and skill books](SkillSystem.md), [SkillBooks.md](SkillBooks.md)
- [Quests](QuestSystem.md) and [quest porting status](QuestPortingStatus.md)
- [Mob movement and replication](NpcMovementReplication.md), [knockback](Knockback.md), and [damage distribution](DamageDistributionAndAoE.md)
- [Death and respawn](DeathAndRespawn.md), [mounts](MountSystem.md), [hair](HairStyles.md), [targeting](TargetAndSelectEffect.md), and [target board](TargetBoard.md)
- [Mining, alchemists, and accessory sockets](Mining.md), [fishing](../Fishing.md), [guild timers](GuildAndTimers.md), [messenger](MessengerSystem.md), and [safe zones](SafeZones.md)

When adding future notes, keep legacy rules distinct from current Unreal behavior, cite exact source files/data, and update `Docs/DocumentationStatus.md` only when adding or retiring a guide.
