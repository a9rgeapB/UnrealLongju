#pragma once

#include "Items/Use/MT2ItemActionTemplate.h"
#include "Items/MT2ItemTemplate.h"
#include "MT2ItemUseCreateStoneTemplate.generated.h"

USTRUCT(BlueprintType)
struct METIN2_API FMT2ItemCreateStoneResult
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Create Stone")
	TSubclassOf<UMT2ItemTemplate> ItemTemplate;

	// Selects one concrete legacy variant from this template's authored VnumRange. This is an
	// offset within a typed item class reference, not an item-vnum reference.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Create Stone", meta = (ClampMin = "0"))
	int32 VnumRangeOffset = 0;
};

/**
 * Legacy USE_CREATE_STONE consumes itself and grants one uniformly selected stone item from
 * the configured result classes (the imported legacy range 28000..28013).
 */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemUseCreateStoneTemplate : public UMT2ItemActionTemplate
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Create Stone")
	TArray<FMT2ItemCreateStoneResult> StoneResults;

	virtual EMT2ItemUseExecution ExecuteUse(AMT2PlayerCharacter& Character,
		UMT2InventoryComponent& Inventory, int32 InventorySlot) const override;
};

