#include "Items/Templates/MT2ItemUseRecipeTemplate.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"
#include "AbilitySystemComponent.h"
#include "Abilities/MT2CoreAttributeSet.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Components/MT2HealthComponent.h"
#include "Components/MT2ManaComponent.h"
#include "Core/MT2VnumRegistry.h"
#include "Core/MT2VnumRegistrySubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Items/MT2InventoryComponent.h"
#include "Items/Templates/MT2ItemFishTemplate.h"
#include "Items/Templates/MT2ItemUseCreateStoneTemplate.h"
#include "Items/Templates/MT2ItemUseTemplate.h"
#include "Items/Templates/MT2ItemUseClearTemplate.h"
#include "Player/MT2PlayerState.h"

namespace
{
	struct FRecipeItemWorld
	{
		TStrongObjectPtr<UGameInstance> Instance{NewObject<UGameInstance>(GEngine)};
		UWorld* World = nullptr;
		AMT2PlayerCharacter* Player = nullptr;
		AMT2PlayerState* State = nullptr;
		UMT2InventoryComponent* Inventory = nullptr;
		UMT2VnumRegistrySubsystem* Registry = nullptr;

		FRecipeItemWorld()
		{
			Instance->InitializeStandalone();
			World = Instance->GetWorld();
			if (!World) return;
			Player = World->SpawnActor<AMT2PlayerCharacter>();
			State = World->SpawnActor<AMT2PlayerState>();
			if (!Player || !State) return;
			Player->SetPlayerState(State);
			auto* Ability = State->GetAbilitySystemComponent();
			Ability->AddAttributeSetSubobject(State->GetCoreAttributes());
			Ability->InitAbilityActorInfo(State, Player);
			Ability->SetNumericAttributeBase(UMT2CoreAttributeSet::GetMaxHealthAttribute(), 100);
			Ability->SetNumericAttributeBase(UMT2CoreAttributeSet::GetHealthAttribute(), 100);
			Ability->SetNumericAttributeBase(UMT2CoreAttributeSet::GetMaxManaAttribute(), 100);
			Ability->SetNumericAttributeBase(UMT2CoreAttributeSet::GetManaAttribute(), 100);
			Player->GetHealthComponent()->InitializeWithAbilitySystem(Ability);
			Player->GetManaComponent()->InitializeWithAbilitySystem(Ability);
			Inventory = Player->GetInventoryComponent();
			Registry = Instance->GetSubsystem<UMT2VnumRegistrySubsystem>();
		}

		~FRecipeItemWorld()
		{
			Instance->Shutdown();
			if (World) { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); }
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2ItemRecipeUseTest,
	"Metin2.Items.Recipe.LegacyTransaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMT2ItemRecipeUseTest::RunTest(const FString&)
{
	FRecipeItemWorld Fixture;
	if (!Fixture.Inventory || !Fixture.Registry || !Fixture.Player || !Fixture.State) return false;

	const TSubclassOf<UMT2ItemTemplate> RecipeClass = UMT2ItemUseRecipeTemplate::StaticClass();
	const TSubclassOf<UMT2ItemTemplate> IngredientClass = UMT2ItemUseClearTemplate::StaticClass();
	const TSubclassOf<UMT2ItemTemplate> BottleClass = UMT2ItemFishTemplate::StaticClass();
	const TSubclassOf<UMT2ItemTemplate> ResultClass = UMT2ItemUseTemplate::StaticClass();
	auto* Recipe = const_cast<UMT2ItemUseRecipeTemplate*>(RecipeClass->GetDefaultObject<UMT2ItemUseRecipeTemplate>());
	auto* Ingredient = const_cast<UMT2ItemTemplate*>(IngredientClass->GetDefaultObject<UMT2ItemTemplate>());
	auto* Bottle = const_cast<UMT2ItemTemplate*>(BottleClass->GetDefaultObject<UMT2ItemTemplate>());
	auto* Result = const_cast<UMT2ItemTemplate*>(ResultClass->GetDefaultObject<UMT2ItemTemplate>());
	if (!TestNotNull(TEXT("Recipe template CDO"), Recipe) || !Ingredient || !Bottle || !Result) return false;

	constexpr int32 RecipeVnum = 99001;
	constexpr int32 IngredientVnum = 99002;
	constexpr int32 BottleVnum = 99003;
	constexpr int32 ResultVnum = 99004;
	const int32 OldRecipeVnum = Recipe->Vnum;
	const int32 OldIngredientVnum = Ingredient->Vnum;
	const int32 OldBottleVnum = Bottle->Vnum;
	const int32 OldResultVnum = Result->Vnum;
	const int32 OldIngredientFlags = Ingredient->Flags;
	const int32 OldIngredientMaxStack = Ingredient->MaxStackSize;
	const int32 OldResultInventorySize = Result->InventorySize;
	const auto OldRecipe = Recipe->Recipe;
	ON_SCOPE_EXIT
	{
		Recipe->Vnum = OldRecipeVnum;
		Ingredient->Vnum = OldIngredientVnum;
		Bottle->Vnum = OldBottleVnum;
		Result->Vnum = OldResultVnum;
		Ingredient->Flags = OldIngredientFlags;
		Ingredient->MaxStackSize = OldIngredientMaxStack;
		Result->InventorySize = OldResultInventorySize;
		Recipe->Recipe = OldRecipe;
	};
	Recipe->Vnum = RecipeVnum;
	Ingredient->Vnum = IngredientVnum;
	Bottle->Vnum = BottleVnum;
	Result->Vnum = ResultVnum;
	Ingredient->Flags = 0;
	Ingredient->MaxStackSize = 1;
	Result->InventorySize = 3;
	Recipe->Recipe.FirstIngredientTemplate = IngredientClass;
	Recipe->Recipe.FirstIngredientCount = 3;
	Recipe->Recipe.SecondIngredientTemplate = nullptr;
	Recipe->Recipe.SecondIngredientCount = 0;
	Recipe->Recipe.RequiredBottleTemplate = BottleClass;
	Recipe->Recipe.ResultItemTemplate = ResultClass;
	Recipe->Recipe.SuccessPercent = 100;

	UMT2VnumRegistry* RegistryAsset = Fixture.Registry->GetRegistry();
	if (!TestNotNull(TEXT("VNUM registry asset"), RegistryAsset)) return false;
	const auto OriginalItemEntries = RegistryAsset->GetItemTemplates();
	const auto OriginalMobEntries = RegistryAsset->GetMobClasses();
	ON_SCOPE_EXIT
	{
		RegistryAsset->SetEntries(TMap<int32, TSoftClassPtr<AMT2Mob>>(OriginalMobEntries),
			TMap<int32, TSoftClassPtr<UMT2ItemTemplate>>(OriginalItemEntries));
	};
	auto ItemEntries = RegistryAsset->GetItemTemplates();
	const auto SoftClass = [](TSubclassOf<UMT2ItemTemplate> ItemClass)
	{
		return TSoftClassPtr<UMT2ItemTemplate>(FSoftObjectPath(ItemClass->GetPathName()));
	};
	ItemEntries.Add(RecipeVnum, SoftClass(RecipeClass));
	ItemEntries.Add(IngredientVnum, SoftClass(IngredientClass));
	ItemEntries.Add(BottleVnum, SoftClass(BottleClass));
	ItemEntries.Add(ResultVnum, SoftClass(ResultClass));
	auto MobEntries = RegistryAsset->GetMobClasses();
	RegistryAsset->SetEntries(MoveTemp(MobEntries), MoveTemp(ItemEntries));

	TestTrue(TEXT("Recipe item is present"), Fixture.Inventory->AddItemByVnum(RecipeVnum, 1));
	TestTrue(TEXT("Three ingredients are present"), Fixture.Inventory->AddItemByVnum(IngredientVnum, 3));
	TestTrue(TEXT("One required bottle is present"), Fixture.Inventory->AddItemByVnum(BottleVnum, 1));
	const int32 RecipeSlot = Fixture.Inventory->GetSlots().IndexOfByPredicate(
		[RecipeVnum](const FMT2ItemSlot& Slot) { return Slot.Vnum == RecipeVnum; });
	TestTrue(TEXT("Legacy recipe succeeds"), Fixture.Inventory->UseItem(RecipeSlot));
	TestEqual(TEXT("Recipe item is not consumed"), Fixture.Inventory->CountItemByVnum(RecipeVnum), 1);
	TestEqual(TEXT("All recipe ingredients are consumed"), Fixture.Inventory->CountItemByVnum(IngredientVnum), 0);
	TestEqual(TEXT("Bottle is consumed"), Fixture.Inventory->CountItemByVnum(BottleVnum), 0);
	TestEqual(TEXT("One configured result is granted"), Fixture.Inventory->CountItemByVnum(ResultVnum), 1);

	Recipe->Recipe.SuccessPercent = 0;
	TestTrue(TEXT("Failure-case ingredients are present"), Fixture.Inventory->AddItemByVnum(IngredientVnum, 3));
	TestFalse(TEXT("Missing bottle rejects without consuming ingredients"), Fixture.Inventory->UseItem(RecipeSlot));
	TestEqual(TEXT("Missing bottle preserves all ingredients"), Fixture.Inventory->CountItemByVnum(IngredientVnum), 3);
	TestTrue(TEXT("Required bottle is then present"), Fixture.Inventory->AddItemByVnum(BottleVnum, 1));
	TestTrue(TEXT("A failed roll still completes the recipe attempt"), Fixture.Inventory->UseItem(RecipeSlot));
	TestEqual(TEXT("Failed roll consumes ingredients"), Fixture.Inventory->CountItemByVnum(IngredientVnum), 0);
	TestEqual(TEXT("Failed roll consumes the bottle"), Fixture.Inventory->CountItemByVnum(BottleVnum), 0);
	TestEqual(TEXT("Failed roll does not grant output"), Fixture.Inventory->CountItemByVnum(ResultVnum), 1);
	TestEqual(TEXT("Failed roll still retains the recipe item"), Fixture.Inventory->CountItemByVnum(RecipeVnum), 1);

	Recipe->Recipe.SuccessPercent = 100;
	TestTrue(TEXT("Capacity-case ingredients are present"), Fixture.Inventory->AddItemByVnum(IngredientVnum, 3));
	TestTrue(TEXT("Capacity-case bottle is present"), Fixture.Inventory->AddItemByVnum(BottleVnum, 1));
	for (int32 Index = 0; Index < UMT2InventoryComponent::SlotCount; ++Index)
	{
		if (!Fixture.Inventory->AddItemByVnum(IngredientVnum, 1)) break;
	}
	const int32 IngredientsBeforeCapacityAttempt = Fixture.Inventory->CountItemByVnum(IngredientVnum);
	TestFalse(TEXT("Successful output with no free slot rejects the transaction"), Fixture.Inventory->UseItem(RecipeSlot));
	TestEqual(TEXT("No-space rejection preserves ingredients"),
		Fixture.Inventory->CountItemByVnum(IngredientVnum), IngredientsBeforeCapacityAttempt);
	TestEqual(TEXT("No-space rejection preserves bottle"), Fixture.Inventory->CountItemByVnum(BottleVnum), 1);
	TestEqual(TEXT("No-space rejection does not create another result"), Fixture.Inventory->CountItemByVnum(ResultVnum), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2CreateStoneUseTest,
	"Metin2.Items.CreateStone.LegacyChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMT2CreateStoneUseTest::RunTest(const FString&)
{
	FRecipeItemWorld Fixture;
	if (!Fixture.Inventory || !Fixture.Registry || !Fixture.Player || !Fixture.State) return false;
	constexpr int32 SourceVnum = 99005;
	const TSubclassOf<UMT2ItemTemplate> SourceClass = UMT2ItemUseCreateStoneTemplate::StaticClass();
	auto* CreateStone = const_cast<UMT2ItemUseCreateStoneTemplate*>(
		SourceClass->GetDefaultObject<UMT2ItemUseCreateStoneTemplate>());
	if (!TestNotNull(TEXT("Create-stone template CDO"), CreateStone)) return false;
	const int32 OriginalSourceVnum = CreateStone->Vnum;
	const auto OriginalResults = CreateStone->StoneResults;
	ON_SCOPE_EXIT
	{
		CreateStone->Vnum = OriginalSourceVnum;
		CreateStone->StoneResults = OriginalResults;
	};
	CreateStone->Vnum = SourceVnum;
	CreateStone->StoneResults.Reset();

	UMT2VnumRegistry* RegistryAsset = Fixture.Registry->GetRegistry();
	if (!TestNotNull(TEXT("VNUM registry asset"), RegistryAsset)) return false;
	const auto OriginalItemEntries = RegistryAsset->GetItemTemplates();
	const auto OriginalMobEntries = RegistryAsset->GetMobClasses();
	ON_SCOPE_EXIT
	{
		RegistryAsset->SetEntries(TMap<int32, TSoftClassPtr<AMT2Mob>>(OriginalMobEntries),
			TMap<int32, TSoftClassPtr<UMT2ItemTemplate>>(OriginalItemEntries));
	};
	auto ItemEntries = RegistryAsset->GetItemTemplates();
	const auto SoftClass = [](TSubclassOf<UMT2ItemTemplate> ItemClass)
	{
		return TSoftClassPtr<UMT2ItemTemplate>(FSoftObjectPath(ItemClass->GetPathName()));
	};
	TArray<TSubclassOf<UMT2ItemTemplate>> RangeClasses;
	TArray<int32> OriginalRanges;
	ON_SCOPE_EXIT
	{
		for (int32 Index = 0; Index < RangeClasses.Num(); ++Index)
		{
			RangeClasses[Index]->GetDefaultObject<UMT2ItemTemplate>()->VnumRange = OriginalRanges[Index];
		}
	};
	for (int32 Vnum = 28000; Vnum <= 28013; ++Vnum)
	{
		const int32 BaseVnum = 28000 + ((Vnum - 28000) / 4) * 4;
		const TSubclassOf<UMT2ItemTemplate> ResultClass = Fixture.Registry->ResolveItemTemplateClass(BaseVnum);
		if (!TestNotNull(*FString::Printf(TEXT("Legacy stone base %d"), BaseVnum), ResultClass.Get())) return false;
		UMT2ItemTemplate* ResultDefaults = ResultClass->GetDefaultObject<UMT2ItemTemplate>();
		if (!RangeClasses.Contains(ResultClass))
		{
			RangeClasses.Add(ResultClass);
			OriginalRanges.Add(ResultDefaults->VnumRange);
		}
		ResultDefaults->VnumRange = FMath::Max(ResultDefaults->VnumRange, 3);
		FMT2ItemCreateStoneResult& Result = CreateStone->StoneResults.AddDefaulted_GetRef();
		Result.ItemTemplate = ResultClass;
		Result.VnumRangeOffset = Vnum - BaseVnum;
		ItemEntries.Add(Vnum, SoftClass(ResultClass));
	}
	ItemEntries.Add(SourceVnum, SoftClass(SourceClass));
	auto MobEntries = RegistryAsset->GetMobClasses();
	RegistryAsset->SetEntries(MoveTemp(MobEntries), MoveTemp(ItemEntries));

	TestTrue(TEXT("Create-stone item is present"), Fixture.Inventory->AddItemByVnum(SourceVnum, 1));
	const int32 SourceSlot = Fixture.Inventory->GetSlots().IndexOfByPredicate(
		[SourceVnum](const FMT2ItemSlot& Slot) { return Slot.Vnum == SourceVnum; });
	TestTrue(TEXT("Legacy create-stone item opens"), Fixture.Inventory->UseItem(SourceSlot));
	TestEqual(TEXT("Create-stone item is consumed"), Fixture.Inventory->CountItemByVnum(SourceVnum), 0);
	int32 GrantedStones = 0;
	for (int32 Vnum = 28000; Vnum <= 28013; ++Vnum)
	{
		GrantedStones += Fixture.Inventory->CountItemByVnum(Vnum);
	}
	TestEqual(TEXT("Exactly one random legacy stone is granted"), GrantedStones, 1);
	return true;
}

#endif
