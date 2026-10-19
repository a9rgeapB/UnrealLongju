/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/Templates/MT2ItemMetinStoneTemplate.h"

bool UMT2ItemMetinStoneTemplate::ConflictsWith(const UMT2ItemMetinStoneTemplate& Other) const
{
	if (!ConflictFamily.IsNull() && !Other.ConflictFamily.IsNull()) return ConflictFamily == Other.ConflictFamily;
	return (!bTypedItemReferences || !Other.bTypedItemReferences) && StoneCategory == Other.StoneCategory;
}


