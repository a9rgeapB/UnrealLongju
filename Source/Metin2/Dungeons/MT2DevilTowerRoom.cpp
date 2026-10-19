#include "Dungeons/MT2DevilTowerRoom.h"
#include "Items/Templates/MT2ItemArmorTemplate.h"
#include "Items/Templates/MT2ItemWeaponTemplate.h"
#include "Player/MT2PlayerState.h"
#include "Player/MT2PlayerController.h"
#include "Dungeons/MT2DungeonSubsystem.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Components/MT2HealthComponent.h"
#include "Core/MT2VnumRegistrySubsystem.h"
#include "Engine/GameInstance.h"
#include "Items/MT2InventoryComponent.h"
#include "Items/MT2ItemTemplate.h"
#include "Items/MT2WorldItem.h"
#include "Mobs/MT2Mob.h"
#include "Quests/MT2QuestManagerComponent.h"
#include "Quests/MT2QuestNode.h"

DEFINE_LOG_CATEGORY_STATIC(LogMT2DevilTower, Log, All);

EMT2QuestNodeResult UMT2DevilTowerChoice::Execute(const FMT2QuestContext& Context)
{
	if (Room.IsValid()) Room->SelectUpperFloors(Context.PlayerState, Context.TargetActor, Epoch, bLeave);
	return EMT2QuestNodeResult::Stop;
}

AMT2DevilTowerRoom::AMT2DevilTowerRoom()
{
	EncounterSeconds = 0.f;
#if WITH_EDITOR
	SetIsSpatiallyLoaded(false);
#endif
}

bool AMT2DevilTowerRoom::JoinPlayer(AMT2PlayerState* Player)
{
	const auto* Registry = GetWorld()->GetSubsystem<UMT2DungeonSubsystem>();
	const bool bAlreadyMember = Registry && Registry->GetPlayerRoom(Player) == this;
	if (!Super::JoinPlayer(Player)) return false;
	if (!bAlreadyMember)
		NotifyPlayer(Player, GetPhase() == EMT2DungeonRoomPhase::Completed ? GetCompletionMessage() : GetObjectiveMessage());
	if (GetPhase() == EMT2DungeonRoomPhase::Idle && !GetWorld()->GetTimerManager().TimerExists(StartTimer))
	{
		// Defer until teleport/possession and the whole group transfer finish. No room Tick.
		StartTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (GetPhase() == EMT2DungeonRoomPhase::Idle && GetParticipantCount() > 0 && !StartEncounter())
				UE_LOG(LogMT2DevilTower, Error, TEXT("Tower floor %d could not start its encounter."), Floor);
		}));
	}
	else if (GetPhase() == EMT2DungeonRoomPhase::Completed && (Floor != 6 || bUpperFloorsSelected)) QueueAdvance();
	if (!bAlreadyMember && Floor == 8)
		if (auto* Pawn = Cast<AMT2PlayerCharacter>(Player->GetPawn()))
		{
			auto* Inventory = Pawn->GetInventoryComponent();
			Inventory->RemoveItemByVnum(30302, Inventory->CountItemByVnum(30302));
		}
	return true;
}

FText AMT2DevilTowerRoom::Legacy(const TCHAR* Key) const
{
	const FText* Message = LegacyMessages.Find(FName(Key));
	return Message ? *Message : FText::GetEmpty();
}

void AMT2DevilTowerRoom::Broadcast(const FText& Message) const
{
	for (auto Member : GetMembers()) if (Member.IsValid()) NotifyPlayer(Member.Get(), Message);
}

bool AMT2DevilTowerRoom::IsMember(const AMT2PlayerState* Player) const
{
	const auto* Registry = GetWorld()->GetSubsystem<UMT2DungeonSubsystem>();
	const auto* Pawn = IsValid(Player) ? Cast<AMT2PlayerCharacter>(Player->GetPawn()) : nullptr;
	return HasAuthority() && Registry && Registry->GetPlayerRoom(Player) == this && Pawn &&
		Pawn->GetHealthComponent() && !Pawn->GetHealthComponent()->IsDead() && ContainsLocation(Pawn->GetActorLocation());
}

void AMT2DevilTowerRoom::OnEncounterMobSpawned(const FMT2DungeonSpawn& Spawn, AMT2Mob* Mob)
{
	if (Spawn.RespawnSeconds <= 0.f) return;
	auto& Group = RegenGroups.FindOrAdd(Spawn.GroupId);
	// The records persist between respawns; register only the original definition once.
	if (!Group.Spawns.ContainsByPredicate([&Spawn](const FMT2DungeonSpawn& Other)
		{ return Other.Vnum == Spawn.Vnum && Other.VariantId == Spawn.VariantId && Other.LocalTransform.Equals(Spawn.LocalTransform); })) Group.Spawns.Add(Spawn);
	Group.Mobs.Add(Mob);
}

bool AMT2DevilTowerRoom::SpawnWave(const TArray<FMT2DungeonSpawn>& Wave)
{
	if (Wave.IsEmpty()) { FailEncounter(); return false; }
	TMap<int32, TArray<AMT2Mob*>> Groups;
	const auto Variants = ChooseSpawnVariants(Wave);
	for (const auto& Spawn : Wave)
	{
		if (Spawn.VariantId > 0 && Variants.FindRef(Spawn.GroupId) != Spawn.VariantId) continue;
		auto* Mob = SpawnEncounterMob(Spawn);
		if (!Mob) { FailEncounter(); return false; }
		if (Spawn.GroupId > 0) Groups.FindOrAdd(Spawn.GroupId).Add(Mob);
	}
	for (const auto& Group : Groups) for (auto* Mob : Group.Value) Mob->SetSpawnGroupMembers(Group.Value);
	for (const auto& Spawn : Wave) if (Spawn.RespawnSeconds > 0.f)
	{
		auto& Records = RegenGroups.FindOrAdd(Spawn.GroupId).Spawns;
		if (!Records.ContainsByPredicate([&Spawn](const auto& Other) { return Other.Vnum == Spawn.Vnum && Other.VariantId == Spawn.VariantId && Other.LocalTransform.Equals(Spawn.LocalTransform); })) Records.Add(Spawn);
	}
	return true;
}
void AMT2DevilTowerRoom::OnSummonedEnemy(AMT2Mob* Mob, AMT2Mob* Source)
{
	for (auto& Pair : RegenGroups) if (Pair.Value.Mobs.Contains(Source))
	{ Pair.Value.Mobs.Add(Mob); break; }
}

AMT2Mob* AMT2DevilTowerRoom::SpawnSupport(const FMT2DungeonSpawn& Spawn)
{
	auto* GI = GetGameInstance(); auto* Registry = GI ? GI->GetSubsystem<UMT2VnumRegistrySubsystem>() : nullptr;
	const auto Class = Registry ? Registry->ResolveMobClass(Spawn.Vnum) : TSubclassOf<AMT2Mob>();
	const auto Transform = Spawn.LocalTransform * GetActorTransform();
	FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
	auto* Mob = Class && ContainsLocation(Transform.GetLocation()) ? GetWorld()->SpawnActor<AMT2Mob>(Class, Transform, Params) : nullptr;
	if (Mob) Supports.Add(Mob);
	else UE_LOG(LogMT2DevilTower, Error, TEXT("Floor %d cannot spawn puzzle NPC %d."), Floor, Spawn.Vnum);
	return Mob;
}

void AMT2DevilTowerRoom::OnEncounterStarted()
{
	for (const auto& Spawn : Spawns) if (Spawn.RespawnSeconds > 0.f)
	{
		auto& Records = RegenGroups.FindOrAdd(Spawn.GroupId).Spawns;
		if (!Records.ContainsByPredicate([&Spawn](const auto& Other) { return Other.Vnum == Spawn.Vnum && Other.VariantId == Spawn.VariantId && Other.LocalTransform.Equals(Spawn.LocalTransform); })) Records.Add(Spawn);
	}
	Kills = Stones = 0; Seals = Floor == 5 ? 5 : 0;
	bPuzzleStarted = bRealKilled = bMapFound = bUpperFloorsSelected = false;
	if (Floor == 5 || Floor == 8)
		for (const auto& Spawn : PuzzleSpawns) if (!SpawnSupport(Spawn)) { FailEncounter(); return; }
	if (Floor == 5) StartWarnings(20);
	if (Floor == 5 || Floor == 8)
		GetWorld()->GetTimerManager().SetTimer(RegenTimer, this, &ThisClass::ProcessRegen, 1.f, true);
}

void AMT2DevilTowerRoom::StartWarnings(int32 Minutes)
{
	MinutesLeft = Minutes; SetEncounterDeadline(Minutes * 60.f);
	GetWorld()->GetTimerManager().SetTimer(WarningTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (GetPhase() != EMT2DungeonRoomPhase::Active) return;
		MinutesLeft -= 5;
		Broadcast(Legacy(MinutesLeft == 15 ? TEXT("_120_dNotice") : MinutesLeft == 10 ? TEXT("_60_dNotice") : TEXT("_70_dNotice")));
	}), 300.f, true);
}

void AMT2DevilTowerRoom::StartStonePuzzle()
{
	if (bPuzzleStarted || PuzzleSpawns.Num() != 7) { FailEncounter(); return; }
	bPuzzleStarted = true;
	const int32 RealIndex = FMath::RandRange(0, 6);
	for (int32 Index = 0; Index < PuzzleSpawns.Num(); ++Index)
	{
		auto* Stone = SpawnEncounterMob(PuzzleSpawns[Index]);
		if (!Stone) { FailEncounter(); return; }
		if (Index == RealIndex) RealStone = Stone; else FakeStones.Add(Stone);
	}
	Broadcast(Legacy(TEXT("_50_dNotice"))); StartWarnings(15);
	GetWorld()->GetTimerManager().SetTimer(PuzzleTimer, this, &ThisClass::CheckFakeStones, 10.f, true);
}

void AMT2DevilTowerRoom::CheckFakeStones()
{
	if (GetPhase() != EMT2DungeonRoomPhase::Active) return;
	if (bRealKilled) { CompleteEncounter(); return; }
	if (!RealStone.IsValid()) { FailEncounter(); return; }
	for (auto& Stone : FakeStones)
		if (Stone.IsValid() && Stone->GetHealthComponent()->GetHealthNormalized() < .5f)
		{
			PurgeEnemy(Stone.Get()); Stone.Reset(); Broadcast(Legacy(TEXT("_90_dNotice")));
		}
}

void AMT2DevilTowerRoom::ProcessRegen()
{
	if (GetPhase() != EMT2DungeonRoomPhase::Active || bMapFound) return;
	const double Now = GetWorld()->GetTimeSeconds();
	// A group returns only after its last member dies, using the imported source delay.
	for (auto& Pair : RegenGroups)
	{
		auto& Group = Pair.Value;
		if (Group.Mobs.ContainsByPredicate([](auto Mob) { return Mob.IsValid() && !Mob->GetHealthComponent()->IsDead(); })) continue;
		if (Group.NextSpawn == 0) { Group.NextSpawn = Now + Group.Spawns[0].RespawnSeconds; continue; }
		if (Now < Group.NextSpawn) continue;
		Group.Mobs.Reset();
		TArray<AMT2Mob*> Spawned;
		const auto Variants = ChooseSpawnVariants(Group.Spawns);
		int32 Expected = 0;
		for (const auto& Spawn : Group.Spawns) if (Spawn.VariantId == 0 || Variants.FindRef(Spawn.GroupId) == Spawn.VariantId) ++Expected;
		for (const auto& Spawn : Group.Spawns)
		{
			if (Spawn.VariantId > 0 && Variants.FindRef(Spawn.GroupId) != Spawn.VariantId) continue;
			auto* Mob = SpawnEncounterMob(Spawn);
			if (!Mob) break;
			Spawned.Add(Mob);
		}
		if (Spawned.Num() != Expected)
		{
			for (auto* Mob : Spawned) PurgeEnemy(Mob);
			Group.Mobs.Reset(); Group.NextSpawn = Now + 1.;
		}
		else
		{
			for (auto* Mob : Spawned) Mob->SetSpawnGroupMembers(Spawned);
			Group.NextSpawn = 0;
		}
	}
}

bool AMT2DevilTowerRoom::DropQuestItem(int32 Vnum, FVector Location)
{
	auto* Item = AMT2WorldItem::SpawnWorldItem(GetWorld(), Location, Vnum, 1, this);
	if (Item) { Drops.Add(Item); if (Vnum == 30302) Item->SetLifeSpan(0.f); }
	else UE_LOG(LogMT2DevilTower, Error, TEXT("Floor %d could not drop quest item %d."), Floor, Vnum);
	return Item != nullptr;
}

void AMT2DevilTowerRoom::OnEnemyKilled(AMT2Mob* Mob)
{
	const int32 Vnum = Mob->GetMobVnum();
	if (Floor == 4 && Vnum == 8016) StartStonePuzzle();
	else if (Floor == 4 && Mob == RealStone.Get()) bRealKilled = true;
	else if (Floor == 5 && Vnum == 1062 && ++Kills >= 50)
	{
		if (DropQuestItem(50084, Mob->GetActorLocation())) Kills = 0;
	}
	else if (Floor == 6 && Vnum == 1092) { Broadcast(Legacy(TEXT("_150_dNotice"))); CompleteEncounter(); }
	else if (Floor == 7 && Vnum == 8018 && ++Stones == 4)
	{
		if (SpawnWave(RegenSpawns)) GetWorld()->GetTimerManager().SetTimer(RegenTimer, this, &ThisClass::ProcessRegen, 1.f, true);
	}
	else if (Floor == 7 && Vnum == 8019) DropQuestItem(30300, Mob->GetActorLocation());
	else if (Floor == 8 && Vnum == 1040 && FMath::RandRange(1, 5) == 1)
		DropQuestItem(FMath::RandRange(1, 10) == 1 ? 30304 : 30303, Mob->GetActorLocation());
	else if (Floor == 9 && Vnum == 1093) CompleteEncounter();
}

bool AMT2DevilTowerRoom::HandleItem(AMT2PlayerState* Player, int32 Slot, AActor* Target)
{
	if (!IsMember(Player) || GetPhase() != EMT2DungeonRoomPhase::Active) return false;
	auto* Pawn = Cast<AMT2PlayerCharacter>(Player->GetPawn()); auto* Inventory = Pawn->GetInventoryComponent();
	const auto& Slots = Inventory->GetSlots();
	if (!Slots.IsValidIndex(Slot) || Slots[Slot].IsEmpty()) return false;
	const int32 Vnum = Slots[Slot].Vnum;
	if (Target)
	{
		auto* Seal = Cast<AMT2Mob>(Target);
		if (!IsValid(Seal) || Seal->IsActorBeingDestroyed() || !Supports.Contains(Seal) ||
			FVector::Dist2D(Pawn->GetActorLocation(), Seal->GetActorLocation()) > 350.f) return false;
		if (!((Floor == 5 && Seal->GetMobVnum() == 20073 && Vnum == 50084 && Seals > 0) ||
			(Floor == 8 && Seal->GetMobVnum() == 20366 && Vnum == 30304))) return false;
		if (!Inventory->ConsumeItemAtSlot(Slot)) return false;
		Supports.Remove(Seal); Seal->Destroy();
		if (Floor == 8 || --Seals == 0) CompleteEncounter();
		else Broadcast(FText::FromString(Legacy(TEXT("_140_dNotice")).ToString().Replace(TEXT("%s"), *FString::FromInt(Seals))));
		return true;
	}
	if (Floor != 7 || (Vnum != 30300 && Vnum != 30302)) return false;
	if (Vnum == 30300)
	{
		if (Stones < 4 || !Inventory->ConsumeItemAtSlot(Slot)) return false;
		if (FMath::RandRange(1, 10) == 1)
		{
			// Do not stop regeneration if the real map could not be created.
			if (DropQuestItem(30302, Pawn->GetActorLocation()))
			{ bMapFound = true; GetWorld()->GetTimerManager().ClearTimer(RegenTimer); }
		}
	}
	else
	{
		if (!bMapFound || Stones < 4 || !Inventory->ConsumeItemAtSlot(Slot)) return false;
		CompleteEncounter();
	}
	return true;
}

bool AMT2DevilTowerRoom::IsBlacksmith(const AActor* Target) const
{
	const auto* Mob = Cast<AMT2Mob>(Target);
	return HasAuthority() && Floor == 6 && GetPhase() == EMT2DungeonRoomPhase::Completed &&
		IsValid(Mob) && !Mob->IsActorBeingDestroyed() && Supports.Contains(Mob) && Mob->GetMobVnum() >= 20074 && Mob->GetMobVnum() <= 20076;
}

bool AMT2DevilTowerRoom::CanRefine(AMT2PlayerState* Player, const AActor* Target, int32 Slot) const
{
	if (!(IsMember(Player) && IsCompletionEligible(Player) && IsBlacksmith(Target) &&
		FVector::Dist2D(Player->GetPawn()->GetActorLocation(), Target->GetActorLocation()) <= 350.f &&
		Player->GetQuestManagerComponent()->GetQuestFlag(TEXT("deviltower_zone.can_refine")) == GetRunSerial())) return false;
	if (Slot == INDEX_NONE) return true;
	const auto* Pawn = Cast<AMT2PlayerCharacter>(Player->GetPawn());
	const auto* Item = Pawn->GetInventoryComponent()->GetTemplateAtSlot(Slot);
	if (!Item || Item->RefineSet <= 0 || Item->RefineSet >= 500) return false;
	const int32 Vnum = CastChecked<AMT2Mob>(Target)->GetMobVnum();
	if (Vnum == 20074) return Cast<UMT2ItemWeaponTemplate>(Item) != nullptr;
	const auto* Armor = Cast<UMT2ItemArmorTemplate>(Item);
	if (!Armor) return false;
	// Legacy ARMOR_BODY=0, ARMOR_HEAD=1, ARMOR_SHIELD=6; the other armor types are accessories.
	const bool bArmor = Armor->ArmorSubType == EMT2ArmorSubType::Body || Armor->ArmorSubType == EMT2ArmorSubType::Head || Armor->ArmorSubType == EMT2ArmorSubType::Ear;
	return Vnum == 20075 ? bArmor : !bArmor;
}

bool AMT2DevilTowerRoom::SelectUpperFloors(AMT2PlayerState* Player, AActor* Target, int32 Epoch, bool bLeave)
{
	if (!IsMember(Player) || !IsBlacksmith(Target) || Epoch != GetRunSerial() ||
		FVector::Dist2D(Player->GetPawn()->GetActorLocation(), Target->GetActorLocation()) > 350.f) return false;
	if (bLeave || Player->GetCharacterLevel() < 75) { ExitPlayer(Player); return true; }
	if (!bUpperFloorsSelected) { bUpperFloorsSelected = true; QueueAdvance(); }
	return true;
}

void AMT2DevilTowerRoom::ExitPlayer(AMT2PlayerState* Player)
{
	if (!HasAuthority() || !IsValid(Player)) return;
	if (auto* Pawn = Cast<AMT2PlayerCharacter>(Player->GetPawn()))
	{
		FMT2QuestContext Context; Context.PlayerState = Player; Context.Player = Pawn; Context.Manager = Player->GetQuestManagerComponent();
		auto* Warp = NewObject<UMT2QuestNode_Warp>(this);
		Warp->bCoordinatesAreLocalMetres = false; Warp->Position = FVector2D(590800, 111200);
		Warp->Execute(Context); // Existing gateway/coordinator transfer and PIE routing.
	}
}

void AMT2DevilTowerRoom::RecoverMapOnLogout(AMT2PlayerState* Player)
{
	if (Floor != 7 || !bMapFound || GetPhase() != EMT2DungeonRoomPhase::Active || !IsValid(Player)) return;
	if (auto* Pawn = Cast<AMT2PlayerCharacter>(Player->GetPawn()); Pawn && Pawn->GetInventoryComponent()->CountItemByVnum(30302) > 0)
		DropQuestItem(30302, Pawn->GetActorLocation());
}

FText AMT2DevilTowerRoom::GetObjectiveMessage() const
{
	switch (Floor)
	{
	case 1: return NSLOCTEXT("MT2Dungeons", "DevilTowerStage1Objective", "[Devil Tower - Stage 1] Destroy the Metin Stone to reach the next stage.");
	case 2: return NSLOCTEXT("MT2Dungeons", "DevilTowerStage2Objective", "[Devil Tower - Stage 2] Defeat all monsters to reach the next stage.");
	case 3: return NSLOCTEXT("MT2Dungeons", "DevilTowerStage3Objective", "[Devil Tower - Stage 3] Defeat the boss and all remaining monsters to complete this stage.");
	case 4: return bPuzzleStarted ? Legacy(TEXT("_50_dNotice")) : NSLOCTEXT("MT2Dungeons", "DevilTowerStage4FirstStone", "Destroy the Metin Stone to reveal the real and fake stones.");
	case 5: return Legacy(TEXT("_110_dNotice"));
	case 6: return Legacy(TEXT("_130_dNotice"));
	case 7: return Legacy(TEXT("_240_dNotice"));
	case 8: return Legacy(TEXT("_260_dNotice"));
	case 9: return Legacy(TEXT("_270_dNotice"));
	default: return FText::GetEmpty();
	}
}

FText AMT2DevilTowerRoom::GetCompletionMessage() const
{
	if (Floor == 4) return Legacy(TEXT("_100_dNotice"));
	if (Floor == 5) return Legacy(TEXT("_130_dNotice"));
	if (Floor == 6) return Legacy(TEXT("_160_dNotice"));
	if (Floor == 7) return Legacy(TEXT("_250_say"));
	if (Floor == 9) return Legacy(TEXT("_280_dNotice"));
	const float Remaining = GetWorld()->GetTimerManager().GetTimerRemaining(AdvanceTimer);
	if (IsValid(NextRoom) && FMath::IsFinite(TransitionSeconds) && TransitionSeconds > 0.f)
		return FText::Format(NSLOCTEXT("MT2Dungeons", "DevilTowerStageCompletedAdvance",
			"[Devil Tower - Stage {0}] Stage completed! Moving to the next stage in {1} seconds."),
			FText::AsNumber(Floor), FText::AsNumber(FMath::CeilToInt(Remaining >= 0.f ? Remaining : TransitionSeconds)));
	return FText::Format(NSLOCTEXT("MT2Dungeons", "DevilTowerStageCompleted", "[Devil Tower - Stage {0}] Stage completed!"), FText::AsNumber(Floor));
}

void AMT2DevilTowerRoom::NotifyPlayer(AMT2PlayerState* Player, const FText& Message) const
{
	APawn* Pawn = IsValid(Player) ? Player->GetPawn() : nullptr;
	if (auto* Controller = IsValid(Pawn) ? Cast<AMT2PlayerController>(Pawn->GetController()) : nullptr)
		Controller->SendDungeonChatMessage(Message);
}

void AMT2DevilTowerRoom::OnEncounterCompleted()
{
	StopStageTimers();
	if (Floor >= 4) PurgeEnemies();
	if (Floor == 6)
	{
		if (PuzzleSpawns.Num() != 3 || !SpawnSupport(PuzzleSpawns[FMath::RandRange(0, 2)]))
		{
			UE_LOG(LogMT2DevilTower, Error, TEXT("Tower reward blacksmith unavailable; room progression cannot continue."));
			for (auto Member : GetMembers()) if (Member.IsValid()) ExitPlayer(Member.Get());
			return;
		}
	}
	if (Floor == 6 || Floor == 9)
		for (auto Member : GetMembers()) if (Member.IsValid() && TryClaimCompletion(Member.Get()))
			Member->GetQuestManagerComponent()->SetQuestFlag(Floor == 6 ? TEXT("deviltower_zone.can_refine") : TEXT("deviltower_zone.9_done"), Floor == 6 ? GetRunSerial() : 1);
	if (Floor != 6) QueueAdvance();
	const FText Message = GetCompletionMessage();
	for (auto Member : GetMembers()) if (Member.IsValid()) NotifyPlayer(Member.Get(), Message);
	if (Floor == 9) Broadcast(Legacy(TEXT("_290_dNotice")));
}

void AMT2DevilTowerRoom::StopStageTimers()
{
	GetWorld()->GetTimerManager().ClearTimer(PuzzleTimer);
	GetWorld()->GetTimerManager().ClearTimer(RegenTimer);
	GetWorld()->GetTimerManager().ClearTimer(WarningTimer);
}

void AMT2DevilTowerRoom::OnEncounterFailed()
{
	StopStageTimers(); PurgeEnemies();
	Broadcast(Legacy(TEXT("_80_dNotice")));
	for (auto Member : GetMembers()) if (Member.IsValid()) ExitPlayer(Member.Get());
	for (auto Support : Supports) if (Support.IsValid()) Support->Destroy();
	Supports.Reset();
}

void AMT2DevilTowerRoom::OnEncounterReset()
{
	StopStageTimers();
	GetWorld()->GetTimerManager().ClearTimer(StartTimer);
	GetWorld()->GetTimerManager().ClearTimer(AdvanceTimer);
	for (auto Support : Supports) if (Support.IsValid()) Support->Destroy();
	for (auto Drop : Drops) if (Drop.IsValid()) Drop->Destroy();
	Supports.Reset(); Drops.Reset(); FakeStones.Reset(); RealStone.Reset(); RegenGroups.Reset();
	Kills = Stones = Seals = MinutesLeft = 0; bPuzzleStarted = bRealKilled = bMapFound = bUpperFloorsSelected = false;
}

void AMT2DevilTowerRoom::QueueAdvance()
{
	if (!HasAuthority() || (!IsValid(NextRoom) && Floor != 9) || GetWorld()->GetTimerManager().TimerExists(AdvanceTimer)) return;
	if (!FMath::IsFinite(TransitionSeconds) || TransitionSeconds <= 0.f) return;
	const int32 Epoch = GetRunSerial();
	GetWorld()->GetTimerManager().SetTimer(AdvanceTimer, FTimerDelegate::CreateWeakLambda(this, [this, Epoch]()
	{
		if (GetRunSerial() != Epoch || GetPhase() != EMT2DungeonRoomPhase::Completed) return;
		const auto Snapshot = GetMembers();
		if (Floor == 9) { for (auto Member : Snapshot) if (Member.IsValid()) ExitPlayer(Member.Get()); return; }
		for (auto Member : Snapshot) if (Member.IsValid() && !AdvancePlayer(Member.Get()))
			UE_LOG(LogMT2DevilTower, Warning, TEXT("Tower floor %d could not transfer %s; player remains in the source room."), Floor, *Member->GetName());
	}), TransitionSeconds, false);
}

void AMT2DevilTowerRoom::EndPlay(const EEndPlayReason::Type Reason)
{
	OnEncounterReset();
	Super::EndPlay(Reason);
}
