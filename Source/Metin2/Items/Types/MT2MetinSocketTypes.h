/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "CoreMinimal.h"
#include "MT2MetinSocketTypes.generated.h"

class UMT2ItemMetinStoneTemplate;
class UMT2ItemTemplate;

UENUM(BlueprintType)
enum class EMT2MetinSocketType : uint8
{
	Silver,
	Gold
};

USTRUCT(BlueprintType)
struct METIN2_API FMT2MetinSocket
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Sockets")
	EMT2MetinSocketType Type = EMT2MetinSocketType::Silver;

	// Null means the socket is open.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Sockets")
	TSubclassOf<UMT2ItemMetinStoneTemplate> Stone;

	// Scripted per-item storage. The old quests reuse item sockets as scratch numbers on quest items
	// (timestamps, counters) via item.set_socket/get_socket, independently of any mounted stone.
	// A socket holding a stone reports that stone's vnum instead of this value.
	UPROPERTY()
	int32 Value = 0;
};

UENUM(BlueprintType)
enum class EMT2MetinSocketApplyResult : uint8
{
	Invalid,
	Success,
	Failed,
	NoSocket,
	Incompatible,
	DuplicateStone
};

