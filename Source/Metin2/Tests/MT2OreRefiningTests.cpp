#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Player/MT2PlayerState.h"
#include "Items/MT2InventoryComponent.h"
#include "Core/MT2VnumRegistrySubsystem.h"
#include "Core/MT2VnumRegistry.h"
#include "Items/MT2ItemTemplate.h"
#include "Abilities/MT2CoreAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/MT2HealthComponent.h"
#include "Mining/MT2MiningRules.h"
#include "Npcs/MT2Npc.h"
#include "Quests/MT2Quest.h"
#include "Quests/MT2QuestNode.h"
#include "Quests/MT2QuestRegistrySubsystem.h"
#include "Quests/MT2QuestManagerComponent.h"
#include "Quests/MT2QuestComponent.h"
#include "UObject/GarbageCollection.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2AlchemistDialogueTest, "Metin2.Mining.AlchemistDialogue", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2AlchemistDialogueTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GEngine)); GI->InitializeStandalone();
	UWorld* World = GI->GetWorld();
	ON_SCOPE_EXIT { GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	auto* Player = World->SpawnActor<AMT2PlayerCharacter>();
	auto* State = World->SpawnActor<AMT2PlayerState>(); Player->SetPlayerState(State);
	auto* ASC = State->GetAbilitySystemComponent();
	ASC->AddAttributeSetSubobject(State->GetCoreAttributes()); ASC->InitAbilityActorInfo(State, Player);
	ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetMaxHealthAttribute(), 100);
	ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetHealthAttribute(), 100);
	Player->GetHealthComponent()->InitializeWithAbilitySystem(ASC);
	auto* Npc = World->SpawnActor<AMT2Npc>();
	FMT2MobDefinition Definition; Definition.Vnum = 20061; Npc->ConfigureFromDefinition(Definition);
	Npc->SetActorLocation(Player->GetActorLocation() + FVector(100, 0, 0));
	auto* Inventory = Player->GetInventoryComponent();
	auto* Manager = State->GetQuestManagerComponent();
	auto* Dialog = State->GetQuestComponent();
	TArray<FMT2ItemSlot> Slots; Slots.SetNum(180);
	TArray<FMT2ItemSlot> Equipment; Equipment.SetNum(32);
	Slots[0].Vnum = 28030; Slots[0].Count = 1;
	Slots[1].Vnum = 50602; Slots[1].Count = 100;
	auto Start = [&]
	{
		Manager->CancelConversation(); Inventory->RestoreItems(Slots, Equipment); State->SetYang(10000);
		TestTrue(TEXT("Ore offer claims alchemist conversation"), Manager->DispatchEvent(EMT2QuestEvent::ItemTake, 20061, Npc, 1));
		TestTrue(TEXT("Confirmation shown"), Dialog->HasPendingDialog());
	};
	Start(); Dialog->ServerAnswerDialog_Implementation(2, Dialog->GetDialogPageId());
	TestEqual(TEXT("Confirmation cancellation preserves money"), State->GetYang(), int64(10000));
	TestEqual(TEXT("Confirmation cancellation preserves ore"), Inventory->CountItemByVnum(50602), 100);
	Start(); Dialog->ServerAnswerDialog_Implementation(1, Dialog->GetDialogPageId());
	TestTrue(TEXT("Material picker shown"), Dialog->HasPendingDialog());
	CollectGarbage(RF_NoFlags);
	Dialog->ServerAnswerDialog_Implementation(1, Dialog->GetDialogPageId());
	TestEqual(TEXT("Selected slot zero stone consumed"), Inventory->CountItemByVnum(28030), 0);
	TestEqual(TEXT("Dialogue consumes exactly one batch"), Inventory->CountItemByVnum(50602), 0);
	TestEqual(TEXT("Dialogue charges independent fee"), State->GetYang(), int64(7000));
	Dialog->ServerAnswerDialog_Implementation(1, Dialog->GetDialogPageId());
	TestEqual(TEXT("Repeated completion answer cannot charge again"), State->GetYang(), int64(7000));
	Start(); Dialog->ServerAnswerDialog_Implementation(1, Dialog->GetDialogPageId()); Inventory->ConsumeItemAtSlot(0);
	Dialog->ServerAnswerDialog_Implementation(1, Dialog->GetDialogPageId());
	TestEqual(TEXT("Stale selected stone cannot charge"), State->GetYang(), int64(10000));
	TestEqual(TEXT("Stale selected stone preserves ore"), Inventory->CountItemByVnum(50602), 100);
	Start(); Dialog->ServerAnswerDialog_Implementation(1, Dialog->GetDialogPageId()); Inventory->ConsumeItemAtSlot(1);
	Dialog->ServerAnswerDialog_Implementation(1, Dialog->GetDialogPageId());
	TestEqual(TEXT("Changed offered ore cannot charge"), State->GetYang(), int64(10000));
	TestEqual(TEXT("Changed offered ore preserves sacrifice"), Inventory->CountItemByVnum(28030), 1);
	Start(); Dialog->ServerAnswerDialog_Implementation(1, Dialog->GetDialogPageId()); Dialog->ServerAnswerDialog_Implementation(0, Dialog->GetDialogPageId());
	TestEqual(TEXT("Picker cancellation preserves money"), State->GetYang(), int64(10000));
	Manager->CancelConversation();
	TArray<FMT2ItemSlot> ManyStones; ManyStones.SetNum(180);
	for (int32 Index = 0; Index < 12; ++Index) { ManyStones[Index].Vnum = 28030; ManyStones[Index].Count = 1; }
	ManyStones[20].Vnum = 50602; ManyStones[20].Count = 100;
	Inventory->RestoreItems(ManyStones, Equipment); State->SetYang(10000);
	TestTrue(TEXT("Large sacrifice selection opens"), Manager->DispatchEvent(EMT2QuestEvent::ItemTake, 20061, Npc, 20));
	Dialog->ServerAnswerDialog_Implementation(1, Dialog->GetDialogPageId());
	Dialog->ServerAnswerDialog_Implementation(9, Dialog->GetDialogPageId()); // Next page after eight material choices.
	TestEqual(TEXT("Changing material page cannot charge"), State->GetYang(), int64(10000));
	Dialog->ServerAnswerDialog_Implementation(4, Dialog->GetDialogPageId()); // Inventory anchor eleven, fourth on page two.
	TestTrue(TEXT("Page two chooses exact last stone"), Inventory->GetSlots()[11].IsEmpty());
	TestEqual(TEXT("Other page's stone is preserved"), Inventory->GetSlots()[8].Count, 1);
	Start(); Dialog->ServerAnswerDialog_Implementation(1, Dialog->GetDialogPageId()); Npc->SetActorLocation(Player->GetActorLocation() + FVector(1000, 0, 0));
	Dialog->ServerAnswerDialog_Implementation(1, Dialog->GetDialogPageId());
	TestEqual(TEXT("Distant NPC cannot charge"), State->GetYang(), int64(10000));
	Npc->SetActorLocation(Player->GetActorLocation() + FVector(100, 0, 0));
	Start();
	const uint32 ConfirmationPage = Dialog->GetDialogPageId();
	Dialog->ServerAnswerDialog_Implementation(1, ConfirmationPage);
	Dialog->ServerAnswerDialog_Implementation(1, ConfirmationPage); // Duplicate of the confirmation, not a new material selection.
	TestEqual(TEXT("Replayed confirmation cannot select and consume a stone"), State->GetYang(), int64(10000));
	Start();
	Dialog->ServerAnswerDialog_Implementation(1, ConfirmationPage);
	TestEqual(TEXT("Previous conversation answer cannot advance replacement"), State->GetYang(), int64(10000));
	Dialog->ServerAnswerDialog_Implementation(1, Dialog->GetDialogPageId());
	TestEqual(TEXT("Replacement still needs a material answer"), State->GetYang(), int64(10000));
	Start(); Dialog->ServerAnswerDialog_Implementation(1, Dialog->GetDialogPageId());
	ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetHealthAttribute(), 0);
	Dialog->ServerAnswerDialog_Implementation(1, Dialog->GetDialogPageId());
	TestEqual(TEXT("Dead player cannot refine"), State->GetYang(), int64(10000));
	ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetHealthAttribute(), 100);
	Start(); Dialog->ServerAnswerDialog_Implementation(1, Dialog->GetDialogPageId()); Npc->Destroy(); Dialog->ServerAnswerDialog_Implementation(1, Dialog->GetDialogPageId());
	TestEqual(TEXT("Destroyed NPC cannot charge"), State->GetYang(), int64(10000));
	Manager->CancelConversation();
	FMT2DialogPayload Input; Input.bRequestsTextInput = true;
	FString ReceivedText;
	Dialog->ShowInputDialog(Input, [&](const FString& Text) { ReceivedText = Text; });
	const uint32 OldInputPage = Dialog->GetDialogPageId();
	Dialog->ShowInputDialog(Input, [&](const FString& Text) { ReceivedText = Text; });
	Dialog->ServerAnswerDialogText_Implementation(TEXT("stale"), OldInputPage);
	TestTrue(TEXT("Old input page cannot answer its replacement"), ReceivedText.IsEmpty());
	const uint32 NewInputPage = Dialog->GetDialogPageId();
	Dialog->ServerAnswerDialogText_Implementation(TEXT("current"), NewInputPage);
	TestEqual(TEXT("Current input still works"), ReceivedText, FString(TEXT("current")));
	Dialog->ServerAnswerDialogText_Implementation(TEXT("replayed"), NewInputPage);
	TestEqual(TEXT("Input answer cannot be replayed"), ReceivedText, FString(TEXT("current")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2AlchemistContentTest, "Metin2.Mining.AlchemistContent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2AlchemistContentTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GEngine)); GI->InitializeStandalone();
	UWorld* World = GI->GetWorld();
	ON_SCOPE_EXIT { GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	const UMT2Quest* Quest = GI->GetSubsystem<UMT2QuestRegistrySubsystem>()->FindQuest(TEXT("guild_building_melt"));
	if (!TestNotNull(TEXT("Alchemist quest registered"), Quest)) { return false; }
	const auto* Start = Quest->FindState(TEXT("start"));
	if (!TestNotNull(TEXT("Alchemist initial state"), Start)) { return false; }
	int32 Offers = 0;
	for (const auto& Trigger : Start->Triggers)
	{
		if (Trigger.Event != EMT2QuestEvent::ItemTake) { continue; }
		++Offers;
		TestTrue(TEXT("Ore offer uses native refining transaction"), Trigger.Nodes.Num() == 1 && Trigger.Nodes[0] &&
			Trigger.Nodes[0]->GetClass()->GetFName() == TEXT("MT2QuestNode_OreRefining"));
	}
	TestEqual(TEXT("All eighteen independent specializations"), Offers, 18);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2OreRefiningTest, "Metin2.Mining.OreRefining", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2OreRefiningTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GEngine));
	GI->InitializeStandalone();
	UWorld* World = GI->GetWorld();
	ON_SCOPE_EXIT { GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	auto* Player = World->SpawnActor<AMT2PlayerCharacter>();
	auto* State = World->SpawnActor<AMT2PlayerState>();
	Player->SetPlayerState(State);
	auto* ASC = State->GetAbilitySystemComponent();
	ASC->AddAttributeSetSubobject(State->GetCoreAttributes()); ASC->InitAbilityActorInfo(State, Player);
	ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetMaxHealthAttribute(), 100);
	ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetHealthAttribute(), 100);
	Player->GetHealthComponent()->InitializeWithAbilitySystem(ASC);
	auto* Inventory = Player->GetInventoryComponent();
	TArray<FMT2ItemSlot> Slots; Slots.SetNum(UMT2InventoryComponent::SlotCount);
	TArray<FMT2ItemSlot> Equipment; Equipment.SetNum(UMT2InventoryComponent::EquipmentCount);
	Slots[0].Vnum = 50601; Slots[0].Count = 100;
	Inventory->RestoreItems(Slots, Equipment);
	State->SetYang(10000);
	const int32 Result = Inventory->RefineOre(0, Slots[0], 20060, INDEX_NONE, FMT2ItemSlot());
	TestTrue(TEXT("Diamond offer resolves an attempt"), Result == 0 || Result == 1);
	TestEqual(TEXT("Attempt consumes 100 raw ore"), Inventory->CountItemByVnum(50601), 0);
	TestEqual(TEXT("Attempt charges 10000 Yang"), State->GetYang(), int64(0));
	TestEqual(TEXT("Success gives exactly one refined diamond"), Inventory->CountItemByVnum(50621), Result == 1 ? 1 : 0);
	TestEqual(TEXT("Replayed snapshot rejected"), Inventory->RefineOre(0, Slots[0], 20060, INDEX_NONE, FMT2ItemSlot()), 2);
	for (int32 Index = 0; Index < 18; ++Index)
	{
		const int32 Npc = Index < 13 ? 20060 + Index : 33009 + Index - 13;
		const int32 Raw = 50601 + Index;
		TestEqual(TEXT("Alchemist specialization"), MT2Mining::AlchemistOre(Npc), Raw);
		TestEqual(TEXT("Refined mapping"), MT2Mining::RefinedOre(Raw), 50621 + Index);
		TestNotNull(TEXT("Imported alchemist"), GI->GetSubsystem<UMT2VnumRegistrySubsystem>()->ResolveMobClass(Npc).Get());
		for (const int32 Roll : {1, 60, 61, 100})
		{
			Slots.Init(FMT2ItemSlot(), UMT2InventoryComponent::SlotCount);
			Slots[1].Vnum = Raw; Slots[1].Count = 101;
			Slots[0].Vnum = 28030; Slots[0].Count = 1;
			Inventory->RestoreItems(Slots, Equipment); State->SetYang(20000);
			const int32 StoneIndex = Index == 0 ? INDEX_NONE : 0;
			TestEqual(TEXT("Matching boundary roll"), Inventory->RefineOreWithRoll(1, Slots[1], Npc, StoneIndex, Slots[0], Roll), Roll <= 60 ? 1 : 0);
			TestEqual(TEXT("One hundred ore consumed"), Inventory->CountItemByVnum(Raw), 1);
			TestEqual(TEXT("Independent base fee"), State->GetYang(), int64(Index == 0 ? 10000 : 17000));
			TestEqual(TEXT("Exact refined reward"), Inventory->CountItemByVnum(50621 + Index), Roll <= 60 ? 1 : 0);
			TestEqual(TEXT("Diamond does not consume stone; others do"), Inventory->CountItemByVnum(28030), Index == 0 ? 1 : 0);
			TestEqual(TEXT("Stale ore snapshot cannot replay"), Inventory->RefineOreWithRoll(1, Slots[1], Npc, StoneIndex, Slots[0], Roll), 2);
		}
	}
	Slots.Init(FMT2ItemSlot(), UMT2InventoryComponent::SlotCount);
	Slots[0].Vnum = 50602; Slots[0].Count = 100;
	Slots[1].Vnum = 28030; Slots[1].Count = 1;
	auto Reset = [&] { Inventory->RestoreItems(Slots, Equipment); State->SetYang(3000); };
	Reset();
	TestEqual(TEXT("Wrong alchemist success at thirty"), Inventory->RefineOreWithRoll(0, Slots[0], 20060, 1, Slots[1], 30), 1);
	Reset();
	TestEqual(TEXT("Wrong alchemist failure above thirty"), Inventory->RefineOreWithRoll(0, Slots[0], 20060, 1, Slots[1], 31), 0);
	Reset(); State->SetYang(2999);
	TestEqual(TEXT("Insufficient Yang rejected"), Inventory->RefineOreWithRoll(0, Slots[0], 20061, 1, Slots[1], 1), 2);
	TestEqual(TEXT("Insufficient Yang preserves ore"), Inventory->CountItemByVnum(50602), 100);
	for (const int32 BadStone : {27999, 28300, 28430, 28960})
	{
		Slots[1].Vnum = BadStone; Reset();
		TestEqual(TEXT("Invalid sacrifice rejected"), Inventory->RefineOreWithRoll(0, Slots[0], 20061, 1, Slots[1], 1), 2);
		TestEqual(TEXT("Invalid sacrifice preserves Yang"), State->GetYang(), int64(3000));
	}
	Slots[1].Vnum = 28030; Reset();
	Inventory->ConsumeItemAtSlot(1);
	TestEqual(TEXT("Removed sacrifice rejected"), Inventory->RefineOreWithRoll(0, Slots[0], 20061, 1, Slots[1], 1), 2);
	TestEqual(TEXT("Non-alchemist rejected"), Inventory->RefineOreWithRoll(0, Slots[0], 9009, 1, Slots[1], 1), 2);
	Slots.Init(FMT2ItemSlot(), UMT2InventoryComponent::SlotCount);
	for (auto& Item : Slots) { Item.Vnum = 27001; Item.Count = 1; }
	Slots[0].Vnum = 50601; Slots[0].Count = 101; Reset(); State->SetYang(10000);
	TestEqual(TEXT("Full bag rejected even on failed roll"), Inventory->RefineOreWithRoll(0, Slots[0], 20060, INDEX_NONE, FMT2ItemSlot(), 100), 2);
	TestEqual(TEXT("Full bag preserves money"), State->GetYang(), int64(10000));
	Slots[0].Count = 100; Reset(); State->SetYang(10000);
	TestEqual(TEXT("Consumed ore frees output cell in full bag"), Inventory->RefineOreWithRoll(0, Slots[0], 20060, INDEX_NONE, FMT2ItemSlot(), 1), 1);
	const FString Saved = State->CapturePersistentStateJson_Implementation();
	TestTrue(TEXT("Refined state restores"), State->ApplyPersistentStateJson_Implementation(Saved));
	TestEqual(TEXT("Refined ore persists"), Inventory->CountItemByVnum(50621), 1);
	TestEqual(TEXT("Refining fee persists"), State->GetYang(), int64(0));
	Slots.Init(FMT2ItemSlot(), UMT2InventoryComponent::SlotCount);
	Slots[0].Vnum = 50601; Slots[0].Count = 100;
	Reset(); State->SetYang(10000);
	TestEqual(TEXT("Live inventory reference is accepted"), Inventory->RefineOreWithRoll(0, Inventory->GetSlots()[0], 20060, INDEX_NONE, FMT2ItemSlot(), 1), 1);
	TestEqual(TEXT("Live inventory reference charges diamond fee"), State->GetYang(), int64(0));
	Reset(); State->SetYang(10000);
	{
		auto* Registry = GI->GetSubsystem<UMT2VnumRegistrySubsystem>()->GetRegistry();
		auto OriginalMobs = Registry->GetMobClasses();
		auto OriginalItems = Registry->GetItemTemplates();
		ON_SCOPE_EXIT { Registry->SetEntries(MoveTemp(OriginalMobs), MoveTemp(OriginalItems)); };
		auto MissingResult = OriginalItems;
		MissingResult.FindOrAdd(50621).Reset();
		Registry->SetEntries(TMap<int32, TSoftClassPtr<AMT2Mob>>(OriginalMobs), MoveTemp(MissingResult));
		TestEqual(TEXT("Missing refined template rejects transaction"), Inventory->RefineOreWithRoll(0, Slots[0], 20060, INDEX_NONE, FMT2ItemSlot(), 1), 2);
		TestEqual(TEXT("Missing template preserves ore"), Inventory->CountItemByVnum(50601), 100);
		TestEqual(TEXT("Missing template preserves money"), State->GetYang(), int64(10000));
	}
	return true;
}
#endif
