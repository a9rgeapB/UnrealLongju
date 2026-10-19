#pragma once

#include "Items/Templates/MT2ItemArmorTemplate.h"
#include "MT2ItemArmorEarTemplate.generated.h"

/**
 * Legacy item-proto variant ARMOR_EAR (ordinal 6).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemArmorTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemArmorEarTemplate : public UMT2ItemArmorTemplate
{
	GENERATED_BODY()
};

