#include "Quests/MT2QuestOreRefining.h"
#include "Items/Templates/MT2ItemMetinStoneTemplate.h"
#include "Quests/MT2QuestManagerComponent.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Player/MT2PlayerState.h"
#include "Npcs/MT2Npc.h"
#include "Items/MT2InventoryComponent.h"
#include "Items/MT2ItemTemplate.h"
#include "Items/MT2ItemUtils.h"
#include "Components/MT2HealthComponent.h"
#include "Trade/MT2TradeComponent.h"
#include "Mining/MT2MiningRules.h"
#include "UObject/StrongObjectPtr.h"

FText UMT2QuestNode_OreRefining::Message(const TCHAR* Key, const TCHAR* Fallback, const TArray<FString>& Arguments) const
{
	const FText* Imported = Messages.Find(FName(Key));
	FString Text = Imported && !Imported->IsEmpty() ? Imported->ToString() : FString(Fallback);
	int32 SearchFrom = 0;
	for (const FString& Argument : Arguments)
	{
		const int32 Index = Text.Find(TEXT("%s"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
		if (Index == INDEX_NONE) { break; }
		Text.RemoveAt(Index, 2); Text.InsertAt(Index, Argument); SearchFrom = Index + Argument.Len();
	}
	Text.ReplaceInline(TEXT("[ENTER]"), TEXT("\n"));
	return FText::FromString(Text);
}

EMT2QuestNodeResult UMT2QuestNode_OreRefining::Execute(const FMT2QuestContext& Context)
{
	auto* Manager = Context.Manager.Get();
	auto* Player = Context.Player.Get();
	auto* State = Context.PlayerState.Get();
	auto* Npc = Cast<AMT2Npc>(Context.TargetActor);
	auto* Inventory = Player ? Player->GetInventoryComponent() : nullptr;
	if (!Manager || !IsValid(Player) || !Player->HasAuthority() || !IsValid(State) || Manager->GetOwner() != State ||
		State->GetPawn() != Player || Player->GetPlayerState() != State || !IsValid(Npc) || Npc->IsActorBeingDestroyed() ||
		Npc->GetWorld() != Player->GetWorld() || !MT2Mining::AlchemistOre(Npc->GetMobVnum()) ||
		FVector::Dist2D(Player->GetActorLocation(), Npc->GetActorLocation()) > 350.f ||
		Player->GetHealthComponent()->IsDead() || (State->GetTradeComponent() && State->GetTradeComponent()->IsTrading()))
	{
		if (Manager) { Manager->AppendSay(FText(), {NSLOCTEXT("Mining", "RefineUnavailable", "Refining is no longer available. Nothing was consumed.")}); }
		return EMT2QuestNodeResult::Continue;
	}
	if (Context.Event == EMT2QuestEvent::Click)
	{
		Manager->AppendSay(FText::FromString(Npc->GetMobDisplayName()), {NSLOCTEXT("Mining", "OfferOre", "Give me at least 100 raw ore to refine it. Diamonds require 10,000 Yang; other ores require 3,000 Yang and a Spirit Stone (+0, +1 or +2).")});
		return EMT2QuestNodeResult::Continue;
	}
	const auto& Ore = Context.EventItemSnapshot;
	const auto& Items = Inventory->GetSlots();
	if (Context.Event != EMT2QuestEvent::ItemTake || !Context.bHasEventItemSnapshot ||
		!MT2Mining::RefinedOre(Ore.Vnum) || !Items.IsValidIndex(Context.EventItemSlot) ||
		Items[Context.EventItemSlot].Count != Ore.Count || !MT2ItemUtils::HaveSameInstanceData(Items[Context.EventItemSlot], Ore))
	{
		Manager->AppendSay(FText(), {NSLOCTEXT("Mining", "OreChanged", "Offer raw ore again. The offered item is no longer available; nothing was consumed.")});
		return EMT2QuestNodeResult::Continue;
	}
	const bool bDiamond = Ore.Vnum == 50601;
	if (Phase == 2)
	{
		if (bResolved) { return EMT2QuestNodeResult::Continue; }
		bResolved = true;
		const int32 Result = Inventory->RefineOre(Context.EventItemSlot, Ore, Npc->GetMobVnum(), StoneSlot, ExpectedStone);
		if (Result == 2) { Manager->AppendSay(FText(), {NSLOCTEXT("Mining", "RefineRejected", "Refining could not start. Check your items, Yang and inventory space. Nothing was consumed.")}); }
		else if (Result == 0) { Manager->AppendSay(FText(), {Message(TEXT("_50_say"), TEXT("The refinement has failed."))}); }
		else
		{
			Manager->AppendSay(FText(), {Message(bDiamond ? TEXT("_40_say") : TEXT("_120_say"), TEXT("Congratulations - your refinement has been successful. You now have:")),
				FText::FromString(FString::Printf(TEXT("{=item_name(%d)}"), Ore.Vnum + 20))});
		}
		return EMT2QuestNodeResult::Continue;
	}
	const auto* Template = Inventory->GetTemplateAtSlot(Context.EventItemSlot);
	const FString Name = MT2ItemUtils::GetDisplayName(Template, Ore.Vnum).ToString();
	if (Ore.Count < 100)
	{
		Manager->AppendSay(FText(), {Message(TEXT("_60_say"), TEXT("You don't have 100 %s."), {Name})});
		return EMT2QuestNodeResult::Continue;
	}
	if (State->GetYang() < MT2Mining::RefiningCost(Ore.Vnum))
	{
		Manager->AppendSay(FText(), {Message(TEXT("_30_say"), TEXT("You do not have enough Yang."))});
		return EMT2QuestNodeResult::Continue;
	}
	TArray<FText> Options;
	TArray<FMT2QuestChoiceBranch> Branches;
	TArray<TStrongObjectPtr<UMT2QuestNode_OreRefining>> HeldChoices;
	const auto AddChoice = [&](const FText& Label, uint8 NextPhase, int32 MaterialSlot, int32 Page = 0)
	{
		TStrongObjectPtr<UMT2QuestNode_OreRefining> Next(NewObject<UMT2QuestNode_OreRefining>(Manager));
		Next->Messages = Messages; Next->Phase = NextPhase; Next->StoneSlot = MaterialSlot;
		Next->MaterialPage = Page;
		if (Items.IsValidIndex(MaterialSlot)) { Next->ExpectedStone = Items[MaterialSlot]; }
		Options.Add(Label); auto& Branch = Branches.AddDefaulted_GetRef();
		Branch.Quest = Context.Quest; Branch.Nodes.Add(Next.Get());
		HeldChoices.Add(MoveTemp(Next));
	};
	if (Phase == 0)
	{
		Manager->AppendSay(FText::FromString(Npc->GetMobDisplayName()), {
			bDiamond ? Message(TEXT("_10_say"), TEXT("A diamond can be made from 100 diamond stones.")) :
				Message(TEXT("_70_say"), TEXT("You need 100 %s and a Spirit Stone (+0, +1 or +2) to produce %s."), {Name, FString::Printf(TEXT("{=item_name(%d)}"), Ore.Vnum + 20)}),
			Message(TEXT("_20_say"), TEXT("The success rate is %s percent. You need %s Yang. Do you want to try it?"),
				{FString::FromInt(MT2Mining::RefiningChance(Ore.Vnum, Npc->GetMobVnum())), FString::FromInt(MT2Mining::RefiningCost(Ore.Vnum))})});
		AddChoice(Message(TEXT("yes"), TEXT("Yes")), bDiamond ? 2 : 1, INDEX_NONE);
	}
	else
	{
		Manager->AppendSay(FText(), {NSLOCTEXT("Mining", "ChooseStone", "Choose the Spirit Stone to consume:")});
		TArray<int32> Materials;
		for (int32 Index = 0; Index < Items.Num(); ++Index)
		{
			if (Items[Index].IsEmpty() || !MT2Mining::IsRefiningStone(Items[Index].Vnum) ||
				!Cast<UMT2ItemMetinStoneTemplate>(Inventory->GetTemplateAtSlot(Index))) { continue; }
			Materials.Add(Index);
		}
		if (Materials.IsEmpty())
		{
			Manager->AppendSay(FText(), {Message(TEXT("_90_say"), TEXT("I cannot produce the item without a Spirit Stone."))});
			return EMT2QuestNodeResult::Continue;
		}
		constexpr int32 PageSize = 8;
		const int32 Page = FMath::Clamp(MaterialPage, 0, (Materials.Num() - 1) / PageSize);
		const int32 End = FMath::Min((Page + 1) * PageSize, Materials.Num());
		for (int32 Position = Page * PageSize; Position < End; ++Position)
		{
			const int32 Index = Materials[Position];
			const FString Label = FString::Printf(TEXT("%s (slot %d)"), *MT2ItemUtils::GetDisplayName(Inventory->GetTemplateAtSlot(Index), Items[Index].Vnum).ToString(), Index + 1);
			AddChoice(FText::FromString(Label), 2, Index);
		}
		if (End < Materials.Num()) { AddChoice(NSLOCTEXT("Mining", "NextStones", "Next page"), 1, INDEX_NONE, Page + 1); }
		if (Page > 0) { AddChoice(NSLOCTEXT("Mining", "PreviousStones", "Previous page"), 1, INDEX_NONE, Page - 1); }
	}
	Options.Add(Message(TEXT("no"), TEXT("Cancel"))); Branches.AddDefaulted();
	Manager->PresentChoices(Options, Branches, NAME_None, FString(), false);
	return EMT2QuestNodeResult::Suspend;
}
