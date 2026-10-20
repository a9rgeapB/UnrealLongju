#pragma once

#include "Items/Templates/MT2ItemBonusConsumableTemplate.h"
#include "MT2ItemUseAddAttributeTemplate.generated.h"

/**
 * Legacy USE_ADD_ATTRIBUTE item: rolls the configured chance and adds one normal equipment
 * attribute when the target still has room.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemUseAddAttributeTemplate : public UMT2ItemBonusConsumableTemplate
{
	GENERATED_BODY()

public:
	virtual EMT2ItemBonusApplyResult ApplyToEquipment(FMT2ItemSlot& StagedTarget,
		const UMT2ItemTemplate& TargetTemplate, const UMT2ItemBonusSettings& Settings) const override;
};

