/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/Templates/MT2ItemEquipmentTemplate.h"
#include "MT2ItemBeltTemplate.generated.h"

UCLASS(Blueprintable)
class METIN2_API UMT2ItemBeltTemplate : public UMT2ItemEquipmentTemplate
{
	GENERATED_BODY()
public:
	virtual TSubclassOf<UMT2ItemTooltipWidget> GetDefaultTooltipClass() const override;
};
