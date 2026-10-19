#include "Items/Templates/MT2ItemMetinRemovalTemplate.h"
#include "Items/Templates/MT2ItemSocketCleanerTemplate.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "AbilitySystemComponent.h"
#include "Abilities/MT2CoreAttributeSet.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Components/MT2HealthComponent.h"
#include "Core/MT2VnumRegistrySubsystem.h"
#include "Config/MT2GameplaySettings.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Items/MT2InventoryComponent.h"
#include "Items/Templates/MT2ItemArmorTemplate.h"
#include "Items/Templates/MT2ItemMetinStoneTemplate.h"
#include "Items/Templates/MT2ItemWeaponTemplate.h"
#include "Player/MT2PlayerState.h"

namespace
{
	struct FMetinSocketItemWorld
	{
		TStrongObjectPtr<UGameInstance> Instance{NewObject<UGameInstance>(GEngine)};
		UWorld* World = nullptr;
		AMT2PlayerCharacter* Player = nullptr;
		UMT2InventoryComponent* Inventory = nullptr;
		UMT2VnumRegistrySubsystem* Registry = nullptr;

		FMetinSocketItemWorld()
		{
			Instance->InitializeStandalone();
			World = Instance->GetWorld();
			if (!World) return;
			Player = World->SpawnActor<AMT2PlayerCharacter>();
			auto* State = World->SpawnActor<AMT2PlayerState>();
			if (!Player || !State) return;
			Player->SetPlayerState(State);
			auto* Ability = State->GetAbilitySystemComponent();
			Ability->AddAttributeSetSubobject(State->GetCoreAttributes());
			Ability->InitAbilityActorInfo(State, Player);
			Ability->SetNumericAttributeBase(UMT2CoreAttributeSet::GetMaxHealthAttribute(), 100);
			Ability->SetNumericAttributeBase(UMT2CoreAttributeSet::GetHealthAttribute(), 100);
			Player->GetHealthComponent()->InitializeWithAbilitySystem(Ability);
			Inventory = Player->GetInventoryComponent();
			Registry = Instance->GetSubsystem<UMT2VnumRegistrySubsystem>();
		}

		~FMetinSocketItemWorld()
		{
			Instance->Shutdown();
			if (World) { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); }
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2MetinRemovalAndSocketCleaningTest,
	"Metin2.Items.MetinSockets.RemovalAndCleaning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMT2MetinRemovalAndSocketCleaningTest::RunTest(const FString&)
{
	FMetinSocketItemWorld F;
	if (!TestNotNull(TEXT("Inventory world"), F.Inventory) ||
		!TestNotNull(TEXT("Item registry"), F.Registry)) return false;

	const auto* Removal = Cast<UMT2ItemUseTemplate>(
		F.Registry->ResolveItemTemplateClass(25100).GetDefaultObject());
	const auto* Cleaner = Cast<UMT2ItemUseTemplate>(
		F.Registry->ResolveItemTemplateClass(39027).GetDefaultObject());
	const auto* Sword = Cast<UMT2ItemWeaponTemplate>(
		F.Registry->ResolveItemTemplateClass(10).GetDefaultObject());
	const auto* StoneA = Cast<UMT2ItemMetinStoneTemplate>(
		F.Registry->ResolveItemTemplateClass(28030).GetDefaultObject());
	const auto* StoneB = Cast<UMT2ItemMetinStoneTemplate>(
		F.Registry->ResolveItemTemplateClass(28031).GetDefaultObject());
	const UClass* BrokenStoneClass =
		UMT2GameplaySettings::Get().BrokenMetinStoneTemplate.LoadSynchronous();
	if (!TestNotNull(TEXT("Whole-stack extraction scroll is imported"), Removal) ||
		!TestNotNull(TEXT("Socket cleaner is imported"), Cleaner) ||
		!TestNotNull(TEXT("Weapon fixture"), Sword) ||
		!TestNotNull(TEXT("First stone"), StoneA) ||
		!TestNotNull(TEXT("Second stone"), StoneB) ||
		!TestNotNull(TEXT("Configured broken-stone template"), BrokenStoneClass)) return false;

	TArray<FMT2ItemSlot> Items;
	Items.SetNum(UMT2InventoryComponent::SlotCount);
	Items[0] = FMT2ItemSlot(25100, Removal->MakeInstanceData());
	Items[1] = FMT2ItemSlot(10, Sword->MakeInstanceData());
	Items[1].EditMetinSockets().SetNum(2);
	Items[1].EditMetinSockets()[0].Type = EMT2MetinSocketType::Silver;
	Items[1].EditMetinSockets()[1].Type = EMT2MetinSocketType::Gold;
	Items[1].EditMetinSockets()[0].Stone = StoneA->GetClass();
	Items[1].EditMetinSockets()[1].Stone = StoneB->GetClass();
	F.Inventory->RestoreItems(Items, {});

	const EMT2MetinSocketItemResult Extracted = F.Inventory->ApplyMetinSocketItem(
		0, 1, false, Items[0], Items[1]);
	TestEqual(TEXT("Full detachment succeeds"), Extracted, EMT2MetinSocketItemResult::Success);
	TestTrue(TEXT("Successful detachment consumes the scroll"), F.Inventory->GetSlots()[0].IsEmpty());
	TestEqual(TEXT("Both stones are returned"), F.Inventory->CountItemByVnum(StoneA->Vnum) +
		F.Inventory->CountItemByVnum(StoneB->Vnum), 2);
	TestTrue(TEXT("Removed stones leave broken sockets"), F.Inventory->GetSlots()[1].GetMetinSockets().ContainsByPredicate(
		[BrokenStoneClass](const FMT2MetinSocket& Socket)
		{
			return Socket.Stone.Get() == BrokenStoneClass;
		}));

	TArray<FMT2ItemSlot> CleanerItems;
	CleanerItems.SetNum(UMT2InventoryComponent::SlotCount);
	CleanerItems[0] = FMT2ItemSlot(39027, Cleaner->MakeInstanceData());
	CleanerItems[1] = F.Inventory->GetSlots()[1];
	F.Inventory->RestoreItems(CleanerItems, {});
	const EMT2MetinSocketItemResult Cleaned = F.Inventory->ApplyMetinSocketItem(
		0, 1, false, CleanerItems[0], CleanerItems[1]);
	TestEqual(TEXT("Cleaner succeeds on a broken socket"), Cleaned, EMT2MetinSocketItemResult::Success);
	TestTrue(TEXT("Successful cleaning consumes one cleaner"), F.Inventory->GetSlots()[0].IsEmpty());
	const auto& CleanedSockets = F.Inventory->GetSlots()[1].GetMetinSockets();
	TestEqual(TEXT("Cleaner removes broken sockets and compacts the survivors"), CleanedSockets.Num(), 2);
	for (const FMT2MetinSocket& Socket : CleanedSockets)
	{
		TestTrue(TEXT("Cleaner never leaves a broken marker"), Socket.Stone.Get() != BrokenStoneClass);
	}

	CleanerItems[0] = FMT2ItemSlot(39027, Cleaner->MakeInstanceData());
	CleanerItems[1] = F.Inventory->GetSlots()[1];
	F.Inventory->RestoreItems(CleanerItems, {});
	const FMT2ItemSlot CleanTargetSnapshot = F.Inventory->GetSlots()[1];
	const FMT2ItemSlot CleanerSnapshot = F.Inventory->GetSlots()[0];
	const EMT2MetinSocketItemResult NoBrokenSockets = F.Inventory->ApplyMetinSocketItem(
		0, 1, false, CleanerSnapshot, CleanTargetSnapshot);
	TestEqual(TEXT("Cleaning without broken sockets is rejected"), NoBrokenSockets,
		EMT2MetinSocketItemResult::NoBrokenSockets);
	TestEqual(TEXT("Rejected cleaner use does not consume the item"),
		F.Inventory->GetSlots()[0].Count, 1);
	TestEqual(TEXT("Rejected cleaner use leaves socket state unchanged"),
		F.Inventory->GetSlots()[1].GetMetinSockets().Num(), CleanTargetSnapshot.GetMetinSockets().Num());

	CleanerItems[0] = FMT2ItemSlot(39027, Cleaner->MakeInstanceData());
	CleanerItems[1] = F.Inventory->GetSlots()[1];
	CleanerItems[1].EditMetinSockets().SetNum(4);
	CleanerItems[1].EditMetinSockets()[0].Stone = StoneA->GetClass();
	CleanerItems[1].EditMetinSockets()[1].Stone = nullptr;
	CleanerItems[1].EditMetinSockets()[1].Value = 1;
	CleanerItems[1].EditMetinSockets()[2].Stone = StoneB->GetClass();
	CleanerItems[1].EditMetinSockets()[3].Value = Cast<UMT2ItemMetinStoneTemplate>(
		BrokenStoneClass->GetDefaultObject())->Vnum;
	F.Inventory->RestoreItems(CleanerItems, {});
	const FMT2ItemSlot LegacyMarkerSource = F.Inventory->GetSlots()[0];
	const FMT2ItemSlot LegacyMarkerTarget = F.Inventory->GetSlots()[1];
	const EMT2MetinSocketItemResult LegacyMarkerCleaned =
		F.Inventory->ApplyMetinSocketItem(0, 1, false, LegacyMarkerSource, LegacyMarkerTarget);
	TestEqual(TEXT("Cleaner recognizes a numeric legacy broken marker"), LegacyMarkerCleaned,
		EMT2MetinSocketItemResult::Success);
	const auto& LegacyCleanedSockets = F.Inventory->GetSlots()[1].GetMetinSockets();
	TestEqual(TEXT("Cleaner compacts stones across an open socket"),
		LegacyCleanedSockets[1].Stone.Get(), StoneB->GetClass());
	TestEqual(TEXT("Numeric broken marker is cleared"), LegacyCleanedSockets[3].Value, 0);
	TestTrue(TEXT("Numeric broken marker leaves an open socket"), !LegacyCleanedSockets[3].Stone);
	TestEqual(TEXT("Cleaning preserves the installed stone"), LegacyCleanedSockets[0].Stone.Get(),
		StoneA->GetClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SingleMetinRemovalTest,
	"Metin2.Items.MetinSockets.SingleStoneRemoval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMT2SingleMetinRemovalTest::RunTest(const FString&)
{
	FMetinSocketItemWorld F;
	if (!TestNotNull(TEXT("Inventory world"), F.Inventory) ||
		!TestNotNull(TEXT("Item registry"), F.Registry)) return false;

	const UMT2ItemUseTemplate* Scroll = Cast<UMT2ItemUseTemplate>(
		F.Registry->ResolveItemTemplateClass(71109).GetDefaultObject());
	const UMT2ItemWeaponTemplate* Weapon = Cast<UMT2ItemWeaponTemplate>(
		F.Registry->ResolveItemTemplateClass(19).GetDefaultObject());
	const UMT2ItemMetinStoneTemplate* FirstStone = Cast<UMT2ItemMetinStoneTemplate>(
		F.Registry->ResolveItemTemplateClass(28030).GetDefaultObject());
	const UMT2ItemMetinStoneTemplate* LastStone = Cast<UMT2ItemMetinStoneTemplate>(
		F.Registry->ResolveItemTemplateClass(28031).GetDefaultObject());
	if (!TestNotNull(TEXT("Single-removal scroll is imported"), Scroll) ||
		!TestNotNull(TEXT("Weapon fixture is imported"), Weapon) ||
		!TestNotNull(TEXT("First Metin stone is imported"), FirstStone) ||
		!TestNotNull(TEXT("Last Metin stone is imported"), LastStone)) return false;

	TArray<FMT2ItemSlot> Items;
	Items.SetNum(UMT2InventoryComponent::SlotCount);
	Items[0] = FMT2ItemSlot(71109, Scroll->MakeInstanceData(2));
	Items[1] = FMT2ItemSlot(19, Weapon->MakeInstanceData());
	Items[1].EditMetinSockets().SetNum(2);
	Items[1].EditMetinSockets()[0].Stone = FirstStone->GetClass();
	Items[1].EditMetinSockets()[1].Stone = LastStone->GetClass();
	F.Inventory->RestoreItems(Items, {});

	UMT2GameplaySettings* Settings = GetMutableDefault<UMT2GameplaySettings>();
	const TSoftClassPtr<UMT2ItemUseTemplate> PreviousConfiguredSingle =
		Settings->SingleMetinRemovalItemTemplate;
	Settings->SingleMetinRemovalItemTemplate = Scroll->GetClass();
	const EMT2UseSubType PreviousScrollSubtype = Scroll->UseSubType;
	const_cast<UMT2ItemUseTemplate*>(Scroll)->UseSubType = EMT2UseSubType::Detachment;
	UMT2ItemMetinRemovalTemplate* TypedRemoval =
		const_cast<UMT2ItemMetinRemovalTemplate*>(
			Cast<UMT2ItemMetinRemovalTemplate>(Scroll));
	const EMT2MetinRemovalMode PreviousMode = TypedRemoval
		? TypedRemoval->RemovalMode : EMT2MetinRemovalMode::SingleStone;
	if (TypedRemoval)
	{
		TypedRemoval->RemovalMode = EMT2MetinRemovalMode::AllStones;
		}

	const EMT2MetinSocketItemResult Result = F.Inventory->ApplyMetinSocketItem(
		0, 1, false, Items[0], Items[1]);
	TestEqual(TEXT("Single-stone extraction succeeds"), Result,
		EMT2MetinSocketItemResult::Success);
	TestEqual(TEXT("Exactly one scroll from the stack is consumed"),
		F.Inventory->GetSlots()[0].Count, 1);
	const TArray<FMT2MetinSocket>& Sockets = F.Inventory->GetSlots()[1].GetMetinSockets();
	TestTrue(TEXT("Most recently inserted stone is removed"), !Sockets[1].Stone);
	TestEqual(TEXT("Earlier stone remains installed"), Sockets[0].Stone.Get(), FirstStone->GetClass());
	TestEqual(TEXT("Removed stone is returned"), F.Inventory->CountItemByVnum(LastStone->Vnum), 1);

	const EMT2MetinSocketItemResult StaleRequest = F.Inventory->ApplyMetinSocketItem(
		0, 1, false, Items[0], Items[1]);
	TestEqual(TEXT("Replayed request with stale snapshots is rejected"), StaleRequest,
		EMT2MetinSocketItemResult::Invalid);
	TestEqual(TEXT("Stale request leaves earlier stone installed"),
		F.Inventory->GetSlots()[1].GetMetinSockets()[0].Stone.Get(), FirstStone->GetClass());

	const_cast<UMT2ItemUseTemplate*>(Scroll)->UseSubType = PreviousScrollSubtype;
	if (TypedRemoval) { TypedRemoval->RemovalMode = PreviousMode; }
	Settings->SingleMetinRemovalItemTemplate = PreviousConfiguredSingle;
	return true;
}

#endif
