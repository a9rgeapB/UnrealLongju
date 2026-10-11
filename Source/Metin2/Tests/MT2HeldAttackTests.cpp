#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Dungeons/MT2DungeonRoom.h"
#include "Components/BoxComponent.h"
#include "Combat/MT2CombatComponent.h"
#include "Mobs/MT2Mob.h"
#include "Abilities/MT2GameplayAbilityBasicAttack.h"
#include "Abilities/MT2GameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Abilities/MT2CoreAttributeSet.h"
#include "Components/MT2HealthComponent.h"
#include "Player/MT2PlayerState.h"
#include "TimerManager.h"

namespace
{
	void InitializeMob(AMT2Mob* Mob)
	{
		auto* ASC = Mob->GetAbilitySystemComponent(); ASC->InitAbilityActorInfo(Mob, Mob);
		ASC->AddAttributeSetSubobject(NewObject<UMT2CoreAttributeSet>(Mob));
		ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetMaxHealthAttribute(), 100.f);
		ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetHealthAttribute(), 100.f);
		Mob->GetHealthComponent()->InitializeWithAbilitySystem(ASC);
	}
	void InitializePlayer(AMT2PlayerCharacter* Player)
	{
		auto* State = Player->GetWorld()->SpawnActor<AMT2PlayerState>(); Player->SetPlayerState(State);
		auto* ASC = State->GetAbilitySystemComponent(); ASC->AddAttributeSetSubobject(State->GetCoreAttributes());
		State->InitializeAbilitySystem(Player);
		ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetMaxHealthAttribute(), 100.f);
		ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetHealthAttribute(), 100.f);
		Player->GetHealthComponent()->InitializeWithAbilitySystem(ASC);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2MeleeRoomOcclusionTest, "Metin2.Combat.MeleeRoomOcclusion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2MeleeRoomOcclusionTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); };
	auto* Player = World->SpawnActor<AMT2PlayerCharacter>();
	auto* Mob = World->SpawnActor<AMT2Mob>();
	InitializeMob(Mob); InitializePlayer(Player);
	Player->SetActorLocation(FVector(0,0,18000)); Mob->SetActorLocation(FVector(120,0,18000));
	auto* Combat = Player->GetCombatComponent(); Combat->ConfigureBasicAttack(200,65,10,.1f);
	TestTrue(TEXT("Untargeted melee finds nearby hostile"), Combat->FindBasicAttackTargets().Contains(Mob));
	auto* Room = World->SpawnActor<AMT2DungeonRoom>(); Room->SetActorLocation(FVector(0,0,18000));
	TestTrue(TEXT("Enclosing room overlap cannot shield mobs"), Combat->FindBasicAttackTargets().Contains(Mob));
	auto* Wall = World->SpawnActor<AActor>(); auto* Box = NewObject<UBoxComponent>(Wall);
	Wall->SetRootComponent(Box); Box->SetBoxExtent(FVector(10,200,200));
	Box->SetCollisionProfileName(TEXT("BlockAllDynamic")); Box->RegisterComponent(); Wall->SetActorLocation(FVector(60,0,18000));
	TestFalse(TEXT("A real solid wall still blocks untargeted melee"), Combat->FindBasicAttackTargets().Contains(Mob));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2HeldAttackFallbackTest, "Metin2.Combat.HeldAttackFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2HeldAttackFallbackTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); };
	auto* Mob = World->SpawnActor<AMT2Mob>(); auto* ASC = Mob->GetAbilitySystemComponent();
	InitializeMob(Mob); ASC->GiveAbility(FGameplayAbilitySpec(UMT2GameplayAbilityBasicAttack::StaticClass()));
	auto* Combat = Mob->GetCombatComponent(); Combat->ConfigureBasicAttack(200,65,10,.1f,20);
	Combat->StartBasicAttackLoop();
	for (int32 I = 0; I < 12; ++I) { ++GFrameCounter; World->GetTimerManager().Tick(.1f); }
	TestTrue(TEXT("Successful ability without animation callbacks continues held swings"), Combat->CurrentComboIndex > 0);
	ASC->AddLooseGameplayTag(MT2GameplayTags::Status_Dead);
	for (int32 I = 0; I < 4; ++I) { ++GFrameCounter; World->GetTimerManager().Tick(.1f); }
	const int32 BlockedCombo = Combat->CurrentComboIndex;
	for (int32 I = 0; I < 4; ++I) { ++GFrameCounter; World->GetTimerManager().Tick(.1f); }
	TestEqual(TEXT("Blocked ability does not swing while retrying"), Combat->CurrentComboIndex, BlockedCombo);
	ASC->RemoveLooseGameplayTag(MT2GameplayTags::Status_Dead);
	for (int32 I = 0; I < 12; ++I) { ++GFrameCounter; World->GetTimerManager().Tick(.1f); }
	TestTrue(TEXT("Held loop resumes after activation blocker clears"), Combat->CurrentComboIndex != BlockedCombo);
	Combat->StopBasicAttackLoop();
	const int32 ReleasedCombo = Combat->CurrentComboIndex;
	for (int32 I = 0; I < 12; ++I) { ++GFrameCounter; World->GetTimerManager().Tick(.1f); }
	TestEqual(TEXT("Release cancels retries and future swings"), Combat->CurrentComboIndex, ReleasedCombo);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2HeldAttackInputTest, "Metin2.Combat.HeldAttackInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2HeldAttackInputTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); };
	auto* Player = World->SpawnActor<AMT2PlayerCharacter>(); auto* Mob = World->SpawnActor<AMT2Mob>();
	InitializeMob(Mob); InitializePlayer(Player);
	Player->bAutoEngageTarget = true; Player->GetCombatComponent()->SetSelectedTarget(Mob);
	Player->HandleAttackPressed(); Mob->SetActorLocation(FVector(1000,0,0));
	Player->UpdateCombatEngagement(.01f);
	TestTrue(TEXT("Out-of-range auto chase preserves held keyboard attack"), Player->GetCombatComponent()->IsBasicAttackHeld());
	Player->GetCombatComponent()->SetSelectedTarget(nullptr); Player->UpdateCombatEngagement(.01f);
	TestTrue(TEXT("Losing a selected target preserves held keyboard attack"), Player->GetCombatComponent()->IsBasicAttackHeld());
	Player->HandleAttackReleased();
	TestFalse(TEXT("Keyboard release ends attack intent"), Player->GetCombatComponent()->IsBasicAttackHeld());
	return true;
}
#endif
