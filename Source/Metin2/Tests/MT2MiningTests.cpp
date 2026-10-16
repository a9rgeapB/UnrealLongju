#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/MT2VnumRegistrySubsystem.h"
#include "Items/MT2ItemTemplate.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"
#include "Mining/MT2MiningRules.h"
#include "Mining/MT2MiningComponent.h"
#include "Mining/MT2OreVein.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Components/MT2HealthComponent.h"
#include "Items/MT2InventoryComponent.h"
#include "Player/MT2PlayerState.h"
#include "Npcs/MT2Npc.h"
#include "Abilities/MT2CoreAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/MT2StatusEffectComponent.h"
#include "Items/MT2WorldItem.h"
#include "EngineUtils.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/MT2CharacterAnimInstance.h"
#include "Config/MT2PathSettings.h"
#include "Characters/MT2CharacterAppearanceSettings.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Animation/AnimMontage.h"
#include "Quests/MT2QuestRegistrySubsystem.h"
#include "Quests/MT2QuestManagerComponent.h"
#include "Quests/MT2Quest.h"
#include "Quests/MT2QuestNode.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2MiningVeinContentTest, "Metin2.Mining.ImportedVeins", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2MiningVeinContentTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GEngine)); GI->InitializeStandalone();
	UWorld* World=GI->GetWorld();
	ON_SCOPE_EXIT { World->EndPlay(EEndPlayReason::Quit); GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	World->InitializeActorsForPlay(FURL()); World->GetWorldSettings()->NotifyBeginPlay();
	auto* Registry=GI->GetSubsystem<UMT2VnumRegistrySubsystem>();
	const int32 Vnums[]={20047,20048,20049,20050,20051,20052,20053,20054,20055,20056,20057,20058,20059,30301,30302,30303,30304,30305};
	for (const int32 Vnum : Vnums)
	{
		UClass* Class=Registry->ResolveMobClass(Vnum);
		if (!TestTrue(FString::Printf(TEXT("Vein %d resolves to an interactable static ore NPC"),Vnum),Class && Class->IsChildOf(AMT2OreVein::StaticClass()))) { continue; }
		auto* Vein=World->SpawnActor<AMT2OreVein>(Class);
		if (!TestNotNull(TEXT("Vein spawns"),Vein)) { continue; }
		TestTrue(TEXT("Vein begins play through normal world initialization"),Vein->HasActorBegunPlay());
		auto* Visual=Vein->GetVeinMesh();
		TestTrue(TEXT("Stationary vein has its imported static mesh"),Visual && Visual->GetStaticMesh());
		if (Visual && Visual->GetStaticMesh())
		{
			const double Feet=Vein->GetActorLocation().Z-Vein->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			TestTrue(TEXT("Vein visual aligns with spawn capsule feet"),FMath::IsNearlyEqual(Visual->CalcBounds(Visual->GetComponentTransform()).GetBox().Min.Z,Feet,.1));
			for (int32 I=0; I<Visual->GetNumMaterials(); ++I) { TestNotNull(TEXT("Vein material resolves"),Visual->GetMaterial(I)); }
		}
		TestFalse(TEXT("Vein cannot be attacked"),Vein->IsAttackable());
		TestFalse(TEXT("Vein cannot be knocked back"),Vein->CanBeKnockedBack());
		TestEqual(TEXT("Vein movement disabled"),int32(Vein->GetCharacterMovement()->MovementMode),int32(MOVE_None));
		TestTrue(TEXT("Independent legacy lifetime"),Vein->GetLifeSpan()>=420.f && Vein->GetLifeSpan()<=900.f);
		TestFalse(TEXT("No continuous movement replication"),Vein->IsReplicatingMovement());
		Vein->Destroy();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2MiningPresentationTest, "Metin2.Mining.Presentation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2MiningPresentationTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GEngine)); GI->InitializeStandalone();
	UWorld* World=GI->GetWorld();
	ON_SCOPE_EXIT { GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	auto* Player=World->SpawnActor<AMT2PlayerCharacter>(); auto* PS=World->SpawnActor<AMT2PlayerState>(); Player->SetPlayerState(PS);
	const auto* Appearance=GetDefault<UMT2CharacterAppearanceSettings>()->FindAppearance(PS->GetCharacterAppearance());
	if (!TestNotNull(TEXT("Default appearance"),Appearance)) { return false; }
	auto* Body=Appearance->Mesh.LoadSynchronous();
	const FString Path=UMT2PathSettings::Format(TEXT("Characters_Animations_Name"),TEXT("%s%s"),TEXT("ABP_Warrior_Male"),TEXT("ABP_Warrior_Male"));
	auto* BP=LoadObject<UAnimBlueprint>(nullptr,*Path);
	if (!TestNotNull(TEXT("Character animation blueprint"),BP)) { return false; }
	Player->GetMesh()->SetSkeletalMesh(Body,true); Player->GetMesh()->SetAnimInstanceClass(BP->GeneratedClass);
	auto* Mining=Player->GetMiningComponent(); auto* Vein=World->SpawnActor<AMT2Npc>();
	Vein->SetActorLocation(Player->GetActorLocation()+FVector(0,100,0)); Player->SetActorRotation(FRotator::ZeroRotator);
	Mining->State.Session=1; Mining->State.Swings=5; Mining->State.Vein=Vein; Mining->State.StartTime=World->GetTimeSeconds()-3;
	Mining->OnRep_State();
	TestTrue(TEXT("Replicated sequence faces vein on presentation client"),FMath::IsNearlyEqual(Player->GetActorRotation().Yaw,90.,.1));
	TestEqual(TEXT("Late observer reconstructs current swing"),Mining->PresentedSwing,1);
	TestNotNull(TEXT("Dig montage starts"),Mining->DigMontage.Get());
	Mining->State.Swings=0; Mining->OnRep_State();
	TestNull(TEXT("Cancellation releases only mining montage"),Mining->DigMontage.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2MiningAuthorityTest, "Metin2.Mining.Authority", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2MiningAuthorityTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GEngine)); GI->InitializeStandalone();
	UWorld* World=GI->GetWorld();
	ON_SCOPE_EXIT { GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	auto* Registry=GI->GetSubsystem<UMT2VnumRegistrySubsystem>();
	const auto* Pick=Cast<UMT2ItemPickTemplate>(Registry->ResolveItemTemplateClass(29101).GetDefaultObject());
	if (!TestNotNull(TEXT("Imported pick"),Pick)) { return false; }
	auto* Player=World->SpawnActor<AMT2PlayerCharacter>(); auto* PS=World->SpawnActor<AMT2PlayerState>(); Player->SetPlayerState(PS);
	auto* ASC=PS->GetAbilitySystemComponent(); ASC->AddAttributeSetSubobject(PS->GetCoreAttributes()); ASC->InitAbilityActorInfo(PS,Player);
	ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetMaxHealthAttribute(),100); ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetHealthAttribute(),100);
	Player->GetHealthComponent()->InitializeWithAbilitySystem(ASC);
	Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	auto* Inventory=Player->GetInventoryComponent(); TArray<FMT2ItemSlot> Slots,Worn; Slots.SetNum(180); Worn.SetNum(32);
	auto* Vein=World->SpawnActor<AMT2Npc>(); FMT2MobDefinition Definition; Definition.Vnum=20047; Vein->ConfigureFromDefinition(Definition);
	Vein->SetActorLocation(Player->GetActorLocation()+FVector(100,0,0));
	auto* Mining=Player->GetMiningComponent();
	TestFalse(TEXT("Wrong equipment denied"),Mining->StartMining(Vein));
	Worn[4]=FMT2ItemSlot(29101,Pick->MakeInstanceData(1)); Worn[4].MetinSockets.SetNum(3); Inventory->RestoreItems(Slots,Worn);
	TestTrue(TEXT("Equipped pick permits mining"),Mining->StartMining(Vein));
	const uint32 Session=Mining->State.Session;
	Mining->Complete(Session);
	TestTrue(TEXT("Early completion cannot resolve"),Mining->IsMining());
	TestEqual(TEXT("No practice before completion"),Inventory->GetEquipment()[4].MetinSockets[0].Value,0);
	TestEqual(TEXT("No premature ore"),Inventory->CountItemByVnum(50601),0);
	Mining->CancelMining();
	TestFalse(TEXT("Cancellation stops sequence"),Mining->IsMining());
	TestEqual(TEXT("Interrupted sequence grants no practice"),Inventory->GetEquipment()[4].MetinSockets[0].Value,0);
	TestTrue(TEXT("Next sequence starts"),Mining->StartMining(Vein));
	Mining->Complete(Session);
	TestTrue(TEXT("Stale callback cannot end new sequence"),Mining->IsMining());
	Mining->State.StartTime=World->GetTimeSeconds()-31;
	Mining->Random.Initialize(1);
	Mining->Complete(Mining->State.Session);
	TestFalse(TEXT("Completed sequence stops"),Mining->IsMining());
	TestEqual(TEXT("Completed sequence practices once"),Inventory->GetEquipment()[4].MetinSockets[0].Value,1);
	const int32 Ore=Inventory->CountItemByVnum(50601);
	Mining->Complete(Mining->State.Session);
	TestEqual(TEXT("Duplicate completion cannot duplicate practice"),Inventory->GetEquipment()[4].MetinSockets[0].Value,1);
	TestEqual(TEXT("Duplicate completion cannot duplicate ore"),Inventory->CountItemByVnum(50601),Ore);
	const FString Payload=PS->CapturePersistentStateJson_Implementation();
	TestTrue(TEXT("Mining item payload restores"),PS->ApplyPersistentStateJson_Implementation(Payload));
	TestEqual(TEXT("Practice survives reload"),Inventory->GetEquipment()[4].MetinSockets[0].Value,1);
	TestTrue(TEXT("Damage cancellation sequence starts"),Mining->StartMining(Vein));
	Player->GetHealthComponent()->SetHealth(90);
	TestFalse(TEXT("Damage interrupts mining immediately"),Mining->IsMining());
	Player->GetHealthComponent()->SetHealth(100);
	FMT2StatusEffect Stun; Stun.Type=FMT2AffectIdentifier::FromLegacyId(999); Stun.Flags=1<<6; Stun.RemainingSeconds=10;
	Player->GetStatusEffectComponent()->AddEffect(Stun);
	TestFalse(TEXT("Stunned players cannot mine"),Mining->StartMining(Vein));
	Mining->CancelMining(); Player->GetStatusEffectComponent()->RemoveEffectsByType(FMT2AffectIdentifier::FromLegacyId(999));
	TestTrue(TEXT("Stun cancellation sequence starts"),Mining->StartMining(Vein));
	Player->GetStatusEffectComponent()->AddEffect(Stun);
	TestFalse(TEXT("Stun interrupts active mining immediately"),Mining->IsMining());
	Player->GetStatusEffectComponent()->RemoveEffectsByType(FMT2AffectIdentifier::FromLegacyId(999));
	// Force a successful real RNG path without changing the production chance table.
	int32 Seed=0;
	for (;; ++Seed) { FRandomStream Trial(Seed); if (Trial.RandRange(1,100)<=23 && Trial.RandRange(1,100)>20) { break; } }
	const auto* RawOre=Registry->ResolveItemTemplateClass(50601).GetDefaultObject();
	if (!TestNotNull(TEXT("Imported raw ore"),RawOre)) { return false; }
	auto* QuestRegistry=GI->GetSubsystem<UMT2QuestRegistrySubsystem>();
	const auto& ImportedQuests=QuestRegistry->GetQuests();
	if (!TestTrue(TEXT("Imported quest class available for pickup fixture"),!ImportedQuests.IsEmpty())) { return false; }
	TStrongObjectPtr<UMT2Quest> QuestOwner(DuplicateObject<UMT2Quest>(ImportedQuests[0],GI.Get()));
	auto* Quest=QuestOwner.Get(); Quest->States.Reset(); Quest->QuestName=TEXT("mining_pickup_probe");
	auto& QuestState=Quest->States.AddDefaulted_GetRef(); QuestState.StateName=TEXT("start");
	auto& Trigger=QuestState.Triggers.AddDefaulted_GetRef(); Trigger.Event=EMT2QuestEvent::ItemPick; Trigger.Vnum=50601;
	auto* CountNode=NewObject<UMT2QuestNode_SetFlag>(Quest); CountNode->FlagName=TEXT("picked"); CountNode->ValueExpression=TEXT("item.get_count()"); Trigger.Nodes.Add(CountNode);
	QuestRegistry->bLoaded=true; QuestRegistry->Quests={Quest}; QuestRegistry->QuestsById.Reset(); QuestRegistry->QuestsById.Add(Quest->GetQuestId(),Quest);
	for (auto& Slot : Slots) { Slot=FMT2ItemSlot(50601,RawOre->MakeInstanceData(999)); }
	Slots[0].Count=998; Inventory->RestoreItems(Slots,Worn);
	TestTrue(TEXT("Overflow sequence starts"),Mining->StartMining(Vein));
	Mining->State.StartTime=World->GetTimeSeconds()-31; Mining->Random.Initialize(Seed); Mining->Complete(Mining->State.Session);
	TestEqual(TEXT("Available stack space filled once"),Inventory->GetSlots()[0].Count,999);
	TestEqual(TEXT("Quest pickup observes only inserted ore, not dropped remainder"),PS->GetQuestManagerComponent()->GetQuestFlag(TEXT("picked"),Quest),1);
	int32 Drops=0;
	for (TActorIterator<AMT2WorldItem> It(World); It; ++It)
	{
		if (It->GetItemVnum()==50601) { ++Drops; TestTrue(TEXT("Only uncollected remainder dropped"),It->GetItemCount()>0 && It->GetItemCount()<90); }
	}
	TestEqual(TEXT("One owned remainder drop"),Drops,1);
	Worn[4].MetinSockets[0].Value=Pick->ImprovementPointsRequired-1;
	Inventory->RestoreItems(Slots,Worn); Inventory->FinishMiningAttempt(); Inventory->FinishMiningAttempt();
	TestEqual(TEXT("Practice stops at imported cap"),Inventory->GetEquipment()[4].MetinSockets[0].Value,Pick->ImprovementPointsRequired);
	Slots.Empty(); Slots.SetNum(180); Inventory->RestoreItems(Slots,Worn);
	TestTrue(TEXT("Movement test starts"),Mining->StartMining(Vein));
	Player->SetActorLocation(Player->GetActorLocation()+FVector(100,0,0)); Mining->CheckAttempt();
	TestFalse(TEXT("Movement cancels"),Mining->IsMining());
	TestTrue(TEXT("Equipment test starts"),Mining->StartMining(Vein));
	TestTrue(TEXT("Can unequip pick"),Inventory->UnequipItem(4)); Mining->CheckAttempt();
	TestFalse(TEXT("Equipment replacement cancels"),Mining->IsMining());
	const int32 PickSlot=Inventory->GetSlots().IndexOfByPredicate([](const FMT2ItemSlot& Item){return Item.Vnum==29101;});
	PS->SetCharacterLevel(75); // Respect imported tool equip requirements.
	TestTrue(TEXT("Inventory accepts pick as wearable tool"),Inventory->EquipItemFromSlot(PickSlot));
	Inventory->RestoreItems(Slots,Worn);
	TestTrue(TEXT("Destruction test starts"),Mining->StartMining(Vein)); Vein->Destroy(); Mining->CheckAttempt();
	TestFalse(TEXT("Vein destruction cancels"),Mining->IsMining());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2MiningRulesTest, "Metin2.Mining.LegacyRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2MiningRulesTest::RunTest(const FString&)
{
	TestEqual(TEXT("First vein"), MT2Mining::RawOre(20047), 50601);
	TestEqual(TEXT("Last original vein"), MT2Mining::RawOre(20059), 50613);
	TestEqual(TEXT("First extended vein"), MT2Mining::RawOre(30301), 50614);
	TestEqual(TEXT("Last vein"), MT2Mining::RawOre(30305), 50618);
	TestEqual(TEXT("Unknown vein rejected"), MT2Mining::RawOre(20060), 0);
	TestEqual(TEXT("Novice chance"), MT2Mining::SuccessChance(0, 0), 23);
	TestEqual(TEXT("Level four"), MT2Mining::SuccessChance(4, 1), 26);
	TestEqual(TEXT("Level five"), MT2Mining::SuccessChance(5, 1), 27);
	TestEqual(TEXT("Level thirty nine"), MT2Mining::SuccessChance(39, 9), 80);
	TestEqual(TEXT("Master chance"), MT2Mining::SuccessChance(40, 9), 81);
	TestEqual(TEXT("Invalid low levels clamped"), MT2Mining::SuccessChance(-1, -1), 23);
	TestEqual(TEXT("Invalid high levels clamped"), MT2Mining::SuccessChance(100, 100), 81);
	const int32 Rolls[] = {1,20,21,50,51,70,71,85,86,90,91,94,95,97,98,99,100};
	const int32 Minimums[] = {1,1,11,11,21,21,31,31,41,41,51,51,61,61,71,71,81};
	for (int32 I=0; I<UE_ARRAY_COUNT(Rolls); ++I)
	{
		TestEqual(TEXT("Weighted lower endpoint"), MT2Mining::Quantity(Rolls[I], 0), Minimums[I]);
		TestEqual(TEXT("Weighted upper endpoint"), MT2Mining::Quantity(Rolls[I], 9), Minimums[I]+9);
	}
	TestEqual(TEXT("Invalid weighted roll rejected"), MT2Mining::Quantity(0, 0), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2MiningPickDataTest, "Metin2.Mining.ImportedPickPractice", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2MiningPickDataTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GEngine));
	GI->InitializeStandalone();
	UWorld* World = GI->GetWorld();
	ON_SCOPE_EXIT { GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	auto* Registry = GI->GetSubsystem<UMT2VnumRegistrySubsystem>();
	const auto* Pick = Cast<UMT2ItemPickTemplate>(Registry->ResolveItemTemplateClass(29101).GetDefaultObject());
	if (!TestNotNull(TEXT("Imported pickaxe"), Pick)) { return false; }
	TestEqual(TEXT("Completed attempts use imported practice odds"), Pick->GetLegacyQuestValue(1), 1);
	TestEqual(TEXT("First pick uses grade zero, not vnum modulo ten"), Pick->RefinementLevel, 0);
	TestTrue(TEXT("Pickaxe has a practice cap"), Pick->ImprovementPointsRequired > 0);
	TestNotNull(TEXT("Pickaxe world mesh loads"),Pick->WorldMesh.LoadSynchronous());
	for (const TCHAR* Race : {TEXT("Warrior"),TEXT("Assassin"),TEXT("Sura"),TEXT("Shaman")})
	for (const TCHAR* Sex : {TEXT("Male"),TEXT("Female")})
	{
		const FString Name=FString::Printf(TEXT("ABP_%s_%s"),Race,Sex);
		const FString Path=UMT2PathSettings::Format(TEXT("Characters_Animations_Name"),TEXT("%s%s"),*Name,*Name);
		const auto* BP=LoadObject<UAnimBlueprint>(nullptr,*Path);
		const auto* Anim=BP && BP->GeneratedClass ? Cast<UMT2CharacterAnimInstance>(BP->GeneratedClass->GetDefaultObject()) : nullptr;
		TestTrue(FString::Printf(TEXT("%s has bound digging animation"),*Name),Anim && Anim->GetAnimation(TEXT("general"),TEXT("dig")));
	}
	return true;
}
#endif
