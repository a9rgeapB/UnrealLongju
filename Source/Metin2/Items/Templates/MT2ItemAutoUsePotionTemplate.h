#pragma once

#include "Items/Templates/MT2ItemAutoUseTemplate.h"
#include "MT2ItemAutoUsePotionTemplate.generated.h"

/**
 * Legacy item-proto variant AUTOUSE_POTION (ordinal 0).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemAutoUseTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemAutoUsePotionTemplate : public UMT2ItemAutoUseTemplate
{
	GENERATED_BODY()
};

