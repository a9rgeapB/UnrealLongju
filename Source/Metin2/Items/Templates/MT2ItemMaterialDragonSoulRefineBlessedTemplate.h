#pragma once

#include "Items/Templates/MT2ItemMaterialTemplate.h"
#include "MT2ItemMaterialDragonSoulRefineBlessedTemplate.generated.h"

/**
 * Legacy item-proto variant MATERIAL_DS_REFINE_BLESSED (ordinal 6).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemMaterialTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemMaterialDragonSoulRefineBlessedTemplate : public UMT2ItemMaterialTemplate
{
	GENERATED_BODY()
};

