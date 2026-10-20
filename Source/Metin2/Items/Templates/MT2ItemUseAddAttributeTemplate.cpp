#include "MT2ItemUseAddAttributeTemplate.h"

#include "Items/MT2ItemBonusSettings.h"
#include "Items/MT2ItemBonusUtils.h"
#include "Items/MT2ItemTemplate.h"

EMT2ItemBonusApplyResult UMT2ItemUseAddAttributeTemplate::ApplyToEquipment(
	FMT2ItemSlot& StagedTarget, const UMT2ItemTemplate& TargetTemplate,
	const UMT2ItemBonusSettings& Settings) const
{
	const EMT2ItemBonusTarget TargetType = MT2ItemBonusUtils::ResolveTarget(&TargetTemplate);
	const int32 NormalCount = MT2ItemBonusUtils::CountBonuses(StagedTarget, EMT2ItemBonusKind::Normal);
	if (TargetType == EMT2ItemBonusTarget::None ||
		NormalCount >= FMath::Min(Settings.NormalBonusAddLimit, Settings.NormalBonusMaximum))
	{
		return EMT2ItemBonusApplyResult::Invalid;
	}

	FMT2ItemSlot Candidate = StagedTarget;
	if (!MT2ItemBonusUtils::AddRandomBonus(Candidate, Settings.NormalBonuses,
		Settings.AddNormalValueLevelWeights, TargetType, EMT2ItemBonusKind::Normal, &TargetTemplate))
	{
		return EMT2ItemBonusApplyResult::Invalid;
	}
	if (FMath::RandRange(1, 100) > Settings.NormalBonusAdditionChances.GetSuccessPercent(NormalCount))
	{
		return EMT2ItemBonusApplyResult::Failed;
	}

	StagedTarget = MoveTemp(Candidate);
	return EMT2ItemBonusApplyResult::Success;
}

