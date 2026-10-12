#include "Dungeons/MT2DungeonSubsystem.h"
#include "Dungeons/MT2DungeonRoom.h"
#include "Player/MT2PlayerState.h"
#include "Mobs/MT2Mob.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "World/MT2MapPresentationActor.h"
#include "EngineUtils.h"

bool UMT2DungeonSubsystem::RegisterRoom(AMT2DungeonRoom* Room)
{
	check(IsInGameThread());
	if (!IsValid(Room) || !Room->HasAuthority() || Room->GetWorld() != GetWorld() || Room->RoomId.IsNone()) return false;
	if (AMT2DungeonRoom* Existing = FindRoom(Room->RoomId); Existing && Existing != Room) return false;
	if (Room->LegacyMapIndex <= 0)
		for (TActorIterator<AMT2MapPresentationActor> It(GetWorld()); It; ++It)
			if (It->MapIndex > 0) { Room->LegacyMapIndex = It->MapIndex; break; }
	if (Room->LegacyMapIndex > 0 && Room->LegacyMapIndex <= MAX_int32 / 10000 && NextQuestDungeonSerial < 10000)
	{
		const int64 Index = int64(Room->LegacyMapIndex) * 10000 + NextQuestDungeonSerial;
		if (Index <= MAX_int32) { ++NextQuestDungeonSerial; Room->SetQuestDungeonIndex(static_cast<int32>(Index)); QuestDungeons.Add(static_cast<int32>(Index), Room); }
	}
	Rooms.Add(Room->RoomId, Room); return true;
}
void UMT2DungeonSubsystem::UnregisterRoom(AMT2DungeonRoom* Room)
{
	if (!Room) return;
	const auto* Existing = Rooms.Find(Room->RoomId);
	if (Existing && Existing->Get() == Room) Rooms.Remove(Room->RoomId);
	QuestDungeons.Remove(Room->GetQuestDungeonIndex());
	for (auto It = Players.CreateIterator(); It; ++It) if (!It.Value().IsValid() || It.Value() == Room) It.RemoveCurrent();
	for (auto It = Enemies.CreateIterator(); It; ++It) if (!It.Value().IsValid() || It.Value() == Room) It.RemoveCurrent();
}
AMT2DungeonRoom* UMT2DungeonSubsystem::FindRoom(FName Id) const
{
	const auto* Value = Rooms.Find(Id); AMT2DungeonRoom* Room = Value ? Value->Get() : nullptr;
	return IsValid(Room) && !Room->IsActorBeingDestroyed() ? Room : nullptr;
}
AMT2DungeonRoom* UMT2DungeonSubsystem::FindQuestDungeon(int32 Index) const
{
	const auto* Value = QuestDungeons.Find(Index);
	AMT2DungeonRoom* Room = Value ? Value->Get() : nullptr;
	return IsValid(Room) && !Room->IsActorBeingDestroyed() ? Room : nullptr;
}
AMT2DungeonRoom* UMT2DungeonSubsystem::FindRoomAtLocation(FVector Location) const
{
	AMT2DungeonRoom* Match = nullptr;
	for (const auto& Pair : Rooms)
		if (auto* Room = Pair.Value.Get(); IsValid(Room) && !Room->IsActorBeingDestroyed() && Room->ContainsLocation(Location))
		{
			if (Match) return nullptr; // Ambiguous overlapping stage bounds must not choose arbitrarily.
			Match = Room;
		}
	return Match;
}
AMT2DungeonRoom* UMT2DungeonSubsystem::FindQuestRoomAtPosition(int32 MapIndex, FVector2D XY) const
{
	AMT2DungeonRoom* Match = nullptr;
	for (const auto& Pair : Rooms)
		if (auto* Room = Pair.Value.Get(); IsValid(Room) && !Room->IsActorBeingDestroyed() && Room->LegacyMapIndex == MapIndex &&
			Room->ContainsLocation(FVector(XY, Room->Bounds->GetComponentLocation().Z)))
		{
			if (Match) return nullptr;
			Match = Room;
		}
	return Match;
}
AMT2DungeonRoom* UMT2DungeonSubsystem::GetPlayerRoom(const AMT2PlayerState* Player) const
{
	const auto* Value = Players.Find(Player); AMT2DungeonRoom* Room = Value ? Value->Get() : nullptr;
	return IsValid(Player) && !Player->IsActorBeingDestroyed() && IsValid(Room) && !Room->IsActorBeingDestroyed() ? Room : nullptr;
}
bool UMT2DungeonSubsystem::AssignPlayer(AMT2PlayerState* Player, AMT2DungeonRoom* Room)
{
	if (!IsValid(Room) || !IsValid(Player) || Player->GetWorld() != GetWorld() || FindRoom(Room->RoomId) != Room) return false;
	if (AMT2DungeonRoom* Existing = GetPlayerRoom(Player); Existing && Existing != Room) return false;
	Players.Add(Player, Room); return true;
}
AMT2DungeonRoom* UMT2DungeonSubsystem::GetEnemyRoom(const AMT2Mob* Mob) const
{
	const auto* Value = Enemies.Find(Mob); auto* Room = Value ? Value->Get() : nullptr;
	return IsValid(Room) && !Room->IsActorBeingDestroyed() ? Room : nullptr;
}
void UMT2DungeonSubsystem::ReconcilePlayer(AMT2PlayerState* Player)
{
	if (!IsValid(Player) || !Player->HasAuthority() || Player->GetWorld() != GetWorld()) return;
	APawn* Pawn = Player->GetPawn();
	if (!IsValid(Pawn)) return;
	if (AMT2DungeonRoom* Existing = GetPlayerRoom(Player))
	{
		if (Existing->Bounds->IsOverlappingActor(Pawn)) return;
		Existing->LeavePlayer(Player);
	}
	TArray<AActor*> Overlaps; Pawn->GetOverlappingActors(Overlaps, AMT2DungeonRoom::StaticClass());
	// Prefer the room containing the pawn center when its capsule touches adjacent volumes.
	for (AActor* Actor : Overlaps)
	{
		auto* Room = Cast<AMT2DungeonRoom>(Actor);
		if (Room && Room->ContainsLocation(Pawn->GetActorLocation()) && Room->JoinPlayer(Player)) return;
	}
	for (AActor* Actor : Overlaps) if (auto* Room = Cast<AMT2DungeonRoom>(Actor); Room && Room->JoinPlayer(Player)) return;
}
void UMT2DungeonSubsystem::RemovePlayer(AMT2PlayerState* Player, AMT2DungeonRoom* Room)
{
	const auto* Existing = Players.Find(Player); if (Existing && Existing->Get() == Room) Players.Remove(Player);
}
bool UMT2DungeonSubsystem::AssignEnemy(AMT2Mob* Mob, AMT2DungeonRoom* Room)
{
	if (!IsValid(Room) || !IsValid(Mob) || Mob->GetWorld() != GetWorld() || FindRoom(Room->RoomId) != Room) return false;
	const auto* Existing = Enemies.Find(Mob);
	if (Existing && Existing->IsValid() && Existing->Get() != Room) return false;
	Enemies.Add(Mob, Room); return true;
}
void UMT2DungeonSubsystem::RemoveEnemy(AMT2Mob* Mob, AMT2DungeonRoom* Room)
{
	const auto* Existing = Enemies.Find(Mob); if (Existing && Existing->Get() == Room) Enemies.Remove(Mob);
}
void UMT2DungeonSubsystem::Deinitialize()
{
	Rooms.Reset(); QuestDungeons.Reset(); Players.Reset(); Enemies.Reset(); Super::Deinitialize();
}
