#pragma once

#include "Items/Templates/MT2ItemToolTemplate.h"
#include "MT2ItemToolFishingRodTemplate.generated.h"

/**
 * Legacy item-proto variant TOOL_FISHING_ROD (ordinal 0).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemToolTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemToolFishingRodTemplate : public UMT2ItemToolTemplate
{
	GENERATED_BODY()
};

