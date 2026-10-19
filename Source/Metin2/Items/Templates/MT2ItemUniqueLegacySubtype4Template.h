#pragma once

#include "Items/Templates/MT2ItemUniqueTemplate.h"
#include "MT2ItemUniqueLegacySubtype4Template.generated.h"

/**
 * Legacy item-proto variant UNIQUE_4 (ordinal 4).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemUniqueTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemUniqueLegacySubtype4Template : public UMT2ItemUniqueTemplate
{
	GENERATED_BODY()
};

