#pragma once

#include "Items/Templates/MT2ItemResourceTemplate.h"
#include "MT2ItemResourceWaterstoneTemplate.generated.h"

/**
 * Legacy item-proto variant RESOURCE_WATERSTONE (ordinal 2).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemResourceTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemResourceWaterstoneTemplate : public UMT2ItemResourceTemplate
{
	GENERATED_BODY()
};

