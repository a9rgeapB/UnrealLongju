#pragma once

#include "Items/Templates/MT2ItemFishTemplate.h"
#include "MT2ItemFishDeadTemplate.generated.h"

/**
 * Legacy item-proto variant FISH_DEAD (ordinal 1).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemFishTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemFishDeadTemplate : public UMT2ItemFishTemplate
{
	GENERATED_BODY()
};

