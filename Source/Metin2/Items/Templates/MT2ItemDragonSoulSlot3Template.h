#pragma once

#include "Items/Templates/MT2ItemDragonSoulTemplate.h"
#include "MT2ItemDragonSoulSlot3Template.generated.h"

/**
 * Legacy item-proto variant DS_SLOT3 (ordinal 2).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemDragonSoulTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemDragonSoulSlot3Template : public UMT2ItemDragonSoulTemplate
{
	GENERATED_BODY()
};

