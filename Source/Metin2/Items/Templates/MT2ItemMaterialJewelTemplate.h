#pragma once

#include "Items/Templates/MT2ItemMaterialTemplate.h"
#include "MT2ItemMaterialJewelTemplate.generated.h"

/**
 * Legacy item-proto variant MATERIAL_JEWEL (ordinal 4).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemMaterialTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemMaterialJewelTemplate : public UMT2ItemMaterialTemplate
{
	GENERATED_BODY()
};

