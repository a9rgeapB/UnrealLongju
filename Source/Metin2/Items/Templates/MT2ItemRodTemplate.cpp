/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/Templates/MT2ItemRodTemplate.h"

int32 UMT2ItemRodTemplate::GetRefinementFailureVnum() const
{
	const auto Class = RefinementFailureTemplate.LoadSynchronous();
	return Class ? Class->GetDefaultObject<UMT2ItemRodTemplate>()->Vnum : bTypedItemReferences ? 0 : RefinementFailureVnum;
}



void UMT2ItemRodTemplate::InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const
{
	Data.FamilyState.InitializeAs<FMT2FishingRodItemState>(DefaultRodState);
}

void UMT2ItemRodTemplate::CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data)
{
	DefaultRodState = Data.GetRodState();
}
