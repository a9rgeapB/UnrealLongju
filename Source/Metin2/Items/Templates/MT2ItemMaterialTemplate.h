/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/MT2ItemTemplate.h"
#include "MT2ItemMaterialTemplate.generated.h"

UCLASS(Blueprintable)
class METIN2_API UMT2ItemMaterialTemplate : public UMT2ItemTemplate
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Material")
	EMT2MaterialSubType MaterialSubType = EMT2MaterialSubType::Leather;
};
