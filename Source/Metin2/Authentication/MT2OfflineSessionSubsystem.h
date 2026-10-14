#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Server/MT2ServerRuntimeTypes.h"
#include "MT2OfflineSessionSubsystem.generated.h"

class AMT2PlayerController;

UCLASS()
class METIN2_API UMT2OfflineSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	static bool ShouldUseOfflineFlow(ENetMode NetMode, bool bClientOnly, bool bServerOnly, const FString& CommandLine);
	bool IsActive() const { return bActive; }
	bool IsReady() const { return bActive && !AccountId.IsEmpty(); }
	bool HasSelectedCharacter() const { return !SelectedCharacter.CharacterId.IsEmpty(); }
	const FMT2CharacterSummary& GetSelectedCharacter() const { return SelectedCharacter; }
	const FString& GetAccountId() const { return AccountId; }
	void BeginCharacterSelection(AMT2PlayerController* Controller);
	void CreateCharacter(AMT2PlayerController* Controller, const FString& Name, const FMT2CharacterAppearance& Appearance, EMT2Empire Empire);
	void SelectCharacter(AMT2PlayerController* Controller, const FString& CharacterId);
	bool PrepareCharacterSelection(const FString& CharacterId, FString& OutPackage, FString& OutError);
	void AbortWorldEntry(AMT2PlayerController* Controller, const FString& Error);
	void ReturnToCharacterSelection(AMT2PlayerController* Controller);
	bool SavePlayer(AMT2PlayerController* Controller, FString& OutError);
	void MarkPlayerLoaded(bool bSucceeded) { bPlayerLoaded = bSucceeded; bOperationPending = false; }
	bool IsPlayerLoaded() const { return bPlayerLoaded; }
	static bool ResolveMapPackage(const FString& MapId, FString& OutPackage, FString& OutError);
private:
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error);
	FDelegateHandle TravelFailureHandle;
	FDelegateHandle WorldTeardownHandle;
	void HandleWorldTeardown(UWorld* World);
	FString PendingEntryError;
	bool bActive = false;
	bool bOperationPending = false;
	bool bPlayerLoaded = false;
	FString AccountId;
	UPROPERTY(Transient) FMT2CharacterSummary SelectedCharacter;
};
