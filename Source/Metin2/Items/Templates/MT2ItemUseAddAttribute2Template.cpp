#include "MT2ItemUseAddAttribute2Template.h"

#include "Items/MT2ItemBonusSettings.h"
#include "Items/MT2ItemBonusUtils.h"
#include "Items/MT2ItemTemplate.h"

EMT2ItemBonusApplyResult UMT2ItemUseAddAttribute2Template::ApplyToEquipment(
	FMT2ItemSlot& StagedTarget, const UMT2ItemTemplate& TargetTemplate,
	const UMT2ItemBonusSettings& Settings) const
{
	const EMT2ItemBonusTarget TargetType = MT2ItemBonusUtils::ResolveTarget(&TargetTemplate);
	const int32 NormalCount = MT2ItemBonusUtils::CountBonuses(StagedTarget, EMT2ItemBonusKind::Normal);
	if (TargetType == EMT2ItemBonusTarget::None ||
		NormalCount != Settings.NormalBonusAddLimit || NormalCount >= Settings.NormalBonusMaximum)
	{
		return EMT2ItemBonusApplyResult::Invalid;
	}

	return MT2ItemBonusUtils::AddRandomBonus(StagedTarget, Settings.NormalBonuses,
		Settings.AddNormalValueLevelWeights, TargetType, EMT2ItemBonusKind::Normal)
		? EMT2ItemBonusApplyResult::Success
		: EMT2ItemBonusApplyResult::Invalid;
}

