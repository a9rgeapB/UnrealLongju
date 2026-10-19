#pragma once

#include "Items/Templates/MT2ItemSpecialTemplate.h"
#include "MT2ItemSpecialMapTemplate.generated.h"

/**
 * Legacy item-proto variant SPECIAL_MAP (ordinal 0).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemSpecialTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemSpecialMapTemplate : public UMT2ItemSpecialTemplate
{
	GENERATED_BODY()
};

