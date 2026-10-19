/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/Templates/MT2ItemEquipmentTemplate.h"

void UMT2ItemEquipmentTemplate::InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const
{
	Data.EditBonuses() = DefaultEquipmentState.Bonuses;
}
void UMT2ItemEquipmentTemplate::CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data)
{
	DefaultEquipmentState.Bonuses = Data.GetBonuses();
}


