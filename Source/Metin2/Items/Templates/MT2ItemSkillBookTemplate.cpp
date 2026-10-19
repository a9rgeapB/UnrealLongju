/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/Templates/MT2ItemSkillBookTemplate.h"
#include "Items/MT2ItemTemplate.h"
#include "Config/MT2GameplaySettings.h"
#include "Items/MT2ItemUtils.h"
#include "UI/MT2ItemTooltipWidget.h"
#include "UI/MT2ItemTooltipWidgets.h"
#include "Skills/MT2SkillSet.h"
#include "Skills/MT2SkillDefinition.h"

void UMT2ItemSkillBookTemplate::InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const
{
	Data.FamilyState.InitializeAs<FMT2SkillBookItemState>(DefaultSkillBookState);
}
void UMT2ItemSkillBookTemplate::CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data)
{
	DefaultSkillBookState = Data.GetSkillBookState();
}

int32 UMT2ItemSkillBookTemplate::GetSkillVnum() const
{
	return Skill ? Skill->Vnum : bTypedItemReferences ? 0 : SkillVnum;
}

void UMT2ItemSkillBookTemplate::InitializeGeneratedInstance(
	FMT2ItemInstanceData& InOutData) const
{
	if (InOutData.GetSkillVnum() > 0)
	{
		return;
	}
	if (GetSkillVnum() > 0)
	{
		InOutData.SetSkillVnum(GetSkillVnum());
		return;
	}

	// The old server rolls vnum 50300 from the available job skills and stores the selected skill
	// in socket 0. Imported definitions are the authority here, so nonexistent skills cannot roll.
	const TArray<int32> SkillVnums = UMT2SkillSet::GetSkillVnumsInRange(1, 111);
	if (!SkillVnums.IsEmpty())
	{
		InOutData.SetSkillVnum(SkillVnums[FMath::RandHelper(SkillVnums.Num())]);
	}
}
