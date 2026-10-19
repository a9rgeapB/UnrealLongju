#pragma once

#include "Items/State/MT2ItemFamilyState.h"
#include "Items/Types/MT2ItemBonusTypes.h"
#include "Items/Types/MT2MetinSocketTypes.h"
#include "Items/Types/MT2AccessorySocketState.h"
#include "MT2LegacyItemRecord.generated.h"

// Read boundary for packages written before typed family state. Never exposed to designers.
USTRUCT()
struct METIN2_API FMT2LegacyItemRecord : public FMT2ItemFamilyState
{
	GENERATED_BODY()
	UPROPERTY() TArray<int32> LegacySockets;
	UPROPERTY() int64 RealtimeExpiresAt = 0;
	UPROPERTY() int32 Count = 1;
	UPROPERTY() TArray<FMT2ItemBonus> Bonuses;
	UPROPERTY() TArray<FMT2MetinSocket> MetinSockets;
	UPROPERTY() FMT2AccessorySocketState AccessorySockets;
	UPROPERTY() int32 SkillVnum = 0;
	UPROPERTY() int32 AutoRecoveryRemainingAmount = 0;
	UPROPERTY() int32 AutoRecoveryMaximumAmount = 0;
	UPROPERTY() bool bAutoRecoveryActive = false;
	UPROPERTY() bool bNewlyAcquired = false;
};

USTRUCT()
struct METIN2_API FMT2LegacyItemSlotRecord : public FMT2LegacyItemRecord
{
	GENERATED_BODY()
	UPROPERTY() int32 Vnum = 0;
};
