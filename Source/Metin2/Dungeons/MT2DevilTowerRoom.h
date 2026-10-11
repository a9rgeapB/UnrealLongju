#pragma once
#include "Dungeons/MT2DungeonRoom.h"
#include "Quests/MT2QuestNode.h"
#include "MT2DevilTowerRoom.generated.h"

// Shared tower stages. Every puzzle, regeneration and transition belongs to the room.
UCLASS()
class METIN2_API AMT2DevilTowerRoom : public AMT2DungeonRoom
{
	GENERATED_BODY()
public:
	AMT2DevilTowerRoom();
	virtual bool JoinPlayer(AMT2PlayerState* Player) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Devil Tower", meta=(ClampMin="1", ClampMax="9")) int32 Floor = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Devil Tower", meta=(ClampMin="0.1")) float TransitionSeconds = 6.f;
	UPROPERTY(EditAnywhere, Category="Devil Tower") TArray<FMT2DungeonSpawn> PuzzleSpawns;
	UPROPERTY(EditAnywhere, Category="Devil Tower") TArray<FMT2DungeonSpawn> RegenSpawns;
	UPROPERTY(EditAnywhere, Category="Devil Tower") TMap<FName, FText> LegacyMessages;
	bool HandleItem(AMT2PlayerState* Player, int32 Slot, AActor* Target);
	bool IsBlacksmith(const AActor* Target) const;
	bool CanRefine(AMT2PlayerState* Player, const AActor* Target, int32 Slot = INDEX_NONE) const;
	bool SelectUpperFloors(AMT2PlayerState* Player, AActor* Target, int32 Epoch, bool bLeave);
	void ExitPlayer(AMT2PlayerState* Player);
	void RecoverMapOnLogout(AMT2PlayerState* Player);
protected:
	virtual bool CompletesOnElimination() const override { return Floor <= 3; }
	virtual void OnEncounterStarted() override;
	virtual void OnEncounterMobSpawned(const FMT2DungeonSpawn& Spawn, AMT2Mob* Mob) override;
	virtual void OnSummonedEnemy(AMT2Mob* Mob, AMT2Mob* Source) override;
	virtual void OnEnemyKilled(AMT2Mob* Mob) override;
	virtual void OnEncounterCompleted() override;
	virtual void OnEncounterFailed() override;
	virtual void OnEncounterReset() override;
private:
	friend class FMT2DevilTowerMessagesTest;
	friend class FMT2DevilTowerPuzzlesTest;
	FText Legacy(const TCHAR* Key) const;
	void Broadcast(const FText& Message) const;
	bool SpawnWave(const TArray<FMT2DungeonSpawn>& Wave);
	AMT2Mob* SpawnSupport(const FMT2DungeonSpawn& Spawn);
	void StartStonePuzzle();
	void CheckFakeStones();
	void ProcessRegen();
	void StopStageTimers();
	void StartWarnings(int32 Minutes);
	bool IsMember(const AMT2PlayerState* Player) const;
	bool DropQuestItem(int32 Vnum, FVector Location);
	FText GetObjectiveMessage() const;
	FText GetCompletionMessage() const;
	void NotifyPlayer(AMT2PlayerState* Player, const FText& Message) const;
	void QueueAdvance();
	FTimerHandle StartTimer, AdvanceTimer;
	FTimerHandle PuzzleTimer, RegenTimer, WarningTimer;
	TArray<TWeakObjectPtr<AMT2Mob>> Supports, FakeStones;
	TArray<TWeakObjectPtr<AActor>> Drops;
	TWeakObjectPtr<AMT2Mob> RealStone;
	struct FRegenGroup
	{
		TArray<FMT2DungeonSpawn> Spawns;
		TArray<TWeakObjectPtr<AMT2Mob>> Mobs;
		double NextSpawn = 0;
	};
	TMap<int32, FRegenGroup> RegenGroups;
	int32 Kills = 0, Stones = 0, Seals = 0, MinutesLeft = 0;
	bool bPuzzleStarted = false, bRealKilled = false, bMapFound = false, bUpperFloorsSelected = false;
};

UCLASS()
class METIN2_API UMT2DevilTowerChoice : public UMT2QuestNode
{
	GENERATED_BODY()
public:
	UPROPERTY() TWeakObjectPtr<AMT2DevilTowerRoom> Room;
	UPROPERTY() int32 Epoch = 0;
	UPROPERTY() bool bLeave = false;
	virtual EMT2QuestNodeResult Execute(const FMT2QuestContext& Context) override;
};
