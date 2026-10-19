/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "CoreMinimal.h"
#include "MT2FishDefinition.generated.h"

class UMT2ItemTemplate;

USTRUCT(BlueprintType)
struct METIN2_API FMT2FishUseOutcome
{
	GENERATED_BODY()
	FMT2FishUseOutcome() = default;
	FMT2FishUseOutcome(int32 InVnum, int32 InWeight) : ItemVnum(InVnum), Weight(InWeight) {}
	// Zero is an intentional empty outcome.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fish") TSoftClassPtr<UMT2ItemTemplate> ItemTemplate;
	UPROPERTY() int32 ItemVnum = 0;
	UPROPERTY() bool bTypedReference = false;
	int32 GetItemVnum() const;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fish", meta=(ClampMin="0")) int32 Weight = 0;
};

USTRUCT(BlueprintType)
struct METIN2_API FMT2FishDefinition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fish") TSoftClassPtr<UMT2ItemTemplate> DeadItemTemplate;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fish") TSoftClassPtr<UMT2ItemTemplate> GrilledItemTemplate;
	UPROPERTY() int32 DeadItemVnum = 0;
	UPROPERTY() int32 GrilledItemVnum = 0;
	UPROPERTY() bool bTypedReferences = false;
	int32 GetDeadItemVnum() const;
	int32 GetGrilledItemVnum() const;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fish") TArray<FMT2FishUseOutcome> UseOutcomes;
	bool IsValid() const;
	int32 ResolveUseRoll(int32 Roll) const;
};

