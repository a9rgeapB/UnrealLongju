#pragma once
#include "Items/State/MT2ItemFamilyState.h"
#include "MT2FishItemState.generated.h"

USTRUCT(BlueprintType)
struct METIN2_API FMT2FishItemState : public FMT2ItemFamilyState
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish", meta=(ClampMin="0", DisplayName="Length (tenths of cm)")) int32 LengthTenths = 0;
};
