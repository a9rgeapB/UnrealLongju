#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MT2DungeonRoom.generated.h"
class UBoxComponent;
class UArrowComponent;
class UPrimitiveComponent;
class AMT2PlayerState;
class AMT2Mob;

UENUM(BlueprintType)
enum class EMT2DungeonRoomPhase : uint8 { Idle, Active, Completed, Failed };

USTRUCT(BlueprintType)
struct METIN2_API FMT2DungeonSpawn
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Vnum = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FTransform LocalTransform;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bForceAggressive = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 GroupId = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RespawnSeconds = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 VariantId = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 VariantWeight = 1;
};

UCLASS(Blueprintable)
class METIN2_API AMT2DungeonRoom : public AActor
{
	GENERATED_BODY()
public:
	AMT2DungeonRoom();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FName RoomId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon") TObjectPtr<UBoxComponent> Bounds;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon") TObjectPtr<UArrowComponent> Entrance;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Dungeon") TObjectPtr<AMT2DungeonRoom> NextRoom;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, ReplicatedUsing=ApplyDoors, Category="Dungeon") TArray<TObjectPtr<AActor>> Doors;
	// Zero disables timeout for legacy stages that have no deadline.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon", meta=(ClampMin="0.0")) float EncounterSeconds = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon", meta=(ClampMin="0.1")) float EmptyResetSeconds = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TArray<FMT2DungeonSpawn> Spawns;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Dungeon") virtual bool JoinPlayer(AMT2PlayerState* Player);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Dungeon") void LeavePlayer(AMT2PlayerState* Player);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Dungeon") bool StartEncounter();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Dungeon") bool RegisterEnemy(AMT2Mob* Mob);
	bool RegisterSummonedEnemy(AMT2Mob* Mob, AMT2Mob* Source);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Dungeon") void CompleteEncounter();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Dungeon") void FailEncounter();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Dungeon") bool ResetWhenEmpty();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Dungeon") bool AdvancePlayer(AMT2PlayerState* Player);
	// Consumes an entitlement only. Rewards remain individual authoritative quest/inventory work.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Dungeon") bool TryClaimCompletion(AMT2PlayerState* Player);
	UFUNCTION(BlueprintPure, Category="Dungeon") EMT2DungeonRoomPhase GetPhase() const { return Phase; }
	UFUNCTION(BlueprintPure, Category="Dungeon") int32 GetRemainingEnemies() const { return RemainingEnemies; }
	UFUNCTION(BlueprintPure, Category="Dungeon") int32 GetParticipantCount() const { return Participants; }
	UFUNCTION(BlueprintPure, Category="Dungeon") int32 GetRunSerial() const { return RunSerial; }
	UFUNCTION(BlueprintPure, Category="Dungeon") double GetSecondsRemaining() const;
	UFUNCTION(BlueprintPure, Category="Dungeon") bool ContainsLocation(FVector Location) const;
	UFUNCTION(BlueprintImplementableEvent, Category="Dungeon") void EncounterStarted();
	UFUNCTION(BlueprintImplementableEvent, Category="Dungeon") void EncounterCompleted();
	UFUNCTION(BlueprintImplementableEvent, Category="Dungeon") void EncounterFailed();
protected:
	virtual bool CompletesOnElimination() const { return true; }
	virtual void OnEncounterStarted() {}
	virtual void OnEncounterMobSpawned(const FMT2DungeonSpawn& Spawn, AMT2Mob* Mob) {}
	virtual void OnSummonedEnemy(AMT2Mob* Mob, AMT2Mob* Source) {}
	virtual void OnEnemyKilled(AMT2Mob* Mob) {}
	virtual void OnEncounterCompleted() {}
	virtual void OnEncounterFailed() {}
	virtual void OnEncounterReset() {}
	AMT2Mob* SpawnEncounterMob(const FMT2DungeonSpawn& Spawn);
	void PurgeEnemies() { ClearEnemies(true); }
	void PurgeEnemy(AMT2Mob* Mob);
	void SetEncounterDeadline(float Seconds);
	bool IsCompletionEligible(const AMT2PlayerState* Player) const;
	static TMap<int32, int32> ChooseSpawnVariants(const TArray<FMT2DungeonSpawn>& Wave);
	TArray<TWeakObjectPtr<AMT2PlayerState>> GetMembers() const { return Members.Array(); }
private:
	UFUNCTION() void OnOverlap(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bFromSweep, const FHitResult& Hit);
	UFUNCTION() void OnEndOverlap(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, int32 BodyIndex);
	UFUNCTION() void OnMemberEndPlay(AActor* Actor, EEndPlayReason::Type Reason);
	UFUNCTION() void OnPawnEndPlay(AActor* Actor, EEndPlayReason::Type Reason);
	UFUNCTION() void OnEnemyDeath();
	UFUNCTION() void OnEnemyEndPlay(AActor* Actor, EEndPlayReason::Type Reason);
	UFUNCTION() void ApplyDoors();
	void SetPhase(EMT2DungeonRoomPhase NewPhase);
	void ClearEnemies(bool bDestroy);
	void ScheduleEmptyReset();
	FString PlayerIdentity(const AMT2PlayerState* Player) const;
	UPROPERTY(ReplicatedUsing=ApplyDoors) EMT2DungeonRoomPhase Phase = EMT2DungeonRoomPhase::Idle;
	UPROPERTY(Replicated) int32 RunSerial = 0;
	UPROPERTY(Replicated) int32 RemainingEnemies = 0;
	UPROPERTY(Replicated) int32 Participants = 0;
	UPROPERTY(Replicated) double Deadline = 0.0;
	TSet<TWeakObjectPtr<AMT2PlayerState>> Members;
	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<AMT2PlayerState>> MemberPawns;
	TSet<TWeakObjectPtr<AMT2Mob>> Enemies, AliveEnemies;
	TSet<FString> EligiblePlayers, ClaimedPlayers;
	FTimerHandle EncounterTimer, ResetTimer;
	bool bRegistered = false;
	bool bSpawning = false;
	bool bResetting = false;
};
