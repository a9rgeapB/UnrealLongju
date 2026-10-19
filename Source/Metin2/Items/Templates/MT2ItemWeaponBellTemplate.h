#pragma once

#include "Items/Templates/MT2ItemWeaponTemplate.h"
#include "MT2ItemWeaponBellTemplate.generated.h"

/**
 * Legacy item-proto variant WEAPON_BELL (ordinal 4).
 * TODO: Implement and validate this variant's dedicated legacy behavior. Keep shared family
 * rules in UMT2ItemWeaponTemplate; this class is the narrow subtype extension point.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemWeaponBellTemplate : public UMT2ItemWeaponTemplate
{
	GENERATED_BODY()
};

