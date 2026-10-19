#pragma once

#include "Items/Templates/MT2ItemMetinStoneTemplate.h"
#include "MT2ItemMetinGoldTemplate.generated.h"

/**
 * Legacy item-proto variant METIN_GOLD (ordinal 1).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemMetinStoneTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemMetinGoldTemplate : public UMT2ItemMetinStoneTemplate
{
	GENERATED_BODY()
};

