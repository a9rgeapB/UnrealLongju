/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/MT2ItemTemplate.h"
#include "Items/State/MT2FishItemState.h"
#include "MT2ItemFishTemplate.generated.h"

UCLASS(Blueprintable)
class METIN2_API UMT2ItemFishTemplate : public UMT2ItemTemplate
{
	GENERATED_BODY()
public:
	virtual void InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const override;
	virtual void CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Initial State")
	FMT2FishItemState DefaultFishState;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fish") EMT2FishSubType FishSubType = EMT2FishSubType::Alive;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fish") FMT2FishDefinition Fish;
	virtual EMT2ItemUseExecution ExecuteUse(AMT2PlayerCharacter& Character, UMT2InventoryComponent& Inventory, int32 InventorySlot) const override;
};
