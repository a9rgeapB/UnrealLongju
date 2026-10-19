#pragma once

#include "Items/Templates/MT2ItemResourceTemplate.h"
#include "MT2ItemResourceCrystalTemplate.generated.h"

/**
 * Legacy item-proto variant RESOURCE_CRYSTAL (ordinal 7).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemResourceTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemResourceCrystalTemplate : public UMT2ItemResourceTemplate
{
	GENERATED_BODY()
};

