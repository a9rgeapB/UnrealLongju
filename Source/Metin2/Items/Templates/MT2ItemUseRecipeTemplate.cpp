#include "MT2ItemUseRecipeTemplate.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Items/MT2InventoryComponent.h"

EMT2ItemUseExecution UMT2ItemUseRecipeTemplate::ExecuteUse(
	AMT2PlayerCharacter& Character, UMT2InventoryComponent& Inventory, int32 InventorySlot) const
{
	if (!Character.HasAuthority() || !Inventory.ExecuteItemRecipe(InventorySlot, *this))
	{
		return EMT2ItemUseExecution::Rejected;
	}
	// Legacy USE_RECIPE scrolls are reusable; the ingredients and bottle are the consumed inputs.
	return EMT2ItemUseExecution::Accepted;
}
