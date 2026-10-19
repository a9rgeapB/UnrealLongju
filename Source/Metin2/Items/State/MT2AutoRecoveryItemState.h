#pragma once
#include "Items/State/MT2ItemFamilyState.h"
#include "MT2AutoRecoveryItemState.generated.h"

USTRUCT(BlueprintType)
struct METIN2_API FMT2AutoRecoveryItemState : public FMT2ItemFamilyState
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Automatic Recovery", meta=(ClampMin="0")) int32 RemainingAmount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Automatic Recovery", meta=(ClampMin="0")) int32 MaximumAmount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Automatic Recovery") bool bActive = false;
};
