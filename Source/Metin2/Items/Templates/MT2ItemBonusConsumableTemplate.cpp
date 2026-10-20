/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/Templates/MT2ItemBonusConsumableTemplate.h"

EMT2ItemBonusApplyResult UMT2ItemBonusConsumableTemplate::ApplyToEquipment(
	FMT2ItemSlot&, const UMT2ItemTemplate&, const UMT2ItemBonusSettings&) const
{
	return EMT2ItemBonusApplyResult::Invalid;
}
