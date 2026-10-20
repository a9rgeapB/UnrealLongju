#include "Items/MT2InventoryComponent.h"

#include "Characters/MT2PlayerCharacter.h"
#include "Components/MT2HealthComponent.h"
#include "Core/MT2VnumRegistrySubsystem.h"
#include "Items/MT2InventoryLayout.h"
#include "Items/MT2ItemTemplate.h"
#include "Items/MT2ItemUtils.h"
#include "Items/Templates/MT2ItemUseRecipeTemplate.h"
#include "Player/MT2PlayerState.h"
#include "Trade/MT2TradeComponent.h"

bool UMT2InventoryComponent::ExecuteItemRecipe(int32 RecipeSlot,
	const UMT2ItemUseRecipeTemplate& Recipe)
{
	auto* Player = Cast<AMT2PlayerCharacter>(GetOwner());
	auto* State = Player ? Player->GetPlayerState<AMT2PlayerState>() : nullptr;
	const UMT2ItemTemplate* RecipeItemTemplate =
		Slots.IsValidIndex(RecipeSlot) && !Slots[RecipeSlot].IsEmpty()
			? ResolveTemplate(Slots[RecipeSlot].Vnum) : nullptr;
	if (!Player || !Player->HasAuthority() || !Player->GetHealthComponent() ||
		Player->GetHealthComponent()->IsDead() ||
		(State && State->GetTradeComponent() && State->GetTradeComponent()->IsTrading()) ||
		!RecipeItemTemplate || !RecipeItemTemplate->IsA(Recipe.GetClass()))
	{
		return false;
	}

	const auto GetTemplateVnum = [](TSubclassOf<UMT2ItemTemplate> ItemClass) -> int32
	{
		const UMT2ItemTemplate* Defaults = ItemClass
			? ItemClass->GetDefaultObject<UMT2ItemTemplate>() : nullptr;
		return Defaults && !ItemClass->HasAnyClassFlags(CLASS_Abstract) ? Defaults->Vnum : 0;
	};
	const FMT2ItemRecipeDefinition& Definition = Recipe.Recipe;
	if (Definition.FirstIngredientCount < 0 || Definition.SecondIngredientCount < 0)
	{
		return false;
	}
	const int32 FirstIngredientVnum = Definition.FirstIngredientCount > 0
		? GetTemplateVnum(Definition.FirstIngredientTemplate) : 0;
	const int32 SecondIngredientVnum = Definition.SecondIngredientCount > 0
		? GetTemplateVnum(Definition.SecondIngredientTemplate) : 0;
	const int32 BottleVnum = GetTemplateVnum(Definition.RequiredBottleTemplate);
	const UMT2ItemTemplate* ResultTemplate = Definition.ResultItemTemplate
		? Definition.ResultItemTemplate->GetDefaultObject<UMT2ItemTemplate>() : nullptr;
	const int32 ResultVnum = GetTemplateVnum(Definition.ResultItemTemplate);
	const auto IsRegisteredReference = [this](int32 Vnum, TSubclassOf<UMT2ItemTemplate> ItemClass)
	{
		const UMT2ItemTemplate* Registered = ResolveTemplate(Vnum);
		return Registered && ItemClass && Registered->GetClass() == ItemClass.Get();
	};
	if ((Definition.FirstIngredientCount > 0 && FirstIngredientVnum <= 0) ||
		(Definition.SecondIngredientCount > 0 && SecondIngredientVnum <= 0) ||
		BottleVnum <= 0 || ResultVnum <= 0 || !ResultTemplate ||
		!IsRegisteredReference(BottleVnum, Definition.RequiredBottleTemplate) ||
		!IsRegisteredReference(ResultVnum, Definition.ResultItemTemplate) ||
		(Definition.FirstIngredientCount > 0 &&
			!IsRegisteredReference(FirstIngredientVnum, Definition.FirstIngredientTemplate)) ||
		(Definition.SecondIngredientCount > 0 &&
			!IsRegisteredReference(SecondIngredientVnum, Definition.SecondIngredientTemplate)))
	{
		return false;
	}

	TMap<int32, int32> RequiredItems;
	const auto AddRequirement = [&RequiredItems](int32 ItemVnum, int32 Count) -> bool
	{
		int32& Existing = RequiredItems.FindOrAdd(ItemVnum);
		if (Count > MAX_int32 - Existing) return false;
		Existing += Count;
		return true;
	};
	if ((FirstIngredientVnum > 0 && !AddRequirement(FirstIngredientVnum, Definition.FirstIngredientCount)) ||
		(SecondIngredientVnum > 0 && !AddRequirement(SecondIngredientVnum, Definition.SecondIngredientCount)) ||
		!AddRequirement(BottleVnum, 1)) return false;
	for (const TPair<int32, int32>& Requirement : RequiredItems)
	{
		if (Requirement.Value <= 0 || CountItemByVnum(Requirement.Key) < Requirement.Value)
		{
			return false;
		}
	}

	const bool bSucceeded = FMath::RandRange(1, 100) <= Definition.SuccessPercent;
	TArray<FMT2ItemSlot> Candidate = Slots;
	TArray<FMT2ItemSlot> RecoveryItems;
	for (const TPair<int32, int32>& Requirement : RequiredItems)
	{
		int32 Remaining = Requirement.Value;
		for (int32 Index = 0; Index < Candidate.Num(); ++Index)
		{
			if (Remaining <= 0) break;
			FMT2ItemSlot& Slot = Candidate[Index];
			if (Slot.IsEmpty() || Slot.Vnum != Requirement.Key) continue;
			const int32 Removed = FMath::Min(Remaining, Slot.Count);
			if (Removed <= 0) continue;
			if (Slot.GetAutoRecoveryState().bActive) RecoveryItems.Add(Slots[Index]);
			Slot.Count -= Removed;
			Remaining -= Removed;
			if (Slot.Count <= 0) Slot = FMT2ItemSlot();
		}
		if (Remaining > 0) return false;
	}

	FMT2ItemSlot Reward;
	if (bSucceeded)
	{
		const int32 MaxStack = ResultTemplate->GetMaxStackSize();
		if (MaxStack <= 0) return false;
		Reward = FMT2ItemSlot(ResultVnum, ResultTemplate->MakeInstanceData());
		Reward.Template = Definition.ResultItemTemplate;
		Reward.bNewlyAcquired = true;
		int32 Remaining = 1;
		for (FMT2ItemSlot& Slot : Candidate)
		{
			if (Slot.IsEmpty() || Slot.Count >= MaxStack ||
				!MT2ItemUtils::HaveSameInstanceData(Slot, Reward)) continue;
			const int32 Added = FMath::Min(Remaining, MaxStack - Slot.Count);
			Slot.Count += Added;
			Slot.bNewlyAcquired = true;
			Remaining -= Added;
			if (Remaining == 0) break;
		}
		TBitArray<> Occupied(false, Candidate.Num());
		for (int32 Index = 0; Index < Candidate.Num(); ++Index)
		{
			if (Candidate[Index].IsEmpty()) continue;
			const UMT2ItemTemplate* ExistingTemplate = ResolveTemplate(Candidate[Index].Vnum);
			if (!ExistingTemplate) return false;
			for (int32 Cell = 0; Cell < FMath::Clamp(ExistingTemplate->InventorySize, 1, 3); ++Cell)
			{
				const int32 Covered = Index + Cell * MT2InventoryLayout::Columns;
				if (!Candidate.IsValidIndex(Covered) || Covered / PageSlotCount != Index / PageSlotCount) return false;
				Occupied[Covered] = true;
			}
		}
		const int32 Size = FMath::Clamp(ResultTemplate->InventorySize, 1, 3);
		for (int32 Index = 0; Index < Candidate.Num() && Remaining > 0; ++Index)
		{
			if ((Index % PageSlotCount) / MT2InventoryLayout::Columns + Size > MT2InventoryLayout::Rows) continue;
			bool bFree = true;
			for (int32 Cell = 0; Cell < Size; ++Cell)
			{
				if (Occupied[Index + Cell * MT2InventoryLayout::Columns]) { bFree = false; break; }
			}
			if (!bFree) continue;
			Candidate[Index] = Reward;
			Candidate[Index].Count = 1;
			Remaining = 0;
			for (int32 Cell = 0; Cell < Size; ++Cell) Occupied[Index + Cell * MT2InventoryLayout::Columns] = true;
		}
		if (Remaining > 0) return false;
	}

	Slots = MoveTemp(Candidate);
	for (const FMT2ItemSlot& Item : RecoveryItems) StopAutoRecoveryEffectForSlot(Item);
	OnInventoryChanged.Broadcast();
	Player->ForceNetUpdate();
	if (bSucceeded) NotifyItemReceived(Reward, 1);
	return true;
}
