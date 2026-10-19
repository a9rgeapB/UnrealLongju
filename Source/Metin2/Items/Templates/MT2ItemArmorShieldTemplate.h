#pragma once

#include "Items/Templates/MT2ItemArmorTemplate.h"
#include "MT2ItemArmorShieldTemplate.generated.h"

/**
 * Legacy item-proto variant ARMOR_SHIELD (ordinal 2).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemArmorTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemArmorShieldTemplate : public UMT2ItemArmorTemplate
{
	GENERATED_BODY()
};

