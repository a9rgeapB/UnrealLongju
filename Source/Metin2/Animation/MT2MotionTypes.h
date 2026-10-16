#pragma once
#include "CoreMinimal.h"
#include "MT2MotionTypes.generated.h"

// Legacy NRaceData::EHitType: NONE=0, GREAT=1, GOOD=2.
UENUM(BlueprintType)
enum class EMT2MotionHittingType : uint8
{
	None=0, Knockdown=1 UMETA(DisplayName="Knockdown Hit"), Normal=2 UMETA(DisplayName="Normal Hit")
};
