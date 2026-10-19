/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/Templates/MT2ItemAutoRecoveryTemplate.h"
#include "Items/MT2ItemTemplate.h"
#include "Config/MT2GameplaySettings.h"
#include "Items/MT2ItemUtils.h"
#include "UI/MT2ItemTooltipWidget.h"
#include "UI/MT2ItemTooltipWidgets.h"
#include "Skills/MT2SkillSet.h"

void UMT2ItemAutoRecoveryTemplate::InitializeGeneratedInstance(
	FMT2ItemInstanceData& InOutData) const
{
	if (InOutData.GetAutoRecoveryState().MaximumAmount <= 0)
	{
		InOutData.EditAutoRecoveryState().MaximumAmount = FMath::Max(RecoveryCapacity, 0);
		InOutData.EditAutoRecoveryState().RemainingAmount = InOutData.GetAutoRecoveryState().MaximumAmount;
	}
	InOutData.EditAutoRecoveryState().RemainingAmount = FMath::Clamp(
		InOutData.GetAutoRecoveryState().RemainingAmount, 0, InOutData.GetAutoRecoveryState().MaximumAmount);
}

void UMT2ItemAutoRecoveryTemplate::InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const
{
	Data.FamilyState.InitializeAs<FMT2AutoRecoveryItemState>(DefaultRecoveryState);
}

void UMT2ItemAutoRecoveryTemplate::CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data)
{
	DefaultRecoveryState = Data.GetAutoRecoveryState();
}
