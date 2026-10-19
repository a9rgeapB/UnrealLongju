#include "Items/MT2ItemTemplate.h"

#include "Characters/MT2PlayerCharacter.h"
#include "Components/MT2HealthComponent.h"
#include "Components/MT2StatusEffectComponent.h"
#include "Items/MT2InventoryComponent.h"
#include "Items/MT2ItemUtils.h"

int32 FMT2BlendDefinition::GetTotalWeight() const
{
	int64 Total = 0;
	for (int32 Weight : Weights)
	{
		if (Weight < 0) return 0;
		Total += Weight;
	}
	return Total > 0 && Total <= MAX_int32 ? static_cast<int32>(Total) : 0;
}

bool FMT2BlendDefinition::IsValid() const
{
	return BonusType != EMT2ItemBonusType::None && StaticEnum<EMT2ItemBonusType>()->IsValidEnumValue(static_cast<int64>(BonusType)) &&
		Magnitudes.Num() == 5 && Durations.Num() == 5 && Weights.Num() == 5 && GetTotalWeight() > 0 &&
		!Magnitudes.ContainsByPredicate([](int32 Value) { return Value <= 0; }) &&
		!Durations.ContainsByPredicate([](int32 Value) { return Value <= 0; });
}

int32 FMT2BlendDefinition::GetRollIndex(int32 Roll) const
{
	if (Roll < 1 || Roll > GetTotalWeight()) return INDEX_NONE;
	for (int32 Index = 0; Index < Weights.Num(); ++Index)
	{
		Roll -= Weights[Index];
		if (Roll <= 0) return Index;
	}
	return INDEX_NONE;
}




