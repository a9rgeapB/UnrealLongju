#include "Items/Templates/MT2ItemBlendTemplate.h"
#include "Items/Templates/MT2ItemFishTemplate.h"
#include "Items/Templates/MT2ItemSkillForgetTemplate.h"
#include "Items/State/MT2SkillForgetItemState.h"
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "AbilitySystemComponent.h"
#include "Abilities/MT2CoreAttributeSet.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Components/MT2HealthComponent.h"
#include "Components/MT2ManaComponent.h"
#include "Components/MT2StatusEffectComponent.h"
#include "Components/MT2StatusEffectDefinition.h"
#include "Core/MT2VnumRegistrySubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Items/MT2InventoryComponent.h"
#include "Items/MT2ItemTemplate.h"
#include "Items/MT2ItemUtils.h"
#include "Player/MT2PlayerState.h"
#include "Skills/MT2SkillComponent.h"
#include "Items/Legacy/MT2LegacyItemRecord.h"
#include "Npcs/MT2Campfire.h"
#include "Fishing/MT2FishingSettings.h"
#include "World/MT2MapAttributes.h"
#include "World/MT2MapPresentationActor.h"
#include "Components/BoxComponent.h"
#include "Items/Templates/MT2ItemRodTemplate.h"
#include "Quests/MT2QuestExpression.h"

namespace
{
	void SetLegacyValues(FMT2ItemInstanceData& Item, std::initializer_list<int32> Values)
	{
		int32 Index = 0;
		for (int32 Value : Values) MT2ItemUtils::SetNumericSocket(Item, Index++, Value);
		while (Index < 3) MT2ItemUtils::SetNumericSocket(Item, Index++, 0);
	}
	struct FItemBehaviorWorld
	{
		TStrongObjectPtr<UGameInstance> Instance{NewObject<UGameInstance>(GEngine)};
		UWorld* World = nullptr;
		AMT2PlayerCharacter* Player = nullptr;
		AMT2PlayerState* State = nullptr;
		UMT2InventoryComponent* Inventory = nullptr;
		UMT2VnumRegistrySubsystem* Registry = nullptr;
		FItemBehaviorWorld()
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
		~FItemBehaviorWorld()
		{
			Instance->Shutdown();
			if (World) { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); }
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2TypedItemQuestSocketTest, "Metin2.Items.Authoring.QuestToolSockets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2TypedItemQuestSocketTest::RunTest(const FString&)
{
	FItemBehaviorWorld F;
	if (!F.Inventory || !F.Registry) return false;
	const auto* Rod = Cast<UMT2ItemRodTemplate>(F.Registry->ResolveItemTemplateClass(27400).GetDefaultObject());
	if (!TestNotNull(TEXT("Rod fixture"), Rod)) return false;
	TArray<FMT2ItemSlot> Items; Items.SetNum(180);
	Items[0] = FMT2ItemSlot(27400, Rod->MakeInstanceData());
	Items[0].EditRodState().Proficiency = 37;
	Items[0].EditRodState().BaitPower = 40;
	F.Inventory->RestoreItems(Items, {});
	FMT2QuestContext Context; Context.Player = F.Player; Context.PlayerState = F.State; Context.EventItemSlot = 0;
	bool bOk = false;
	TestEqual(TEXT("Lua reads named rod bait"), FMT2QuestExpression::Evaluate(TEXT("item.get_socket(2)"), Context, bOk).AsInt(), 40);
	TestTrue(TEXT("Lua socket getter is recognized"), bOk);
	TestTrue(TEXT("Lua-compatible setter updates rod practice"), F.Inventory->SetItemSocketValue(0, 0, 38));
	const auto& Updated = F.Inventory->GetSlots()[0];
	TestNotNull(TEXT("Socket writes retain rod-family state"), Updated.FindState<FMT2FishingRodItemState>());
	TestEqual(TEXT("Socket write updates proficiency"), Updated.GetRodState().Proficiency, 38);
	TestEqual(TEXT("Unrelated bait is retained"), Updated.GetRodState().BaitPower, 40);
	TestFalse(TEXT("Unrelated equipment bonus edit is rejected safely"), F.Inventory->SetItemBonusValue(0, 0, static_cast<int32>(EMT2ItemBonusType::MaxHP), 100));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2TypedItemRealtimeExpiryTest, "Metin2.Items.Authoring.QuestRealtimeExpiry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2TypedItemRealtimeExpiryTest::RunTest(const FString&)
{
	FItemBehaviorWorld F;
	if (!F.Inventory || !F.Registry) return false;
	const auto* Template = F.Registry->ResolveItemTemplateClass(30270).GetDefaultObject();
	if (!TestNotNull(TEXT("Realtime quest item fixture"), Template)) return false;
	TArray<FMT2ItemSlot> Items; Items.SetNum(180);
	Items[0] = FMT2ItemSlot(30270, Template->MakeInstanceData());
	F.Inventory->RestoreItems(Items, {});
	const int32 Future = static_cast<int32>(FDateTime::UtcNow().ToUnixTimestamp() + 3600);
	TestTrue(TEXT("Quest supplies its expiry"), F.Inventory->SetItemSocketValue(0, 0, Future));
	TestTrue(TEXT("Realtime expiry starts"), F.Inventory->StartItemRealtimeExpiry(0));
	TestFalse(TEXT("Future-dated item is retained"), F.Inventory->GetSlots()[0].IsEmpty());
	TestEqual(TEXT("Named quest timestamp is used"), F.Inventory->GetSlots()[0].RealtimeExpiresAt, int64(Future));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SkillForgetItemTest, "Metin2.Items.SkillForget.LegacySkillDowngrade",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2SkillForgetItemTest::RunTest(const FString&)
{
	FItemBehaviorWorld F;
	if (!F.Inventory || !F.Registry || !F.State || !F.State->GetSkillComponent()) return false;
	const UMT2ItemSkillForgetTemplate* ForgetTemplate =
		Cast<UMT2ItemSkillForgetTemplate>(F.Registry->ResolveItemTemplateClass(70037).GetDefaultObject());
	UMT2SkillComponent* Skills = F.State->GetSkillComponent();
	Skills->RestorePersistedSkillGroup(static_cast<int32>(EMT2SkillGroup::First));
	const UMT2SkillDefinition* Skill = Skills->FindSkillDefinition(1);
	if (!TestNotNull(TEXT("Legacy skill-forget item is imported"), ForgetTemplate) ||
		!TestNotNull(TEXT("Legacy skill definition is available"), Skill)) return false;
	const FMT2ItemInstanceData GeneratedForgetItem = ForgetTemplate->MakeInstanceData();
	TestNotNull(TEXT("Generated item stores a typed skill-forget state"),
		GeneratedForgetItem.FindState<FMT2SkillForgetItemState>());
	TestTrue(TEXT("Generated legacy skill is in the general scroll pool"),
		GeneratedForgetItem.GetSkillVnum() >= 1 && GeneratedForgetItem.GetSkillVnum() <= 111);

	FMT2ItemSlot ForgetItem(70037, ForgetTemplate->MakeInstanceData());
	FMT2LegacyItemRecord LegacyItemState;
	LegacyItemState.SkillVnum = Skill->Vnum;
	ForgetItem.FamilyState.InitializeAs< FMT2LegacyItemRecord >(LegacyItemState);
	TArray<FMT2ItemSlot> Items;
	Items.SetNum(UMT2InventoryComponent::SlotCount);
	Items[0] = ForgetItem;
	F.Inventory->RestoreItems(Items, {});

	Skills->SetSkillLevel(Skill->Vnum, 1);
	const int32 CombatPointsBefore = F.State->GetUnspentSkillPoints();
	const int32 SubSkillPointsBefore = F.State->GetUnspentSubSkillPoints();
	const int32 HorseSkillPointsBefore = F.State->GetUnspentHorseSkillPoints();
	TestTrue(TEXT("An owned normal-mastery skill can be forgotten"), F.Inventory->UseItem(0));
	TestEqual(TEXT("Legacy item lowers exactly one skill level"), Skills->GetSkillLevel(Skill->Vnum), 0);
	switch (Skill->LegacySkillType.Type)
	{
	case EMT2LegacySkillClass::Support:
		TestEqual(TEXT("Support skill refund uses the legacy sub-skill pool"), F.State->GetUnspentSubSkillPoints(), SubSkillPointsBefore + 1);
		break;
	case EMT2LegacySkillClass::Horse:
		TestEqual(TEXT("Horse skill refund uses the legacy horse-skill pool"), F.State->GetUnspentHorseSkillPoints(), HorseSkillPointsBefore + 1);
		break;
	default:
		TestEqual(TEXT("Combat skill refund uses the legacy skill pool"), F.State->GetUnspentSkillPoints(), CombatPointsBefore + 1);
		break;
	}
	TestTrue(TEXT("Successful forget consumes the item"), F.Inventory->GetSlots()[0].IsEmpty());
	const FString SavedState = F.State->CapturePersistentStateJson_Implementation();
	EMT2SkillPointPool ExpectedPool = EMT2SkillPointPool::Combat;
	if (Skill->LegacySkillType.Type == EMT2LegacySkillClass::Support) ExpectedPool = EMT2SkillPointPool::SubSkill;
	else if (Skill->LegacySkillType.Type == EMT2LegacySkillClass::Horse) ExpectedPool = EMT2SkillPointPool::HorseSkill;
	TestTrue(TEXT("Point pool can be changed for persistence round-trip"), F.State->AwardSkillPoint(ExpectedPool));
	TestTrue(TEXT("Player state restores saved skill point pools"), F.State->ApplyPersistentStateJson_Implementation(SavedState));
	if (ExpectedPool == EMT2SkillPointPool::SubSkill)
		TestEqual(TEXT("Sub-skill refund persists"), F.State->GetUnspentSubSkillPoints(), SubSkillPointsBefore + 1);
	else if (ExpectedPool == EMT2SkillPointPool::HorseSkill)
		TestEqual(TEXT("Horse-skill refund persists"), F.State->GetUnspentHorseSkillPoints(), HorseSkillPointsBefore + 1);
	else
		TestEqual(TEXT("Combat-skill refund persists"), F.State->GetUnspentSkillPoints(), CombatPointsBefore + 1);

	Items[0] = FMT2ItemSlot();
	Items[1] = ForgetItem;
	F.Inventory->RestoreItems(Items, {});
	TestFalse(TEXT("An unlearned skill is not forgotten"), F.Inventory->UseItem(1));
	TestFalse(TEXT("Failed forget keeps the item"), F.Inventory->GetSlots()[1].IsEmpty());

	FMT2ItemSlot UnassignedItem(70037, FMT2ItemInstanceData());
	Items[1] = FMT2ItemSlot();
	Items[2] = UnassignedItem;
	F.Inventory->RestoreItems(Items, {});
	TestFalse(TEXT("An unassigned legacy item reports failure as in char_item.cpp"), F.Inventory->UseItem(2));
	TestTrue(TEXT("Unassigned item is removed"), F.Inventory->GetSlots()[2].IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2NumericItemSocketsTest, "Metin2.Items.NumericSockets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2NumericItemSocketsTest::RunTest(const FString&)
{
	FItemBehaviorWorld F;
	if (!TestNotNull(TEXT("Inventory fixture"), F.Inventory) || !F.Registry) return false;
	const auto* Book = F.Registry->ResolveItemTemplateClass(50322).GetDefaultObject();
	const auto* Pick = F.Registry->ResolveItemTemplateClass(29101).GetDefaultObject();
	if (!TestNotNull(TEXT("Imported polymorph book"), Book) || !TestNotNull(TEXT("Imported pickaxe"), Pick)) return false;
	TArray<FMT2ItemSlot> Items, Equipment;
	Items.SetNum(UMT2InventoryComponent::SlotCount); Equipment.SetNum(UMT2InventoryComponent::EquipmentCount);
	Items[0] = FMT2ItemSlot(50322, Book->MakeInstanceData());
	SetLegacyValues(Items[0], {101, 3, 12});
	Items[3] = FMT2ItemSlot(50322, Book->MakeInstanceData());
	SetLegacyValues(Items[3], {0, 0, 600});
	Equipment[4] = FMT2ItemSlot(29101, Pick->MakeInstanceData());
	Equipment[4].EditPickaxeState().Proficiency = 37;
	F.Inventory->RestoreItems(Items, Equipment);
	const FString Payload = F.State->CapturePersistentStateJson_Implementation();
	TestTrue(TEXT("Save payload restores"), F.State->ApplyPersistentStateJson_Implementation(Payload));
	for (int32 Index = 0; Index < 3; ++Index)
		TestEqual(TEXT("Numeric socket values survive save/load"), MT2ItemUtils::GetNumericSocket(F.Inventory->GetSlots()[0], Index), MT2ItemUtils::GetNumericSocket(Items[0], Index));
	TestEqual(TEXT("Sparse socket retains its index"), MT2ItemUtils::GetNumericSocket(F.Inventory->GetSlots()[3], 2), 600);
	TestTrue(TEXT("Numeric item is not decoded as Metin stones"), F.Inventory->GetSlots()[0].GetMetinSockets().IsEmpty());
	TestEqual(TEXT("Pickaxe practice survives save/load"), F.Inventory->GetEquipment()[4].GetPickaxeState().Proficiency, 37);
	TestFalse(TEXT("Fourth socket is rejected"), F.Inventory->SetItemSocketValue(0, 3, 100));
	TestFalse(TEXT("Negative socket is rejected"), F.Inventory->SetItemSocketValue(0, -1, 100));
	TestTrue(TEXT("Quest socket edit accepted"), F.Inventory->SetItemSocketValue(0, 1, 2));
	TestEqual(TEXT("Quest edit updates numeric state"), MT2ItemUtils::GetNumericSocket(F.Inventory->GetSlots()[0], 1), 2);
	FMT2ItemSlot Dropped;
	TestTrue(TEXT("Extract item for transfer"), F.Inventory->ExtractItemForDrop(0, 1, Dropped));
	TestEqual(TEXT("Transferred item retains its target"), MT2ItemUtils::GetNumericSocket(Dropped, 0), 101);
	TestEqual(TEXT("Reinsert transfer"), F.Inventory->AddItemSlotPartial(Dropped), 1);
	FMT2ItemSlot Different = Dropped;
	MT2ItemUtils::SetNumericSocket(Different, 1, 1);
	TestFalse(TEXT("Different socket progress cannot merge"), MT2ItemUtils::HaveSameInstanceData(Dropped, Different));
	Different = Dropped; SetLegacyValues(Different, {0, 0, 0});
	FMT2ItemSlot Fresh = Different; SetLegacyValues(Fresh, {});
	TestTrue(TEXT("Missing trailing zero sockets remain stack-compatible"), MT2ItemUtils::HaveSameInstanceData(Fresh, Different));
	FMT2ItemSlot Unknown;
	Unknown.EditMetinSockets().SetNum(1); Unknown.EditMetinSockets()[0].Value = 777;
	MT2ItemUtils::NormalizeNumericSockets(Unknown, nullptr);
	TestEqual(TEXT("Unavailable template never clears unknown socket data"), Unknown.GetMetinSockets().Num(), 1);
	if (Unknown.GetMetinSockets().Num() == 1) TestEqual(TEXT("Unavailable template preserves original payload"), Unknown.GetMetinSockets()[0].Value, 777);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2BlendRollsTest, "Metin2.Items.Blend.Rolls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2BlendRollsTest::RunTest(const FString&)
{
	TStrongObjectPtr<UMT2ItemBlendTemplate> Item(NewObject<UMT2ItemBlendTemplate>());
	Item->Blend.BonusType = EMT2ItemBonusType::CriticalChance;
	Item->Blend.Magnitudes = {8, 10, 12, 15, 20};
	Item->Blend.Durations = {60, 120, 180, 300, 600};
	TestTrue(TEXT("Complete distribution is valid"), Item->Blend.IsValid());
	for (const auto Pair : {TPair<int32, int32>(1,0), {10,0}, {11,1}, {30,1}, {31,2}, {70,2}, {71,3}, {90,3}, {91,4}, {100,4}})
		TestEqual(TEXT("Legacy weighted roll boundary"), Item->Blend.GetRollIndex(Pair.Key), Pair.Value);
	TestEqual(TEXT("Invalid low roll"), Item->Blend.GetRollIndex(0), INDEX_NONE);
	TestEqual(TEXT("Invalid high roll"), Item->Blend.GetRollIndex(101), INDEX_NONE);
	auto Instance = Item->MakeInstanceData();
	TestEqual(TEXT("Generated socket type"), MT2ItemUtils::GetNumericSocket(Instance, 0), 15);
	TestTrue(TEXT("Generated magnitude belongs to table"), Item->Blend.Magnitudes.Contains(MT2ItemUtils::GetNumericSocket(Instance, 1)));
	TestTrue(TEXT("Generated duration belongs to table"), Item->Blend.Durations.Contains(MT2ItemUtils::GetNumericSocket(Instance, 2)));
	SetLegacyValues(Instance, {15, 8, 600});
	Item->InitializeGeneratedInstance(Instance);
	TestEqual(TEXT("Receiving existing item cannot reroll magnitude"), MT2ItemUtils::GetNumericSocket(Instance, 1), 8);
	TestEqual(TEXT("Receiving existing item cannot reroll duration"), MT2ItemUtils::GetNumericSocket(Instance, 2), 600);
	Item->Blend.Weights[0] = -1;
	TestFalse(TEXT("Malformed negative weight rejected"), Item->Blend.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2BlendUseTest, "Metin2.Items.Blend.Authority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2BlendUseTest::RunTest(const FString&)
{
	FItemBehaviorWorld F;
	if (!TestNotNull(TEXT("Inventory fixture"), F.Inventory) || !F.Registry) return false;
	const auto* Template = Cast<UMT2ItemBlendTemplate>(F.Registry->ResolveItemTemplateClass(50821).GetDefaultObject());
	if (!TestNotNull(TEXT("Imported blend"), Template) || !TestTrue(TEXT("Imported blend distribution"), Template->Blend.IsValid())) return false;
	TArray<FMT2ItemSlot> Items; Items.SetNum(UMT2InventoryComponent::SlotCount);
	Items[0] = FMT2ItemSlot(50821, Template->MakeInstanceData(2)); Items[0].Count = 2; SetLegacyValues(Items[0], {15, 12, 180});
	Items[1] = Items[0]; Items[1].Count = 1; SetLegacyValues(Items[1], {15, 20, 600});
	Items[2] = Items[1]; Items[2].Vnum = 50822; SetLegacyValues(Items[2], {16, 10, 120});
	Items[3] = Items[1]; SetLegacyValues(Items[3], {999, 20, 600});
	F.Inventory->RestoreItems(Items, {});
	TestTrue(TEXT("Blend dispatch grants effect"), F.Inventory->UseItem(0));
	TestEqual(TEXT("Consumes exactly one"), F.Inventory->GetSlots()[0].Count, 1);
	TestFalse(TEXT("Same-apply blend cannot stack"), F.Inventory->UseItem(1));
	TestEqual(TEXT("Rejected duplicate is not consumed"), F.Inventory->GetSlots()[1].Count, 1);
	TestTrue(TEXT("Different blend applies coexist"), F.Inventory->UseItem(2));
	TestFalse(TEXT("Invalid socket apply rejected"), F.Inventory->UseItem(3));
	auto* Effects = F.Player->GetStatusEffectComponent();
	TestEqual(TEXT("Two distinct affects"), Effects->GetEffects().Num(), 2);
	const auto* Critical = Effects->GetEffects().FindByPredicate([](const FMT2StatusEffect& Effect) { return Effect.ApplyType == EMT2ItemBonusType::CriticalChance; });
	if (TestNotNull(TEXT("Critical blend effect"), Critical))
	{
		TestEqual(TEXT("Effect uses instance magnitude"), Critical->ApplyValue, 12);
		TestEqual(TEXT("Effect uses instance duration"), Critical->RemainingSeconds, 180);
	}
	TestTrue(TEXT("Blend inventory and affects restore"), F.State->ApplyPersistentStateJson_Implementation(F.State->CapturePersistentStateJson_Implementation()));
	TestFalse(TEXT("Restored active effect still prevents stacking"), F.Inventory->UseItem(1));
	TestEqual(TEXT("Death removes temporary blend effects"), Effects->RemoveEffectsOnDeath(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2BlendRestoreTest, "Metin2.Items.Blend.RestoreUninitialized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2BlendRestoreTest::RunTest(const FString&)
{
	FItemBehaviorWorld F;
	if (!TestNotNull(TEXT("Inventory fixture"), F.Inventory)) return false;
	const auto* Blend = Cast<UMT2ItemBlendTemplate>(F.Registry->ResolveItemTemplateClass(50821).GetDefaultObject());
	if (!TestNotNull(TEXT("Imported blend"), Blend)) return false;
	TArray<FMT2ItemSlot> Items, Equipment;
	Items.SetNum(UMT2InventoryComponent::SlotCount); Equipment.SetNum(UMT2InventoryComponent::EquipmentCount);
	Items[0] = FMT2ItemSlot(50821, FMT2ItemInstanceData());
	Items[1] = Items[0]; SetLegacyValues(Items[1], {15, 12, 180});
	F.Inventory->RestoreItems(Items, Equipment);
	const auto Migrated = F.Inventory->GetSlots()[0];
	TestEqual(TEXT("Empty saved blend receives its apply type"), MT2ItemUtils::GetNumericSocket(Migrated, 0), static_cast<int32>(Blend->Blend.BonusType));
	TestTrue(TEXT("Empty saved blend receives a valid magnitude"), Blend->Blend.Magnitudes.Contains(MT2ItemUtils::GetNumericSocket(Migrated, 1)));
	TestTrue(TEXT("Empty saved blend receives a valid duration"), Blend->Blend.Durations.Contains(MT2ItemUtils::GetNumericSocket(Migrated, 2)));
	TestEqual(TEXT("Existing magnitude is not rerolled"), MT2ItemUtils::GetNumericSocket(F.Inventory->GetSlots()[1], 1), 12);
	TestEqual(TEXT("Existing duration is not rerolled"), MT2ItemUtils::GetNumericSocket(F.Inventory->GetSlots()[1], 2), 180);
	F.State->ApplyPersistentStateJson_Implementation(F.State->CapturePersistentStateJson_Implementation());
	for (int32 Index = 0; Index < 3; ++Index)
		TestEqual(TEXT("Migrated roll remains stable after another load"), MT2ItemUtils::GetNumericSocket(F.Inventory->GetSlots()[0], Index), MT2ItemUtils::GetNumericSocket(Migrated, Index));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2ContinuousItemRecoveryTest, "Metin2.Items.ContinuousRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2ContinuousItemRecoveryTest::RunTest(const FString&)
{
	FItemBehaviorWorld F;
	if (!TestNotNull(TEXT("Player fixture"), F.Player)) return false;
	auto* Health = F.Player->GetHealthComponent();
	FMT2StatusEffect Effect;
	Effect.Kind = EMT2StatusEffectKind::ContinuousHealthRecovery;
	Effect.ApplyValue = 7; Effect.RemainingSeconds = 3;
	const auto* Definition = UMT2StatusEffectDefinition::Get(Effect.Kind);
	TestFalse(TEXT("Full HP does not discard timed recovery"), Definition->Tick(F.Player, Effect));
	Health->SetCurrentValue(80);
	TestFalse(TEXT("Second tick continues"), Definition->Tick(F.Player, Effect));
	TestEqual(TEXT("Flat amount recovered per tick"), Health->GetHealth(), 87.f);
	TestTrue(TEXT("Final tick ends fixed duration"), Definition->Tick(F.Player, Effect));
	TestEqual(TEXT("Final tick still grants recovery"), Health->GetHealth(), 94.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2EquipmentAmplificationTest, "Metin2.Items.Blend.EquipmentAmplification",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2EquipmentAmplificationTest::RunTest(const FString&)
{
	TArray<FMT2ItemSlot> Equipment;
	Equipment.SetNum(UMT2InventoryComponent::EquipmentCount);
	FMT2ItemBonus Strength;
	Strength.Type = EMT2ItemBonusType::Strength; Strength.Value = 15;
	for (const auto Slot : {EMT2ItemWearFlag::Body, EMT2ItemWearFlag::Weapon, EMT2ItemWearFlag::CostumeBody})
	{
		auto& Item = Equipment[static_cast<int32>(Slot)];
		Item.Vnum = 19; Item.Count = 1; Item.EditBonuses().Add(Strength);
	}
	const TArray<EMT2ItemWearFlag> Slots = {EMT2ItemWearFlag::Body, EMT2ItemWearFlag::Weapon, EMT2ItemWearFlag::Body};
	auto Result = MT2ItemUtils::GetAmplifiedEquipmentAttributes(Equipment, Slots, 10);
	TestEqual(TEXT("Sum then truncate; duplicate and costume slots excluded"), Result.FindRef(Strength.Type), 3);
	TestTrue(TEXT("Zero amplification has no bonuses"), MT2ItemUtils::GetAmplifiedEquipmentAttributes(Equipment, Slots, 0).IsEmpty());
	TestEqual(TEXT("Costume attributes have independent eligible slots"),
		MT2ItemUtils::GetAmplifiedEquipmentAttributes(Equipment, {EMT2ItemWearFlag::CostumeBody}, 20).FindRef(Strength.Type), 3);
	Equipment[static_cast<int32>(EMT2ItemWearFlag::Weapon)].EditBonuses()[0].Value = -30;
	TestEqual(TEXT("Negative totals truncate toward zero"),
		MT2ItemUtils::GetAmplifiedEquipmentAttributes(Equipment, Slots, 10).FindRef(Strength.Type), -1);
	FItemBehaviorWorld F;
	if (!TestNotNull(TEXT("Player fixture"), F.Player)) return false;
	Equipment[static_cast<int32>(EMT2ItemWearFlag::Weapon)].EditBonuses()[0].Value = 15;
	TArray<FMT2ItemSlot> Items; Items.SetNum(UMT2InventoryComponent::SlotCount);
	F.Inventory->RestoreItems(Items, Equipment);
	FMT2StatusEffect Effect;
	Effect.Type = FMT2AffectIdentifier::FromBuiltin(EMT2BuiltinAffect::Blend);
	Effect.ApplyType = EMT2ItemBonusType::Energy; Effect.ApplyValue = 10; Effect.RemainingSeconds = 60;
	F.Player->GetStatusEffectComponent()->AddEffect(Effect, false);
	Effect.ApplyType = EMT2ItemBonusType::CostumeAttributeBonus; Effect.ApplyValue = 20;
	F.Player->GetStatusEffectComponent()->AddEffect(Effect, false);
	F.Player->RecalculateEquipmentStats();
	TestEqual(TEXT("Real stat recomputation applies independent energy and costume amplification"),
		F.Player->GetItemApplyBonus(Strength.Type), 51);
	F.Player->GetStatusEffectComponent()->RemoveEffectsByType(Effect.Type);
	F.Player->RecalculateEquipmentStats();
	TestEqual(TEXT("Removing amplification restores original equipment attributes"), F.Player->GetItemApplyBonus(Strength.Type), 45);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2ItemExchangeTest, "Metin2.Items.Transactions.Exchange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2ItemExchangeTest::RunTest(const FString&)
{
	FItemBehaviorWorld F;
	if (!TestNotNull(TEXT("Inventory fixture"), F.Inventory)) return false;
	TArray<FMT2ItemSlot> Items, Equipment;
	Items.SetNum(UMT2InventoryComponent::SlotCount); Equipment.SetNum(UMT2InventoryComponent::EquipmentCount);
	Items[0] = FMT2ItemSlot(27803, FMT2ItemInstanceData());
	F.Inventory->RestoreItems(Items, Equipment);
	TestTrue(TEXT("Fish converts into its roasted result"), F.Inventory->ExchangeItemAtSlot(0, 1, 27863, 1));
	TestEqual(TEXT("Source consumed once"), F.Inventory->CountItemByVnum(27803), 0);
	TestEqual(TEXT("Reward delivered once"), F.Inventory->CountItemByVnum(27863), 1);
	TestFalse(TEXT("Missing reward rejects conversion"), F.Inventory->ExchangeItemAtSlot(0, 1, MAX_int32, 1));
	TestEqual(TEXT("Invalid reward preserves source"), F.Inventory->CountItemByVnum(27863), 1);
	for (auto& Item : Items) Item = FMT2ItemSlot(27803, FMT2ItemInstanceData());
	Items[0].Count = 2;
	F.Inventory->RestoreItems(Items, Equipment);
	TestFalse(TEXT("No room for reward rejects entire exchange"), F.Inventory->ExchangeItemAtSlot(0, 1, 27863, 1));
	TestEqual(TEXT("Full inventory retains source stack"), F.Inventory->GetSlots()[0].Count, 2);
	TestTrue(TEXT("No-reward outcome consumes exactly one"), F.Inventory->ExchangeItemAtSlot(0, 1, 0, 0));
	TestEqual(TEXT("One source left"), F.Inventory->GetSlots()[0].Count, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2CampfireRoastingTest, "Metin2.Items.Fish.Campfire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2CampfireRoastingTest::RunTest(const FString&)
{
	FItemBehaviorWorld F;
	if (!TestNotNull(TEXT("Inventory fixture"), F.Inventory)) return false;
	TArray<FMT2ItemSlot> Items, Equipment;
	Items.SetNum(UMT2InventoryComponent::SlotCount); Equipment.SetNum(UMT2InventoryComponent::EquipmentCount);
	Items[0] = FMT2ItemSlot(27803, FMT2ItemInstanceData());
	Items[0].Count = 3;
	Items[4] = FMT2ItemSlot(27600, FMT2ItemInstanceData());
	F.Inventory->RestoreItems(Items, Equipment);
	TestFalse(TEXT("Campfire cannot be placed outside a registered map"), F.Inventory->UseItem(4));
	TestEqual(TEXT("Rejected placement preserves item"), F.Inventory->CountItemByVnum(27600), 1);
	auto* Fire = F.World->SpawnActor<AMT2Campfire>();
	if (!TestNotNull(TEXT("Campfire NPC"), Fire)) return false;
	Fire->SetActorLocation(F.Player->GetActorLocation());
	Fire->InitializeComponents();
	Fire->DispatchBeginPlay();
	TestEqual(TEXT("Authoritative fire expires at configured lifetime"), Fire->GetLifeSpan(), GetDefault<UMT2FishingSettings>()->CampfireLifetime);
	TestFalse(TEXT("Fire is not attackable"), Fire->IsAttackable());
	TestFalse(TEXT("Fire cannot be knocked back"), Fire->CanBeKnockedBack());
	TestTrue(TEXT("Roast whole stack"), Fire->Roast(*F.Player, 0));
	TestEqual(TEXT("All raw fish consumed"), F.Inventory->CountItemByVnum(27803), 0);
	TestEqual(TEXT("All cooked fish awarded"), F.Inventory->CountItemByVnum(27863), 3);
	TestFalse(TEXT("Cooked fish cannot roast twice"), Fire->Roast(*F.Player, 0));
	F.Inventory->RestoreItems(Items, Equipment);
	Fire->SetActorLocation(F.Player->GetActorLocation() + FVector(100000, 0, 0));
	TestFalse(TEXT("Remote campfire rejected"), Fire->Roast(*F.Player, 0));
	TestEqual(TEXT("Remote use preserves fish"), F.Inventory->CountItemByVnum(27803), 3);
	auto* Map = F.World->SpawnActor<AMT2MapPresentationActor>();
	Map->WorldMin = FVector2D(-1000, -1000); Map->WorldMax = FVector2D(1000, 1000);
	Map->Attributes.Size = FIntPoint(1, 1); Map->Attributes.Flags = {MT2MapAttribute::Water};
	F.World->GetSubsystem<UMT2MapAttributeSubsystem>()->RegisterMap(Map);
	TestFalse(TEXT("Water rejects placement"), F.Inventory->UseItem(4));
	Map->Attributes.Flags[0] = 0;
	auto* Ground = F.World->SpawnActor<AActor>();
	auto* Box = NewObject<UBoxComponent>(Ground);
	Ground->SetRootComponent(Box); Ground->AddInstanceComponent(Box);
	Box->SetBoxExtent(FVector(1000, 1000, 10));
	Box->SetCollisionObjectType(ECC_WorldStatic);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Box->SetCollisionResponseToAllChannels(ECR_Block);
	Box->RegisterComponent(); Ground->SetActorLocation(FVector(0, 0, -10));
	TestTrue(TEXT("Valid dry ground accepts placement"), F.Inventory->UseItem(4));
	TestEqual(TEXT("Successful placement consumes one campfire"), F.Inventory->CountItemByVnum(27600), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2FishOpeningTest, "Metin2.Items.Fish.Opening",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2FishOpeningTest::RunTest(const FString&)
{
	FItemBehaviorWorld F;
	if (!TestNotNull(TEXT("Inventory fixture"), F.Inventory)) return false;
	TArray<FMT2ItemSlot> Items, Equipment;
	Items.SetNum(UMT2InventoryComponent::SlotCount); Equipment.SetNum(UMT2InventoryComponent::EquipmentCount);
	Items[0] = FMT2ItemSlot(27803, FMT2ItemInstanceData());
	Items[0].Count = 2;
	F.Inventory->RestoreItems(Items, Equipment);
	TStrongObjectPtr<UMT2ItemFishTemplate> Fish(NewObject<UMT2ItemFishTemplate>());
	Fish->Vnum = 27803;
	auto& Outcome = Fish->Fish.UseOutcomes.AddDefaulted_GetRef(); Outcome.ItemVnum = 27799; Outcome.Weight = 10000;
	TestEqual(TEXT("Accepted opening owns its atomic consumption"), Fish->ExecuteUse(*F.Player, *F.Inventory, 0), EMT2ItemUseExecution::Accepted);
	TestEqual(TEXT("One live fish remains"), F.Inventory->CountItemByVnum(27803), 1);
	TestEqual(TEXT("One bone awarded"), F.Inventory->CountItemByVnum(27799), 1);
	Fish->FishSubType = EMT2FishSubType::Dead;
	TestEqual(TEXT("Dead fish has no right-click opening"), Fish->ExecuteUse(*F.Player, *F.Inventory, 0), EMT2ItemUseExecution::Rejected);
	TestEqual(TEXT("Rejected opening preserves fish"), F.Inventory->CountItemByVnum(27803), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2ConsumableAffectsTest, "Metin2.Items.Consumables.Affects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2ConsumableAffectsTest::RunTest(const FString&)
{
	FItemBehaviorWorld F;
	if (!TestNotNull(TEXT("Inventory fixture"), F.Inventory)) return false;
	TArray<FMT2ItemSlot> Items, Equipment;
	Items.SetNum(UMT2InventoryComponent::SlotCount); Equipment.SetNum(UMT2InventoryComponent::EquipmentCount);
	Items[0] = FMT2ItemSlot(27870, FMT2ItemInstanceData());
	Items[1] = FMT2ItemSlot(27053, FMT2ItemInstanceData()); Items[1].Count = 3;
	Items[2] = FMT2ItemSlot(27874, FMT2ItemInstanceData()); Items[2].Count = 2;
	F.Inventory->RestoreItems(Items, Equipment);
	TestTrue(TEXT("Grilled fish grants strength"), F.Inventory->UseItem(0));
	auto* Effects = F.Player->GetStatusEffectComponent();
	TestEqual(TEXT("Correct strength magnitude"), Effects->GetEffectValueByType(FMT2AffectIdentifier::FromBuiltin(EMT2BuiltinAffect::Strength)), 10);
	TestTrue(TEXT("Attack speed potion applies"), F.Inventory->UseItem(1));
	TestTrue(TEXT("Active speed potion refreshes"), F.Inventory->UseItem(1));
	TestEqual(TEXT("Refresh is not additive stacking"), Effects->GetEffects().Num(), 2);
	FMT2StatusEffect Poison;
	Poison.Type = FMT2AffectIdentifier::FromBuiltin(EMT2BuiltinAffect::Poison); Poison.RemainingSeconds = 10;
	Effects->AddEffect(Poison);
	Poison.Type = FMT2AffectIdentifier::FromSkillVnum(80);
	Effects->AddEffect(Poison);
	TestTrue(TEXT("Cleansing item accepted"), F.Inventory->UseItem(2));
	TestEqual(TEXT("Only harmful affects removed"), Effects->GetEffects().Num(), 2);
	TestTrue(TEXT("Legacy cleansing still consumes without harmful affects"), F.Inventory->UseItem(2));
	TestEqual(TEXT("Both cleansing items consumed"), F.Inventory->CountItemByVnum(27874), 0);
	F.Player->GetHealthComponent()->SetCurrentValue(0);
	TestFalse(TEXT("Dead player cannot use potion"), F.Inventory->UseItem(1));
	TestEqual(TEXT("Rejected potion not consumed"), F.Inventory->GetSlots()[1].Count, 1);
	return true;
}

#endif
