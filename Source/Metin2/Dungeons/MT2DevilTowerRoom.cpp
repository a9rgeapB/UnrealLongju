#include "Dungeons/MT2DevilTowerRoom.h"
#include "Player/MT2PlayerState.h"
#include "Player/MT2PlayerController.h"
#include "Dungeons/MT2DungeonSubsystem.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogMT2DevilTower, Log, All);

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
	else if (GetPhase() == EMT2DungeonRoomPhase::Completed) QueueAdvance();
	return true;
}

FText AMT2DevilTowerRoom::GetObjectiveMessage() const
{
	switch (Floor)
	{
	case 1: return NSLOCTEXT("MT2Dungeons", "DevilTowerStage1Objective", "[Devil Tower - Stage 1] Destroy the Metin Stone to reach the next stage.");
	case 2: return NSLOCTEXT("MT2Dungeons", "DevilTowerStage2Objective", "[Devil Tower - Stage 2] Defeat all monsters to reach the next stage.");
	case 3: return NSLOCTEXT("MT2Dungeons", "DevilTowerStage3Objective", "[Devil Tower - Stage 3] Defeat the boss and all remaining monsters to complete this stage.");
	default: return FText::GetEmpty();
	}
}

FText AMT2DevilTowerRoom::GetCompletionMessage() const
{
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
	QueueAdvance();
	const FText Message = GetCompletionMessage();
	for (auto Member : GetMembers()) if (Member.IsValid()) NotifyPlayer(Member.Get(), Message);
}

void AMT2DevilTowerRoom::QueueAdvance()
{
	if (!HasAuthority() || !IsValid(NextRoom) || GetWorld()->GetTimerManager().TimerExists(AdvanceTimer)) return;
	if (!FMath::IsFinite(TransitionSeconds) || TransitionSeconds <= 0.f) return;
	const int32 Epoch = GetRunSerial();
	GetWorld()->GetTimerManager().SetTimer(AdvanceTimer, FTimerDelegate::CreateWeakLambda(this, [this, Epoch]()
	{
		if (GetRunSerial() != Epoch || GetPhase() != EMT2DungeonRoomPhase::Completed) return;
		const auto Snapshot = GetMembers();
		for (auto Member : Snapshot) if (Member.IsValid() && !AdvancePlayer(Member.Get()))
			UE_LOG(LogMT2DevilTower, Warning, TEXT("Tower floor %d could not transfer %s; player remains in the source room."), Floor, *Member->GetName());
	}), TransitionSeconds, false);
}

void AMT2DevilTowerRoom::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorld()->GetTimerManager().ClearTimer(StartTimer);
	GetWorld()->GetTimerManager().ClearTimer(AdvanceTimer);
	Super::EndPlay(Reason);
}
