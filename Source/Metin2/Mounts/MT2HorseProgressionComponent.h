#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Stats/MT2StatTypes.h"
#include "MT2HorseProgressionComponent.generated.h"

UCLASS(ClassGroup=(Metin2), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class METIN2_API UMT2HorseProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMT2HorseProgressionComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category="Horse") int32 GetLevel() const { return Level; }
	UFUNCTION(BlueprintPure, Category="Horse") int32 GetGrade() const;
	UFUNCTION(BlueprintPure, Category="Horse") int32 GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure, Category="Horse") int32 GetMaxHealth() const;
	UFUNCTION(BlueprintPure, Category="Horse") int32 GetStamina() const { return Stamina; }
	UFUNCTION(BlueprintPure, Category="Horse") int32 GetMaxStamina() const;
	UFUNCTION(BlueprintPure, Category="Horse") int32 GetMinimumRiderLevel() const;
	FMT2PrimaryStats GetMountedMinimumStats() const;
	UFUNCTION(BlueprintPure, Category="Horse") bool IsDead() const { return Level > 0 && Health <= 0; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Horse") bool SetLevel(int32 NewLevel);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Horse") bool AdvanceLevel();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Horse") bool Feed();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Horse") bool Revive();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Horse") bool SetRiding(bool bNewRiding);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Horse|Persistence") void RestoreProgression(int32 InLevel, int32 InHealth, int32 InStamina, int64 InHealthDropAtUnix, int64 InStaminaUpdatedAtUnix);
	int64 GetHealthDropAtUnix() const { return HealthDropAtUnix; }
	int64 GetStaminaUpdatedAtUnix() const { return StaminaUpdatedAtUnix; }

private:
	void RefreshTimers();
	void ConsumeStamina();
	void RegenerateStamina();
	void DropHealth();
	void MarkChanged();

	UPROPERTY(Replicated, VisibleInstanceOnly, Category="Horse") int32 Level = 0;
	UPROPERTY(Replicated, VisibleInstanceOnly, Category="Horse") int32 Health = 0;
	UPROPERTY(Replicated, VisibleInstanceOnly, Category="Horse") int32 Stamina = 0;
	UPROPERTY(Replicated, VisibleInstanceOnly, Category="Horse") bool bRiding = false;
	UPROPERTY(VisibleInstanceOnly, Category="Horse|Persistence") int64 HealthDropAtUnix = 0;
	UPROPERTY(VisibleInstanceOnly, Category="Horse|Persistence") int64 StaminaUpdatedAtUnix = 0;

	FTimerHandle StaminaTimer;
	FTimerHandle HealthTimer;
};
