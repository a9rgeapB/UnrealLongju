/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once

#include "Items/Templates/MT2ItemUseTemplate.h"
#include "MT2ItemMetinRemovalTemplate.generated.h"

UENUM(BlueprintType)
enum class EMT2MetinRemovalMode : uint8
{
	SingleStone,
	AllStones
};

UCLASS(Blueprintable)
class METIN2_API UMT2ItemMetinRemovalTemplate : public UMT2ItemUseTemplate
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Metin Removal")
	EMT2MetinRemovalMode RemovalMode = EMT2MetinRemovalMode::AllStones;
};
