#pragma once

#include "Items/Templates/MT2ItemUseTemplate.h"
#include "MT2ItemUsePutIntoBeltSocketTemplate.generated.h"

/**
 * Legacy item-proto variant USE_PUT_INTO_BELT_SOCKET (ordinal 29).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemUseTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemUsePutIntoBeltSocketTemplate : public UMT2ItemUseTemplate
{
	GENERATED_BODY()
};

