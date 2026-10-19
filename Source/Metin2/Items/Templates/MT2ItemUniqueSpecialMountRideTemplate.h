#pragma once

#include "Items/Templates/MT2ItemUniqueTemplate.h"
#include "MT2ItemUniqueSpecialMountRideTemplate.generated.h"

/**
 * Legacy item-proto variant UNIQUE_3 (legacy enum value UNIQUE_SPECIAL_MOUNT_RIDE) (ordinal 3).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemUniqueTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemUniqueSpecialMountRideTemplate : public UMT2ItemUniqueTemplate
{
	GENERATED_BODY()
};

