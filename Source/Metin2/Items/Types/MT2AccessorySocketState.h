/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "CoreMinimal.h"
#include "MT2AccessorySocketState.generated.h"

USTRUCT(BlueprintType)
struct METIN2_API FMT2AccessorySocketState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Accessory Sockets")
	int32 CurrentGrade = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Accessory Sockets")
	int32 MaximumGrade = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Accessory Sockets")
	int32 RemainingSeconds = 0;
	// Carry fractional equipped wear through moves without a timer or replication update per second.
	UPROPERTY(Transient, NotReplicated)
	double WearFraction = 0;

	bool operator==(const FMT2AccessorySocketState& Other) const = default;
};

UENUM(BlueprintType)
enum class EMT2AccessorySocketApplyResult : uint8
{
	Invalid, Incompatible, Full, Success, Failed
};

