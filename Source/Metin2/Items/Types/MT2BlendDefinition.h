/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "CoreMinimal.h"
#include "Items/Types/MT2ItemBonusTypes.h"
#include "MT2BlendDefinition.generated.h"

USTRUCT(BlueprintType)
struct METIN2_API FMT2BlendDefinition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blend")
	EMT2ItemBonusType BonusType = EMT2ItemBonusType::None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blend")
	TArray<int32> Magnitudes;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blend")
	TArray<int32> Durations;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blend")
	TArray<int32> Weights = {10, 20, 40, 20, 10};
	bool IsValid() const;
	int32 GetRollIndex(int32 Roll) const;
	int32 GetTotalWeight() const;
};

