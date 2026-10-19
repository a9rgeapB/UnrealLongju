#pragma once

#include "Items/Templates/MT2ItemUseTemplate.h"
#include "MT2ItemUseClearTemplate.generated.h"

/**
 * Legacy item-proto variant USE_CLEAR (ordinal 12).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemUseTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemUseClearTemplate : public UMT2ItemUseTemplate
{
	GENERATED_BODY()
};

