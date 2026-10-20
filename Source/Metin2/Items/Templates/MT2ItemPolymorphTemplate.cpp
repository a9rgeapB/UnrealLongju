/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/Templates/MT2ItemPolymorphTemplate.h"

#include "Characters/MT2PlayerCharacter.h"
#include "Config/MT2GameplaySettings.h"
#include "Components/MT2HealthComponent.h"
#include "Components/MT2StatusEffectComponent.h"
#include "Core/MT2VnumRegistrySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Items/MT2InventoryComponent.h"
#include "Items/MT2ItemUtils.h"
#include "Mobs/MT2Mob.h"
#include "Mounts/MT2MountComponent.h"
#include "Player/MT2PlayerState.h"
#include "Skills/MT2SkillComponent.h"
#include "Skills/MT2SkillDefinition.h"

namespace
{
	int32 GetPolymorphSkillVnum(const UMT2GameplaySettings& Settings)
	{
		const UMT2SkillDefinition* SkillDefinition = Settings.GetPolymorphSkillDefinition();
		return SkillDefinition ? SkillDefinition->Vnum : 0;
	}

	TSubclassOf<AMT2Mob> ResolveTargetMob(const FMT2ItemSlot& Item, UWorld* World)
	{
		if (const auto* State = Item.FindState<FMT2PolymorphItemState>(); State && State->TargetMob)
		{
			return State->TargetMob;
		}

		const int32 TargetMobVnum = MT2ItemUtils::GetNumericSocket(Item, 0);
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		UMT2VnumRegistrySubsystem* Registry = GameInstance
			? GameInstance->GetSubsystem<UMT2VnumRegistrySubsystem>() : nullptr;
		return TargetMobVnum > 0 && Registry ? Registry->ResolveMobClass(TargetMobVnum) : nullptr;
	}

	int32 GetPractice(const FMT2ItemSlot& Item)
	{
		if (const auto* State = Item.FindState<FMT2PolymorphItemState>())
		{
			return State->RemainingPractice;
		}
		return MT2ItemUtils::GetNumericSocket(Item, 1);
	}

	int32 GetSuccessBonus(const FMT2ItemSlot& Item)
	{
		if (const auto* State = Item.FindState<FMT2PolymorphItemState>())
		{
			return State->SuccessBonus;
		}
		return MT2ItemUtils::GetNumericSocket(Item, 2);
	}

	bool IsAlreadyPolymorphed(const UMT2StatusEffectComponent& Effects)
	{
		const FMT2AffectIdentifier Polymorph =
			FMT2AffectIdentifier::FromBuiltin(EMT2BuiltinAffect::Polymorph);
		return Effects.HasEffectType(Polymorph);
	}

	FMT2StatusEffect MakePolymorphEffect(const UMT2ItemPolymorphTemplate& Template,
		const FMT2ItemSlot& Item, const FMT2AffectIdentifier& Polymorph,
		int32 DurationSeconds, int32 TargetMobVnum)
	{
		FMT2StatusEffect Effect;
		Effect.Type = Polymorph;
		Effect.SourceItemVnum = Item.Vnum;
		Effect.ApplyType = EMT2ItemBonusType::None;
		Effect.ApplyValue = TargetMobVnum;
		Effect.RemainingSeconds = DurationSeconds;
		Effect.Icon = Template.Icon;
		Effect.bRemoveOnDeath = true;
		return Effect;
	}
}

int32 UMT2ItemPolymorphTemplate::GetBallDurationSeconds(int32 SkillLevel,
	const UMT2GameplaySettings& Settings)
{
	return Settings.GetPolymorphBallDurationSeconds(SkillLevel);
}

int32 UMT2ItemPolymorphTemplate::GetBallDurationSeconds(int32 SkillLevel)
{
	return GetBallDurationSeconds(SkillLevel, UMT2GameplaySettings::Get());
}

int32 UMT2ItemPolymorphTemplate::GetBallAttackBonus(int32 SkillLevel,
	const UMT2GameplaySettings& Settings)
{
	return Settings.GetPolymorphBallAttackBonus(SkillLevel);
}

int32 UMT2ItemPolymorphTemplate::GetBallAttackBonus(int32 SkillLevel)
{
	return GetBallAttackBonus(SkillLevel, UMT2GameplaySettings::Get());
}

int32 UMT2ItemPolymorphTemplate::GetBookDurationSeconds(EMT2SkillMastery Mastery,
	const UMT2GameplaySettings& Settings)
{
	return Settings.GetPolymorphBookDurationSeconds(Mastery);
}

int32 UMT2ItemPolymorphTemplate::GetBookDurationSeconds(EMT2SkillMastery Mastery)
{
	return GetBookDurationSeconds(Mastery, UMT2GameplaySettings::Get());
}

int32 UMT2ItemPolymorphTemplate::GetBookSuccessChance(int32 PlayerLevel, int32 MobLevel,
	int32 SuccessBonus, int32 SkillLevel, const UMT2GameplaySettings& Settings)
{
	return Settings.GetPolymorphBookSuccessChance(PlayerLevel, MobLevel, SuccessBonus, SkillLevel);
}

int32 UMT2ItemPolymorphTemplate::GetBookSuccessChance(int32 PlayerLevel, int32 MobLevel,
	int32 SuccessBonus, int32 SkillLevel)
{
	return GetBookSuccessChance(PlayerLevel, MobLevel, SuccessBonus, SkillLevel,
		UMT2GameplaySettings::Get());
}

void UMT2ItemPolymorphTemplate::InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const
{
	Data.FamilyState.InitializeAs<FMT2PolymorphItemState>(DefaultPolymorphState);
}

void UMT2ItemPolymorphTemplate::CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data)
{
	if (const FMT2PolymorphItemState* State = Data.FindState<FMT2PolymorphItemState>())
	{
		DefaultPolymorphState = *State;
	}
}

EMT2ItemUseExecution UMT2ItemPolymorphTemplate::ExecuteUse(
	AMT2PlayerCharacter& Character, UMT2InventoryComponent& Inventory, int32 InventorySlot) const
{
	if (!Character.HasAuthority() || !Character.GetHealthComponent() || Character.GetHealthComponent()->IsDead() ||
		!Inventory.GetSlots().IsValidIndex(InventorySlot))
	{
		return EMT2ItemUseExecution::Rejected;
	}

	const FMT2ItemSlot& Item = Inventory.GetSlots()[InventorySlot];
	if (Item.IsEmpty() || Item.Vnum != Vnum)
	{
		return EMT2ItemUseExecution::Rejected;
	}

	UMT2StatusEffectComponent* Effects = Character.GetStatusEffectComponent();
	if (!Effects || IsAlreadyPolymorphed(*Effects) ||
		(Character.GetMountComponent() && Character.GetMountComponent()->IsMounted()))
	{
		return EMT2ItemUseExecution::Rejected;
	}
	const UMT2GameplaySettings& GameplaySettings = UMT2GameplaySettings::Get();
	const UClass* ConfiguredBookClass = GameplaySettings.GetPolymorphBookItemClass();
	if (!ConfiguredBookClass)
	{
		return EMT2ItemUseExecution::Rejected;
	}
	const bool bIsBook = GetClass()->IsChildOf(ConfiguredBookClass);

	const TSubclassOf<AMT2Mob> TargetClass = ResolveTargetMob(Item, Character.GetWorld());
	const AMT2Mob* TargetDefaults = TargetClass ? TargetClass->GetDefaultObject<AMT2Mob>() : nullptr;
	if (!TargetDefaults || TargetDefaults->GetMobVnum() <= 0)
	{
		// Legacy item processing consumes a malformed ball/book whose target mob cannot resolve.
		return EMT2ItemUseExecution::ConsumeAndReject;
	}

	const AMT2PlayerState* PlayerState = Character.GetPlayerState<AMT2PlayerState>();
	const UMT2SkillComponent* Skills = PlayerState ? PlayerState->GetSkillComponent() : nullptr;
	const int32 PolymorphSkillVnum = GetPolymorphSkillVnum(GameplaySettings);
	const int32 SkillLevel = Skills && PolymorphSkillVnum > 0
		? FMath::Max(0, Skills->GetSkillLevel(PolymorphSkillVnum)) : 0;
	const FMT2AffectIdentifier Polymorph =
		FMT2AffectIdentifier::FromBuiltin(EMT2BuiltinAffect::Polymorph);

	if (bIsBook)
	{
		const int32 Chance = GetBookSuccessChance(PlayerState ? PlayerState->GetCharacterLevel() : 1,
			TargetDefaults->GetMobLevel(), GetSuccessBonus(Item), SkillLevel, GameplaySettings);
		const int32 SuccessRollMinimum = FMath::Max(0, GameplaySettings.PolymorphBookSuccessRollMinimum);
		const int32 SuccessRollMaximum = FMath::Max(SuccessRollMinimum,
			GameplaySettings.PolymorphBookSuccessRollMaximum);
		if (Chance <= 0 || FMath::RandRange(SuccessRollMinimum, SuccessRollMaximum) > Chance)
		{
			return EMT2ItemUseExecution::Rejected;
		}

		const EMT2SkillMastery Mastery = Skills && PolymorphSkillVnum > 0
			? Skills->GetSkillMastery(PolymorphSkillVnum) : EMT2SkillMastery::Normal;
		const int32 DurationSeconds = GetBookDurationSeconds(Mastery, GameplaySettings);
		FMT2StatusEffect Transform = MakePolymorphEffect(*this, Item, Polymorph,
			DurationSeconds, TargetDefaults->GetMobVnum());
		if (!Effects->AddEffect(Transform, false))
		{
			return EMT2ItemUseExecution::Rejected;
		}

		// Legacy polymorph-book bonus tables currently define movement speed only for selected mob classes.
		const int32 Bonus = FMath::Max(0, SkillLevel + GetSuccessBonus(Item));
		if (Bonus > 0 && GameplaySettings.IsPolymorphBookMovementBonusMob(TargetClass.Get()))
		{
			FMT2StatusEffect MovementBonus = Transform;
			MovementBonus.ApplyType = EMT2ItemBonusType::MovementSpeed;
			MovementBonus.ApplyValue = Bonus;
			MovementBonus.RemainingSeconds = FMath::Max(DurationSeconds -
				GameplaySettings.PolymorphBookMovementBonusExpirationOffsetSeconds,
				GameplaySettings.PolymorphMinimumEffectDurationSeconds);
			if (!Effects->AddEffect(MovementBonus, false))
			{
				Effects->RemoveEffectsByType(Polymorph);
				return EMT2ItemUseExecution::Rejected;
			}
		}

		const int32 Practice = GetPractice(Item);
		if (Practice > 0)
		{
			Inventory.SetItemSocketValue(InventorySlot, 1, Practice - 1);
		}
		return EMT2ItemUseExecution::Accepted;
	}

	const int32 PlayerLevel = PlayerState ? PlayerState->GetCharacterLevel() : 1;
	const int32 MinimumMobLevel = GameplaySettings.GetPolymorphBallMinimumTargetLevel(PlayerLevel);
	if (TargetDefaults->GetMobLevel() >= MinimumMobLevel)
	{
		return EMT2ItemUseExecution::Rejected;
	}

	const int32 DurationSeconds = GetBallDurationSeconds(SkillLevel, GameplaySettings);
	FMT2StatusEffect Transform = MakePolymorphEffect(*this, Item, Polymorph,
		DurationSeconds, TargetDefaults->GetMobVnum());
	if (!Effects->AddEffect(Transform, false))
	{
		return EMT2ItemUseExecution::Rejected;
	}

	FMT2StatusEffect AttackBonus = Transform;
	AttackBonus.ApplyType = EMT2ItemBonusType::AttackValue;
	AttackBonus.ApplyValue = GetBallAttackBonus(SkillLevel, GameplaySettings);
	if (!Effects->AddEffect(AttackBonus, false))
	{
		Effects->RemoveEffectsByType(Polymorph);
		return EMT2ItemUseExecution::Rejected;
	}
	return EMT2ItemUseExecution::Consume;
}

