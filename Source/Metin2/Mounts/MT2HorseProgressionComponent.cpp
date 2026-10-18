#include "Mounts/MT2HorseProgressionComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Mounts/MT2MountComponent.h"
#include "Player/MT2PlayerState.h"
#include "Persistence/MT2PersistenceComponent.h"
#include "Skills/MT2SkillComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace
{
	constexpr int32 HorseSkillVnum = 130;
	constexpr int32 HorseMaxLevel = 30;
	constexpr int32 StaminaConsumeSeconds = 6 * 60;
	constexpr int32 StaminaRegenerateSeconds = 12 * 60;
	constexpr int64 HealthDropSeconds = 3 * 24 * 60 * 60;
	constexpr int32 MinimumRiderLevelByHorseLevel[HorseMaxLevel + 1] = {
		0, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
		35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
		50, 50, 50, 50, 50, 50, 50, 50, 50, 50
	};
	constexpr int32 HorsePrimaryStats[HorseMaxLevel + 1][4] = {
		{0, 0, 0, 0}, {26, 35, 18, 9}, {27, 36, 18, 9}, {28, 38, 19, 9},
		{29, 39, 19, 10}, {30, 40, 20, 10}, {31, 41, 21, 10}, {32, 42, 21, 11},
		{33, 44, 22, 11}, {34, 45, 22, 11}, {35, 46, 23, 12}, {40, 53, 27, 13},
		{41, 54, 27, 14}, {42, 56, 28, 14}, {43, 57, 28, 14}, {44, 58, 29, 15},
		{44, 59, 30, 15}, {45, 60, 30, 15}, {46, 62, 31, 15}, {47, 63, 31, 16},
		{48, 64, 32, 16}, {53, 71, 36, 18}, {55, 74, 37, 18}, {57, 76, 38, 19},
		{59, 78, 39, 20}, {60, 80, 40, 20}, {61, 81, 40, 20}, {62, 83, 42, 21},
		{63, 84, 42, 21}, {65, 87, 43, 22}, {67, 89, 45, 22}
	};
	constexpr int32 MaxHealthByLevel[HorseMaxLevel + 1] = {
		1, 3, 4, 5, 7, 8, 9, 11, 12, 13, 15, 18, 19, 21, 22, 24,
		25, 27, 28, 30, 32, 35, 36, 37, 38, 40, 42, 44, 46, 48, 50
	};
	constexpr int32 MaxStaminaByLevel[HorseMaxLevel + 1] = {
		1, 4, 4, 5, 5, 6, 6, 7, 7, 8, 10, 30, 35, 40, 50, 55,
		60, 65, 70, 80, 100, 120, 125, 130, 135, 140, 145, 150, 160, 170, 200
	};
}

UMT2HorseProgressionComponent::UMT2HorseProgressionComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UMT2HorseProgressionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMT2HorseProgressionComponent, Level);
	DOREPLIFETIME(UMT2HorseProgressionComponent, Health);
	DOREPLIFETIME(UMT2HorseProgressionComponent, Stamina);
	DOREPLIFETIME(UMT2HorseProgressionComponent, bRiding);
}

void UMT2HorseProgressionComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		RefreshTimers();
	}
}

int32 UMT2HorseProgressionComponent::GetMaxHealth() const
{
	return Level > 0 ? MaxHealthByLevel[FMath::Clamp(Level, 0, HorseMaxLevel)] : 0;
}

int32 UMT2HorseProgressionComponent::GetMaxStamina() const
{
	return Level > 0 ? MaxStaminaByLevel[FMath::Clamp(Level, 0, HorseMaxLevel)] : 0;
}

int32 UMT2HorseProgressionComponent::GetMinimumRiderLevel() const
{
	return MinimumRiderLevelByHorseLevel[FMath::Clamp(Level, 0, HorseMaxLevel)];
}

FMT2PrimaryStats UMT2HorseProgressionComponent::GetMountedMinimumStats() const
{
	FMT2PrimaryStats Result;
	if (Level > 0 && Level <= HorseMaxLevel)
	{
		const int32* Stats = HorsePrimaryStats[Level];
		Result.Strength = Stats[0];
		Result.Dexterity = Stats[1];
		Result.Constitution = Stats[2];
		Result.Intelligence = Stats[3];
	}
	return Result;
}

bool UMT2HorseProgressionComponent::SetLevel(int32 NewLevel)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	Level = FMath::Clamp(NewLevel, 0, HorseMaxLevel);
	Health = Level > 0 ? GetMaxHealth() : 0;
	Stamina = Level > 0 ? GetMaxStamina() : 0;
	if (Level == 0) bRiding = false;
	HealthDropAtUnix = Level > 0 ? FDateTime::UtcNow().ToUnixTimestamp() + HealthDropSeconds : 0;
	StaminaUpdatedAtUnix = Level > 0 ? FDateTime::UtcNow().ToUnixTimestamp() : 0;
	if (AMT2PlayerState* State = Cast<AMT2PlayerState>(GetOwner()))
	{
		if (UMT2SkillComponent* Skills = State->GetSkillComponent()) Skills->SetSkillLevel(HorseSkillVnum, Level);
		if (APawn* Pawn = State->GetPawn())
		{
			if (UMT2MountComponent* Mount = Pawn->FindComponentByClass<UMT2MountComponent>(); Mount && Mount->IsMounted() && Mount->GetMountedKind() == EMT2MountKind::Horse && Level == 0)
			{
				Mount->ForceDismount();
			}
		}
		if (AMT2PlayerCharacter* Character = Cast<AMT2PlayerCharacter>(State->GetPawn()))
		{
			Character->RefreshDerivedStatsForMount();
		}
	}
	RefreshTimers();
	MarkChanged();
	return true;
}

bool UMT2HorseProgressionComponent::AdvanceLevel()
{
	return Level < HorseMaxLevel && SetLevel(Level + 1);
}

bool UMT2HorseProgressionComponent::Feed()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Level <= 0 || IsDead()) return false;
	Health = FMath::Min(Health + 1, GetMaxHealth());
	HealthDropAtUnix = FDateTime::UtcNow().ToUnixTimestamp() + HealthDropSeconds;
	MarkChanged();
	return true;
}

bool UMT2HorseProgressionComponent::Revive()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Level <= 0 || !IsDead()) return false;
	Health = GetMaxHealth();
	Stamina = GetMaxStamina();
	HealthDropAtUnix = FDateTime::UtcNow().ToUnixTimestamp() + HealthDropSeconds;
	StaminaUpdatedAtUnix = FDateTime::UtcNow().ToUnixTimestamp();
	RefreshTimers();
	MarkChanged();
	return true;
}

bool UMT2HorseProgressionComponent::SetRiding(bool bNewRiding)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (bNewRiding && (Level <= 0 || IsDead() || Stamina <= 0)) return false;
	bRiding = bNewRiding;
	StaminaUpdatedAtUnix = FDateTime::UtcNow().ToUnixTimestamp();
	RefreshTimers();
	MarkChanged();
	return true;
}

void UMT2HorseProgressionComponent::RestoreProgression(int32 InLevel, int32 InHealth, int32 InStamina, int64 InHealthDropAtUnix, int64 InStaminaUpdatedAtUnix)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	Level = FMath::Clamp(InLevel, 0, HorseMaxLevel);
	Health = Level > 0 ? FMath::Clamp(InHealth, 0, GetMaxHealth()) : 0;
	Stamina = Level > 0 ? FMath::Clamp(InStamina, 0, GetMaxStamina()) : 0;
	bRiding = false; // Riding is transient and never restored from persistence.
	HealthDropAtUnix = Level > 0 ? (InHealthDropAtUnix > 0 ? InHealthDropAtUnix : FDateTime::UtcNow().ToUnixTimestamp() + HealthDropSeconds) : 0;
	const int64 Now = FDateTime::UtcNow().ToUnixTimestamp();
	StaminaUpdatedAtUnix = Level > 0 ? Now : 0;
	if (Level > 0 && InStaminaUpdatedAtUnix > 0 && Now > InStaminaUpdatedAtUnix)
	{
		const int64 OfflineRecovery = (Now - InStaminaUpdatedAtUnix) / StaminaRegenerateSeconds;
		Stamina = FMath::Min(GetMaxStamina(), Stamina + static_cast<int32>(FMath::Min<int64>(OfflineRecovery, MAX_int32)));
	}
	RefreshTimers();
	if (AMT2PlayerState* State = Cast<AMT2PlayerState>(GetOwner()))
	{
		if (UMT2SkillComponent* Skills = State->GetSkillComponent()) Skills->SetSkillLevel(HorseSkillVnum, Level);
	}
	MarkChanged();
}

void UMT2HorseProgressionComponent::RefreshTimers()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World || !Owner->HasAuthority()) return;
	World->GetTimerManager().ClearTimer(StaminaTimer);
	World->GetTimerManager().ClearTimer(HealthTimer);
	if (Level <= 0 || IsDead()) return;
	if (bRiding)
	{
		World->GetTimerManager().SetTimer(StaminaTimer, this, &UMT2HorseProgressionComponent::ConsumeStamina,
			static_cast<float>(StaminaConsumeSeconds), true);
	}
	else if (Stamina < GetMaxStamina())
	{
		World->GetTimerManager().SetTimer(StaminaTimer, this, &UMT2HorseProgressionComponent::RegenerateStamina,
			static_cast<float>(StaminaRegenerateSeconds), true);
	}
	if (HealthDropAtUnix > 0)
	{
		const int64 Now = FDateTime::UtcNow().ToUnixTimestamp();
		const float Delay = static_cast<float>(FMath::Max<int64>(HealthDropAtUnix - Now, 1));
		World->GetTimerManager().SetTimer(HealthTimer, this, &UMT2HorseProgressionComponent::DropHealth, Delay, false);
	}
}

void UMT2HorseProgressionComponent::ConsumeStamina()
{
	if (!bRiding || IsDead()) { RefreshTimers(); return; }
	Stamina = FMath::Max(0, Stamina - 1);
	StaminaUpdatedAtUnix = FDateTime::UtcNow().ToUnixTimestamp();
	if (Stamina == 0)
	{
		bRiding = false;
		if (AMT2PlayerState* State = Cast<AMT2PlayerState>(GetOwner()))
		{
			if (APawn* Pawn = State->GetPawn())
			{
				if (UMT2MountComponent* Mount = Pawn->FindComponentByClass<UMT2MountComponent>(); Mount && Mount->IsMounted() && Mount->GetMountedKind() == EMT2MountKind::Horse)
				{
					Mount->ForceDismount();
				}
			}
		}
	}
	MarkChanged();
	RefreshTimers();
}

void UMT2HorseProgressionComponent::RegenerateStamina()
{
	if (bRiding || IsDead()) { RefreshTimers(); return; }
	Stamina = FMath::Min(GetMaxStamina(), Stamina + 1);
	StaminaUpdatedAtUnix = FDateTime::UtcNow().ToUnixTimestamp();
	MarkChanged();
	RefreshTimers();
}

void UMT2HorseProgressionComponent::DropHealth()
{
	if (Level <= 0 || IsDead()) return;
	const int64 Now = FDateTime::UtcNow().ToUnixTimestamp();
	const int64 ElapsedDrops = HealthDropAtUnix < Now ? ((Now - HealthDropAtUnix) / HealthDropSeconds) + 1 : 1;
	Health = FMath::Max(0, Health - static_cast<int32>(FMath::Min<int64>(ElapsedDrops, MAX_int32)));
	HealthDropAtUnix += ElapsedDrops * HealthDropSeconds;
	if (IsDead())
	{
		bRiding = false;
		HealthDropAtUnix = 0;
		if (AMT2PlayerState* State = Cast<AMT2PlayerState>(GetOwner()))
		{
			if (APawn* Pawn = State->GetPawn())
			{
				if (UMT2MountComponent* Mount = Pawn->FindComponentByClass<UMT2MountComponent>(); Mount && Mount->IsMounted() && Mount->GetMountedKind() == EMT2MountKind::Horse)
				{
					Mount->ForceDismount();
				}
			}
		}
	}
	MarkChanged();
	RefreshTimers();
}

void UMT2HorseProgressionComponent::MarkChanged()
{
	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
		if (AMT2PlayerState* State = Cast<AMT2PlayerState>(Owner))
		{
			if (UMT2PersistenceComponent* Persistence = State->GetPersistenceComponent()) Persistence->MarkDirty();
		}
	}
}
