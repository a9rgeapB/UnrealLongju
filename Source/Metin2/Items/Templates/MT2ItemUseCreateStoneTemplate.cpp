#include "MT2ItemUseCreateStoneTemplate.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Items/MT2InventoryComponent.h"

EMT2ItemUseExecution UMT2ItemUseCreateStoneTemplate::ExecuteUse(
	AMT2PlayerCharacter& Character, UMT2InventoryComponent& Inventory, int32 InventorySlot) const
{
	if (!Character.HasAuthority() || !Inventory.GetSlots().IsValidIndex(InventorySlot) ||
		Inventory.GetSlots()[InventorySlot].IsEmpty() || StoneResults.IsEmpty())
	{
		return EMT2ItemUseExecution::Rejected;
	}
	TArray<int32> ValidResultIndices;
	for (int32 Index = 0; Index < StoneResults.Num(); ++Index)
	{
		const FMT2ItemCreateStoneResult& Result = StoneResults[Index];
		const UMT2ItemTemplate* Defaults = Result.ItemTemplate
			? Result.ItemTemplate->GetDefaultObject<UMT2ItemTemplate>() : nullptr;
		if (Defaults && Defaults->Vnum > 0 && Result.VnumRangeOffset >= 0 &&
			Result.VnumRangeOffset <= Defaults->VnumRange &&
			!Result.ItemTemplate->HasAnyClassFlags(CLASS_Abstract))
		{
			ValidResultIndices.Add(Index);
		}
	}
	if (ValidResultIndices.IsEmpty()) return EMT2ItemUseExecution::Rejected;

	const FMT2ItemCreateStoneResult& Result =
		StoneResults[ValidResultIndices[FMath::RandRange(0, ValidResultIndices.Num() - 1)]];
	const int32 ResultVnum = Result.ItemTemplate->GetDefaultObject<UMT2ItemTemplate>()->Vnum +
		Result.VnumRangeOffset;
	return Inventory.ExchangeItemAtSlot(InventorySlot, 1, ResultVnum, 1)
		? EMT2ItemUseExecution::Accepted : EMT2ItemUseExecution::Rejected;
}

