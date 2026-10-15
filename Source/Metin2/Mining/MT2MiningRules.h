#pragma once
#include "CoreMinimal.h"

namespace MT2Mining
{
	inline int32 AlchemistOre(int32 Vnum)
	{
		if (Vnum >= 20060 && Vnum <= 20072) { return 50601 + Vnum - 20060; }
		if (Vnum >= 33009 && Vnum <= 33013) { return 50614 + Vnum - 33009; }
		return 0;
	}
	inline int32 RefinedOre(int32 Vnum) { return Vnum >= 50601 && Vnum <= 50618 ? Vnum + 20 : 0; }
	inline int32 RefiningCost(int32 Vnum) { return Vnum == 50601 ? 10000 : 3000; }
	inline int32 RefiningChance(int32 Vnum, int32 Npc) { return AlchemistOre(Npc) == Vnum ? 60 : 30; }
	inline bool IsRefiningStone(int32 Vnum) { return Vnum >= 28000 && Vnum < 28300; }
	inline int32 RawOre(int32 VeinVnum)
	{
		if (VeinVnum >= 20047 && VeinVnum <= 20059) { return 50601 + VeinVnum - 20047; }
		if (VeinVnum >= 30301 && VeinVnum <= 30305) { return 50614 + VeinVnum - 30301; }
		return 0;
	}
	inline int32 SuccessChance(int32 SkillLevel, int32 PickGrade)
	{
		static constexpr int32 Grades[] = {3,5,8,11,15,20,26,32,40,50};
		const int32 Level = FMath::Clamp(SkillLevel, 0, 40);
		const int32 Skill = Level == 40 ? 11 : (Level + 3) / 4;
		return 20 + Skill + Grades[FMath::Clamp(PickGrade, 0, 9)];
	}
	// Separate weighted bucket and uniform offset: exactly one quantity draw per success.
	inline int32 Quantity(int32 Percentile, int32 Offset)
	{
		static constexpr int32 Weights[] = {20,30,20,15,5,4,3,2,1};
		if (Percentile < 1 || Percentile > 100 || Offset < 0 || Offset > 9) { return 0; }
		for (int32 I=0; I<UE_ARRAY_COUNT(Weights); ++I)
		{
			if (Percentile <= Weights[I]) { return 1 + I*10 + Offset; }
			Percentile -= Weights[I];
		}
		return 0;
	}
}
