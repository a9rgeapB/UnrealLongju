/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once

#include "Player/MT2PlayerTypes.h"
#include "Skills/MT2SkillTypes.h"
#include "Stats/MT2StatTypes.h"

struct FMT2PlayerDerivedStats
{
	FMT2CombatStats Combat;
	float MaxMana = 0.0f;
	float MaxStamina = 0.0f;
};

namespace MT2PlayerStatFormula
{
	METIN2_API FMT2PrimaryStats GetInitialPrimaryStats(EMT2CharacterRace Race);
	METIN2_API FMT2PlayerDerivedStats Calculate(
		EMT2CharacterRace Race, int32 Level, const FMT2PrimaryStats& PrimaryStats);
	// Legacy GetPolymorphPoint substitution, evaluated from the current player/mob points and
	// skill-derived power. The caller applies the result only while the authoritative poly affect is active.
	METIN2_API FMT2PrimaryStats CalculatePolymorphStats(EMT2CharacterRace Race,
		EMT2SkillGroup SkillGroup, const FMT2PrimaryStats& PlayerStats,
		const FMT2PrimaryStats& MobStats, int32 PowerPercent);
	METIN2_API float CalculateAttackRating(
		int32 AttackerLevel, int32 AttackerDexterity, int32 DefenderLevel, int32 DefenderDexterity);
}
