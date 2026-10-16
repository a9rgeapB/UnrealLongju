/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/MT2ItemBonusUtils.h"

#include "Items/MT2ItemTemplate.h"

EMT2ItemBonusTarget MT2ItemBonusUtils::ResolveTarget(const UMT2ItemTemplate* Template)
{
	if (const UMT2ItemWeaponTemplate* Weapon = Cast<UMT2ItemWeaponTemplate>(Template))
	{
		return Weapon->WeaponSubType == EMT2WeaponSubType::Arrow
			? EMT2ItemBonusTarget::None : EMT2ItemBonusTarget::Weapon;
	}
	const UMT2ItemArmorTemplate* Armor = Cast<UMT2ItemArmorTemplate>(Template);
	if (!Armor)
	{
		return EMT2ItemBonusTarget::None;
	}
	switch (Armor->ArmorSubType)
	{
	case EMT2ArmorSubType::Body: return EMT2ItemBonusTarget::Body;
	case EMT2ArmorSubType::Head: return EMT2ItemBonusTarget::Head;
	case EMT2ArmorSubType::Shield: return EMT2ItemBonusTarget::Shield;
	case EMT2ArmorSubType::Wrist: return EMT2ItemBonusTarget::Wrist;
	case EMT2ArmorSubType::Foots: return EMT2ItemBonusTarget::Foots;
	case EMT2ArmorSubType::Neck: return EMT2ItemBonusTarget::Neck;
	case EMT2ArmorSubType::Ear: return EMT2ItemBonusTarget::Ear;
	default: return EMT2ItemBonusTarget::None;
	}
}

int32 MT2ItemBonusUtils::RollWeightedIndex(const TArray<int32>& Weights)
{
	int32 Total = 0;
	for (const int32 Weight : Weights)
	{
		Total += FMath::Max(Weight, 0);
	}
	if (Total <= 0)
	{
		return INDEX_NONE;
	}
	int32 Roll = FMath::RandRange(1, Total);
	for (int32 Index = 0; Index < Weights.Num(); ++Index)
	{
		Roll -= FMath::Max(Weights[Index], 0);
		if (Roll <= 0)
		{
			return Index;
		}
	}
	return Weights.Num() - 1;
}

bool MT2ItemBonusUtils::HasBonusType(
	const TArray<FMT2ItemBonus>& Bonuses, int32 Type, EMT2ItemBonusKind Kind)
{
	return Bonuses.ContainsByPredicate([Type, Kind](const FMT2ItemBonus& Bonus)
	{
		return Bonus.IsValid() && Bonus.GetTypeId() == Type && Bonus.Kind == Kind;
	});
}

bool MT2ItemBonusUtils::AddRandomBonus(
	FMT2ItemSlot& Item, const TArray<FMT2ItemBonusDefinition>& Definitions,
	const TArray<int32>& LevelWeights, EMT2ItemBonusTarget Target, EMT2ItemBonusKind Kind)
{
	TArray<const FMT2ItemBonusDefinition*> Available;
	TArray<int32> DefinitionWeights;
	for (const FMT2ItemBonusDefinition& Definition : Definitions)
	{
		const int32* MaximumLevel = Definition.MaxLevelByItemType.Find(Target);
		if (Definition.ApplyType == EMT2ItemBonusType::None || Definition.SelectionWeight <= 0 ||
			!MaximumLevel || *MaximumLevel <= 0 ||
			(Definition.EligibleItemFlags & static_cast<int32>(Target)) == 0 ||
			HasBonusType(Item.Bonuses, static_cast<int32>(Definition.ApplyType), Kind))
		{
			continue;
		}
		Available.Add(&Definition);
		DefinitionWeights.Add(Definition.SelectionWeight);
	}
	const int32 DefinitionIndex = RollWeightedIndex(DefinitionWeights);
	if (!Available.IsValidIndex(DefinitionIndex))
	{
		return false;
	}

	const FMT2ItemBonusDefinition& Definition = *Available[DefinitionIndex];
	const int32 RolledLevel = RollWeightedIndex(LevelWeights);
	const int32 MaxLevel = FMath::Clamp(
		Definition.MaxLevelByItemType.FindRef(Target), 1, 5);
	const int32 LevelIndex = FMath::Clamp(RolledLevel, 0, MaxLevel - 1);
	if (!Definition.Values.IsValidIndex(LevelIndex) || Definition.Values[LevelIndex] == 0)
	{
		return false;
	}
	FMT2ItemBonus& Bonus = Item.Bonuses.AddDefaulted_GetRef();
	Bonus.Type = Definition.ApplyType;
	Bonus.Value = Definition.Values[LevelIndex];
	Bonus.Kind = Kind;
	return true;
}

void MT2ItemBonusUtils::AddDamageAddon(FMT2ItemSlot& Item, const FMT2DamageAddonFormula& Formula)
{
	const float U1 = FMath::Max(FMath::FRand(), UE_SMALL_NUMBER);
	const float U2 = FMath::FRand();
	const float Gaussian = FMath::Sqrt(-2.0f * FMath::Loge(U1)) *
		FMath::Cos(2.0f * UE_PI * U2);
	const int32 Skill = FMath::Clamp(
		FMath::RoundToInt(Formula.SkillDamageMean + Gaussian * Formula.SkillDamageDeviation),
		Formula.SkillDamageMin, Formula.SkillDamageMax);
	const int32 Noise = FMath::RandRange(
		Formula.AverageDamageNoiseRange.X, Formula.AverageDamageNoiseRange.Y) +
		FMath::RandRange(Formula.AverageDamageNoiseRange.X, Formula.AverageDamageNoiseRange.Y);
	const FIntPoint ExtraRange = FMath::Abs(Skill) <= Formula.SkillDamageThreshold
		? Formula.AverageDamageLowSkillExtraRange : Formula.AverageDamageHighSkillExtraRange;
	const int32 Average = FMath::RoundToInt(Formula.AverageDamageSkillMultiplier * Skill) +
		(FMath::Abs(Skill) <= Formula.SkillDamageThreshold ? FMath::Abs(Noise) : 0) +
		FMath::RandRange(ExtraRange.X, ExtraRange.Y);
	FMT2ItemBonus& AverageBonus = Item.Bonuses.AddDefaulted_GetRef();
	AverageBonus.Type = Formula.AverageDamageApplyType;
	AverageBonus.Value = Average;
	FMT2ItemBonus& SkillBonus = Item.Bonuses.AddDefaulted_GetRef();
	SkillBonus.Type = Formula.SkillDamageApplyType;
	SkillBonus.Value = Skill;
}
