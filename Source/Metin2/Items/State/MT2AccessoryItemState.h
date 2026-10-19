#pragma once
#include "Items/State/MT2EquipmentItemState.h"
#include "Items/Types/MT2AccessorySocketState.h"
#include "MT2AccessoryItemState.generated.h"

USTRUCT(BlueprintType)
struct METIN2_API FMT2AccessoryItemState : public FMT2EquipmentItemState
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Accessory Sockets") FMT2AccessorySocketState Sockets;
};
