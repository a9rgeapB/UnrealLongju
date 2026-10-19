/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/Templates/MT2ItemToolTemplate.h"
#include "Items/State/MT2PickaxeItemState.h"
#include "MT2ItemPickTemplate.generated.h"

UCLASS(Blueprintable)
class METIN2_API UMT2ItemPickTemplate : public UMT2ItemToolTemplate
{
	GENERATED_BODY()

public:
	virtual void InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const override;
	virtual void CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Initial State")
	FMT2PickaxeItemState DefaultPickaxeState;

	virtual int32 GetLegacyQuestValue(int32 Index) const override
	{
		return Index == 1 ? PracticeChanceDenominator : Index == 2 ? ImprovementPointsRequired
			: (Index == 3 ? RefinementSuccessPercent : 0);
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mining")
	int32 PracticeChanceDenominator = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mining")
	int32 ImprovementPointsRequired = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mining")
	int32 RefinementSuccessPercent = 0;
};
