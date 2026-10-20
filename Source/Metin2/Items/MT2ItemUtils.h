/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once

#include "CoreMinimal.h"

class UMT2ItemTemplate;
struct FMT2ItemSlot;
struct FMT2ItemInstanceData;
enum class EMT2ItemBonusType : uint8;
enum class EMT2ItemWearFlag : uint8;

namespace MT2ItemUtils
{
	METIN2_API const UMT2ItemTemplate* ResolveTemplate(const UObject* WorldContextObject, int32 Vnum);
	METIN2_API int32 GetInventorySize(const UObject* WorldContextObject, int32 Vnum);
	METIN2_API FText GetDisplayName(const UMT2ItemTemplate* Template, int32 FallbackVnum = 0);
	METIN2_API bool HaveSameInstanceData(const FMT2ItemSlot& Left, const FMT2ItemSlot& Right);
	METIN2_API bool IsConsumableHairLook(const UMT2ItemTemplate* Template);
	METIN2_API bool IsRightClickUseItem(const UMT2ItemTemplate* Template);
	METIN2_API bool UsesNumericSockets(const UMT2ItemTemplate* Template);
	METIN2_API int32 GetNumericSocket(const FMT2ItemInstanceData& Item, int32 Index);
	METIN2_API bool SetNumericSocket(FMT2ItemInstanceData& Item, int32 Index, int32 Value);
	METIN2_API void NormalizeNumericSockets(FMT2ItemInstanceData& Item, const UMT2ItemTemplate* Template);
	METIN2_API TMap<EMT2ItemBonusType, int32> GetAmplifiedEquipmentAttributes(
		const TArray<FMT2ItemSlot>& Equipment, const TArray<EMT2ItemWearFlag>& WearSlots, int32 Percent);

	template <typename T>
	const T* ResolveTemplate(const UObject* WorldContextObject, int32 Vnum)
	{
		return Cast<T>(ResolveTemplate(WorldContextObject, Vnum));
	}
}
