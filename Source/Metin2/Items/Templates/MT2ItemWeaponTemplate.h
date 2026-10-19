/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/Templates/MT2ItemEquipmentTemplate.h"
#include "MT2ItemWeaponTemplate.generated.h"

UCLASS(Blueprintable)
class METIN2_API UMT2ItemWeaponTemplate : public UMT2ItemEquipmentTemplate
{
	GENERATED_BODY()

public:
	virtual void InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const override;
	virtual void CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Sockets")
	TArray<FMT2MetinSocket> DefaultSockets;
	virtual TSubclassOf<UMT2ItemTooltipWidget> GetDefaultTooltipClass() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	EMT2WeaponSubType WeaponSubType = EMT2WeaponSubType::Sword;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage")
	int32 MagicDamageMin = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage")
	int32 MagicDamageMax = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage")
	int32 PhysicalDamageMin = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage")
	int32 PhysicalDamageMax = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage")
	int32 RefinementDamage = 0;

	UFUNCTION(BlueprintPure, Category = "Weapon|Damage")
	int32 GetPhysicalDamageMin() const
	{
		return PhysicalDamageMin + RefinementDamage;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon|Damage")
	int32 GetPhysicalDamageMax() const
	{
		return PhysicalDamageMax + RefinementDamage;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon|Damage")
	int32 GetMagicDamageMin() const
	{
		return MagicDamageMin + RefinementDamage;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon|Damage")
	int32 GetMagicDamageMax() const
	{
		return MagicDamageMax + RefinementDamage;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon|Damage")
	int32 GetAdditionalDamage() const
	{
		return RefinementDamage;
	}
};
