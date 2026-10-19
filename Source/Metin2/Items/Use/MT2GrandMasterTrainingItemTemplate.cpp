/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/Use/MT2GrandMasterTrainingItemTemplate.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Player/MT2PlayerState.h"

EMT2ItemUseExecution UMT2GrandMasterTrainingItemTemplate::ExecuteUse(
	AMT2PlayerCharacter& Character, UMT2InventoryComponent& Inventory,
	int32 InventorySlot) const
{
	return Character.BeginGrandMasterTraining(InventorySlot)
		? EMT2ItemUseExecution::Accepted : EMT2ItemUseExecution::Rejected;
}
