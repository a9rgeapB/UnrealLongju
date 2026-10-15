/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/MT2ItemBonusSettings.h"

namespace
{
enum class EBonusDisplayFormat : uint8 { Percent, Flat, Boolean, Meters, Multiplier };
struct FBonusDisplay
{
	const TCHAR* Name;
	EBonusDisplayFormat Format = EBonusDisplayFormat::Percent;
};

// Runtime data, not UEnum display metadata (which is unavailable in cooked builds).
// Indexed by the server's EApplyTypes ordinals, independently of random-bonus eligibility.
constexpr FBonusDisplay BonusDisplays[] = {
	{TEXT("None"), EBonusDisplayFormat::Flat},
	{TEXT("Maximum HP"), EBonusDisplayFormat::Flat},
	{TEXT("Maximum SP"), EBonusDisplayFormat::Flat},
	{TEXT("Constitution (VIT)"), EBonusDisplayFormat::Flat},
	{TEXT("Intelligence (INT)"), EBonusDisplayFormat::Flat},
	{TEXT("Strength (STR)"), EBonusDisplayFormat::Flat},
	{TEXT("Dexterity (DEX)"), EBonusDisplayFormat::Flat},
	{TEXT("Attack Speed")},
	{TEXT("Movement Speed")},
	{TEXT("Casting Speed")},
	{TEXT("HP Regeneration")},
	{TEXT("SP Regeneration")},
	{TEXT("Poison Chance")},
	{TEXT("Stun Chance")},
	{TEXT("Slow Chance")},
	{TEXT("Critical Hit Chance")},
	{TEXT("Penetrating Hit Chance")},
	{TEXT("Strong Against Humans")},
	{TEXT("Strong Against Animals")},
	{TEXT("Strong Against Orcs")},
	{TEXT("Strong Against Mystics")},
	{TEXT("Strong Against Undead")},
	{TEXT("Strong Against Devils")},
	{TEXT("HP Absorption")},
	{TEXT("SP Absorption")},
	{TEXT("Mana Burn Chance")},
	{TEXT("Recover SP on Damage")},
	{TEXT("Block Chance")},
	{TEXT("Dodge Chance")},
	{TEXT("Sword Resistance")},
	{TEXT("Two-Handed Sword Resistance")},
	{TEXT("Dagger Resistance")},
	{TEXT("Bell Resistance")},
	{TEXT("Fan Resistance")},
	{TEXT("Arrow Resistance")},
	{TEXT("Fire Resistance")},
	{TEXT("Lightning Resistance")},
	{TEXT("Magic Resistance")},
	{TEXT("Wind Resistance")},
	{TEXT("Reflect Melee Damage")},
	{TEXT("Reflect Curse")},
	{TEXT("Poison Resistance")},
	{TEXT("Recover SP on Kill")},
	{TEXT("Double Experience Chance")},
	{TEXT("Double Yang Chance")},
	{TEXT("Item Drop Bonus")},
	{TEXT("Potion Effect Bonus")},
	{TEXT("Recover HP on Kill")},
	{TEXT("Immune to Stun"), EBonusDisplayFormat::Boolean},
	{TEXT("Immune to Slow"), EBonusDisplayFormat::Boolean},
	{TEXT("Immune to Knockdown"), EBonusDisplayFormat::Boolean},
	{TEXT("Skill"), EBonusDisplayFormat::Flat},
	{TEXT("Bow Distance"), EBonusDisplayFormat::Meters},
	{TEXT("Attack Value"), EBonusDisplayFormat::Flat},
	{TEXT("Defense Bonus"), EBonusDisplayFormat::Flat},
	{TEXT("Magic Attack Value"), EBonusDisplayFormat::Flat},
	{TEXT("Magic Defense Value"), EBonusDisplayFormat::Flat},
	{TEXT("Curse Chance")},
	{TEXT("Maximum Stamina"), EBonusDisplayFormat::Flat},
	{TEXT("Strong Against Warriors")},
	{TEXT("Strong Against Assassins")},
	{TEXT("Strong Against Sura")},
	{TEXT("Strong Against Shamans")},
	{TEXT("Strong Against Monsters")},
	{TEXT("Mall Attack Bonus")},
	{TEXT("Mall Defense Bonus")},
	{TEXT("Mall Experience Bonus")},
	{TEXT("Mall Item Drop Multiplier"), EBonusDisplayFormat::Multiplier},
	{TEXT("Mall Yang Multiplier"), EBonusDisplayFormat::Multiplier},
	{TEXT("Maximum HP")},
	{TEXT("Maximum SP")},
	{TEXT("Skill Damage")},
	{TEXT("Average Damage")},
	{TEXT("Skill Damage Resistance")},
	{TEXT("Average Damage Resistance")},
	{TEXT("PC Bang Experience Bonus")},
	{TEXT("PC Bang Drop Bonus")},
	{TEXT("Extract HP")},
	{TEXT("Warrior Resistance")},
	{TEXT("Assassin Resistance")},
	{TEXT("Sura Resistance")},
	{TEXT("Shaman Resistance")},
	{TEXT("Energy"), EBonusDisplayFormat::Flat},
	{TEXT("Defense Value"), EBonusDisplayFormat::Flat},
	{TEXT("Costume Attribute Bonus")},
	{TEXT("Magic Attack")},
	{TEXT("Melee/Magic Attack")},
	{TEXT("Ice Resistance")},
	{TEXT("Earth Resistance")},
	{TEXT("Dark Resistance")},
	{TEXT("Critical Resistance")},
	{TEXT("Penetrating Resistance")}
};
static_assert(UE_ARRAY_COUNT(BonusDisplays) == static_cast<int32>(EMT2ItemBonusType::AntiPenetratingChance) + 1);
}

const FMT2ItemBonusDefinition* UMT2ItemBonusSettings::FindDefinition(
	int32 ApplyType, EMT2ItemBonusKind Kind) const
{
	const TArray<FMT2ItemBonusDefinition>& Definitions =
		Kind == EMT2ItemBonusKind::Rare ? RareBonuses : NormalBonuses;
	return Definitions.FindByPredicate([ApplyType](const FMT2ItemBonusDefinition& Definition)
	{
		return Definition.ApplyType == ApplyType;
	});
}

FString UMT2ItemBonusSettings::FormatBonus(int32 ApplyType, int32 Value) const
{
	const FMT2ItemBonusDefinition* Definition = FindDefinition(ApplyType, EMT2ItemBonusKind::Normal);
	if (!Definition)
	{
		Definition = FindDefinition(ApplyType, EMT2ItemBonusKind::Rare);
	}
	if (!Definition || Definition->DisplayName.IsEmpty())
	{
		if (ApplyType >= 0 && ApplyType < UE_ARRAY_COUNT(BonusDisplays))
		{
			const FBonusDisplay& Display = BonusDisplays[ApplyType];
			switch (Display.Format)
			{
			case EBonusDisplayFormat::Boolean: return Display.Name;
			case EBonusDisplayFormat::Multiplier: return FString::Printf(TEXT("%s x%.1f"), Display.Name, Value / 10.0);
			case EBonusDisplayFormat::Meters: return FString::Printf(TEXT("%s %+dm"), Display.Name, Value);
			case EBonusDisplayFormat::Flat: return FString::Printf(TEXT("%s %+d"), Display.Name, Value);
			default: return FString::Printf(TEXT("%s %+d%%"), Display.Name, Value);
			}
		}
		return FString::Printf(TEXT("Bonus %d %+d"), ApplyType, Value);
	}
	if (Definition->ValueFormat == EMT2ItemBonusValueFormat::Boolean)
	{
		return Definition->DisplayName;
	}
	return FString::Printf(TEXT("%s %+d%s"), *Definition->DisplayName, Value,
		Definition->ValueFormat == EMT2ItemBonusValueFormat::Percent ? TEXT("%") : TEXT(""));
}
