#pragma once

#include "Items/Templates/MT2ItemUseTemplate.h"
#include "MT2ItemUseSpecialTemplate.generated.h"

/**
 * Legacy item-proto variant USE_SPECIAL (ordinal 10).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemUseTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemUseSpecialTemplate : public UMT2ItemUseTemplate
{
	GENERATED_BODY()
};

