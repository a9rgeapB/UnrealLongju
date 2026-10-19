#pragma once

#include "Items/Templates/MT2ItemExtractTemplate.h"
#include "MT2ItemExtractDragonHeartTemplate.generated.h"

/**
 * Legacy item-proto variant EXTRACT_DRAGON_HEART (ordinal 1).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemExtractTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemExtractDragonHeartTemplate : public UMT2ItemExtractTemplate
{
	GENERATED_BODY()
};

