#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "AbilitySystemComponent.h"
#include "Abilities/MT2CoreAttributeSet.h"
#include "AIController.h"
#include "UObject/StrongObjectPtr.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Player/MT2PlayerState.h"
#include "Items/MT2InventoryComponent.h"
#include "Items/MT2ItemUtils.h"
#include "Quests/MT2QuestManagerComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2HairConsumptionTest, "Metin2.Quests.PlayerApi.HairConsumption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2HairConsumptionTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>(GEngine));
	Instance->InitializeStandalone();
	UWorld* World = Instance->GetWorld();
	if (!TestNotNull(TEXT("World"), World)) return false;
	ON_SCOPE_EXIT { Instance->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	World->InitializeActorsForPlay(FURL());
	World->GetWorldSettings()->NotifyBeginPlay();
	auto* Controller = World->SpawnActor<AAIController>();
	auto* State = World->SpawnActor<AMT2PlayerState>();
	State->GetAbilitySystemComponent()->AddAttributeSetSubobject(State->GetCoreAttributes());
	State->GetAbilitySystemComponent()->InitAbilityActorInfo(State, State);
	Controller->SetPlayerState(State);
	auto* Player = World->SpawnActor<AMT2PlayerCharacter>();
	Controller->Possess(Player);
	State->SetCharacterLevel(90);
	auto* Inventory = Player->GetInventoryComponent();
	Inventory->RestoreItems({}, {});
	auto Find = [&](int32 Vnum) -> int32
	{
		for (int32 Index = 0; Index < Inventory->GetSlots().Num(); ++Index)
			if (Inventory->GetSlots()[Index].Vnum == Vnum && !Inventory->GetSlots()[Index].IsEmpty()) return Index;
		return INDEX_NONE;
	};
	auto Use = [&](int32 Vnum, bool bEquipRequest)
	{
		State->GetQuestManagerComponent()->CancelConversation();
		const int32 Slot = Find(Vnum);
		if (!TestTrue(TEXT("Hair exists in inventory"), Slot != INDEX_NONE)) return false;
		return TestTrue(TEXT("Hair request succeeds"), bEquipRequest ? Inventory->EquipItemFromSlot(Slot) : Inventory->UseItem(Slot));
	};
	TestTrue(TEXT("Imported cosmetic hair is consumable"), MT2ItemUtils::IsConsumableHairLook(MT2ItemUtils::ResolveTemplate(Player, 73001)));
	TestTrue(TEXT("Imported bonus hair is consumable"), MT2ItemUtils::IsConsumableHairLook(MT2ItemUtils::ResolveTemplate(Player, 74001)));
	TestTrue(TEXT("Add cosmetic hair"), Inventory->AddItemByVnum(73001, 2));
	if (!Use(73001, true)) return false;
	TestEqual(TEXT("Equip/drag route consumes exactly one"), Inventory->CountItemByVnum(73001), 1);
	TestTrue(TEXT("No hairstyle kept as equipment"), Inventory->GetEquipment()[20].IsEmpty());
	if (!Use(73001, false)) return false;
	TestEqual(TEXT("Reusing same look consumes remaining item"), Inventory->CountItemByVnum(73001), 0);
	TestTrue(TEXT("Add different hair"), Inventory->AddItemByVnum(73002, 1));
	if (!Use(73002, true)) return false;
	TestEqual(TEXT("Different look consumed"), Inventory->CountItemByVnum(73002), 0);
	TestEqual(TEXT("Previous hair not refunded"), Inventory->CountItemByVnum(73001), 0);
	TestTrue(TEXT("Add timed hair"), Inventory->AddItemByVnum(74001, 1));
	if (!Use(74001, true)) return false;
	TestEqual(TEXT("Quest-driven hair consumed exactly once"), Inventory->CountItemByVnum(74001), 0);
	TestTrue(TEXT("Timed hair stays out of equipment"), Inventory->GetEquipment()[20].IsEmpty());
	TArray<FMT2ItemSlot> Worn; Worn.SetNum(UMT2InventoryComponent::EquipmentCount);
	Worn[20].Vnum = 73002; Worn[20].Count = 1;
	Inventory->RestoreItems({}, Worn);
	TestTrue(TEXT("Add replacement for existing equipped look"), Inventory->AddItemByVnum(73001, 1));
	if (!Use(73001, false)) return false;
	TestTrue(TEXT("Existing equipped hairstyle discarded"), Inventory->GetEquipment()[20].IsEmpty());
	TestEqual(TEXT("Existing equipped hairstyle not refunded"), Inventory->CountItemByVnum(73002), 0);
	Inventory->RestoreItems({}, Worn);
	TestTrue(TEXT("Unequip obsolete hairstyle"), Inventory->UnequipItem(20));
	TestEqual(TEXT("Unequip does not refund obsolete hairstyle"), Inventory->CountItemByVnum(73002), 0);
	return true;
}
#endif
