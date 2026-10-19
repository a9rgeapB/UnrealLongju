/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "CoreMinimal.h"
#include "MT2ItemActionResults.generated.h"

UENUM(BlueprintType)
enum class EMT2ItemBonusApplyResult : uint8
{
	Invalid,
	Failed,
	Success
};

UENUM(BlueprintType)
enum class EMT2MetinSocketItemResult : uint8
{
	Invalid,
	Incompatible,
	NoRemovableStones,
	NoBrokenSockets,
	MissingBrokenStoneTemplate,
	CannotReturnStone,
	Success
};

UENUM(BlueprintType)
enum class EMT2LootCrateOpenResult : uint8
{
	Invalid,
	LevelTooLow,
	KeyRequired,
	WrongKey,
	NoRewards,
	Success
};

UENUM(BlueprintType)
enum class EMT2LootCrateRewardKind : uint8
{
	Item,
	Yang,
	Experience
};

