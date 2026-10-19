#pragma once
#include "Items/State/MT2ItemFamilyState.h"
#include "MT2FishingRodItemState.generated.h"

USTRUCT(BlueprintType)
struct METIN2_API FMT2FishingRodItemState : public FMT2ItemFamilyState
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ClampMin="0")) int32 Proficiency = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ClampMin="0")) int32 BaitPower = 0;
};
