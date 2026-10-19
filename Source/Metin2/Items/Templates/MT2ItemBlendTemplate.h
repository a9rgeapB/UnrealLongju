/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/Templates/MT2ItemConsumableTemplate.h"
#include "Items/State/MT2BlendItemState.h"
#include "MT2ItemBlendTemplate.generated.h"

UCLASS(Blueprintable)
class METIN2_API UMT2ItemBlendTemplate : public UMT2ItemConsumableTemplate
{
	GENERATED_BODY()
public:
	virtual void InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const override;
	virtual void CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Initial State")
	FMT2BlendItemState DefaultBlendState;

	virtual void InitializeGeneratedInstance(FMT2ItemInstanceData& InOutData) const override;
	virtual EMT2ItemUseExecution ExecuteUse(AMT2PlayerCharacter& Character,
		UMT2InventoryComponent& Inventory, int32 InventorySlot) const override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Blend")
	FMT2BlendDefinition Blend;
};
