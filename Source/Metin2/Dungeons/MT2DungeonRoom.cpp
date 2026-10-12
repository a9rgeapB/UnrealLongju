#include "Dungeons/MT2DungeonRoom.h"
#include "Dungeons/MT2DungeonSubsystem.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/MT2HealthComponent.h"
#include "Core/MT2VnumRegistrySubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameStateBase.h"
#include "Mobs/MT2Mob.h"
#include "Mobs/MT2MetinStone.h"
#include "Mobs/MT2MobAIComponent.h"
#include "Net/UnrealNetwork.h"
#include "Persistence/MT2PersistenceComponent.h"
#include "Player/MT2PlayerState.h"
#include "TimerManager.h"
#include "Quests/MT2QuestManagerComponent.h"
#include "Quests/MT2QuestTableAsset.h"
#include "Quests/MT2QuestNode.h"
#include "Mobs/MT2MobSpawnComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogMT2Dungeon, Log, All);

AMT2DungeonRoom::AMT2DungeonRoom()
{
	PrimaryActorTick.bCanEverTick = false; bReplicates = true;
	SetNetUpdateFrequency(2.f); SetMinNetUpdateFrequency(1.f);
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("RoomBounds")); SetRootComponent(Bounds);
	Bounds->SetBoxExtent(FVector(1000,1000,500)); Bounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Bounds->SetCollisionResponseToAllChannels(ECR_Ignore); Bounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Bounds->SetGenerateOverlapEvents(true);
	Entrance = CreateDefaultSubobject<UArrowComponent>(TEXT("Entrance")); Entrance->SetupAttachment(Bounds);
	Entrance->SetRelativeLocation(FVector(0,0,100));
	ScriptExecutor = CreateDefaultSubobject<UMT2QuestManagerComponent>(TEXT("DungeonQuestExecutor"));
	ScriptExecutor->SetIsReplicated(false);
}
void AMT2DungeonRoom::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) { ApplyDoors(); return; }
	auto* Registry = GetWorld()->GetSubsystem<UMT2DungeonSubsystem>();
	bRegistered = Registry && Registry->RegisterRoom(this);
	if (!bRegistered) { UE_LOG(LogMT2Dungeon, Error, TEXT("Dungeon room %s requires a unique nonempty RoomId."), *GetPathName()); return; }
	ApplyDoors();
	SetNetCullDistanceSquared(FMath::Max(GetNetCullDistanceSquared(), FMath::Square(Bounds->GetScaledBoxExtent().Size() + 5000.0)));
	Bounds->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnOverlap);
	Bounds->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnEndOverlap);
	TArray<AActor*> Overlapping; Bounds->GetOverlappingActors(Overlapping, AMT2PlayerCharacter::StaticClass());
	for (AActor* Actor : Overlapping) if (auto* Player = Cast<AMT2PlayerCharacter>(Actor)) JoinPlayer(Player->GetPlayerState<AMT2PlayerState>());
}
void AMT2DungeonRoom::EndPlay(const EEndPlayReason::Type Reason)
{
	ClearScriptRegen();
	bRegistered = false;
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	Bounds->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::OnOverlap);
	Bounds->OnComponentEndOverlap.RemoveDynamic(this, &ThisClass::OnEndOverlap);
	for (auto Member : Members) if (Member.IsValid()) Member->OnEndPlay.RemoveDynamic(this, &ThisClass::OnMemberEndPlay);
	for (const auto& Pair : MemberPawns) if (Pair.Key.IsValid()) Pair.Key->OnEndPlay.RemoveDynamic(this, &ThisClass::OnPawnEndPlay);
	const bool bLocalRemoval = Reason == EEndPlayReason::Destroyed || Reason == EEndPlayReason::RemovedFromWorld;
	ClearEnemies(HasAuthority() && bLocalRemoval && !GetWorld()->bIsTearingDown);
	if (auto* Registry = GetWorld()->GetSubsystem<UMT2DungeonSubsystem>()) Registry->UnregisterRoom(this);
	Members.Reset(); MemberPawns.Reset(); bRegistered = false; Super::EndPlay(Reason);
}
void AMT2DungeonRoom::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMT2DungeonRoom, Doors);
	DOREPLIFETIME(AMT2DungeonRoom, Phase); DOREPLIFETIME(AMT2DungeonRoom, RunSerial);
	DOREPLIFETIME(AMT2DungeonRoom, RemainingEnemies); DOREPLIFETIME(AMT2DungeonRoom, Participants); DOREPLIFETIME(AMT2DungeonRoom, Deadline);
}
bool AMT2DungeonRoom::ContainsLocation(FVector Location) const
{
	const FVector Local = Bounds->GetComponentTransform().InverseTransformPosition(Location), Extent = Bounds->GetUnscaledBoxExtent();
	return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}
FString AMT2DungeonRoom::PlayerIdentity(const AMT2PlayerState* Player) const
{
	const auto* Persistence = IsValid(Player) ? Player->GetPersistenceComponent() : nullptr;
	return Persistence ? Persistence->GetEntityId() : FString();
}
bool AMT2DungeonRoom::JoinPlayer(AMT2PlayerState* Player)
{
	if (!HasAuthority() || !bRegistered || Phase == EMT2DungeonRoomPhase::Failed || !IsValid(Player) || Player->IsActorBeingDestroyed()) return false;
	APawn* Pawn = Player->GetPawn();
	if (!IsValid(Pawn) || Pawn->GetWorld() != GetWorld() ||
		(!ContainsLocation(Pawn->GetActorLocation()) && !Bounds->IsOverlappingActor(Pawn))) return false;
	auto* Registry = GetWorld()->GetSubsystem<UMT2DungeonSubsystem>();
	if (!Registry || !Registry->AssignPlayer(Player, this)) return false;
	if (!Members.Contains(Player)) { Members.Add(Player); Player->OnEndPlay.AddDynamic(this, &ThisClass::OnMemberEndPlay); }
	for (auto It = MemberPawns.CreateIterator(); It; ++It) if (It.Value().Get() == Player && It.Key().Get() != Pawn)
	{
		if (It.Key().IsValid()) It.Key()->OnEndPlay.RemoveDynamic(this, &ThisClass::OnPawnEndPlay);
		It.RemoveCurrent();
	}
	MemberPawns.Add(Pawn, Player); Pawn->OnEndPlay.AddUniqueDynamic(this, &ThisClass::OnPawnEndPlay);
	if (Phase == EMT2DungeonRoomPhase::Active)
	{
		const FString Id = PlayerIdentity(Player);
		if (!Id.IsEmpty()) EligiblePlayers.Add(Id);
	}
	Participants = Members.Num(); GetWorld()->GetTimerManager().ClearTimer(ResetTimer); ForceNetUpdate(); return true;
}
void AMT2DungeonRoom::LeavePlayer(AMT2PlayerState* Player)
{
	if (!HasAuthority() || !Player || !Members.Remove(Player)) return;
	Player->OnEndPlay.RemoveDynamic(this, &ThisClass::OnMemberEndPlay);
	for (auto It = MemberPawns.CreateIterator(); It; ++It) if (It.Value().Get() == Player)
	{
		if (It.Key().IsValid()) It.Key()->OnEndPlay.RemoveDynamic(this, &ThisClass::OnPawnEndPlay);
		It.RemoveCurrent();
	}
	if (auto* Registry = GetWorld()->GetSubsystem<UMT2DungeonSubsystem>()) Registry->RemovePlayer(Player, this);
	Participants = Members.Num(); ForceNetUpdate(); ScheduleEmptyReset();
}
void AMT2DungeonRoom::OnOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (auto* Player = Cast<AMT2PlayerCharacter>(Other))
		if (auto* Registry = GetWorld()->GetSubsystem<UMT2DungeonSubsystem>()) Registry->ReconcilePlayer(Player->GetPlayerState<AMT2PlayerState>());
}
void AMT2DungeonRoom::OnEndOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32)
{
	if (auto* Player = Cast<AMT2PlayerCharacter>(Other); Player && !Bounds->IsOverlappingActor(Player))
	{
		auto* State = Player->GetPlayerState<AMT2PlayerState>();
		if (State && State->GetPawn() == Player)
			if (auto* Registry = GetWorld()->GetSubsystem<UMT2DungeonSubsystem>()) Registry->ReconcilePlayer(State);
		ScheduleEmptyReset();
	}
}
void AMT2DungeonRoom::OnMemberEndPlay(AActor* Actor, EEndPlayReason::Type) { LeavePlayer(Cast<AMT2PlayerState>(Actor)); }
void AMT2DungeonRoom::OnPawnEndPlay(AActor* Actor, EEndPlayReason::Type)
{
	const auto* Member = MemberPawns.Find(Actor);
	if (Member) LeavePlayer(Member->Get());
}
bool AMT2DungeonRoom::StartEncounter()
{
	if (!HasAuthority() || !bRegistered || bResetting || Phase != EMT2DungeonRoomPhase::Idle || Members.IsEmpty() ||
		!FMath::IsFinite(EncounterSeconds) || EncounterSeconds < 0.f) return false;
	RunSerial = RunSerial == MAX_int32 ? 1 : RunSerial + 1;
	const float RunSeconds = bScriptControlled ? 0.f : EncounterSeconds;
	EligiblePlayers.Reset(); ClaimedPlayers.Reset();
	for (auto Member : Members) { const FString Id = PlayerIdentity(Member.Get()); if (!Id.IsEmpty()) EligiblePlayers.Add(Id); }
	SetPhase(EMT2DungeonRoomPhase::Active); Deadline = RunSeconds > 0.f ? GetWorld()->GetTimeSeconds() + RunSeconds : 0.0;
	const int32 Epoch = RunSerial;
	if (RunSeconds > 0.f) GetWorld()->GetTimerManager().SetTimer(EncounterTimer, FTimerDelegate::CreateWeakLambda(this, [this, Epoch]()
		{ if (RunSerial == Epoch) FailEncounter(); }), RunSeconds, false);
	bSpawning = true;
	auto* GI = GetGameInstance(); auto* Vnums = GI ? GI->GetSubsystem<UMT2VnumRegistrySubsystem>() : nullptr;
	TMap<int32, TArray<AMT2Mob*>> Groups;
	const auto Variants = ChooseSpawnVariants(Spawns);
	for (const auto& Spawn : Spawns)
	{
		if (Spawn.VariantId > 0 && Variants.FindRef(Spawn.GroupId) != Spawn.VariantId) continue;
		const auto MobClass = Vnums ? Vnums->ResolveMobClass(Spawn.Vnum) : TSubclassOf<AMT2Mob>();
		const FTransform Transform = Spawn.LocalTransform * GetActorTransform();
		FActorSpawnParameters Parameters; Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
		AMT2Mob* Mob = MobClass && ContainsLocation(Transform.GetLocation()) ? GetWorld()->SpawnActor<AMT2Mob>(MobClass, Transform, Parameters) : nullptr;
		if (!Mob || !RegisterEnemy(Mob))
		{
			if (Mob) Mob->Destroy();
			bSpawning = false; UE_LOG(LogMT2Dungeon, Error, TEXT("Room %s failed encounter spawn vnum=%d."), *RoomId.ToString(), Spawn.Vnum);
			ClearEnemies(true); FailEncounter(); return false;
		}
		if (Spawn.bForceAggressive) Mob->GetMobAIComponent()->SetForceAggressive(true);
		if (Spawn.GroupId > 0) Groups.FindOrAdd(Spawn.GroupId).Add(Mob);
		OnEncounterMobSpawned(Spawn, Mob);
	}
	for (const auto& Pair : Groups) for (AMT2Mob* Mob : Pair.Value) Mob->SetSpawnGroupMembers(Pair.Value);
	bSpawning = false; OnEncounterStarted(); EncounterStarted(); return Phase == EMT2DungeonRoomPhase::Active;
}
TMap<int32, int32> AMT2DungeonRoom::ChooseSpawnVariants(const TArray<FMT2DungeonSpawn>& Wave)
{
	TMap<int32, TMap<int32, int32>> Weights;
	for (const auto& Spawn : Wave) if (Spawn.VariantId > 0)
		Weights.FindOrAdd(Spawn.GroupId).FindOrAdd(Spawn.VariantId) = FMath::Max(1, Spawn.VariantWeight);
	TMap<int32, int32> Result;
	for (const auto& Group : Weights)
	{
		int32 Total = 0; for (const auto& Variant : Group.Value) Total += Variant.Value;
		int32 Roll = FMath::RandRange(1, Total);
		for (const auto& Variant : Group.Value) if ((Roll -= Variant.Value) <= 0) { Result.Add(Group.Key, Variant.Key); break; }
	}
	return Result;
}
AMT2Mob* AMT2DungeonRoom::SpawnEncounterMob(const FMT2DungeonSpawn& Spawn)
{
	if (!HasAuthority() || GetPhase() != EMT2DungeonRoomPhase::Active) return nullptr;
	auto* GI = GetGameInstance(); auto* Vnums = GI ? GI->GetSubsystem<UMT2VnumRegistrySubsystem>() : nullptr;
	const auto Class = Vnums ? Vnums->ResolveMobClass(Spawn.Vnum) : TSubclassOf<AMT2Mob>();
	const FTransform Transform = Spawn.LocalTransform * GetActorTransform();
	FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
	auto* Mob = Class && ContainsLocation(Transform.GetLocation()) ? GetWorld()->SpawnActor<AMT2Mob>(Class, Transform, Params) : nullptr;
	if (Mob && !RegisterEnemy(Mob)) { Mob->Destroy(); return nullptr; }
	if (Mob && Spawn.bForceAggressive) Mob->GetMobAIComponent()->SetForceAggressive(true);
	if (Mob) OnEncounterMobSpawned(Spawn, Mob);
	return Mob;
}
void AMT2DungeonRoom::PurgeEnemy(AMT2Mob* Mob)
{
	if (!HasAuthority() || !IsValid(Mob)) return;
	// A stone's EndPlay purges its summons. Detach room callbacks first so this intentional
	// purge cannot be mistaken for the unexpected disappearance of a living objective.
	for (auto Enemy : Enemies.Array()) if (Enemy.IsValid() && Enemy.Get() != Mob && Enemy->GetSpawningStone() == Mob)
		PurgeEnemy(Enemy.Get());
	Enemies.Remove(Mob); AliveEnemies.Remove(Mob); RemainingEnemies = AliveEnemies.Num();
	if (auto* Health = Mob->GetHealthComponent()) Health->OnDeath.RemoveDynamic(this, &ThisClass::OnEnemyDeath);
	Mob->OnEndPlay.RemoveDynamic(this, &ThisClass::OnEnemyEndPlay);
	if (auto* Registry = GetWorld()->GetSubsystem<UMT2DungeonSubsystem>()) Registry->RemoveEnemy(Mob, this);
	Mob->Destroy(); ForceNetUpdate();
}
void AMT2DungeonRoom::SetEncounterDeadline(float Seconds)
{
	if (!HasAuthority() || Phase != EMT2DungeonRoomPhase::Active || !FMath::IsFinite(Seconds) || Seconds <= 0.f) return;
	Deadline = GetWorld()->GetTimeSeconds() + Seconds;
	const int32 Epoch = RunSerial;
	GetWorld()->GetTimerManager().SetTimer(EncounterTimer, FTimerDelegate::CreateWeakLambda(this, [this, Epoch]()
		{ if (RunSerial == Epoch) FailEncounter(); }), Seconds, false);
	ForceNetUpdate();
}
bool AMT2DungeonRoom::RegisterEnemy(AMT2Mob* Mob)
{
	if (!HasAuthority() || !bRegistered || bResetting || Phase != EMT2DungeonRoomPhase::Active || !IsValid(Mob) || Mob->IsActorBeingDestroyed() ||
		!ContainsLocation(Mob->GetActorLocation()) || !Mob->GetHealthComponent() || Mob->GetHealthComponent()->IsDead()) return false;
	auto* Registry = GetWorld()->GetSubsystem<UMT2DungeonSubsystem>();
	if (!Registry || !Registry->AssignEnemy(Mob, this)) return false;
	if (!Enemies.Contains(Mob))
	{
		Enemies.Add(Mob); AliveEnemies.Add(Mob);
		Mob->GetHealthComponent()->OnDeath.AddDynamic(this, &ThisClass::OnEnemyDeath);
		Mob->OnEndPlay.AddDynamic(this, &ThisClass::OnEnemyEndPlay);
	}
	RemainingEnemies = AliveEnemies.Num(); ForceNetUpdate(); return true;
}
bool AMT2DungeonRoom::RegisterSummonedEnemy(AMT2Mob* Mob, AMT2Mob* Source)
{
	if (!Enemies.Contains(Source) || !RegisterEnemy(Mob)) return false;
	OnSummonedEnemy(Mob, Source); return true;
}
void AMT2DungeonRoom::OnEnemyDeath()
{
	if (Phase != EMT2DungeonRoomPhase::Active) return;
	TArray<TWeakObjectPtr<AMT2Mob>> Killed;
	for (auto It = AliveEnemies.CreateIterator(); It; ++It) if (It->IsValid() && (*It)->GetHealthComponent()->IsDead())
	{ Killed.Add(*It); It.RemoveCurrent(); }
	RemainingEnemies = AliveEnemies.Num(); ForceNetUpdate();
	for (auto Mob : Killed) if (Mob.IsValid() && Phase == EMT2DungeonRoomPhase::Active) OnEnemyKilled(Mob.Get());
	if (bScriptControlled && !bSpawning) CheckScriptElimination();
	if (CompletesOnElimination() && !bSpawning && RemainingEnemies == 0 && !Enemies.IsEmpty()) CompleteEncounter();
}
void AMT2DungeonRoom::OnEnemyEndPlay(AActor* Actor, EEndPlayReason::Type Reason)
{
	AMT2Mob* Mob = Cast<AMT2Mob>(Actor);
	const bool bTracked = Enemies.Contains(Mob);
	const auto* Health = Mob ? Mob->GetHealthComponent() : nullptr;
	const bool bAlive = AliveEnemies.Contains(Mob) && (!Health || !Health->IsInitialized() || !Health->IsDead());
	AliveEnemies.Remove(Mob); Enemies.Remove(Mob); RemainingEnemies = AliveEnemies.Num();
	if (Mob)
	{
		if (auto* Component = Mob->GetHealthComponent()) Component->OnDeath.RemoveDynamic(this, &ThisClass::OnEnemyDeath);
		Mob->OnEndPlay.RemoveDynamic(this, &ThisClass::OnEnemyEndPlay);
	}
	if (auto* Registry = GetWorld()->GetSubsystem<UMT2DungeonSubsystem>()) Registry->RemoveEnemy(Mob, this);
	if (GetWorld()->bIsTearingDown || (Reason != EEndPlayReason::Destroyed && Reason != EEndPlayReason::RemovedFromWorld)) return;
	// Despawning a live objective must not count as a kill or open progression doors.
	if (bAlive && Phase == EMT2DungeonRoomPhase::Active) FailEncounter();
	else if (CompletesOnElimination() && bTracked && Phase == EMT2DungeonRoomPhase::Active && !bSpawning && AliveEnemies.IsEmpty()) CompleteEncounter();
}
void AMT2DungeonRoom::SetPhase(EMT2DungeonRoomPhase NewPhase) { Phase = NewPhase; ApplyDoors(); ForceNetUpdate(); }
void AMT2DungeonRoom::ApplyDoors()
{
	const bool bOpen = Phase != EMT2DungeonRoomPhase::Active;
	for (AActor* Door : Doors) if (IsValid(Door) && Door != this && Door->GetWorld() == GetWorld())
	{ Door->SetActorHiddenInGame(bOpen); Door->SetActorEnableCollision(!bOpen); }
}
void AMT2DungeonRoom::CompleteEncounter()
{
	if (!HasAuthority() || !bRegistered || Phase != EMT2DungeonRoomPhase::Active) return;
	GetWorld()->GetTimerManager().ClearTimer(EncounterTimer); Deadline = 0.0;
	SetPhase(EMT2DungeonRoomPhase::Completed); OnEncounterCompleted(); EncounterCompleted(); ScheduleEmptyReset();
}
void AMT2DungeonRoom::FailEncounter()
{
	if (!HasAuthority() || !bRegistered || Phase != EMT2DungeonRoomPhase::Active) return;
	GetWorld()->GetTimerManager().ClearTimer(EncounterTimer); Deadline = 0.0;
	SetPhase(EMT2DungeonRoomPhase::Failed); OnEncounterFailed(); EncounterFailed(); ScheduleEmptyReset();
}
void AMT2DungeonRoom::ClearEnemies(bool bDestroy)
{
	const auto Snapshot = Enemies.Array(); Enemies.Reset(); AliveEnemies.Reset(); RemainingEnemies = 0;
	for (auto Enemy : Snapshot) if (AMT2Mob* Mob = Enemy.Get())
	{
		if (auto* Health = Mob->GetHealthComponent()) Health->OnDeath.RemoveDynamic(this, &ThisClass::OnEnemyDeath);
		Mob->OnEndPlay.RemoveDynamic(this, &ThisClass::OnEnemyEndPlay);
		if (auto* Registry = GetWorld()->GetSubsystem<UMT2DungeonSubsystem>()) Registry->RemoveEnemy(Mob, this);
		if (bDestroy) Mob->Destroy();
	}
}
void AMT2DungeonRoom::ScheduleEmptyReset()
{
	if (!HasAuthority() || !bRegistered || !Members.IsEmpty() || Phase == EMT2DungeonRoomPhase::Idle) return;
	if (!FMath::IsFinite(EmptyResetSeconds) || EmptyResetSeconds <= 0.f)
	{ UE_LOG(LogMT2Dungeon, Error, TEXT("Room %s has invalid empty-reset delay."), *RoomId.ToString()); return; }
	GetWorld()->GetTimerManager().SetTimer(ResetTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { ResetWhenEmpty(); }), EmptyResetSeconds, false);
}
bool AMT2DungeonRoom::ResetWhenEmpty()
{
	if (!HasAuthority() || !bRegistered || bResetting || !Members.IsEmpty()) return false;
	TArray<AActor*> Occupants; Bounds->GetOverlappingActors(Occupants, AMT2PlayerCharacter::StaticClass());
	for (AActor* Actor : Occupants) if (IsValid(Actor) && !Actor->IsActorBeingDestroyed()) return false;
	bResetting = true;
	GetWorld()->GetTimerManager().ClearTimer(EncounterTimer); GetWorld()->GetTimerManager().ClearTimer(ResetTimer);
	EligiblePlayers.Reset(); ClaimedPlayers.Reset(); Deadline = 0.0;
	OnEncounterReset(); ClearEnemies(true); ScriptFlags.Reset(); ScriptUniques.Reset(); ScriptExecutor->ResetDungeonExecution();
	ClearScriptRegen(); ScriptEliminationDelay = -1.f; GetWorld()->GetTimerManager().ClearTimer(ScriptEliminationTimer); bScriptControlled = false;
	SetPhase(EMT2DungeonRoomPhase::Idle); bResetting = false; ForceNetUpdate(); return true;
}

void AMT2DungeonRoom::SetScriptFlag(const FString& Name, int32 Value)
{
	if (HasAuthority() && !Name.IsEmpty()) { bScriptControlled = true; ScriptFlags.Add(Name, Value); }
}
bool AMT2DungeonRoom::SetScriptUnique(const FString& Name, AMT2Mob* Mob)
{
	if (!HasAuthority() || Name.IsEmpty() || !IsValid(Mob) || !Enemies.Contains(Mob)) return false;
	ScriptUniques.Add(Name, Mob); return true;
}
AMT2Mob* AMT2DungeonRoom::GetScriptUnique(const FString& Name) const
{
	const auto* Value = ScriptUniques.Find(Name);
	return Value && Value->IsValid() && !(*Value)->IsActorBeingDestroyed() ? Value->Get() : nullptr;
}
void AMT2DungeonRoom::RemoveScriptUnique(const FString& Name, bool bKill)
{
	if (!HasAuthority()) return;
	auto* Mob = GetScriptUnique(Name); ScriptUniques.Remove(Name);
	if (!Mob) return;
	if (bKill) Mob->GetHealthComponent()->SetHealth(0.f);
	else PurgeEnemy(Mob);
}
void AMT2DungeonRoom::PurgeScriptEnemies() { if (HasAuthority()) ClearEnemies(true); }
void AMT2DungeonRoom::KillScriptEnemies()
{
	if (!HasAuthority()) return;
	for (auto Enemy : AliveEnemies.Array()) if (Enemy.IsValid()) Enemy->GetHealthComponent()->SetHealth(0.f);
}

bool AMT2DungeonRoom::SpawnScriptRegen(const FString& Path, bool bRespawning)
{
	if (!HasAuthority() || !bRegistered) return false;
	FString Key = Path.TrimStartAndEnd(); FPaths::NormalizeFilename(Key);
	const auto* Tables = LoadObject<UMT2QuestTableAsset>(nullptr, UMT2QuestTableAsset::GetAssetPath());
	const auto* Map = Tables ? Tables->FindMapByIndex(LegacyMapIndex) : nullptr;
	const auto* Regen = Tables ? Tables->DungeonRegens.FindByPredicate([&](const auto& Row) { return Row.Path == Key; }) : nullptr;
	if (!Map || !Regen) { UE_LOG(LogMT2Dungeon, Error, TEXT("Room %s has no cooked regen '%s' for map %d."), *RoomId.ToString(), *Key, LegacyMapIndex); return false; }
	// Joining an already-running shared stage must not duplicate its initial wave.
	if (ScriptRegenPaths.Contains(Key)) return true;
	bScriptControlled = true;
	if (Phase == EMT2DungeonRoomPhase::Idle && !StartEncounter()) return false;
	if (Phase != EMT2DungeonRoomPhase::Active) return false;
	TArray<FMT2MobSpawnEntry> Entries = Regen->Entries;
	for (auto& Entry : Entries)
	{
		Entry.Center.X += Map->WorldSize.X; Entry.Center.Z = Entrance->GetComponentLocation().Z;
		if (!ContainsLocation(Entry.Center)) { UE_LOG(LogMT2Dungeon, Error, TEXT("Regen '%s' extends outside room %s."), *Key, *RoomId.ToString()); return false; }
		if (!bRespawning) Entry.RespawnDelay = 0.f;
	}
	auto* Spawner = NewObject<UMT2MobSpawnComponent>(this);
	Spawner->SpawnRegistration.BindUObject(this, &AMT2DungeonRoom::RegisterEnemy);
	Spawner->SetSpawnEntries(Entries); ScriptRegens.Add(Spawner); ScriptRegenPaths.Add(Key);
	AddInstanceComponent(Spawner); Spawner->RegisterComponent(); Spawner->ProcessSpawns(); return true;
}
void AMT2DungeonRoom::ClearScriptRegen()
{
	for (const auto& Spawner : ScriptRegens) if (IsValid(Spawner)) Spawner->DestroyComponent();
	ScriptRegens.Reset(); ScriptRegenPaths.Reset();
}
AMT2Mob* AMT2DungeonRoom::SpawnScriptMob(int32 Vnum, const FVector2D& XY, float Yaw)
{
	if (!HasAuthority() || !bRegistered || Vnum <= 0) return nullptr;
	bScriptControlled = true;
	if (Phase == EMT2DungeonRoomPhase::Idle && !StartEncounter()) return nullptr;
	const FVector Center = Bounds->GetComponentLocation(), Extent = Bounds->GetScaledBoxExtent();
	FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(MT2DungeonScriptSpawn), false, this);
	if (!GetWorld()->LineTraceSingleByChannel(Hit, FVector(XY, Center.Z + Extent.Z), FVector(XY, Center.Z - Extent.Z), ECC_WorldStatic, Query)) return nullptr;
	FMT2DungeonSpawn Spawn; Spawn.Vnum = Vnum;
	Spawn.LocalTransform.SetLocation(GetActorTransform().InverseTransformPosition(Hit.ImpactPoint + FVector(0,0,100)));
	Spawn.LocalTransform.SetRotation(GetActorQuat().Inverse() * FRotator(0,Yaw,0).Quaternion());
	return SpawnEncounterMob(Spawn);
}
bool AMT2DungeonRoom::JumpScriptMembers(AMT2DungeonRoom* Destination, const FVector2D& XY)
{
	if (!HasAuthority() || !IsValid(Destination) || Destination->GetWorld() != GetWorld() || !Destination->bRegistered ||
		Destination->Phase == EMT2DungeonRoomPhase::Failed) return false;
	const FVector Center = Destination->Bounds->GetComponentLocation();
	if (!Destination->ContainsLocation(FVector(XY, Center.Z))) return false;
	const bool bInitialize = Destination != this && Destination->Phase == EMT2DungeonRoomPhase::Idle;
	if (bInitialize) { Destination->ScriptFlags = ScriptFlags; Destination->bScriptControlled = true; }
	const bool bMoved = Destination->JumpScriptPlayers(GetScriptMembers(), XY);
	if (bMoved && Destination != this)
	{
		ClearScriptRegen(); ScriptExecutor->ClearDungeonTimers();
		ScriptEliminationDelay = -1.f; GetWorld()->GetTimerManager().ClearTimer(ScriptEliminationTimer);
		if (Phase == EMT2DungeonRoomPhase::Active) CompleteEncounter();
	}
	return bMoved;
}
bool AMT2DungeonRoom::JumpScriptPlayers(const TArray<TWeakObjectPtr<AMT2PlayerState>>& Players, const FVector2D& XY)
{
	if (!HasAuthority() || !bRegistered || Phase == EMT2DungeonRoomPhase::Failed || !FMath::IsFinite(XY.X) || !FMath::IsFinite(XY.Y)) return false;
	AMT2DungeonRoom* Destination = this;
	const FVector Center = Bounds->GetComponentLocation(), Extent = Bounds->GetScaledBoxExtent();
	if (!ContainsLocation(FVector(XY, Center.Z))) return false;
	bScriptControlled = true;
	bool bMoved = false;
	for (auto Member : Players)
	{
		auto* Player = Member.IsValid() ? Cast<AMT2PlayerCharacter>(Member->GetPawn()) : nullptr;
		if (!Player) continue;
		bool bTeleported = false;
		for (int32 Attempt = 0; Attempt < 256 && !bTeleported; ++Attempt)
		{
			const FVector2D Offset = Attempt == 0 ? FVector2D::ZeroVector : FVector2D((Attempt % 16 - 8)*100., (Attempt / 16 - 8)*100.);
			const FVector2D Candidate = XY + Offset;
			FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(MT2DungeonScriptJump), false, Player);
			if (!GetWorld()->LineTraceSingleByChannel(Hit, FVector(Candidate, Center.Z + Extent.Z), FVector(Candidate, Center.Z - Extent.Z), ECC_WorldStatic, Query)) continue;
			const FVector Position = Hit.ImpactPoint + FVector(0,0,Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2);
			if (!Destination->ContainsLocation(Position)) continue;
			Player->GetCharacterMovement()->StopMovementImmediately();
			bTeleported = Player->TeleportTo(Position, Player->GetActorRotation(), false, false);
		}
		if (!bTeleported) { UE_LOG(LogMT2Dungeon, Error, TEXT("Room %s could not move a member to %s."), *RoomId.ToString(), *Destination->RoomId.ToString()); continue; }
		if (auto* Registry = GetWorld()->GetSubsystem<UMT2DungeonSubsystem>())
			if (auto* Previous = Registry->GetPlayerRoom(Member.Get()); Previous && Previous != this) Previous->LeavePlayer(Member.Get());
		if (!JoinPlayer(Member.Get())) { UE_LOG(LogMT2Dungeon, Error, TEXT("Script jump could not register a player in room %s."), *RoomId.ToString()); continue; }
		bMoved = true;
	}
	if (bMoved && Phase == EMT2DungeonRoomPhase::Idle) StartEncounter();
	return bMoved;
}
void AMT2DungeonRoom::SetScriptElimination(float Delay, const FVector2D& XY, const FString& Regen)
{
	if (!HasAuthority() || !FMath::IsFinite(Delay) || Delay < 0.f) return;
	bScriptControlled = true; ScriptEliminationDelay = Delay; ScriptEliminationXY = XY; ScriptEliminationRegen = Regen;
}
void AMT2DungeonRoom::CheckScriptElimination()
{
	if (!HasAuthority() || ScriptEliminationDelay < 0.f || GetScriptMonsterCount() > 0 || GetWorld()->GetTimerManager().IsTimerActive(ScriptEliminationTimer)) return;
	const auto Perform = [this]()
	{
		auto* Registry = GetWorld()->GetSubsystem<UMT2DungeonSubsystem>();
		auto* Destination = Registry ? Registry->FindQuestRoomAtPosition(LegacyMapIndex, ScriptEliminationXY) : nullptr;
		const FString Regen = ScriptEliminationRegen; ScriptEliminationDelay = -1.f;
		ClearScriptRegen();
		if (!Destination || !JumpScriptMembers(Destination, ScriptEliminationXY)) { UE_LOG(LogMT2Dungeon, Error, TEXT("Room %s elimination transition failed."), *RoomId.ToString()); return; }
		if (!Regen.IsEmpty()) Destination->SpawnScriptRegen(Regen, false);
	};
	if (ScriptEliminationDelay == 0.f) Perform();
	else GetWorld()->GetTimerManager().SetTimer(ScriptEliminationTimer, FTimerDelegate::CreateWeakLambda(this, Perform), ScriptEliminationDelay, false);
}
void AMT2DungeonRoom::SetScriptExit(int32 MapIndex, FVector2D Position)
{
	if (HasAuthority() && MapIndex >= 0 && FMath::IsFinite(Position.X) && FMath::IsFinite(Position.Y))
	{ ScriptExitMap = MapIndex; ScriptExitPosition = Position; }
}
void AMT2DungeonRoom::ExitScriptMembers()
{
	if (!HasAuthority()) return;
	for (auto Member : GetScriptMembers()) if (Member.IsValid())
	{
		FMT2QuestContext Context; Context.PlayerState = Member.Get(); Context.Player = Cast<AMT2PlayerCharacter>(Member->GetPawn());
		Context.Manager = Member->GetQuestManagerComponent();
		auto* Warp = NewObject<UMT2QuestNode_Warp>(this);
		Warp->bToEmpireVillage = ScriptExitMap == 0;
		Warp->MapIndexExpression = FString::FromInt(ScriptExitMap);
		Warp->Position = ScriptExitPosition * 100.;
		Warp->Execute(Context);
	}
}
int32 AMT2DungeonRoom::GetScriptMonsterCount() const
{
	int32 Count = 0;
	for (auto Mob : AliveEnemies) if (Mob.IsValid() &&
		(Mob->GetMobType() == EMT2MobType::Monster || Mob->IsMetinStone())) ++Count;
	return Count;
}
bool AMT2DungeonRoom::TryClaimCompletion(AMT2PlayerState* Player)
{
	const FString Id = PlayerIdentity(Player);
	if (!HasAuthority() || Phase != EMT2DungeonRoomPhase::Completed || !Members.Contains(Player) ||
		Id.IsEmpty() || !EligiblePlayers.Contains(Id) || ClaimedPlayers.Contains(Id)) return false;
	if (!IsValid(Player->GetPawn()) || !ContainsLocation(Player->GetPawn()->GetActorLocation())) return false;
	ClaimedPlayers.Add(Id); return true;
}
bool AMT2DungeonRoom::IsCompletionEligible(const AMT2PlayerState* Player) const
{
	return EligiblePlayers.Contains(PlayerIdentity(Player));
}
bool AMT2DungeonRoom::AdvancePlayer(AMT2PlayerState* Player)
{
	if (!HasAuthority() || Phase != EMT2DungeonRoomPhase::Completed || !Members.Contains(Player) || !IsValid(NextRoom) ||
		NextRoom == this || !NextRoom->bRegistered || NextRoom->GetWorld() != GetWorld() || NextRoom->GetPhase() == EMT2DungeonRoomPhase::Failed) return false;
	APawn* Pawn = IsValid(Player) ? Player->GetPawn() : nullptr;
	const FVector Location = NextRoom->Entrance->GetComponentLocation();
	if (!IsValid(Pawn) || !NextRoom->ContainsLocation(Location)) return false;
	const FVector OldLocation = Pawn->GetActorLocation(); const FRotator OldRotation = Pawn->GetActorRotation();
	bool bTeleported = false;
	for (int32 Attempt = 0; Attempt < 25 && !bTeleported; ++Attempt)
	{
		// Several members can advance together. Avoid stacking every capsule on the entrance.
		const FVector Offset(100.f * (Attempt % 5 - 2), 100.f * (Attempt / 5 - 2), 0);
		const FVector Candidate = Attempt == 0 ? Location : Location + Offset;
		if (NextRoom->ContainsLocation(Candidate))
			bTeleported = Pawn->TeleportTo(Candidate, NextRoom->Entrance->GetComponentRotation(), false, false);
	}
	if (!bTeleported) return false;
	LeavePlayer(Player);
	if (NextRoom->JoinPlayer(Player)) return true;
	Pawn->TeleportTo(OldLocation, OldRotation, false, true); JoinPlayer(Player); return false;
}
double AMT2DungeonRoom::GetSecondsRemaining() const
{
	const AGameStateBase* State = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const double Now = State ? State->GetServerWorldTimeSeconds() : GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	return Phase == EMT2DungeonRoomPhase::Active && Deadline > 0.0 ? FMath::Max(Deadline - Now, 0.0) : 0.0;
}
