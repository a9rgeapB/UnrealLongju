/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/Use/MT2ItemActionTemplate.h"
#include "MT2GrandMasterTrainingItemTemplate.generated.h"

UCLASS(Blueprintable)
class METIN2_API UMT2GrandMasterTrainingItemTemplate : public UMT2ItemActionTemplate
{
	GENERATED_BODY()

public:
	virtual EMT2ItemUseExecution ExecuteUse(
		AMT2PlayerCharacter& Character, UMT2InventoryComponent& Inventory,
		int32 InventorySlot) const override;
};
