/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "CoreMinimal.h"
#include "MT2ItemLimitTypes.generated.h"

UENUM(BlueprintType)
enum class EMT2ItemLimitType : uint8
{
	None = 0,
	Level = 1,
	Strength = 2,
	Dexterity = 3,
	Intelligence = 4,
	Constitution = 5,
	PCBang = 6,
	RealTime = 7,
	RealTimeFromFirstUse = 8,
	TimerWhileEquipped = 9
};

USTRUCT(BlueprintType)
struct METIN2_API FMT2ItemLimit
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Limit")
	EMT2ItemLimitType Type = EMT2ItemLimitType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Limit")
	int32 Value = 0;
};

