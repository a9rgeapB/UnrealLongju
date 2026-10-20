/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Config/MT2GameplaySettings.h"

#include "Items/Templates/MT2ItemPolymorphTemplate.h"
#include "Mobs/MT2Mob.h"
#include "Skills/MT2SkillDefinition.h"

const UMT2SkillDefinition* UMT2GameplaySettings::GetPolymorphSkillDefinition() const
{
	if (!CachedPolymorphSkillDefinition && !PolymorphSkillDefinition.IsNull())
	{
		CachedPolymorphSkillDefinition = PolymorphSkillDefinition.LoadSynchronous();
	}
	return CachedPolymorphSkillDefinition;
}

UClass* UMT2GameplaySettings::GetPolymorphBookItemClass() const
{
	return PolymorphBookItemTemplate.LoadSynchronous();
}

bool UMT2GameplaySettings::IsPolymorphBookMovementBonusMob(const UClass* MobClass) const
{
	if (!MobClass)
	{
		return false;
	}

	for (const TSoftClassPtr<AMT2Mob>& ConfiguredMobClass : PolymorphBookMovementBonusMobClasses)
	{
		const UClass* LoadedClass = ConfiguredMobClass.LoadSynchronous();
		if (LoadedClass && MobClass->IsChildOf(LoadedClass))
		{
			return true;
		}
	}
	return false;
}
