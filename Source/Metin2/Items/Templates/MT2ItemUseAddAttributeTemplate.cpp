#include "MT2ItemUseAddAttributeTemplate.h"

#include "Items/MT2ItemBonusSettings.h"
#include "Items/MT2ItemBonusUtils.h"
#include "Items/MT2ItemTemplate.h"

EMT2ItemBonusApplyResult UMT2ItemUseAddAttributeTemplate::ApplyToEquipment(
	FMT2ItemSlot& StagedTarget, const UMT2ItemTemplate& TargetTemplate,
	const UMT2ItemBonusSettings& Settings) const
{
	const EMT2ItemBonusTarget TargetType = MT2ItemBonusUtils::ResolveTarget(&TargetTemplate);
	if (TargetType == EMT2ItemBonusTarget::None ||
		MT2ItemBonusUtils::CountBonuses(StagedTarget, EMT2ItemBonusKind::Normal) >=
			Settings.NormalBonusAddLimit)
	{
		return EMT2ItemBonusApplyResult::Invalid;
	}

	if (FMath::FRand() > Settings.AddNormalBonusSuccessChance)
	{
		return EMT2ItemBonusApplyResult::Failed;
	}

	return MT2ItemBonusUtils::AddRandomBonus(StagedTarget, Settings.NormalBonuses,
		Settings.AddNormalValueLevelWeights, TargetType, EMT2ItemBonusKind::Normal)
		? EMT2ItemBonusApplyResult::Success
		: EMT2ItemBonusApplyResult::Invalid;
}

