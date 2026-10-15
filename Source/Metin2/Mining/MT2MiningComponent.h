#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "MT2MiningComponent.generated.h"

class AMT2Npc;
class UAnimMontage;
class UMT2ItemPickTemplate;

USTRUCT(BlueprintType)
struct FMT2MiningState
{
	GENERATED_BODY()
	UPROPERTY() uint32 Session = 0;
	UPROPERTY() TObjectPtr<AMT2Npc> Vein;
	UPROPERTY() double StartTime = 0;
	UPROPERTY() uint8 Swings = 0;
};

UCLASS(ClassGroup="MT2", meta=(BlueprintSpawnableComponent))
class METIN2_API UMT2MiningComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UMT2MiningComponent();
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
	UFUNCTION(BlueprintPure, Category="Mining") bool IsMining() const { return State.Swings > 0; }
	bool HasPickaxeEquipped() const;
	// Authority-only interaction shared by native clicks and pc.mining().
	bool StartMining(AMT2Npc* Vein);
	void CancelMining();
	void RequestCancelMining();
	UFUNCTION(Server, Reliable) void ServerCancelMining(uint32 ExpectedSession);
private:
	friend class FMT2MiningAuthorityTest;
	friend class FMT2MiningPresentationTest;
	const UMT2ItemPickTemplate* EquippedPick() const;
	bool CanMine(AMT2Npc* Vein) const;
	UFUNCTION() void CheckAttempt();
	void Complete(uint32 ExpectedSession);
	void Say(const FString& Message) const;
	double ServerTime() const;
	UFUNCTION() void OnEquipmentChanged();
	UFUNCTION() void OnHealthChanged(float OldValue, float NewValue);
	UFUNCTION() void OnRep_State();
	void UpdatePresentation();
	void StopPresentation();
	UPROPERTY(ReplicatedUsing=OnRep_State) FMT2MiningState State;
	UPROPERTY(Transient) TObjectPtr<UAnimMontage> DigMontage;
	FVector Origin = FVector::ZeroVector;
	uint32 SessionCounter = 0;
	uint32 PresentedSession = 0;
	int32 PresentedSwing = INDEX_NONE;
	FRandomStream Random;
	FTimerHandle CompletionTimer, GuardTimer, PresentationTimer;
	bool bResolving = false;
	bool bFacingApplied = false;
};
