/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/Templates/MT2ItemPickTemplate.h"



void UMT2ItemPickTemplate::InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const
{
	Data.FamilyState.InitializeAs<FMT2PickaxeItemState>(DefaultPickaxeState);
}

void UMT2ItemPickTemplate::CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data)
{
	DefaultPickaxeState = Data.GetPickaxeState();
}
