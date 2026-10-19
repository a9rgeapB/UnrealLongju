#pragma once

#include "Items/Templates/MT2ItemCostumeTemplate.h"
#include "MT2ItemCostumeBodyTemplate.generated.h"

/**
 * Legacy item-proto variant COSTUME_BODY (ordinal 0).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemCostumeTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemCostumeBodyTemplate : public UMT2ItemCostumeTemplate
{
	GENERATED_BODY()
};

