/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/MT2ItemTemplate.h"
#include "MT2ItemCampfireTemplate.generated.h"

UCLASS(Blueprintable)
class METIN2_API UMT2ItemCampfireTemplate : public UMT2ItemTemplate
{
	GENERATED_BODY()
public:
	virtual EMT2ItemUseExecution ExecuteUse(AMT2PlayerCharacter& Character, UMT2InventoryComponent& Inventory, int32 InventorySlot) const override;
};
