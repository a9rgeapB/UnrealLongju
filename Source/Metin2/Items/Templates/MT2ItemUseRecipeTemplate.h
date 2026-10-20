#pragma once

#include "Items/Use/MT2ItemActionTemplate.h"
#include "MT2ItemUseRecipeTemplate.generated.h"

class UMT2ItemTemplate;

USTRUCT(BlueprintType)
struct METIN2_API FMT2ItemRecipeDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	TSubclassOf<UMT2ItemTemplate> FirstIngredientTemplate;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe", meta = (ClampMin = "0"))
	int32 FirstIngredientCount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	TSubclassOf<UMT2ItemTemplate> SecondIngredientTemplate;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe", meta = (ClampMin = "0"))
	int32 SecondIngredientCount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	TSubclassOf<UMT2ItemTemplate> RequiredBottleTemplate;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	TSubclassOf<UMT2ItemTemplate> ResultItemTemplate;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe", meta = (ClampMin = "0", ClampMax = "100"))
	int32 SuccessPercent = 0;
};

/** Legacy USE_RECIPE: consumes ingredients and one bottle on either success or failed roll. */
UCLASS(Blueprintable)
class METIN2_API UMT2ItemUseRecipeTemplate : public UMT2ItemActionTemplate
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recipe")
	FMT2ItemRecipeDefinition Recipe;

	virtual EMT2ItemUseExecution ExecuteUse(AMT2PlayerCharacter& Character,
		UMT2InventoryComponent& Inventory, int32 InventorySlot) const override;
};

