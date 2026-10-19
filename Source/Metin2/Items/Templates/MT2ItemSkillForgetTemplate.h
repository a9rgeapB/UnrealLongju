/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/Templates/MT2ItemConsumableTemplate.h"
#include "Items/State/MT2SkillForgetItemState.h"
#include "MT2ItemSkillForgetTemplate.generated.h"

class UMT2SkillDefinition;

UCLASS(Blueprintable)
class METIN2_API UMT2ItemSkillForgetTemplate : public UMT2ItemConsumableTemplate
{
	GENERATED_BODY()

public:
	virtual void InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const override;
	virtual void CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data) override;
	virtual void InitializeGeneratedInstance(FMT2ItemInstanceData& Data) const override;
	virtual EMT2ItemUseExecution ExecuteUse(AMT2PlayerCharacter& Character,
		UMT2InventoryComponent& Inventory, int32 InventorySlot) const override;

protected:
	// Optional authored skill selection; when empty, the legacy item rolls from its valid pool.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Forget")
	TObjectPtr<UMT2SkillDefinition> Skill;

private:
	UPROPERTY()
	FMT2SkillForgetItemState DefaultSkillForgetState;
};
