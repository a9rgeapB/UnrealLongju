#pragma once

#include "Items/Use/MT2ItemActionTemplate.h"
#include "MT2ItemUseBaitTemplate.generated.h"

/**
 * Legacy item-proto variant USE_BAIT (ordinal 6).
 * Value0 is the power bonus applied to the equipped fishing rod. Legacy char_item.cpp rejects
 * bait use while fishing, requires a rod in WEAR_WEAPON, then replaces the rod's current bait.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemUseBaitTemplate : public UMT2ItemActionTemplate
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Bait")
	int32 BaitPower = 0;

	virtual EMT2ItemUseExecution ExecuteUse(
		AMT2PlayerCharacter& Character, UMT2InventoryComponent& Inventory,
		int32 InventorySlot) const override;
};

