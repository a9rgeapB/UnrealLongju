/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once

#include "Items/Templates/MT2ItemUseTemplate.h"
#include "MT2ItemBonusConsumableTemplate.generated.h"

class UMT2ItemBonusSettings;
class UMT2ItemTemplate;

/** Shared contract for consumables that apply an equipment bonus. Concrete items own the rule. */
UCLASS(Abstract, Blueprintable)
class METIN2_API UMT2ItemBonusConsumableTemplate : public UMT2ItemUseTemplate
{
	GENERATED_BODY()

public:
	// Inventory passes a staged target copy; it commits the copy only on Success and consumes the
	// source item on either Success or Failed. Invalid leaves the inventory transaction untouched.
	virtual EMT2ItemBonusApplyResult ApplyToEquipment(FMT2ItemSlot& StagedTarget,
		const UMT2ItemTemplate& TargetTemplate, const UMT2ItemBonusSettings& Settings) const;

	virtual bool IsBonusRandomizer() const { return false; }
};
