#pragma once

#include "Items/Templates/MT2ItemExtractTemplate.h"
#include "MT2ItemExtractDragonSoulTemplate.generated.h"

/**
 * Legacy item-proto variant EXTRACT_DRAGON_SOUL (ordinal 0).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemExtractTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemExtractDragonSoulTemplate : public UMT2ItemExtractTemplate
{
	GENERATED_BODY()
};

