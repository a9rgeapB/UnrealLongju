#pragma once
#include "CoreMinimal.h"
#include "Items/MT2ItemTypes.h"
class UMT2ItemTemplate;
namespace MT2AccessorySockets
{
	METIN2_API bool IsAccessory(const UMT2ItemTemplate* Item);
	METIN2_API bool IsMaterial(const UMT2ItemTemplate* Item);
	METIN2_API bool Compatible(const UMT2ItemTemplate* Material, const UMT2ItemTemplate* Target);
	METIN2_API int32 RequiredMineral(int32 Vnum, int32 ArmorSubType);
	METIN2_API int32 Bonus(int32 BaseValue, int32 Grade);
	METIN2_API int32 Duration(int32 Grade);
	METIN2_API int32 InsertionChance(int32 Grade);
	METIN2_API FMT2AccessorySocketState Normalize(FMT2AccessorySocketState State);
	METIN2_API FMT2AccessorySocketState Advance(FMT2AccessorySocketState State, double Elapsed);
}
