/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/Templates/MT2ItemContainerTemplate.h"
#include "MT2ItemLootCrateTemplate.generated.h"

UCLASS(Blueprintable)
class METIN2_API UMT2ItemLootCrateTemplate : public UMT2ItemContainerTemplate
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot Crate")
	TSubclassOf<UMT2LootTable> LootTable;

	// Empty means right-click opens the crate. Set means that exact item template must be moved onto
	// the crate; one key and one crate are consumed atomically.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot Crate")
	TSubclassOf<UMT2ItemTemplate> RequiredKeyTemplate;
};
