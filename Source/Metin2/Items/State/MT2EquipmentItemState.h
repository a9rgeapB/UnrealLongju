#pragma once
#include "Items/State/MT2ItemFamilyState.h"
#include "Items/Types/MT2ItemBonusTypes.h"
#include "MT2EquipmentItemState.generated.h"

USTRUCT(BlueprintType)
struct METIN2_API FMT2EquipmentItemState : public FMT2ItemFamilyState
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment") TArray<FMT2ItemBonus> Bonuses;
};
