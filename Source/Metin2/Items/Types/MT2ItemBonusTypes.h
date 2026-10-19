/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "CoreMinimal.h"
#include "MT2ItemBonusTypes.generated.h"

UENUM(BlueprintType)
enum class EMT2ItemBonusKind : uint8
{
	Normal,
	Rare
};

UENUM(BlueprintType)
enum class EMT2ItemBonusType : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	MaxHP = 1 UMETA(DisplayName = "Maximum HP"),
	MaxSP = 2 UMETA(DisplayName = "Maximum SP"),
	Constitution = 3 UMETA(DisplayName = "Constitution (VIT)"),
	Intelligence = 4 UMETA(DisplayName = "Intelligence (INT)"),
	Strength = 5 UMETA(DisplayName = "Strength (STR)"),
	Dexterity = 6 UMETA(DisplayName = "Dexterity (DEX)"),
	AttackSpeed = 7 UMETA(DisplayName = "Attack Speed"),
	MovementSpeed = 8 UMETA(DisplayName = "Movement Speed"),
	CastSpeed = 9 UMETA(DisplayName = "Casting Speed"),
	HPRegeneration = 10 UMETA(DisplayName = "HP Regeneration"),
	SPRegeneration = 11 UMETA(DisplayName = "SP Regeneration"),
	PoisonChance = 12 UMETA(DisplayName = "Poison Chance"),
	StunChance = 13 UMETA(DisplayName = "Stun Chance"),
	SlowChance = 14 UMETA(DisplayName = "Slow Chance"),
	CriticalChance = 15 UMETA(DisplayName = "Critical Hit Chance"),
	PenetratingChance = 16 UMETA(DisplayName = "Penetrating Hit Chance"),
	StrongAgainstHuman = 17 UMETA(DisplayName = "Strong Against Humans"),
	StrongAgainstAnimal = 18 UMETA(DisplayName = "Strong Against Animals"),
	StrongAgainstOrc = 19 UMETA(DisplayName = "Strong Against Orcs"),
	StrongAgainstMystic = 20 UMETA(DisplayName = "Strong Against Mystics"),
	StrongAgainstUndead = 21 UMETA(DisplayName = "Strong Against Undead"),
	StrongAgainstDevil = 22 UMETA(DisplayName = "Strong Against Devils"),
	StealHP = 23 UMETA(DisplayName = "HP Absorption"),
	StealSP = 24 UMETA(DisplayName = "SP Absorption"),
	ManaBurnChance = 25 UMETA(DisplayName = "Mana Burn Chance"),
	RecoverSPOnDamage = 26 UMETA(DisplayName = "Recover SP on Damage"),
	BlockChance = 27 UMETA(DisplayName = "Block Chance"),
	DodgeChance = 28 UMETA(DisplayName = "Dodge Chance"),
	SwordResistance = 29 UMETA(DisplayName = "Sword Resistance"),
	TwoHandResistance = 30 UMETA(DisplayName = "Two-Handed Sword Resistance"),
	DaggerResistance = 31 UMETA(DisplayName = "Dagger Resistance"),
	BellResistance = 32 UMETA(DisplayName = "Bell Resistance"),
	FanResistance = 33 UMETA(DisplayName = "Fan Resistance"),
	BowResistance = 34 UMETA(DisplayName = "Arrow Resistance"),
	FireResistance = 35 UMETA(DisplayName = "Fire Resistance"),
	LightningResistance = 36 UMETA(DisplayName = "Lightning Resistance"),
	MagicResistance = 37 UMETA(DisplayName = "Magic Resistance"),
	WindResistance = 38 UMETA(DisplayName = "Wind Resistance"),
	ReflectMelee = 39 UMETA(DisplayName = "Reflect Melee Damage"),
	ReflectCurse = 40 UMETA(DisplayName = "Reflect Curse"),
	PoisonResistance = 41 UMETA(DisplayName = "Poison Resistance"),
	RecoverSPOnKill = 42 UMETA(DisplayName = "Recover SP on Kill"),
	DoubleExperienceChance = 43 UMETA(DisplayName = "Double Experience Chance"),
	DoubleYangChance = 44 UMETA(DisplayName = "Double Yang Chance"),
	ItemDropBonus = 45 UMETA(DisplayName = "Item Drop Bonus"),
	PotionBonus = 46 UMETA(DisplayName = "Potion Effect Bonus"),
	RecoverHPOnKill = 47 UMETA(DisplayName = "Recover HP on Kill"),
	ImmuneStun = 48 UMETA(DisplayName = "Immune to Stun"),
	ImmuneSlow = 49 UMETA(DisplayName = "Immune to Slow"),
	ImmuneFall = 50 UMETA(DisplayName = "Immune to Knockdown"),
	Skill = 51 UMETA(DisplayName = "Skill"),
	BowDistance = 52 UMETA(DisplayName = "Bow Distance"),
	AttackValue = 53 UMETA(DisplayName = "Attack Value"),
	DefenseBonus = 54 UMETA(DisplayName = "Defense Bonus"),
	MagicAttackValue = 55 UMETA(DisplayName = "Magic Attack Value"),
	MagicDefenseValue = 56 UMETA(DisplayName = "Magic Defense Value"),
	CurseChance = 57 UMETA(DisplayName = "Curse Chance"),
	MaxStamina = 58 UMETA(DisplayName = "Maximum Stamina"),
	StrongAgainstWarrior = 59 UMETA(DisplayName = "Strong Against Warriors"),
	StrongAgainstAssassin = 60 UMETA(DisplayName = "Strong Against Assassins"),
	StrongAgainstSura = 61 UMETA(DisplayName = "Strong Against Sura"),
	StrongAgainstShaman = 62 UMETA(DisplayName = "Strong Against Shamans"),
	StrongAgainstMonster = 63 UMETA(DisplayName = "Strong Against Monsters"),
	MallAttackBonus = 64 UMETA(DisplayName = "Mall Attack Bonus"),
	MallDefenseBonus = 65 UMETA(DisplayName = "Mall Defense Bonus"),
	MallExperienceBonus = 66 UMETA(DisplayName = "Mall Experience Bonus"),
	MallItemDropBonus = 67 UMETA(DisplayName = "Mall Item Drop Bonus"),
	MallYangBonus = 68 UMETA(DisplayName = "Mall Yang Bonus"),
	MaxHPPercent = 69 UMETA(DisplayName = "Maximum HP (%)"),
	MaxSPPercent = 70 UMETA(DisplayName = "Maximum SP (%)"),
	SkillDamage = 71 UMETA(DisplayName = "Skill Damage"),
	AverageDamage = 72 UMETA(DisplayName = "Average Damage"),
	SkillDamageResistance = 73 UMETA(DisplayName = "Skill Damage Resistance"),
	AverageDamageResistance = 74 UMETA(DisplayName = "Average Damage Resistance"),
	PCBangExperienceBonus = 75 UMETA(DisplayName = "PC Bang Experience Bonus"),
	PCBangDropBonus = 76 UMETA(DisplayName = "PC Bang Drop Bonus"),
	ExtractHP = 77 UMETA(DisplayName = "Extract HP"),
	WarriorResistance = 78 UMETA(DisplayName = "Warrior Resistance"),
	AssassinResistance = 79 UMETA(DisplayName = "Assassin Resistance"),
	SuraResistance = 80 UMETA(DisplayName = "Sura Resistance"),
	ShamanResistance = 81 UMETA(DisplayName = "Shaman Resistance"),
	Energy = 82 UMETA(DisplayName = "Energy"),
	DefenseValue = 83 UMETA(DisplayName = "Defense Value"),
	CostumeAttributeBonus = 84 UMETA(DisplayName = "Costume Attribute Bonus"),
	MagicAttackPercent = 85 UMETA(DisplayName = "Magic Attack (%)"),
	MeleeMagicAttackPercent = 86 UMETA(DisplayName = "Melee/Magic Attack (%)"),
	IceResistance = 87 UMETA(DisplayName = "Ice Resistance"),
	EarthResistance = 88 UMETA(DisplayName = "Earth Resistance"),
	DarkResistance = 89 UMETA(DisplayName = "Dark Resistance"),
	AntiCriticalChance = 90 UMETA(DisplayName = "Critical Resistance"),
	AntiPenetratingChance = 91 UMETA(DisplayName = "Penetrating Resistance")
};

USTRUCT(BlueprintType)
struct METIN2_API FMT2ItemBonus
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Bonus")
	EMT2ItemBonusType Type = EMT2ItemBonusType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Bonus")
	int32 Value = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Bonus")
	EMT2ItemBonusKind Kind = EMT2ItemBonusKind::Normal;

	int32 GetTypeId() const { return static_cast<int32>(Type); }
	bool IsValid() const { return Type != EMT2ItemBonusType::None && Value != 0; }
};

USTRUCT(BlueprintType)
struct METIN2_API FMT2ItemApply
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apply")
	EMT2ItemBonusType Type = EMT2ItemBonusType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apply")
	int32 Value = 0;
};

