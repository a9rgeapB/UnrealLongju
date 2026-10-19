#include "Items/Templates/MT2ItemMetinStoneTemplate.h"
#include "Items/Templates/MT2ItemPickTemplate.h"
#include "Items/MT2InventoryComponent.h"
#include "Items/MT2ItemTemplate.h"
#include "Items/MT2ItemUtils.h"
#include "Mining/MT2MiningRules.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Player/MT2PlayerState.h"
#include "Trade/MT2TradeComponent.h"
#include "Components/MT2HealthComponent.h"

int32 UMT2InventoryComponent::RefineOre(int32 OreSlot, const FMT2ItemSlot& ExpectedOre,
	int32 AlchemistVnum, int32 StoneSlot, const FMT2ItemSlot& ExpectedStone)
{
	return RefineOreWithRoll(OreSlot, ExpectedOre, AlchemistVnum, StoneSlot, ExpectedStone, FMath::RandRange(1, 100));
}

int32 UMT2InventoryComponent::RefineOreWithRoll(int32 OreSlot, const FMT2ItemSlot& ExpectedOre,
	int32 AlchemistVnum, int32 StoneSlot, const FMT2ItemSlot& ExpectedStone, int32 Roll)
{
	auto* Player = Cast<AMT2PlayerCharacter>(GetOwner());
	auto* State = Player ? Player->GetPlayerState<AMT2PlayerState>() : nullptr;
	const int32 ResultVnum = MT2Mining::RefinedOre(ExpectedOre.Vnum);
	const int32 YangCost = MT2Mining::RefiningCost(ExpectedOre.Vnum);
	const auto Matches = [this](int32 Index, const FMT2ItemSlot& Expected)
	{
		return Slots.IsValidIndex(Index) && !Expected.IsEmpty() && Slots[Index].Count == Expected.Count &&
			MT2ItemUtils::HaveSameInstanceData(Slots[Index], Expected);
	};
	if (bResolvingOre || !IsValid(Player) || !Player->HasAuthority() || !IsValid(State) ||
		Player->GetHealthComponent()->IsDead() || (State->GetTradeComponent() && State->GetTradeComponent()->IsTrading()) ||
		!ResultVnum || !MT2Mining::AlchemistOre(AlchemistVnum) || !Matches(OreSlot, ExpectedOre) ||
		ExpectedOre.Count < 100 || Roll < 1 || Roll > 100) { return 2; }
	TGuardValue<bool> Guard(bResolvingOre, true);
	const UMT2ItemTemplate* ResultTemplate = ResolveTemplate(ResultVnum);
	if (!ResolveTemplate(ExpectedOre.Vnum) || !ResultTemplate || State->GetYang() < YangCost) { return 2; }
	const bool bDiamond = ExpectedOre.Vnum == 50601;
	if (!bDiamond && (OreSlot == StoneSlot || !Matches(StoneSlot, ExpectedStone) ||
		!MT2Mining::IsRefiningStone(ExpectedStone.Vnum) || !Cast<UMT2ItemMetinStoneTemplate>(ResolveTemplate(ExpectedStone.Vnum)))) { return 2; }
	if (bDiamond && StoneSlot != INDEX_NONE) { return 2; }

	TArray<FMT2ItemSlot> Candidate = Slots;
	Candidate[OreSlot].Count -= 100;
	const int32 FreedOre = Candidate[OreSlot].Count == 0 ? OreSlot : INDEX_NONE;
	if (FreedOre != INDEX_NONE) { Candidate[OreSlot] = FMT2ItemSlot(); }
	int32 FreedStone = INDEX_NONE;
	if (!bDiamond)
	{
		if (--Candidate[StoneSlot].Count == 0) { Candidate[StoneSlot] = FMT2ItemSlot(); FreedStone = StoneSlot; }
	}
	FMT2ItemSlot Reward(ResultVnum, ResultTemplate->MakeInstanceData(1));
	Reward.Template = ResultTemplate->GetClass();
	ResultTemplate->InitializeGeneratedInstance(Reward);
	Reward.bNewlyAcquired = true;
	int32 Destination = INDEX_NONE;
	bool bStack = false;
	if (ResultTemplate->GetMaxStackSize() > 1)
	{
		Destination = Candidate.IndexOfByPredicate([&](const FMT2ItemSlot& Item)
		{
			return !Item.IsEmpty() && Item.Count < ResultTemplate->GetMaxStackSize() && MT2ItemUtils::HaveSameInstanceData(Item, Reward);
		});
		bStack = Destination != INDEX_NONE;
	}
	if (Destination == INDEX_NONE)
	{
		for (int32 Index = 0; Index < Candidate.Num(); ++Index)
		{
			if (Candidate[Index].IsEmpty() && CanPlaceAt(Index, FMath::Clamp(ResultTemplate->InventorySize, 1, 3), FreedOre, FreedStone))
			{ Destination = Index; break; }
		}
	}
	// Reserve the successful result before the roll; invalid/full inventories never pay for a failure.
	if (Destination == INDEX_NONE) { return 2; }
	const bool bSuccess = Roll <= MT2Mining::RefiningChance(ExpectedOre.Vnum, AlchemistVnum);
	if (bSuccess)
	{
		if (bStack) { ++Candidate[Destination].Count; Candidate[Destination].bNewlyAcquired = true; }
		else { Candidate[Destination] = Reward; }
	}
	// Publish the complete item state before Yang delegates can save or inspect the exchange.
	Slots = MoveTemp(Candidate);
	State->AddYang(-YangCost);
	OnInventoryChanged.Broadcast();
	if (bSuccess) { NotifyItemReceived(Reward, 1); }
	Player->ForceNetUpdate();
	return bSuccess ? 1 : 0;
}

bool UMT2InventoryComponent::FinishMiningAttempt()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Equipment.IsValidIndex(4)) { return false; }
	const auto* Pick=Cast<UMT2ItemPickTemplate>(ResolveTemplate(Equipment[4].Vnum));
	if (!Pick || Pick->GetRefinedVnum()<=0 || Pick->PracticeChanceDenominator<=0 || Pick->ImprovementPointsRequired<=0) { return false; }
	const int32 Points=Equipment[4].GetPickaxeState().Proficiency;
	if (Points>=Pick->ImprovementPointsRequired || FMath::RandRange(1,Pick->PracticeChanceDenominator)!=1) { return false; }
	Equipment[4].EditPickaxeState().Proficiency=Points+1;
	OnInventoryChanged.Broadcast(); OnEquipmentChanged.Broadcast();
	return true;
}
