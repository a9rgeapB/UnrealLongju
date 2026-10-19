/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/Templates/MT2ItemWeaponTemplate.h"

void UMT2ItemWeaponTemplate::InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const
{
	Super::InitializeDefaultFamilyState(Data);
	Data.EditMetinSockets() = DefaultSockets;
}
void UMT2ItemWeaponTemplate::CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data)
{
	Super::CaptureDefaultFamilyState(Data);
	DefaultSockets = Data.GetMetinSockets();
}
#include "Items/MT2ItemTemplate.h"
#include "Config/MT2GameplaySettings.h"
#include "Items/MT2ItemUtils.h"
#include "UI/MT2ItemTooltipWidget.h"
#include "UI/MT2ItemTooltipWidgets.h"
#include "Skills/MT2SkillSet.h"

TSubclassOf<UMT2ItemTooltipWidget> UMT2ItemWeaponTemplate::GetDefaultTooltipClass() const
{
	return UMT2WeaponTooltipWidget::StaticClass();
}
