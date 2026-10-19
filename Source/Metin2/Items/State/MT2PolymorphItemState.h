#pragma once
#include "Items/State/MT2ItemFamilyState.h"
#include "MT2PolymorphItemState.generated.h"

class AMT2Mob;

USTRUCT(BlueprintType)
struct METIN2_API FMT2PolymorphItemState : public FMT2ItemFamilyState
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Polymorph") TSubclassOf<AMT2Mob> TargetMob;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Polymorph", meta=(ClampMin="0")) int32 RemainingPractice = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Polymorph") int32 SuccessBonus = 0;
};
