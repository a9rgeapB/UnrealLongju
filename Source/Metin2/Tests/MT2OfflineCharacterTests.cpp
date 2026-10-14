#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Authentication/MT2OfflineSessionSubsystem.h"
#include "Persistence/MT2PersistenceBackend.h"
#include "Persistence/MT2PersistenceManager.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Widgets/SOverlay.h"
#include "Slate/SGameLayerManager.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"
#include "Misc/ScopeExit.h"
#include "Authentication/MT2ClientSessionSubsystem.h"
#include "Player/MT2PlayerController.h"
#include "Player/MT2PlayerState.h"
#include "Game/MT2GameModeBase.h"
#include "Persistence/MT2PersistenceComponent.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerStart.h"
#include "Components/BoxComponent.h"
#include "AbilitySystemComponent.h"
#include "Abilities/MT2CoreAttributeSet.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Items/MT2InventoryComponent.h"
#include "Voice/MT2VoiceRelaySubsystem.h"
#include "Server/MT2AdminSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2OfflineLaunchTest, "Metin2.Offline.LaunchPolicy", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2OfflineLaunchTest::RunTest(const FString&)
{
	TestTrue(TEXT("Standalone Game launches offline"), UMT2OfflineSessionSubsystem::ShouldUseOfflineFlow(NM_Standalone, false, false, TEXT("")));
	TestFalse(TEXT("Online Client startup cannot become offline"), UMT2OfflineSessionSubsystem::ShouldUseOfflineFlow(NM_Standalone, true, false, TEXT("")));
	TestFalse(TEXT("Server target cannot become offline"), UMT2OfflineSessionSubsystem::ShouldUseOfflineFlow(NM_Standalone, false, true, TEXT("")));
	TestFalse(TEXT("Network client remains online"), UMT2OfflineSessionSubsystem::ShouldUseOfflineFlow(NM_Client, false, false, TEXT("")));
	TestFalse(TEXT("Listen server remains online"), UMT2OfflineSessionSubsystem::ShouldUseOfflineFlow(NM_ListenServer, false, false, TEXT("")));
	for (const TCHAR* Args : {TEXT("-Gateway"), TEXT("-Coordinator"), TEXT("-server"), TEXT("-connect=127.0.0.1:11000"), TEXT("127.0.0.1:11000")})
		TestFalse(Args, UMT2OfflineSessionSubsystem::ShouldUseOfflineFlow(NM_Standalone, false, false, Args));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2OfflinePersistenceTest, "Metin2.Offline.LocalPersistence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2OfflinePersistenceTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GEngine));
	GI->InitializeStandalone();
	UWorld* World = GI->GetWorld();
	ON_SCOPE_EXIT { GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	auto* Manager = GI->GetSubsystem<UMT2PersistenceManager>();
	const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/Offline"), FGuid::NewGuid().ToString());
	FString Error;
	if (!TestTrue(TEXT("Separate offline persistence enabled"), Manager->EnableOfflinePersistence(Directory, Error))) { AddError(Error); return false; }
	FMT2AccountLoginResult Profile;
	Manager->LoadOfflineProfile([&Profile](FMT2AccountLoginResult&& Result) { Profile = MoveTemp(Result); });
	if (!TestTrue(TEXT("Local profile ready without credentials"), Profile.bSucceeded)) { AddError(Profile.Error); return false; }
	TestEqual(TEXT("New profile is empty"), Profile.Characters.Num(), 0);
	FMT2CharacterAppearance Appearance;
	FMT2CharacterCreateResult Created;
	Manager->CreateCharacter(Profile.AccountId, TEXT("OfflineHero"), Appearance, EMT2Empire::Jinno, TEXT("pyungmoo"),
		[&Created](FMT2CharacterCreateResult&& Result) { Created = MoveTemp(Result); });
	TestTrue(TEXT("Local character creation commits"), Created.bSucceeded);
	const FString CharacterId = Created.Character.CharacterId;
	Manager->CreateCharacter(Profile.AccountId, TEXT("OfflineHero"), Appearance, EMT2Empire::Jinno, TEXT("pyungmoo"),
		[&Created](FMT2CharacterCreateResult&& Result) { Created = MoveTemp(Result); });
	TestFalse(TEXT("Duplicate name rejected"), Created.bSucceeded);
	Appearance.Race = static_cast<EMT2CharacterRace>(255);
	Manager->CreateCharacter(Profile.AccountId, TEXT("InvalidHero"), Appearance, EMT2Empire::Jinno, TEXT("pyungmoo"),
		[&Created](FMT2CharacterCreateResult&& Result) { Created = MoveTemp(Result); });
	TestFalse(TEXT("Invalid appearance rejected"), Created.bSucceeded);
	Appearance.Race = EMT2CharacterRace::Warrior;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Manager->CreateCharacter(Profile.AccountId, FString::Printf(TEXT("Hero%d"), Index), Appearance, EMT2Empire::Jinno, TEXT("pyungmoo"),
			[&Created](FMT2CharacterCreateResult&& Result) { Created = MoveTemp(Result); });
		TestEqual(TEXT("Four slot limit enforced"), Created.bSucceeded, Index < 3);
	}
	Manager->FindCharacter(TEXT("foreign-profile"), CharacterId, [&Created](FMT2CharacterCreateResult&& Result) { Created = MoveTemp(Result); });
	TestFalse(TEXT("Foreign identity cannot select character"), Created.bSucceeded);
	Manager->Deinitialize();
	TestTrue(TEXT("Same offline database reopens"), Manager->EnableOfflinePersistence(Directory, Error));
	Manager->LoadOfflineProfile([&Profile](FMT2AccountLoginResult&& Result) { Profile = MoveTemp(Result); });
	TestTrue(TEXT("Reopened profile available"), Profile.bSucceeded);
	TestEqual(TEXT("Roster persists across reopen"), Profile.Characters.Num(), 4);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2OfflineSelectionTest, "Metin2.Offline.CharacterSelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2OfflineSelectionTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GEngine)); GI->InitializeStandalone();
	UWorld* World = GI->GetWorld();
	ON_SCOPE_EXIT { GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	FString Error;
	const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/Offline"), FGuid::NewGuid().ToString());
	if (!TestTrue(TEXT("Offline store ready"), GI->GetSubsystem<UMT2PersistenceManager>()->EnableOfflinePersistence(Directory, Error))) return false;
	auto* Controller = World->SpawnActor<AMT2PlayerController>();
	Controller->SetAsLocalPlayerController();
	auto* Offline = GI->GetSubsystem<UMT2OfflineSessionSubsystem>();
	auto* Session = GI->GetSubsystem<UMT2ClientSessionSubsystem>();
	Offline->BeginCharacterSelection(Controller);
	TestTrue(TEXT("Offline session active"), Offline->IsReady());
	TestEqual(TEXT("Login skipped for selection"), Session->GetState(), EMT2ClientSessionState::CharacterSelection);
	Offline->CreateCharacter(Controller, TEXT("LocalWarrior"), {}, EMT2Empire::Jinno);
	TestEqual(TEXT("Local character appears immediately"), Session->GetCharacters().Num(), 1);
	Offline->CreateCharacter(Controller, TEXT("LocalWarrior"), {}, EMT2Empire::Jinno);
	TestEqual(TEXT("Rejected creation does not add a roster entry"), Session->GetCharacters().Num(), 1);
	Offline->BeginCharacterSelection(Controller);
	TestEqual(TEXT("Returning to selection does not duplicate roster"), Session->GetCharacters().Num(), 1);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2OfflineMapSelectionTest, "Metin2.Offline.MapSelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2OfflineMapSelectionTest::RunTest(const FString&)
{
	FString Package, Error;
	TestTrue(TEXT("Imported town map resolves"), UMT2OfflineSessionSubsystem::ResolveMapPackage(TEXT("pyungmoo"), Package, Error));
	TestEqual(TEXT("Local gameplay package"), Package, FString(TEXT("/Game/Maps/Game/pyungmoo")));
	TestFalse(TEXT("Missing cooked map rejected"), UMT2OfflineSessionSubsystem::ResolveMapPackage(TEXT("nonexistent_offline_map"), Package, Error));
	TestFalse(TEXT("Map path injection rejected"), UMT2OfflineSessionSubsystem::ResolveMapPackage(TEXT("../System/Gateway?listen"), Package, Error));
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GEngine)); GI->InitializeStandalone();
	UWorld* World = GI->GetWorld();
	ON_SCOPE_EXIT { GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	GI->GetSubsystem<UMT2PersistenceManager>()->EnableOfflinePersistence(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/Offline"), FGuid::NewGuid().ToString()), Error);
	auto* Controller = World->SpawnActor<AMT2PlayerController>(); Controller->SetAsLocalPlayerController();
	auto* Offline = GI->GetSubsystem<UMT2OfflineSessionSubsystem>();
	Offline->BeginCharacterSelection(Controller);
	Offline->CreateCharacter(Controller, TEXT("MapHero"), {}, EMT2Empire::Jinno);
	const auto* Session = GI->GetSubsystem<UMT2ClientSessionSubsystem>();
	if (!TestEqual(TEXT("Test character created"), Session->GetCharacters().Num(), 1)) return false;
	TestFalse(TEXT("Invalid character cannot be selected"), Offline->PrepareCharacterSelection(TEXT("foreign"), Package, Error));
	TestFalse(TEXT("Invalid selection leaves identity empty"), Offline->HasSelectedCharacter());
	TestTrue(TEXT("Owned character can enter local map"), Offline->PrepareCharacterSelection(Session->GetCharacters()[0].CharacterId, Package, Error));
	TestTrue(TEXT("Identity survives controller replacement"), Offline->HasSelectedCharacter());
	TestEqual(TEXT("Selected character name"), Offline->GetSelectedCharacter().CharacterName, FString(TEXT("MapHero")));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2OfflineRestoreTest, "Metin2.Offline.RestoreAndSave", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2OfflineRestoreTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GEngine)); GI->InitializeStandalone();
	UWorld* World = GI->GetWorld();
	TStrongObjectPtr<UGameViewportClient> Viewport(NewObject<UGameViewportClient>(GEngine));
	FWorldContext& Context = GEngine->GetWorldContextFromWorldChecked(World);
	Context.GameViewport = Viewport.Get();
	Viewport->Init(Context, GI.Get(), false);
	const TSharedRef<SOverlay> Overlay = SNew(SOverlay);
	const TSharedRef<SGameLayerManager> Layers = SNew(SGameLayerManager)[Overlay];
	Viewport->SetViewportOverlayWidget(nullptr, Overlay);
	Viewport->SetGameLayerManager(Layers);
	ON_SCOPE_EXIT { GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	FURL URL; URL.AddOption(TEXT("game=/Script/Metin2.MT2GameModeBase"));
	World->SetGameMode(URL); World->InitializeActorsForPlay(URL); World->GetWorldSettings()->NotifyBeginPlay();
	FString Error, Package;
	auto* Manager = GI->GetSubsystem<UMT2PersistenceManager>();
	Manager->EnableOfflinePersistence(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/Offline"), FGuid::NewGuid().ToString()), Error);
	auto* Controller = World->SpawnActor<AMT2PlayerController>();
	ULocalPlayer* LocalPlayer = GI->CreateLocalPlayer(0, Error, false);
	if (!TestNotNull(TEXT("Local player for gameplay UI notifications"), LocalPlayer)) return false;
	Controller->SetPlayer(LocalPlayer);
	auto* Offline = GI->GetSubsystem<UMT2OfflineSessionSubsystem>();
	Offline->BeginCharacterSelection(Controller); Offline->CreateCharacter(Controller, TEXT("SavedHero"), {}, EMT2Empire::Jinno);
	const auto* Session = GI->GetSubsystem<UMT2ClientSessionSubsystem>();
	if (!TestEqual(TEXT("Character ready"), Session->GetCharacters().Num(), 1)) return false;
	const FString CharacterId = Session->GetCharacters()[0].CharacterId;
	if (!TestTrue(TEXT("Select local character"), Offline->PrepareCharacterSelection(CharacterId, Package, Error))) return false;
	auto* State = Controller->GetPlayerState<AMT2PlayerState>();
	if (!TestNotNull(TEXT("Native PlayerState created"), State)) return false;
	State->GetAbilitySystemComponent()->AddAttributeSetSubobject(State->GetCoreAttributes());
	State->GetAbilitySystemComponent()->InitAbilityActorInfo(State, State);
	World->SpawnActor<APlayerStart>();
	auto* Presentation = World->SpawnActor<AMT2MapPresentationActor>();
	Presentation->Configure(TEXT("pyungmoo"), 21, FIntPoint(1, 1), FVector2D(-1000, -1000), FVector2D(1000, 1000), {});
	auto* Ground = World->SpawnActor<AActor>();
	auto* Collision = NewObject<UBoxComponent>(Ground);
	Ground->SetRootComponent(Collision); Collision->SetBoxExtent(FVector(1000, 1000, 10));
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); Collision->SetCollisionObjectType(ECC_WorldStatic);
	Collision->SetCollisionResponseToAllChannels(ECR_Block); Collision->RegisterComponent();
	auto* Mode = World->GetAuthGameMode<AMT2GameModeBase>();
	if (!TestNotNull(TEXT("Offline gameplay GameMode"), Mode)) return false;
	Mode->StartOfflinePlayer(Controller);
	TestTrue(TEXT("Restoration completes before world-ready"), Offline->IsPlayerLoaded());
	TestTrue(TEXT("New local character has admin permissions"), State->IsAdmin());
	TestEqual(TEXT("Restored character name"), State->GetCharacterName(), FString(TEXT("SavedHero")));
	TestEqual(TEXT("Restored empire"), State->GetEmpire(), EMT2Empire::Jinno);
	if (!TestTrue(TEXT("Playable pawn possessed"), Controller->GetPawn() != nullptr)) return false;
	State->SetCharacterLevel(12);
	auto* Player = Cast<AMT2PlayerCharacter>(Controller->GetPawn());
	if (!TestNotNull(TEXT("Player inventory owner"), Player)) return false;
	TestTrue(TEXT("Collect a saved item"), Player->GetInventoryComponent()->AddItemByVnum(73001, 1));
	TestTrue(TEXT("Final save commits synchronously"), Offline->SavePlayer(Controller, Error));
	FMT2PersistenceLoadResult Saved;
	Manager->LoadRecord(TEXT("player"), CharacterId, [&Saved](FMT2PersistenceLoadResult&& Result) { Saved = MoveTemp(Result); });
	TestTrue(TEXT("Saved player is durable"), Saved.bSucceeded && Saved.bFound);
	TestTrue(TEXT("Saved level preserved"), Saved.Record.PayloadJson.Contains(TEXT("\"level\":12")) || Saved.Record.PayloadJson.Contains(TEXT("\"level\": 12")));
	TestTrue(TEXT("Saved world position present"), Saved.Record.bHasWorldPosition);
	Offline->MarkPlayerLoaded(false);
	Controller->UnPossess(); Player->Destroy();
	State->SetCharacterLevel(2);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mode->StartOfflinePlayer(Controller);
	TestFalse(TEXT("Restored character waits for ground collision"), Offline->IsPlayerLoaded());
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mode->FinishOfflineSpawn(Controller, false, 1);
	TestTrue(TEXT("Restored character admitted after ground becomes ready"), Offline->IsPlayerLoaded());
	TestTrue(TEXT("Existing local character retains admin permissions on re-entry"), State->IsAdmin());
	TestEqual(TEXT("Saved level restores on re-entry"), State->GetCharacterLevel(), 12);
	Player = Cast<AMT2PlayerCharacter>(Controller->GetPawn());
	if (!TestNotNull(TEXT("New player pawn on re-entry"), Player)) return false;
	TestEqual(TEXT("Collected item restores without duplication"), Player->GetInventoryComponent()->CountItemByVnum(73001), 1);
	Offline->MarkPlayerLoaded(false);
	TestFalse(TEXT("Incomplete restoration cannot overwrite character"), Offline->SavePlayer(Controller, Error));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2OfflineAdminTest, "Metin2.Offline.AdminPermissions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2OfflineAdminTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GEngine)); GI->InitializeStandalone();
	UWorld* World = GI->GetWorld();
	ON_SCOPE_EXIT { GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	FString Error;
	auto* Manager = GI->GetSubsystem<UMT2PersistenceManager>();
	if (!TestTrue(TEXT("Isolated local store ready"), Manager->EnableOfflinePersistence(
		FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/Offline"), FGuid::NewGuid().ToString()), Error))) return false;
	auto* Admin = GI->GetSubsystem<UMT2AdminSubsystem>();
	bool bAdmin = true;
	Admin->QueryAdminStatus(World, TEXT("LocalAdminHero"), [&bAdmin](bool Result) { bAdmin = Result; });
	TestFalse(TEXT("Standalone alone does not grant admin without an offline session"), bAdmin);
	auto* Controller = World->SpawnActor<AMT2PlayerController>(); Controller->SetAsLocalPlayerController();
	auto* Offline = GI->GetSubsystem<UMT2OfflineSessionSubsystem>();
	Offline->BeginCharacterSelection(Controller);
	for (const TCHAR* Name : { TEXT("LocalAdminHero"), TEXT("OtherLocalHero") })
	{
		Offline->CreateCharacter(Controller, Name, {}, EMT2Empire::Jinno);
		bAdmin = false;
		Admin->QueryAdminStatus(World, Name, [&bAdmin](bool Result) { bAdmin = Result; });
		TestTrue(FString::Printf(TEXT("Local character %s is admin without a registry entry"), Name), bAdmin);
	}
	TestEqual(TEXT("Both local characters created"), GI->GetSubsystem<UMT2ClientSessionSubsystem>()->GetCharacters().Num(), 2);
	Admin->QueryAdminStatus(nullptr, TEXT("LocalAdminHero"), [&bAdmin](bool Result) { bAdmin = Result; });
	TestFalse(TEXT("Offline privilege requires a standalone world"), bAdmin);
	FMT2PersistenceLoadResult RegistryEntry;
	Manager->LoadRecord(TEXT("admin"), TEXT("localadminhero"), [&RegistryEntry](FMT2PersistenceLoadResult&& Result) { RegistryEntry = MoveTemp(Result); });
	TestTrue(TEXT("Admin registry lookup succeeds"), RegistryEntry.bSucceeded);
	TestFalse(TEXT("Offline privilege creates no admin registry record"), RegistryEntry.bFound);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2OfflineVoiceTest, "Metin2.Offline.NoVoiceRelay", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2OfflineVoiceTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GEngine)); GI->InitializeStandalone();
	UWorld* World = GI->GetWorld();
	ON_SCOPE_EXIT { GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	FString Error;
	GI->GetSubsystem<UMT2PersistenceManager>()->EnableOfflinePersistence(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/Offline"), FGuid::NewGuid().ToString()), Error);
	auto* Controller = World->SpawnActor<AMT2PlayerController>(); Controller->SetAsLocalPlayerController();
	GI->GetSubsystem<UMT2OfflineSessionSubsystem>()->BeginCharacterSelection(Controller);
	auto* Relay = World->GetSubsystem<UMT2VoiceRelaySubsystem>();
	Relay->Tick(.01f);
	TestTrue(TEXT("Offline game opens no voice listening socket"), Relay->Socket == nullptr);
	return true;
}
#endif
