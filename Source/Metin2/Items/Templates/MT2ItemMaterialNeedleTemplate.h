#pragma once

#include "Items/Templates/MT2ItemMaterialTemplate.h"
#include "MT2ItemMaterialNeedleTemplate.generated.h"

/**
 * Legacy item-proto variant MATERIAL_NEEDLE (ordinal 3).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemMaterialTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemMaterialNeedleTemplate : public UMT2ItemMaterialTemplate
{
	GENERATED_BODY()
};

