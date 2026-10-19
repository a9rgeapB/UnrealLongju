#pragma once

#include "Items/Templates/MT2ItemLotteryTemplate.h"
#include "MT2ItemLotteryInstantTemplate.generated.h"

/**
 * Legacy item-proto variant LOTTERY_INSTANT (ordinal 1).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemLotteryTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemLotteryInstantTemplate : public UMT2ItemLotteryTemplate
{
	GENERATED_BODY()
};

