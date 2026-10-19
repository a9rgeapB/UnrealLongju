#pragma once
#include "Items/State/MT2ItemFamilyState.h"
#include "MT2PickaxeItemState.generated.h"

USTRUCT(BlueprintType)
struct METIN2_API FMT2PickaxeItemState : public FMT2ItemFamilyState
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mining", meta=(ClampMin="0")) int32 Proficiency = 0;
};
