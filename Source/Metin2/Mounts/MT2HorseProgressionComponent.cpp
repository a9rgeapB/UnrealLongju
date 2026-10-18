#include "Mounts/MT2HorseProgressionComponent.h"

#include "Config/MT2GameplaySettings.h"
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
	const FMT2HorseLevelSettings* GetHorseLevelRules(int32 Level)
	{
		return UMT2GameplaySettings::Get().GetHorseLevelSettings(Level);
	}

	int32 GetSafeInterval(int32 Value)
	{
		return FMath::Max(Value, 1);
	}

	int64 GetHealthDecayInterval()
	{
		return static_cast<int64>(GetSafeInterval(UMT2GameplaySettings::Get().HorseHealthDecayIntervalSeconds));
	}
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
	const FMT2HorseLevelSettings* Rules = GetHorseLevelRules(Level);
	return Level > 0 && Rules ? FMath::Max(Rules->MaximumHealth, 1) : 0;
}

int32 UMT2HorseProgressionComponent::GetMaxStamina() const
{
	const FMT2HorseLevelSettings* Rules = GetHorseLevelRules(Level);
	return Level > 0 && Rules ? FMath::Max(Rules->MaximumStamina, 1) : 0;
}

int32 UMT2HorseProgressionComponent::GetMinimumRiderLevel() const
{
	const FMT2HorseLevelSettings* Rules = GetHorseLevelRules(Level);
	return Rules ? FMath::Max(Rules->MinimumRiderLevel, 0) : 0;
}

int32 UMT2HorseProgressionComponent::GetGrade() const
{
	if (Level <= 0) return 0;
	const UMT2GameplaySettings& Settings = UMT2GameplaySettings::Get();
	const int32 LevelsPerGrade = FMath::Max(Settings.HorseLevelsPerGrade, 1);
	return FMath::Clamp(((Level - 1) / LevelsPerGrade) + 1, 1, FMath::Max(Settings.MaximumHorseGrade, 1));
}

FMT2PrimaryStats UMT2HorseProgressionComponent::GetMountedMinimumStats() const
{
	FMT2PrimaryStats Result;
	if (Level > 0)
	{
		if (const FMT2HorseLevelSettings* Rules = GetHorseLevelRules(Level))
		{
			Result = Rules->MinimumPrimaryStats;
		}
	}
	return Result;
}

bool UMT2HorseProgressionComponent::SetLevel(int32 NewLevel)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	const UMT2GameplaySettings& Settings = UMT2GameplaySettings::Get();
	Level = FMath::Clamp(NewLevel, 0, Settings.GetConfiguredMaximumHorseLevel());
	Health = Level > 0 ? GetMaxHealth() : 0;
	Stamina = Level > 0 ? GetMaxStamina() : 0;
	if (Level == 0) bRiding = false;
	HealthDropAtUnix = Level > 0 ? FDateTime::UtcNow().ToUnixTimestamp() + GetHealthDecayInterval() : 0;
	StaminaUpdatedAtUnix = Level > 0 ? FDateTime::UtcNow().ToUnixTimestamp() : 0;
	if (AMT2PlayerState* State = Cast<AMT2PlayerState>(GetOwner()))
	{
		if (UMT2SkillComponent* Skills = State->GetSkillComponent()) Skills->SetSkillLevel(Settings.HorseSummonSkillVnum, Level);
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
	return Level < UMT2GameplaySettings::Get().GetConfiguredMaximumHorseLevel() && SetLevel(Level + 1);
}

bool UMT2HorseProgressionComponent::Feed()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Level <= 0 || IsDead()) return false;
	Health = FMath::Min(Health + FMath::Max(UMT2GameplaySettings::Get().HorseFeedHealthAmount, 1), GetMaxHealth());
	HealthDropAtUnix = FDateTime::UtcNow().ToUnixTimestamp() + GetHealthDecayInterval();
	MarkChanged();
	return true;
}

bool UMT2HorseProgressionComponent::Revive()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Level <= 0 || !IsDead()) return false;
	Health = GetMaxHealth();
	Stamina = GetMaxStamina();
	HealthDropAtUnix = FDateTime::UtcNow().ToUnixTimestamp() + GetHealthDecayInterval();
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
	const UMT2GameplaySettings& Settings = UMT2GameplaySettings::Get();
	Level = FMath::Clamp(InLevel, 0, Settings.GetConfiguredMaximumHorseLevel());
	Health = Level > 0 ? FMath::Clamp(InHealth, 0, GetMaxHealth()) : 0;
	Stamina = Level > 0 ? FMath::Clamp(InStamina, 0, GetMaxStamina()) : 0;
	bRiding = false; // Riding is transient and never restored from persistence.
	HealthDropAtUnix = Level > 0 ? (InHealthDropAtUnix > 0 ? InHealthDropAtUnix : FDateTime::UtcNow().ToUnixTimestamp() + GetHealthDecayInterval()) : 0;
	const int64 Now = FDateTime::UtcNow().ToUnixTimestamp();
	StaminaUpdatedAtUnix = Level > 0 ? Now : 0;
	if (Level > 0 && InStaminaUpdatedAtUnix > 0 && Now > InStaminaUpdatedAtUnix)
	{
		const int64 OfflineRecovery = (Now - InStaminaUpdatedAtUnix) /
			GetSafeInterval(Settings.HorseStaminaRegenerationIntervalSeconds);
		Stamina = FMath::Min(GetMaxStamina(), Stamina + static_cast<int32>(FMath::Min<int64>(OfflineRecovery, MAX_int32)));
	}
	RefreshTimers();
	if (AMT2PlayerState* State = Cast<AMT2PlayerState>(GetOwner()))
	{
		if (UMT2SkillComponent* Skills = State->GetSkillComponent()) Skills->SetSkillLevel(Settings.HorseSummonSkillVnum, Level);
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
			static_cast<float>(GetSafeInterval(UMT2GameplaySettings::Get().HorseStaminaDrainIntervalSeconds)), true);
	}
	else if (Stamina < GetMaxStamina())
	{
		World->GetTimerManager().SetTimer(StaminaTimer, this, &UMT2HorseProgressionComponent::RegenerateStamina,
			static_cast<float>(GetSafeInterval(UMT2GameplaySettings::Get().HorseStaminaRegenerationIntervalSeconds)), true);
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
	const int64 HealthDecayInterval = GetHealthDecayInterval();
	const int64 ElapsedDrops = HealthDropAtUnix < Now ? ((Now - HealthDropAtUnix) / HealthDecayInterval) + 1 : 1;
	Health = FMath::Max(0, Health - static_cast<int32>(FMath::Min<int64>(ElapsedDrops, MAX_int32)));
	HealthDropAtUnix += ElapsedDrops * HealthDecayInterval;
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
