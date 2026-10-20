#pragma once

#include "Items/Templates/MT2ItemBonusConsumableTemplate.h"
#include "MT2ItemUseAddAttribute2Template.generated.h"

/**
 * Legacy USE_ADD_ATTRIBUTE2 item: adds the fifth normal attribute after the standard four have
 * already been filled, up to the configured normal-attribute maximum.
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemUseAddAttribute2Template : public UMT2ItemBonusConsumableTemplate
{
	GENERATED_BODY()

public:
	virtual EMT2ItemBonusApplyResult ApplyToEquipment(FMT2ItemSlot& StagedTarget,
		const UMT2ItemTemplate& TargetTemplate, const UMT2ItemBonusSettings& Settings) const override;
};

