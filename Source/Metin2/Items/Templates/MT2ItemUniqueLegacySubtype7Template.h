#pragma once

#include "Items/Templates/MT2ItemUniqueTemplate.h"
#include "MT2ItemUniqueLegacySubtype7Template.generated.h"

/**
 * Legacy item-proto variant UNIQUE_7 (ordinal 7).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemUniqueTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemUniqueLegacySubtype7Template : public UMT2ItemUniqueTemplate
{
	GENERATED_BODY()
};

