/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "CoreMinimal.h"
#include "MT2ItemFlags.generated.h"

UENUM(BlueprintType, meta = (Bitflags))
enum class EMT2ItemFlag : uint8
{
	Refineable = 0,
	Save = 1,
	Stackable = 2,
	CountPerGold = 3,
	SlowQuery = 4,
	Rare = 5,
	Unique = 6,
	MakeCount = 7,
	Irremovable = 8,
	ConfirmWhenUse = 9,
	QuestUse = 10,
	QuestUseMultiple = 11,
	QuestGive = 12,
	Log = 13,
	Applicable = 14
};

UENUM(BlueprintType, meta = (Bitflags))
enum class EMT2ItemAntiFlag : uint8
{
	Female = 0,
	Male = 1,
	Warrior = 2,
	Assassin = 3,
	Sura = 4,
	Shaman = 5,
	Get = 6,
	Drop = 7,
	Sell = 8,
	Shinsoo = 9,
	Chunjo = 10,
	Jinno = 11,
	Save = 12,
	Give = 13,
	PlayerKillDrop = 14,
	Stack = 15,
	PrivateShop = 16,
	Safebox = 17
};

UENUM(BlueprintType, meta = (Bitflags))
enum class EMT2ItemImmuneFlag : uint8
{
	Paralysis = 0,
	Curse = 1,
	Stun = 2,
	Sleep = 3,
	Slow = 4,
	Poison = 5,
	Terror = 6
};

UENUM(BlueprintType, meta = (Bitflags))
enum class EMT2ItemWearFlag : uint8
{
	Body = 0,
	Head = 1,
	Foots = 2,
	Wrist = 3,
	Weapon = 4,
	Neck = 5,
	Ear = 6,
	Unique1 = 7,
	Unique2 = 8,
	Arrow = 9,
	Shield = 10,
	Ability1 = 11,
	Ability2 = 12,
	Ability3 = 13,
	Ability4 = 14,
	Ability5 = 15,
	Ability6 = 16,
	Ability7 = 17,
	Ability8 = 18,
	CostumeBody = 19,
	CostumeHair = 20,
	Ring1 = 21,
	Ring2 = 22,
	Belt = 23
};

