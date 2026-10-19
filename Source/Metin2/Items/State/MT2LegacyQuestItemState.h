#pragma once
#include "Items/State/MT2EquipmentItemState.h"
#include "MT2LegacyQuestItemState.generated.h"

// Private compatibility storage for original Lua item.get_socket/set_socket. Known families are
// decoded to named state before gameplay uses them; these values are not an authoring schema.
USTRUCT()
struct METIN2_API FMT2LegacyQuestItemState : public FMT2EquipmentItemState
{
	GENERATED_BODY()
	int32 GetValue(int32 Index) const { return Values.IsValidIndex(Index) ? Values[Index] : 0; }
	bool SetValue(int32 Index, int32 Value)
	{
		if (Index < 0 || Index >= 3) return false;
		if (Values.Num() <= Index) Values.SetNum(Index + 1);
		Values[Index] = Value;
		while (!Values.IsEmpty() && Values.Last() == 0) Values.Pop();
		return true;
	}
private:
	UPROPERTY() TArray<int32> Values;
};
