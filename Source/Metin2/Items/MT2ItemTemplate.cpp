/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/MT2ItemTemplate.h"
#include "Config/MT2GameplaySettings.h"
#include "Items/MT2ItemUtils.h"
#include "UI/MT2ItemTooltipWidget.h"
#include "UI/MT2ItemTooltipWidgets.h"
#include "Skills/MT2SkillSet.h"

int32 UMT2ItemTemplate::GetMaxStackSize() const
{
	return IsStackable() ? FMath::Max(UMT2GameplaySettings::Get().MaximumItemStackSize, 1) : 1;
}

int32 UMT2ItemTemplate::GetRefinedVnum() const
{
	const auto Class = RefinedItemTemplate.LoadSynchronous();
	return Class ? Class->GetDefaultObject<UMT2ItemTemplate>()->Vnum : bTypedItemReferences ? 0 : RefinedVnum;
}

int32 UMT2ItemTemplate::GetPreviousRefinedVnum() const
{
	const auto Class = PreviousRefinedItemTemplate.LoadSynchronous();
	return Class ? Class->GetDefaultObject<UMT2ItemTemplate>()->Vnum : bTypedItemReferences ? 0 : PreviousRefinedVnum;
}

void UMT2ItemTemplate::ResetImportedItemReferences()
{
	bTypedItemReferences = false;
	RefinedItemTemplate.Reset();
	PreviousRefinedItemTemplate.Reset();
}

void UMT2ItemTemplate::PostLoad()
{
	Super::PostLoad();
	if (!Sockets_DEPRECATED.IsEmpty())
	{
		Sockets_DEPRECATED.Reset();
	}
}

FMT2ItemInstanceData UMT2ItemTemplate::MakeInstanceData(int32 Count) const
{
	FMT2ItemInstanceData Result = DefaultInstanceData;
	if (bTypedFamilyDefaults) InitializeDefaultFamilyState(Result);
	Result.Count = FMath::Clamp(Count, 1, GetMaxStackSize());
	MT2ItemUtils::NormalizeNumericSockets(Result, this);
	InitializeGeneratedInstance(Result);
	return Result;
}

void UMT2ItemTemplate::FinalizeFamilyDefaults()
{
	if (bTypedFamilyDefaults) return;
	MT2ItemUtils::NormalizeNumericSockets(DefaultInstanceData, this);
	CaptureDefaultFamilyState(DefaultInstanceData);
	// Arbitrary quest state stays opaque for the legacy Lua boundary. Named family payloads now
	// live on their template and are no longer duplicated on the common defaults.
	if (!MT2ItemUtils::UsesNumericSockets(this) || DefaultInstanceData.FindState<FMT2BlendItemState>()) DefaultInstanceData.FamilyState.Reset();
	bTypedFamilyDefaults = true;
}





TSubclassOf<UMT2ItemTooltipWidget> UMT2ItemTemplate::ResolveTooltipClass() const
{
	if (TooltipWidgetOverride)
	{
		return TooltipWidgetOverride;
	}
	return GetDefaultTooltipClass();
}

TSubclassOf<UMT2ItemTooltipWidget> UMT2ItemTemplate::GetDefaultTooltipClass() const
{
	// Base/material/quest/etc.: name + description + any bonuses.
	return UMT2ItemTooltipWidget::StaticClass();
}
