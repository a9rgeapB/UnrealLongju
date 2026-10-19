#pragma once
#include "Items/State/MT2ItemFamilyState.h"
#include "Items/Types/MT2ItemBonusTypes.h"
#include "MT2BlendItemState.generated.h"

USTRUCT(BlueprintType)
struct METIN2_API FMT2BlendItemState : public FMT2ItemFamilyState
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blend") EMT2ItemBonusType BonusType = EMT2ItemBonusType::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blend", meta=(ClampMin="0")) int32 Magnitude = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blend", meta=(ClampMin="0", Units="s")) int32 DurationSeconds = 0;
	UPROPERTY() int32 UnresolvedLegacyBonus = 0;
};
