/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "CoreMinimal.h"
#include "MT2ItemRefinementTypes.generated.h"

class UMT2ItemMetinStoneTemplate;
class UMT2ItemTemplate;

UENUM(BlueprintType)
enum class EMT2RefinementResult : uint8
{
	Invalid,
	Success,
	FailedDestroyed,
	FailedDowngraded,
	FailedAtMinimum,
	NotEnoughYang,
	MissingMaterials,
	NoInventorySpace
};

USTRUCT(BlueprintType)
struct METIN2_API FMT2RefinementMaterial
{
	GENERATED_BODY()
	FMT2RefinementMaterial() = default;
	FMT2RefinementMaterial(int32 InVnum, int32 InCount) : ItemVnum(InVnum), Count(InCount) {}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinement")
	TSoftClassPtr<UMT2ItemTemplate> ItemTemplate;
	UPROPERTY()
	int32 ItemVnum = 0;
	UPROPERTY() bool bTypedReference = false;
	int32 GetItemVnum() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinement", meta = (ClampMin = "1"))
	int32 Count = 1;
};

USTRUCT(BlueprintType)
struct METIN2_API FMT2RefinementRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinement", meta = (ClampMin = "0"))
	int64 YangCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinement",
		meta = (ClampMin = "0", ClampMax = "100", Units = "%"))
	int32 SuccessPercent = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refinement")
	TArray<FMT2RefinementMaterial> Materials;

	bool IsValid() const { return SuccessPercent >= 0; }
};

