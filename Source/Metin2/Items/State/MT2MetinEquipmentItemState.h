#pragma once
#include "Items/State/MT2EquipmentItemState.h"
#include "Items/Types/MT2MetinSocketTypes.h"
#include "MT2MetinEquipmentItemState.generated.h"

USTRUCT(BlueprintType)
struct METIN2_API FMT2MetinEquipmentItemState : public FMT2EquipmentItemState
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Metin Sockets") TArray<FMT2MetinSocket> Sockets;
};
