/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/Templates/MT2ItemUseTemplate.h"
#include "MT2ItemRefinementScrollTemplate.generated.h"

UCLASS(Blueprintable)
class METIN2_API UMT2ItemRefinementScrollTemplate : public UMT2ItemUseTemplate
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Refinement",
		meta = (ClampMin = "0", ClampMax = "100", Units = "%"))
	int32 AdditionalSuccessPercent = 10;
};
