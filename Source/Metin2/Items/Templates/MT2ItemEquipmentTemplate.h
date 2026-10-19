/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/MT2ItemTemplate.h"
#include "Items/State/MT2EquipmentItemState.h"
#include "MT2ItemEquipmentTemplate.generated.h"

UCLASS(Abstract, Blueprintable)
class METIN2_API UMT2ItemEquipmentTemplate : public UMT2ItemTemplate
{
	GENERATED_BODY()

public:
	virtual void InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const override;
	virtual void CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Equipment|Initial State")
	FMT2EquipmentItemState DefaultEquipmentState;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment",
		meta = (Bitmask, BitmaskEnum = "/Script/Metin2.EMT2ItemWearFlag"))
	int32 WearFlags = 0;

	// The old server calls this GAIN_SOCKET_PCT, but uses it as the initial silver-socket count.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Sockets",
		meta = (ClampMin = "0", ClampMax = "3"))
	int32 DefaultSilverSocketCount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Sockets",
		meta = (ClampMin = "0", ClampMax = "3"))
	int32 DefaultGoldSocketCount = 0;

	// Old item_proto addon_type. -1 enables the paired average/skill-damage weapon formula.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Bonuses")
	FMT2ItemAddonSelection BonusAddonType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Presentation")
	int32 Specular = 0;
};
