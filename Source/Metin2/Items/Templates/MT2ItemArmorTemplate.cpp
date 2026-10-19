/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/Templates/MT2ItemArmorTemplate.h"
#include "Items/MT2AccessorySocketRules.h"
#include "Items/MT2ItemTemplate.h"
#include "Config/MT2GameplaySettings.h"
#include "Items/MT2ItemUtils.h"
#include "UI/MT2ItemTooltipWidget.h"
#include "UI/MT2ItemTooltipWidgets.h"
#include "Skills/MT2SkillSet.h"

void UMT2ItemArmorTemplate::InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const
{
	Super::InitializeDefaultFamilyState(Data);
	if (MT2AccessorySockets::IsAccessory(this)) Data.EditAccessorySockets() = DefaultAccessorySockets;
	else Data.EditMetinSockets() = DefaultMetinSockets;
}
void UMT2ItemArmorTemplate::CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data)
{
	Super::CaptureDefaultFamilyState(Data);
	DefaultMetinSockets = Data.GetMetinSockets();
	DefaultAccessorySockets = Data.GetAccessorySockets();
}

TSubclassOf<UMT2ItemTooltipWidget> UMT2ItemArmorTemplate::GetDefaultTooltipClass() const
{
	return UMT2ArmorTooltipWidget::StaticClass();
}
