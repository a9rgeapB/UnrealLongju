#include "MT2ItemUseBaitTemplate.h"

#include "Items/MT2InventoryComponent.h"

EMT2ItemUseExecution UMT2ItemUseBaitTemplate::ExecuteUse(
	AMT2PlayerCharacter& Character, UMT2InventoryComponent& Inventory, int32 InventorySlot) const
{
	if (!Character.HasAuthority() || Inventory.GetOwner() != &Character || BaitPower <= 0)
	{
		return EMT2ItemUseExecution::Rejected;
	}
	return Inventory.ApplyFishingBait(InventorySlot, *this)
		? EMT2ItemUseExecution::Accepted : EMT2ItemUseExecution::Rejected;
}

