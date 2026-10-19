#pragma once

#include "Items/Templates/MT2ItemArmorTemplate.h"
#include "MT2ItemArmorNeckTemplate.generated.h"

/**
 * Legacy item-proto variant ARMOR_NECK (ordinal 5).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemArmorTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemArmorNeckTemplate : public UMT2ItemArmorTemplate
{
	GENERATED_BODY()
};

