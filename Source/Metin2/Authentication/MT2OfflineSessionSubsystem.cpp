#include "Authentication/MT2OfflineSessionSubsystem.h"

#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformProperties.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Player/MT2PlayerController.h"
#include "Persistence/MT2PersistenceManager.h"
#include "Authentication/MT2ClientSessionSubsystem.h"
#include "Config/MT2PathSettings.h"
#include "Misc/PackageName.h"
#include "Engine/Engine.h"
#include "Player/MT2PlayerState.h"
#include "Persistence/MT2PersistenceComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogMT2Offline, Log, All);

void UMT2OfflineSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (GEngine) TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UMT2OfflineSessionSubsystem::HandleTravelFailure);
	WorldTeardownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &UMT2OfflineSessionSubsystem::HandleWorldTeardown);
}

void UMT2OfflineSessionSubsystem::Deinitialize()
{
	if (GEngine) GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	FWorldDelegates::OnWorldBeginTearDown.Remove(WorldTeardownHandle);
	Super::Deinitialize();
}

bool UMT2OfflineSessionSubsystem::ShouldUseOfflineFlow(
	ENetMode NetMode, bool bClientOnly, bool bServerOnly, const FString& CommandLine)
{
	if (NetMode != NM_Standalone || bClientOnly || bServerOnly ||
		FParse::Param(*CommandLine, TEXT("server")) || FParse::Param(*CommandLine, TEXT("Gateway")) ||
		FParse::Param(*CommandLine, TEXT("Coordinator"))) return false;
	FString Address;
	if (FParse::Value(*CommandLine, TEXT("connect="), Address)) return false;
	TArray<FString> Tokens;
	CommandLine.ParseIntoArrayWS(Tokens);
	for (const FString& Token : Tokens)
	{
		if (Token.StartsWith(TEXT("-")) || Token.StartsWith(TEXT("/"))) continue;
		int32 Colon;
		if (Token.FindLastChar(TEXT(':'), Colon) && Colon > 0 && Token.Mid(Colon + 1).IsNumeric()) return false;
	}
	return true;
}

void UMT2OfflineSessionSubsystem::BeginCharacterSelection(AMT2PlayerController* Controller)
{
	if (!Controller || Controller->GetGameInstance() != GetGameInstance() || !Controller->IsLocalController() ||
		!ShouldUseOfflineFlow(Controller->GetNetMode(), FPlatformProperties::IsClientOnly(),
			FPlatformProperties::IsServerOnly(), FCommandLine::Get())) return;
	bActive = true;
	SelectedCharacter = {};
	bPlayerLoaded = false;
	bOperationPending = false;
	UMT2PersistenceManager* Manager = GetGameInstance()->GetSubsystem<UMT2PersistenceManager>();
	FString Error;
	const FString Directory = FPaths::Combine(FPlatformProcess::UserSettingsDir(), TEXT("UnrealLongju/Offline"));
	if (!Manager || !Manager->EnableOfflinePersistence(Directory, Error))
	{
		AccountId.Reset();
		Controller->ClientLoginResult_Implementation(false, Error.IsEmpty() ? TEXT("Offline save storage is unavailable.") : Error, {});
		Controller->ClientTravelFailed_Implementation(Error);
		return;
	}
	Manager->LoadOfflineProfile([this, Controller](FMT2AccountLoginResult&& Result)
	{
		AccountId = Result.bSucceeded ? Result.AccountId : FString();
		if (Result.bSucceeded && !PendingEntryError.IsEmpty()) Result.Error = MoveTemp(PendingEntryError);
		Controller->ClientLoginResult_Implementation(Result.bSucceeded, Result.Error, Result.Characters);
		if (!Result.bSucceeded) Controller->ClientTravelFailed_Implementation(Result.Error);
	});
}

bool UMT2OfflineSessionSubsystem::ResolveMapPackage(const FString& MapId, FString& OutPackage, FString& OutError)
{
	OutPackage.Reset();
	OutError.Reset();
	const FString CleanId = MapId.TrimStartAndEnd().ToLower();
	if (CleanId.IsEmpty()) { OutError = TEXT("The saved character has no map."); return false; }
	for (const TCHAR Char : CleanId)
	{
		if (!FChar::IsAlnum(Char) && Char != TEXT('_')) { OutError = TEXT("The saved map identifier is invalid."); return false; }
	}
	const FString Package = UMT2PathSettings::Format(TEXT("GameMapTemplate"), TEXT("%s"), *CleanId);
	if (!FPackageName::DoesPackageExist(Package))
	{
		OutError = FString::Printf(TEXT("The map '%s' is not included in this build."), *CleanId);
		return false;
	}
	OutPackage = Package;
	return true;
}

bool UMT2OfflineSessionSubsystem::PrepareCharacterSelection(const FString& CharacterId, FString& OutPackage, FString& OutError)
{
	OutPackage.Reset();
	if (!IsReady() || bOperationPending || HasSelectedCharacter()) { OutError = TEXT("Offline selection is unavailable or busy."); return false; }
	FMT2CharacterCreateResult Result;
	GetGameInstance()->GetSubsystem<UMT2PersistenceManager>()->FindCharacter(AccountId, CharacterId,
		[&Result](FMT2CharacterCreateResult&& Found) { Result = MoveTemp(Found); });
	if (!Result.bSucceeded) { OutError = Result.Error; return false; }
	if (!ResolveMapPackage(Result.Character.MapId, OutPackage, OutError)) return false;
	SelectedCharacter = Result.Character;
	bPlayerLoaded = false;
	return true;
}

void UMT2OfflineSessionSubsystem::SelectCharacter(AMT2PlayerController* Controller, const FString& CharacterId)
{
	if (!Controller || Controller->GetGameInstance() != GetGameInstance() || !Controller->IsLocalController()) return;
	FString Package, Error;
	if (!PrepareCharacterSelection(CharacterId, Package, Error)) { Controller->ClientTravelFailed_Implementation(Error); return; }
	bOperationPending = true;
	GetGameInstance()->GetSubsystem<UMT2ClientSessionSubsystem>()->MarkTravelingToMap();
	Controller->ClientTravel(Package, TRAVEL_Absolute);
}

void UMT2OfflineSessionSubsystem::AbortWorldEntry(AMT2PlayerController* Controller, const FString& Error)
{
	bPlayerLoaded = false;
	bOperationPending = false;
	SelectedCharacter = {};
	PendingEntryError = Error;
	UE_LOG(LogMT2Offline, Error, TEXT("Offline world entry failed: %s"), *Error);
	if (Controller)
	{
		Controller->ClientTravelFailed_Implementation(Error);
		Controller->ClientTravel(UMT2PathSettings::Path(TEXT("Maps_System_Gateway")), TRAVEL_Absolute);
	}
}

void UMT2OfflineSessionSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type, const FString& Error)
{
	if (!bActive || !World || World->GetGameInstance() != GetGameInstance()) return;
	bOperationPending = false;
	if (!bPlayerLoaded) SelectedCharacter = {};
	if (auto* Controller = Cast<AMT2PlayerController>(GetGameInstance()->GetFirstLocalPlayerController())) Controller->ClientTravelFailed_Implementation(Error);
}

void UMT2OfflineSessionSubsystem::CreateCharacter(AMT2PlayerController* Controller,
	const FString& Name, const FMT2CharacterAppearance& Appearance, EMT2Empire Empire)
{
	if (!Controller || Controller->GetGameInstance() != GetGameInstance() || !Controller->IsLocalController()) return;
	if (!IsReady() || bOperationPending || HasSelectedCharacter())
	{
		Controller->ClientCharacterCreated_Implementation(false, TEXT("Offline character selection is unavailable or busy."), {});
		return;
	}
	TGuardValue<bool> Pending(bOperationPending, true);
	UMT2PersistenceManager* Manager = GetGameInstance()->GetSubsystem<UMT2PersistenceManager>();
	Manager->CreateCharacter(AccountId, Name, Appearance, Empire, MT2GetEmpireStartMap(Empire),
		[Controller](FMT2CharacterCreateResult&& Result)
		{
			Controller->ClientCharacterCreated_Implementation(Result.bSucceeded, Result.Error, Result.Character);
		});
}

bool UMT2OfflineSessionSubsystem::SavePlayer(AMT2PlayerController* Controller, FString& OutError)
{
	check(IsInGameThread());
	OutError.Reset();
	if (!bActive || !HasSelectedCharacter()) return true;
	auto* Manager = GetGameInstance()->GetSubsystem<UMT2PersistenceManager>();
	auto* State = Controller ? Controller->GetPlayerState<AMT2PlayerState>() : nullptr;
	auto* Persistence = State ? State->GetPersistenceComponent() : nullptr;
	if (!bPlayerLoaded || !Controller || Controller->GetGameInstance() != GetGameInstance() ||
		Controller->GetNetMode() != NM_Standalone || !Manager || !Manager->IsOfflinePersistence() || !Persistence ||
		Persistence->GetEntityId() != SelectedCharacter.CharacterId || !Controller->GetPawn())
	{
		OutError = TEXT("The offline character has not finished loading; its saved data was not changed.");
		return false;
	}
	if (Persistence->GetPersistenceState() == EMT2PersistenceState::Loading ||
		Persistence->GetPersistenceState() == EMT2PersistenceState::Saving)
	{
		OutError = TEXT("An offline persistence operation is already in progress.");
		return false;
	}
	// Offline manager operations are synchronous. Reject busy states above rather than capturing
	// stack references in a deferred Flush completion.
	bool bSucceeded = false;
	Persistence->Flush([&bSucceeded, &OutError](bool bSaved, const FString& Error)
	{
		bSucceeded = bSaved;
		OutError = Error;
	});
	return bSucceeded;
}

void UMT2OfflineSessionSubsystem::ReturnToCharacterSelection(AMT2PlayerController* Controller)
{
	if (!Controller || !bActive || bOperationPending) return;
	FString Error;
	if (!SavePlayer(Controller, Error)) { Controller->ClientTravelFailed_Implementation(Error); return; }
	bPlayerLoaded = false;
	SelectedCharacter = {};
	GetGameInstance()->GetSubsystem<UMT2ClientSessionSubsystem>()->MarkTravelingToMap();
	Controller->ClientTravel(UMT2PathSettings::Path(TEXT("Maps_System_Gateway")), TRAVEL_Absolute);
}

void UMT2OfflineSessionSubsystem::HandleWorldTeardown(UWorld* World)
{
	if (!bActive || !bPlayerLoaded || !World || World->GetGameInstance() != GetGameInstance()) return;
	auto* Controller = Cast<AMT2PlayerController>(World->GetFirstPlayerController());
	FString Error;
	if (!SavePlayer(Controller, Error)) UE_LOG(LogMT2Offline, Error, TEXT("Final offline save failed: %s"), *Error);
	bPlayerLoaded = false;
}
