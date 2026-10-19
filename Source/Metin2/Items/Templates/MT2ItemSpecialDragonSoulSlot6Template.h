#pragma once

#include "Items/Templates/MT2ItemSpecialDragonSoulTemplate.h"
#include "MT2ItemSpecialDragonSoulSlot6Template.generated.h"

/**
 * Legacy item-proto variant DS_SLOT6 (ordinal 5).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemSpecialDragonSoulTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemSpecialDragonSoulSlot6Template : public UMT2ItemSpecialDragonSoulTemplate
{
	GENERATED_BODY()
};

