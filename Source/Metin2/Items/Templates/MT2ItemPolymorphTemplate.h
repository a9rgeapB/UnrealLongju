/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/Templates/MT2ItemConsumableTemplate.h"
#include "Items/State/MT2PolymorphItemState.h"
#include "Skills/MT2SkillTypes.h"
#include "MT2ItemPolymorphTemplate.generated.h"

class UMT2GameplaySettings;

UCLASS(Blueprintable)
class METIN2_API UMT2ItemPolymorphTemplate : public UMT2ItemConsumableTemplate
{
	GENERATED_BODY()

public:
	virtual EMT2ItemUseExecution ExecuteUse(AMT2PlayerCharacter& Character,
		UMT2InventoryComponent& Inventory, int32 InventorySlot) const override;
	virtual void InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const override;
	virtual void CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Polymorph")
	FMT2PolymorphItemState DefaultPolymorphState;

	static int32 GetBallDurationSeconds(int32 SkillLevel,
		const UMT2GameplaySettings& Settings);
	static int32 GetBallDurationSeconds(int32 SkillLevel);
	static int32 GetBallAttackBonus(int32 SkillLevel,
		const UMT2GameplaySettings& Settings);
	static int32 GetBallAttackBonus(int32 SkillLevel);
	static int32 GetBookDurationSeconds(EMT2SkillMastery Mastery,
		const UMT2GameplaySettings& Settings);
	static int32 GetBookDurationSeconds(EMT2SkillMastery Mastery);
	static int32 GetBookSuccessChance(int32 PlayerLevel, int32 MobLevel, int32 SuccessBonus,
		int32 SkillLevel, const UMT2GameplaySettings& Settings);
	static int32 GetBookSuccessChance(int32 PlayerLevel, int32 MobLevel, int32 SuccessBonus, int32 SkillLevel);
};
