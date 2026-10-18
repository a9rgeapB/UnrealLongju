#pragma once

#include "CoreMinimal.h"
#include "Items/Use/MT2ItemActionTemplate.h"
#include "MT2HorseCareItemTemplate.generated.h"

UENUM(BlueprintType)
enum class EMT2HorseCareAction : uint8
{
	Feed,
	Revive
};

UCLASS(Blueprintable)
class METIN2_API UMT2HorseCareItemTemplate : public UMT2ItemActionTemplate
{
	GENERATED_BODY()

public:
	virtual EMT2ItemUseExecution ExecuteUse(AMT2PlayerCharacter& Character,
		UMT2InventoryComponent& Inventory, int32 InventorySlot) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Horse")
	EMT2HorseCareAction Action = EMT2HorseCareAction::Feed;
};
