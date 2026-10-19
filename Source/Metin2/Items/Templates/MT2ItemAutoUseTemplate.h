/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/Templates/MT2ItemConsumableTemplate.h"
#include "MT2ItemAutoUseTemplate.generated.h"

UCLASS(Blueprintable)
class METIN2_API UMT2ItemAutoUseTemplate : public UMT2ItemConsumableTemplate
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Use")
	EMT2AutoUseSubType AutoUseSubType = EMT2AutoUseSubType::Potion;
};
