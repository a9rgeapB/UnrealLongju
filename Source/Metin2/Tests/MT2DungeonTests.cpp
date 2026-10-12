#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerController.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/MT2HealthComponent.h"
#include "AbilitySystemComponent.h"
#include "Abilities/MT2CoreAttributeSet.h"
#include "Dungeons/MT2DungeonRoom.h"
#include "Dungeons/MT2DevilTowerRoom.h"
#include "Dungeons/MT2DungeonSubsystem.h"
#include "Mobs/MT2Mob.h"
#include "Player/MT2PlayerState.h"
#include "Persistence/MT2PersistenceComponent.h"
#include "Quests/MT2QuestExpression.h"
#include "UObject/StrongObjectPtr.h"
#include "TimerManager.h"
#include "Player/MT2PlayerController.h"
#include "UI/MT2ChatWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "Mobs/MT2MobTypes.h"
#include "Items/MT2InventoryComponent.h"
#include "Items/MT2ItemTemplate.h"
#include "Quests/MT2QuestManagerComponent.h"

namespace MT2DungeonFixture
{
	struct FRig
	{
		TStrongObjectPtr<UGameInstance> Instance{NewObject<UGameInstance>(GEngine)};
		UWorld* World = nullptr;
		UWorld* OriginalWorld = GWorld;
		FRig()
		{
			Instance->InitializeStandalone(); World = Instance->GetWorld();
			World->InitializeActorsForPlay(FURL()); World->GetWorldSettings()->NotifyBeginPlay();
		}
		~FRig()
		{
			World->EndPlay(EEndPlayReason::Quit); Instance->Shutdown();
			World->DestroyWorld(false); GEngine->DestroyWorldContext(World); GWorld = OriginalWorld;
		}
		AMT2DungeonRoom* Room(FName Id, FVector Location = FVector::ZeroVector, TSubclassOf<AMT2DungeonRoom> Class = AMT2DungeonRoom::StaticClass())
		{
			const FTransform Transform(Location);
			auto* Result = World->SpawnActorDeferred<AMT2DungeonRoom>(Class, Transform);
			Result->RoomId = Id; Result->FinishSpawning(Transform); return Result;
		}
		AMT2PlayerState* Player(const FString& Id, FVector Location = FVector::ZeroVector, bool bCharacter = false)
		{
			auto* Controller = World->SpawnActor<APlayerController>();
			auto* State = World->SpawnActor<AMT2PlayerState>(); Controller->SetPlayerState(State);
			State->GetPersistenceComponent()->ConfigurePersistence(TEXT("character"), Id, TEXT("account"));
			APawn* Pawn = bCharacter ? World->SpawnActor<AMT2PlayerCharacter>() : World->SpawnActor<APawn>();
			if (!Pawn->GetRootComponent())
			{
				auto* Root = NewObject<USceneComponent>(Pawn); Pawn->SetRootComponent(Root); Root->RegisterComponent();
			}
			Pawn->SetActorLocation(Location); Controller->Possess(Pawn); return State;
		}
		AMT2Mob* Mob(FVector Location = FVector::ZeroVector)
		{
			auto* Mob = World->SpawnActor<AMT2Mob>(); Mob->SetActorLocation(Location);
			auto* ASC = Mob->GetAbilitySystemComponent();
			ASC->AddAttributeSetSubobject(NewObject<UMT2CoreAttributeSet>(Mob)); ASC->InitAbilityActorInfo(Mob, Mob);
			ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetMaxHealthAttribute(), 100.f);
			ASC->SetNumericAttributeBase(UMT2CoreAttributeSet::GetHealthAttribute(), 100.f);
			Mob->GetHealthComponent()->InitializeWithAbilitySystem(ASC); return Mob;
		}
		void TimerTick(float Seconds) { ++GFrameCounter; World->GetTimerManager().Tick(Seconds); }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2DungeonUniqueBindingsTest, "Metin2.Dungeons.UniqueBindings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2DungeonUniqueBindingsTest::RunTest(const FString&)
{
	MT2DungeonFixture::FRig Rig;
	auto* Room = Rig.Room(TEXT("unique.probe")); auto* Player = Rig.Player(TEXT("unique.player"));
	Room->JoinPlayer(Player); Room->StartEncounter();
	auto* Boss = Rig.Mob(FVector(100,0,0)); auto* Other = Rig.Mob(FVector(-100,0,0));
	TestTrue(TEXT("Boss registered"), Room->RegisterEnemy(Boss));
	TestTrue(TEXT("Second mob registered"), Room->RegisterEnemy(Other));
	TestTrue(TEXT("Unique lower case"), Room->SetScriptUnique(TEXT("boss"), Boss));
	TestTrue(TEXT("Unique upper case"), Room->SetScriptUnique(TEXT("BOSS"), Other));
	FMT2QuestContext Context; Context.Manager = Room->GetScriptExecutor(); Context.DungeonRoom = Room;
	auto Read = [&](const TCHAR* Expression)
	{
		bool bOk = false; auto Value = FMT2QuestExpression::Evaluate(Expression, Context, bOk);
		TestTrue(FString(TEXT("Evaluate ")) + Expression, bOk); return Value;
	};
	Boss->GetHealthComponent()->SetHealth(50.f);
	TestEqual(TEXT("Unique HP percentage"), Read(TEXT("d.unique_get_hp_perc('boss')")).Number, 50.0);
	TestEqual(TEXT("Names keep distinct entity IDs"), Room->GetScriptUnique(TEXT("BOSS")), Other);
	TestFalse(TEXT("Live unique not dead"), Read(TEXT("d.is_unique_dead('boss')")).AsBool());
	Read(TEXT("d.kill_unique('boss')"));
	TestTrue(TEXT("Unique kill updates health"), Boss->GetHealthComponent()->IsDead());
	TestTrue(TEXT("Killed unique removed"), Read(TEXT("d.is_unique_dead('boss')")).AsBool());
	TestEqual(TEXT("Only one monster remains"), Read(TEXT("d.count_monster()")).Number, 1.0);
	Read(TEXT("d.purge_unique('BOSS')"));
	TestEqual(TEXT("Purge removes room ownership"), Room->GetRemainingEnemies(), 0);
	TestNull(TEXT("Purged unique no longer resolves"), Room->GetScriptUnique(TEXT("BOSS")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2DungeonScriptTransitionTest, "Metin2.Dungeons.ScriptTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2DungeonScriptTransitionTest::RunTest(const FString&)
{
	MT2DungeonFixture::FRig Rig;
	auto* Floor = Rig.World->SpawnActor<AActor>();
	auto* Surface = NewObject<UBoxComponent>(Floor); Floor->SetRootComponent(Surface);
	Surface->SetBoxExtent(FVector(10000,10000,10)); Surface->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Surface->SetCollisionObjectType(ECC_WorldStatic); Surface->SetCollisionResponseToAllChannels(ECR_Block);
	Surface->RegisterComponent(); Floor->SetActorLocation(FVector(0,0,-10));
	auto* Room = Rig.Room(TEXT("jump.first")); auto* Next = Rig.Room(TEXT("jump.second"), FVector(5000,0,0));
	auto* Player = Rig.Player(TEXT("jump.player"), FVector(0,0,100), true);
	TestTrue(TEXT("Source membership"), Room->JoinPlayer(Player));
	Room->SetScriptFlag(TEXT("level"), 2); Room->StartEncounter();
	TestTrue(TEXT("Collision-backed shared jump"), Room->JumpScriptMembers(Next, FVector2D(5000,0)));
	TestEqual(TEXT("Destination owns moved player"), Rig.World->GetSubsystem<UMT2DungeonSubsystem>()->GetPlayerRoom(Player), Next);
	TestEqual(TEXT("Stage flags transferred into idle destination"), Next->GetScriptFlag(TEXT("level")), 2);
	TestEqual(TEXT("Source stage completes"), Room->GetPhase(), EMT2DungeonRoomPhase::Completed);
	TestEqual(TEXT("Destination starts once"), Next->GetPhase(), EMT2DungeonRoomPhase::Active);
	TestFalse(TEXT("Unconfigured/outside destination rejected"), Next->JumpScriptMembers(Room, FVector2D(50000,0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2DungeonSharedRoomTest, "Metin2.Dungeons.SharedRoom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2DungeonSharedRoomTest::RunTest(const FString&)
{
	MT2DungeonFixture::FRig Rig;
	auto* Room = Rig.Room(TEXT("Tower.Floor1")); auto* Next = Rig.Room(TEXT("Tower.Floor2"), FVector(5000,0,0));
	auto* Registry = Rig.World->GetSubsystem<UMT2DungeonSubsystem>();
	auto* A = Rig.Player(TEXT("A")); auto* B = Rig.Player(TEXT("B"));
	if (!TestNotNull(TEXT("Possessed player pawn"), A->GetPawn())) return false;
	TestTrue(TEXT("First participant enters stage"), Room->JoinPlayer(A));
	TestTrue(TEXT("Second participant shares same stage"), Room->JoinPlayer(B));
	TestTrue(TEXT("Duplicate entry is idempotent"), Room->JoinPlayer(A));
	TestEqual(TEXT("One shared participant count"), Room->GetParticipantCount(), 2);
	TestEqual(TEXT("Same logical room for both players"), Registry->GetPlayerRoom(B), Room);
	TestFalse(TEXT("Cannot join a distant room by request alone"), Next->JoinPlayer(A));
	auto* Door = Rig.World->SpawnActor<AActor>(); Room->Doors.Add(Door);
	TestTrue(TEXT("Encounter starts once"), Room->StartEncounter());
	const int32 Epoch = Room->GetRunSerial();
	TestFalse(TEXT("Second start cannot duplicate encounter"), Room->StartEncounter());
	TestEqual(TEXT("Restart attempt preserves run serial"), Room->GetRunSerial(), Epoch);
	TestFalse(TEXT("Shared door closed during encounter"), Door->IsHidden());
	auto* Late = Rig.Player(TEXT("late")); const double Countdown = Room->GetSecondsRemaining();
	TestTrue(TEXT("Late arrivals share active encounter"), Room->JoinPlayer(Late));
	TestEqual(TEXT("Joining cannot reset shared countdown"), Room->GetSecondsRemaining(), Countdown);
	auto* First = Rig.Mob(); auto* Boss = Rig.Mob(FVector(300,0,0));
	TestTrue(TEXT("Objective registered"), Room->RegisterEnemy(First));
	TestTrue(TEXT("Boss uses same tracking"), Room->RegisterEnemy(Boss));
	TestTrue(TEXT("Duplicate registration cannot double count"), Room->RegisterEnemy(Boss));
	TestEqual(TEXT("Shared remaining count"), Room->GetRemainingEnemies(), 2);
	First->GetHealthComponent()->SetHealth(0.f);
	TestTrue(TEXT("One kill does not complete entire room"), Room->GetPhase() == EMT2DungeonRoomPhase::Active);
	Boss->GetHealthComponent()->SetHealth(0.f);
	TestTrue(TEXT("All objectives complete shared encounter"), Room->GetPhase() == EMT2DungeonRoomPhase::Completed);
	TestTrue(TEXT("Shared door opens"), Door->IsHidden());
	TestTrue(TEXT("Initial participant receives own entitlement"), Room->TryClaimCompletion(A));
	TestFalse(TEXT("Replay cannot duplicate completion entitlement"), Room->TryClaimCompletion(A));
	TestTrue(TEXT("Second participant entitlement independent"), Room->TryClaimCompletion(B));
	TestTrue(TEXT("Entry before completion receives own entitlement"), Room->TryClaimCompletion(Late));
	Room->LeavePlayer(Late); Room->JoinPlayer(Late);
	TestFalse(TEXT("Rejoining cannot duplicate a claimed entitlement"), Room->TryClaimCompletion(Late));
	auto* After = Rig.Player(TEXT("after"));
	TestTrue(TEXT("Entry after completion can share the completed room"), Room->JoinPlayer(After));
	TestFalse(TEXT("Entry after completion cannot inherit entitlement"), Room->TryClaimCompletion(After));
	TestFalse(TEXT("Occupied room cannot reset"), Room->ResetWhenEmpty());
	Room->NextRoom = Next;
	TestTrue(TEXT("Stage advance stays in same world"), Room->AdvancePlayer(A));
	TestEqual(TEXT("Stage membership transfers"), Registry->GetPlayerRoom(A), Next);
	Room->LeavePlayer(B); Room->LeavePlayer(Late); Room->LeavePlayer(After);
	TestTrue(TEXT("Only empty room resets"), Room->ResetWhenEmpty());
	TestTrue(TEXT("Room returns to idle"), Room->GetPhase() == EMT2DungeonRoomPhase::Idle);
	TestTrue(TEXT("Room-owned corpses cleaned without affecting next stage"), First->IsActorBeingDestroyed() && Boss->IsActorBeingDestroyed());
	TestEqual(TEXT("Sibling stage retains player"), Next->GetParticipantCount(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2DungeonLifetimeTest, "Metin2.Dungeons.Lifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2DungeonLifetimeTest::RunTest(const FString&)
{
	MT2DungeonFixture::FRig Rig;
	auto* Room = Rig.Room(TEXT("stage")); auto* Player = Rig.Player(TEXT("player"));
	Room->EncounterSeconds = .1f; Room->EmptyResetSeconds = .1f;
	TestTrue(TEXT("Join before timeout test"), Room->JoinPlayer(Player));
	TestTrue(TEXT("Start timed encounter"), Room->StartEncounter());
	Rig.TimerTick(.1f); Rig.TimerTick(.11f);
	TestTrue(TEXT("Room timer survives independently of player quest timers"), Room->GetPhase() == EMT2DungeonRoomPhase::Failed);
	TestFalse(TEXT("Failed room denies new admission"), Room->JoinPlayer(Rig.Player(TEXT("late"))));
	Player->GetPawn()->Destroy();
	TestEqual(TEXT("Pawn destruction removes membership even with living PlayerState"), Room->GetParticipantCount(), 0);
	Rig.TimerTick(.1f); Rig.TimerTick(.11f);
	TestTrue(TEXT("Empty failed room resets through grace timer"), Room->GetPhase() == EMT2DungeonRoomPhase::Idle);
	auto* Other = Rig.Player(TEXT("other")); Room->JoinPlayer(Other); Room->StartEncounter();
	auto* Mob = Rig.Mob(); Room->RegisterEnemy(Mob); Mob->Destroy();
	TestTrue(TEXT("Live objective despawn is failure, not completion"), Room->GetPhase() == EMT2DungeonRoomPhase::Failed);
	Room->LeavePlayer(Other); Room->ResetWhenEmpty(); Room->JoinPlayer(Other); Room->StartEncounter();
	auto* OwnedMob = Rig.Mob();
	TestTrue(TEXT("Enemy registered before local room removal"), Room->RegisterEnemy(OwnedMob));
	Room->Destroy();
	TestTrue(TEXT("Local room removal cleans owned live enemies"), OwnedMob->IsActorBeingDestroyed());
	TestNull(TEXT("Room destruction removes registry identity"), Rig.World->GetSubsystem<UMT2DungeonSubsystem>()->FindRoom(TEXT("stage")));
	TestNull(TEXT("Room destruction clears membership"), Rig.World->GetSubsystem<UMT2DungeonSubsystem>()->GetPlayerRoom(Other));
	Rig.TimerTick(10.f); // Destroyed room must have no surviving callbacks.
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2DungeonQuestAndValidationTest, "Metin2.Dungeons.QuestAndValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2DungeonQuestAndValidationTest::RunTest(const FString&)
{
	MT2DungeonFixture::FRig Rig;
	auto* Room = Rig.Room(TEXT("tower")); auto* Player = Rig.Player(TEXT("player"), FVector::ZeroVector, true);
	FMT2QuestContext Context; Context.PlayerState = Player; Context.Player = Cast<AMT2PlayerCharacter>(Player->GetPawn());
	bool bOk = false;
	TestEqual(TEXT("No dungeon context outside a room"), FMT2QuestExpression::Evaluate(TEXT("pc.in_dungeon()"), Context, bOk).Number, 0.0);
	TestTrue(TEXT("Player enters logical room"), Room->JoinPlayer(Player));
	TestEqual(TEXT("Quest condition follows room membership"), FMT2QuestExpression::Evaluate(TEXT("pc.in_dungeon()"), Context, bOk).Number, 1.0);
	TestTrue(TEXT("Dungeon expression supported"), bOk);
	Context.Player->SetActorLocation(FVector(-2000,0,0));
	TestNull(TEXT("Physical exit clears membership"), Rig.World->GetSubsystem<UMT2DungeonSubsystem>()->GetPlayerRoom(Player));
	Context.Player->SetActorLocation(FVector(-1005,0,0));
	TestFalse(TEXT("Capsule-edge entry precedes center crossing"), Room->ContainsLocation(Context.Player->GetActorLocation()));
	TestEqual(TEXT("Capsule overlap admits player without per-frame polling"), Rig.World->GetSubsystem<UMT2DungeonSubsystem>()->GetPlayerRoom(Player), Room);
	Context.Player->SetActorLocation(FVector::ZeroVector);
	AddExpectedError(TEXT("requires a unique nonempty RoomId"), EAutomationExpectedErrorFlags::Contains, 1);
	auto* Duplicate = Rig.Room(TEXT("tower"));
	TestEqual(TEXT("Duplicate cannot replace existing shared stage"), Rig.World->GetSubsystem<UMT2DungeonSubsystem>()->FindRoom(TEXT("tower")), Room);
	Duplicate->Destroy();
	TestEqual(TEXT("Rejected room teardown cannot remove original"), Rig.World->GetSubsystem<UMT2DungeonSubsystem>()->FindRoom(TEXT("tower")), Room);
	FMT2DungeonSpawn InvalidSpawn; InvalidSpawn.Vnum = -1; Room->Spawns.Add(InvalidSpawn);
	AddExpectedError(TEXT("failed encounter spawn"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Invalid spawn explicitly fails initialization"), Room->StartEncounter());
	TestTrue(TEXT("Failed spawn cannot auto-complete stage"), Room->GetPhase() == EMT2DungeonRoomPhase::Failed);
	TestFalse(TEXT("Failure cannot grant completion entitlement"), Room->TryClaimCompletion(Player));
	Room->LeavePlayer(Player);
	TestEqual(TEXT("Quest condition clears on leave"), FMT2QuestExpression::Evaluate(TEXT("pc.in_dungeon()"), Context, bOk).Number, 0.0);
	TestFalse(TEXT("Physical occupancy prevents reset even without admission"), Room->ResetWhenEmpty());
	Context.Player->SetActorLocation(FVector(-5000,0,0));
	TestTrue(TEXT("Failed room can reset after physical exit"), Room->ResetWhenEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2DevilTowerOpeningTest, "Metin2.Dungeons.DevilTowerOpening",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2DevilTowerOpeningTest::RunTest(const FString&)
{
	MT2DungeonFixture::FRig Rig;
	auto* First = Cast<AMT2DevilTowerRoom>(Rig.Room(TEXT("DevilTower.Floor1"), FVector::ZeroVector, AMT2DevilTowerRoom::StaticClass()));
	auto* Second = Cast<AMT2DevilTowerRoom>(Rig.Room(TEXT("DevilTower.Floor2"), FVector(5000,0,0), AMT2DevilTowerRoom::StaticClass()));
	auto* Third = Cast<AMT2DevilTowerRoom>(Rig.Room(TEXT("DevilTower.Floor3"), FVector(10000,0,0), AMT2DevilTowerRoom::StaticClass()));
	First->NextRoom = Second; Second->NextRoom = Third;
	First->TransitionSeconds = .1f; Second->TransitionSeconds = .1f;
	auto* A = Rig.Player(TEXT("A")); auto* B = Rig.Player(TEXT("B"));
	TestTrue(TEXT("Tower entry admits player"), First->JoinPlayer(A)); First->JoinPlayer(B);
	Rig.TimerTick(.01f);
	TestTrue(TEXT("Tower automatically starts on admission"), First->GetPhase() == EMT2DungeonRoomPhase::Active);
	TestEqual(TEXT("Opening legacy stages have no failure deadline"), First->GetSecondsRemaining(), 0.0);
	auto* Stone = Rig.Mob(); First->RegisterEnemy(Stone); Stone->GetHealthComponent()->SetHealth(0.f);
	A->GetPawn()->Destroy(); // The killer's logout must not cancel a shared transition.
	Rig.TimerTick(.1f); Rig.TimerTick(.11f); Rig.TimerTick(.01f);
	TestEqual(TEXT("Room-owned transition survives one participant disconnect"), Rig.World->GetSubsystem<UMT2DungeonSubsystem>()->GetPlayerRoom(B), static_cast<AMT2DungeonRoom*>(Second));
	TestTrue(TEXT("Next floor starts one shared encounter"), Second->GetPhase() == EMT2DungeonRoomPhase::Active);
	auto* Soldier = Rig.Mob(FVector(5000,0,0)); auto* Boss = Rig.Mob(FVector(5100,0,0));
	Second->RegisterEnemy(Soldier); Second->RegisterEnemy(Boss);
	Soldier->GetHealthComponent()->SetHealth(0.f);
	TestTrue(TEXT("Partial elimination cannot advance"), Second->GetPhase() == EMT2DungeonRoomPhase::Active);
	Boss->GetHealthComponent()->SetHealth(0.f);
	Rig.TimerTick(.1f); Rig.TimerTick(.11f); Rig.TimerTick(.01f);
	TestEqual(TEXT("Complete elimination advances to third floor"), Rig.World->GetSubsystem<UMT2DungeonSubsystem>()->GetPlayerRoom(B), static_cast<AMT2DungeonRoom*>(Third));
	TestTrue(TEXT("Third floor starts"), Third->GetPhase() == EMT2DungeonRoomPhase::Active);
	Third->CompleteEncounter(); Rig.TimerTick(5.f);
	TestEqual(TEXT("Unconfigured later stages cannot silently teleport players"), Rig.World->GetSubsystem<UMT2DungeonSubsystem>()->GetPlayerRoom(B), static_cast<AMT2DungeonRoom*>(Third));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2DevilTowerMessagesTest, "Metin2.Dungeons.DevilTowerMessages",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2DevilTowerMessagesTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	auto* Room = World->SpawnActor<AMT2DevilTowerRoom>(); Room->RoomId = TEXT("DevilTower.Floor1"); Room->DispatchBeginPlay();
	TArray<AMT2PlayerState*> Players; TArray<UMT2ChatWidget*> Widgets;
	for (int32 I = 0; I < 3; ++I)
	{
		auto* Controller = World->SpawnActor<AMT2PlayerController>();
		Controller->Player = NewObject<ULocalPlayer>(GEngine);
		auto* Player = World->SpawnActor<AMT2PlayerState>(); Controller->SetPlayerState(Player);
		auto* Pawn = World->SpawnActor<APawn>(); auto* Root = NewObject<USceneComponent>(Pawn);
		Pawn->SetRootComponent(Root); Root->RegisterComponent(); Controller->Possess(Pawn);
		auto* Widget = NewObject<UMT2ChatWidget>(Controller); Widget->WidgetTree = NewObject<UWidgetTree>(Widget);
		Widget->HistoryBox = Widget->WidgetTree->ConstructWidget<UScrollBox>();
		Widget->RewardHistoryBox = Widget->WidgetTree->ConstructWidget<UScrollBox>();
		Controller->ChatWidget = Widget; Players.Add(Player); Widgets.Add(Widget);
	}
	TestTrue(TEXT("First entrant admitted"), Room->JoinPlayer(Players[0]));
	Room->JoinPlayer(Players[0]);
	TestEqual(TEXT("Repeated overlap does not repeat objective"), Widgets[0]->HistoryBox->GetChildrenCount(), 1);
	++GFrameCounter; World->GetTimerManager().Tick(.01f);
	TestTrue(TEXT("Late entrant admitted to active stage"), Room->JoinPlayer(Players[1]));
	TestEqual(TEXT("Late entrant gets current objective"), Widgets[1]->HistoryBox->GetChildrenCount(), 1);
	Room->CompleteEncounter(); Room->CompleteEncounter();
	for (int32 I = 0; I < 2; ++I)
	{
		TestEqual(TEXT("Each participant gets one completion notice"), Widgets[I]->HistoryBox->GetChildrenCount(), 2);
		TestEqual(TEXT("Dungeon messages never enter info/loot log"), Widgets[I]->RewardHistoryBox->GetChildrenCount(), 0);
	}
	TestEqual(TEXT("Nonparticipant does not get broadcast"), Widgets[2]->HistoryBox->GetChildrenCount(), 0);
	Room->JoinPlayer(Players[2]);
	TestEqual(TEXT("Completed-stage arrival gets current completion state"), Widgets[2]->HistoryBox->GetChildrenCount(), 1);
	if (Widgets[2]->HistoryBox->GetChildrenCount() == 1)
		TestEqual(TEXT("Completed arrival does not receive a stale objective"), Cast<UTextBlock>(Widgets[2]->HistoryBox->GetChildAt(0))->GetText().ToString(), Room->GetCompletionMessage().ToString());
	for (int32 Floor = 1; Floor <= 3; ++Floor) { Room->Floor = Floor; TestFalse(TEXT("Every implemented floor has an objective"), Room->GetObjectiveMessage().IsEmpty()); }
	World->EndPlay(EEndPlayReason::Quit); GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2DevilTowerPuzzlesTest, "Metin2.Dungeons.DevilTowerPuzzles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2DevilTowerPuzzlesTest::RunTest(const FString&)
{
	const auto SetVnum = [](AMT2Mob* Mob, int32 Vnum)
	{ FMT2MobDefinition Definition; Definition.Vnum = Vnum; Mob->ConfigureFromDefinition(Definition); };
	{
		MT2DungeonFixture::FRig Rig;
		auto* Room = Cast<AMT2DevilTowerRoom>(Rig.Room(TEXT("puzzle"), FVector::ZeroVector, AMT2DevilTowerRoom::StaticClass()));
		Room->Floor = 4;
		auto* Player = Rig.Player(TEXT("stone-player")); Room->JoinPlayer(Player); Rig.TimerTick(.01f);
		auto* Fake = Rig.Mob(FVector(300,0,0)); auto* Real = Rig.Mob(FVector(-300,0,0));
		Room->RegisterEnemy(Fake); Room->RegisterEnemy(Real);
		Room->FakeStones.Add(Fake); Room->RealStone = Real; Room->bPuzzleStarted = true;
		Fake->GetHealthComponent()->SetHealth(49.f); Room->CheckFakeStones();
		TestTrue(TEXT("Fake stone purged below half health"), Fake->IsActorBeingDestroyed());
		TestTrue(TEXT("Intentional fake purge cannot fail or complete puzzle"), Room->GetPhase() == EMT2DungeonRoomPhase::Active);
		Real->GetHealthComponent()->SetHealth(0.f);
		TestTrue(TEXT("Real kill waits for room-owned ten-second update"), Room->GetPhase() == EMT2DungeonRoomPhase::Active);
		Room->CheckFakeStones();
		TestTrue(TEXT("Real stone completes puzzle without elimination bypass"), Room->GetPhase() == EMT2DungeonRoomPhase::Completed);
		Room->LeavePlayer(Player);
		TestTrue(TEXT("Empty puzzle resets"), Room->ResetWhenEmpty());
		TestFalse(TEXT("Reset clears puzzle state"), Room->bPuzzleStarted);
	}
	{
		MT2DungeonFixture::FRig Rig;
		auto* Room = Cast<AMT2DevilTowerRoom>(Rig.Room(TEXT("seals"), FVector::ZeroVector, AMT2DevilTowerRoom::StaticClass()));
		Room->Floor = 5;
		auto* Player = Rig.Player(TEXT("seal-player"), FVector::ZeroVector, true);
		auto* Pawn = CastChecked<AMT2PlayerCharacter>(Player->GetPawn());
		Pawn->GetHealthComponent()->SetHealth(Pawn->GetHealthComponent()->GetMaxHealth());
		Room->JoinPlayer(Player); Rig.TimerTick(.01f);
		TestTrue(TEXT("Seal stage starts a twenty-minute deadline"), Room->GetSecondsRemaining() > 1199.);
		auto* Inventory = Pawn->GetInventoryComponent();
		if (!TestTrue(TEXT("Key templates usable"), Inventory->AddItemByVnum(50084, 5))) return false;
		auto* Soldier = Rig.Mob(FVector(500,0,0)); SetVnum(Soldier, 1062); Room->RegisterEnemy(Soldier);
		Soldier->GetHealthComponent()->SetHealth(0.f);
		TestEqual(TEXT("One death counts once independent of damage-credit recipients"), Room->Kills, 1);
		TestTrue(TEXT("Elimination cannot bypass seals"), Room->GetPhase() == EMT2DungeonRoomPhase::Active);
		for (int32 Index = 0; Index < 5; ++Index)
		{
			auto* Seal = Rig.Mob(FVector(200,0,0)); SetVnum(Seal, 20073); Room->Supports.Add(Seal);
			const int32 Slot = Inventory->GetSlots().IndexOfByPredicate([](const auto& Item) { return Item.Vnum == 50084 && !Item.IsEmpty(); });
			TestTrue(TEXT("Valid nearby seal consumes one key"), Room->HandleItem(Player, Slot, Seal));
			TestFalse(TEXT("Destroyed seal cannot consume another key"), Room->HandleItem(Player, Slot, Seal));
		}
		TestEqual(TEXT("Exactly five keys consumed"), Inventory->CountItemByVnum(50084), 0);
		TestTrue(TEXT("Fifth seal completes shared stage"), Room->GetPhase() == EMT2DungeonRoomPhase::Completed);
		TestFalse(TEXT("Completion cancels regeneration"), Rig.World->GetTimerManager().TimerExists(Room->RegenTimer));
		TestFalse(TEXT("Completion cancels warning timers"), Rig.World->GetTimerManager().TimerExists(Room->WarningTimer));
	}
	{
		MT2DungeonFixture::FRig Rig;
		auto* Room = Cast<AMT2DevilTowerRoom>(Rig.Room(TEXT("maps"), FVector::ZeroVector, AMT2DevilTowerRoom::StaticClass()));
		Room->Floor = 7;
		auto* Player = Rig.Player(TEXT("map-player"), FVector::ZeroVector, true);
		auto* Pawn = CastChecked<AMT2PlayerCharacter>(Player->GetPawn());
		Pawn->GetHealthComponent()->SetHealth(Pawn->GetHealthComponent()->GetMaxHealth());
		Room->JoinPlayer(Player); Rig.TimerTick(.01f);
		auto* Inventory = Pawn->GetInventoryComponent();
		TestTrue(TEXT("Real map template available"), Inventory->AddItemByVnum(30302, 1));
		TestFalse(TEXT("Carried map cannot bypass this run's stones"), Room->HandleItem(Player, 0, nullptr));
		Room->bMapFound = true; Room->Stones = 4;
		TestTrue(TEXT("Real map advances completed map puzzle"), Room->HandleItem(Player, 0, nullptr));
		TestFalse(TEXT("Map replay rejected"), Room->HandleItem(Player, 0, nullptr));
		TestTrue(TEXT("Map stage completed"), Room->GetPhase() == EMT2DungeonRoomPhase::Completed);
	}
	{
		MT2DungeonFixture::FRig Rig;
		auto* Room = Cast<AMT2DevilTowerRoom>(Rig.Room(TEXT("final"), FVector::ZeroVector, AMT2DevilTowerRoom::StaticClass()));
		Room->Floor = 9; Room->TransitionSeconds = 60;
		auto* First = Rig.Player(TEXT("first")); Room->JoinPlayer(First); Rig.TimerTick(.01f);
		auto* Late = Rig.Player(TEXT("late")); Room->JoinPlayer(Late);
		auto* Boss = Rig.Mob(FVector(500,0,0)); SetVnum(Boss, 1093); Room->RegisterEnemy(Boss);
		Boss->GetHealthComponent()->SetHealth(0.f);
		TestTrue(TEXT("Final boss completes the stage"), Room->GetPhase() == EMT2DungeonRoomPhase::Completed);
		for (auto* Member : {First, Late}) TestEqual(TEXT("Participants including late joiner get completion flag"), Member->GetQuestManagerComponent()->GetQuestFlag(TEXT("deviltower_zone.9_done")), 1);
		auto* After = Rig.Player(TEXT("after")); Room->JoinPlayer(After);
		TestEqual(TEXT("Post-completion arrival cannot inherit reward"), After->GetQuestManagerComponent()->GetQuestFlag(TEXT("deviltower_zone.9_done")), 0);
		TestTrue(TEXT("Final exit timer owned by room"), Rig.World->GetTimerManager().TimerExists(Room->AdvanceTimer));
	}
	{
		MT2DungeonFixture::FRig Rig;
		auto* Room = Cast<AMT2DevilTowerRoom>(Rig.Room(TEXT("blacksmith"), FVector::ZeroVector, AMT2DevilTowerRoom::StaticClass()));
		Room->Floor = 6; Room->NextRoom = Rig.Room(TEXT("upper"), FVector(5000,0,0));
		for (int32 Vnum : {20074,20075,20076})
		{ auto& Spawn = Room->PuzzleSpawns.AddDefaulted_GetRef(); Spawn.Vnum = Vnum; Spawn.LocalTransform.SetLocation(FVector(500,0,120)); }
		auto* Player = Rig.Player(TEXT("reward-player"), FVector::ZeroVector, true);
		Room->JoinPlayer(Player); Rig.TimerTick(.01f);
		auto* Boss = Rig.Mob(FVector(-500,0,0)); SetVnum(Boss, 1092); Room->RegisterEnemy(Boss); Boss->GetHealthComponent()->SetHealth(0.f);
		TestTrue(TEXT("Proud Demon King completes boss stage"), Room->GetPhase() == EMT2DungeonRoomPhase::Completed);
		TestEqual(TEXT("One randomly selected blacksmith"), Room->Supports.Num(), 1);
		TestFalse(TEXT("Blacksmith reward stage does not automatically advance"), Rig.World->GetTimerManager().TimerExists(Room->AdvanceTimer));
		if (!Room->Supports.IsEmpty() && Room->Supports[0].IsValid())
		{
			auto* Blacksmith = Room->Supports[0].Get(); Blacksmith->SetActorLocation(FVector(200,0,120));
			TestTrue(TEXT("Eligible participant gets material-free refinement entitlement"), Room->CanRefine(Player, Blacksmith));
			auto* Inventory = CastChecked<AMT2PlayerCharacter>(Player->GetPawn())->GetInventoryComponent();
			TestTrue(TEXT("Refinable sword template available"), Inventory->AddItemByVnum(16, 1));
			const int32 SwordSlot = Inventory->GetSlots().IndexOfByPredicate([](const auto& Item) { return Item.Vnum == 16 && !Item.IsEmpty(); });
			const auto* Sword = Inventory->GetTemplateAtSlot(SwordSlot);
			TestTrue(TEXT("Imported sword has a working refinement recipe"), Inventory->CanRefineItem(SwordSlot));
			if (TestNotNull(TEXT("Sword recipe template"), Sword) && Inventory->CanRefineItem(SwordSlot))
			{
				TestFalse(TEXT("Recipe normally requires materials"), Sword->RefinementRecipe.Materials.IsEmpty());
				Player->AddYang(Sword->RefinementRecipe.YangCost + 100);
				const int64 Before = Player->GetYang();
				const auto Result = Inventory->RefineItem(SwordSlot, INDEX_NONE, true);
				TestTrue(TEXT("Money-only refinement performs one normal success/failure roll without materials"), Result == EMT2RefinementResult::Success || Result == EMT2RefinementResult::FailedDestroyed);
				TestEqual(TEXT("Money-only refinement still charges recipe Yang"), Player->GetYang(), Before - Sword->RefinementRecipe.YangCost);
			}
			auto* After = Rig.Player(TEXT("after-reward"), FVector(0,100,0), true); Room->JoinPlayer(After);
			After->GetQuestManagerComponent()->SetQuestFlag(TEXT("deviltower_zone.can_refine"), Room->GetRunSerial());
			TestFalse(TEXT("Stale saved flag cannot reward post-completion arrival"), Room->CanRefine(After, Blacksmith));
			TestFalse(TEXT("A stale NPC dialogue epoch cannot advance"), Room->SelectUpperFloors(Player, Blacksmith, Room->GetRunSerial() - 1, false));
		}
	}
	{
		MT2DungeonFixture::FRig Rig;
		auto* Room = Cast<AMT2DevilTowerRoom>(Rig.Room(TEXT("key"), FVector::ZeroVector, AMT2DevilTowerRoom::StaticClass()));
		Room->Floor = 8;
		auto* Player = Rig.Player(TEXT("key-player"), FVector::ZeroVector, true);
		Room->JoinPlayer(Player); Rig.TimerTick(.01f);
		auto* Seal = Rig.Mob(FVector(200,0,0)); SetVnum(Seal, 20366); Room->Supports.Add(Seal);
		auto* Inventory = CastChecked<AMT2PlayerCharacter>(Player->GetPawn())->GetInventoryComponent();
		Inventory->AddItemByVnum(30303, 1);
		TestFalse(TEXT("Fake key cannot open Sa-Soe seal"), Room->HandleItem(Player, 0, Seal));
		Inventory->AddItemByVnum(30304, 1);
		const int32 Slot = Inventory->GetSlots().IndexOfByPredicate([](const auto& Item) { return Item.Vnum == 30304 && !Item.IsEmpty(); });
		Seal->SetActorLocation(FVector(600,0,0));
		TestFalse(TEXT("Remote hand-in rejected"), Room->HandleItem(Player, Slot, Seal));
		Seal->SetActorLocation(FVector(200,0,0));
		TestTrue(TEXT("Real key opens owned seal"), Room->HandleItem(Player, Slot, Seal));
		TestTrue(TEXT("Key stage completed"), Room->GetPhase() == EMT2DungeonRoomPhase::Completed);
		TestEqual(TEXT("Fake key preserved"), Inventory->CountItemByVnum(30303), 1);
	}
	return true;
}
#endif
