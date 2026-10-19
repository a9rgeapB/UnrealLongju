/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/Templates/MT2ItemToolTemplate.h"
#include "Items/State/MT2FishingRodItemState.h"
#include "MT2ItemRodTemplate.generated.h"

UCLASS(Blueprintable)
class METIN2_API UMT2ItemRodTemplate : public UMT2ItemToolTemplate
{
	GENERATED_BODY()

public:
	virtual void InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const override;
	virtual void CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Initial State")
	FMT2FishingRodItemState DefaultRodState;

	int32 GetRefinementFailureVnum() const;
	virtual int32 GetLegacyQuestValue(int32 Index) const override
	{
		switch (Index)
		{
		case 0: return FishingDelayTenths;
		case 1: return PracticeChanceDenominator;
		case 2: return ImprovementPointsRequired;
		case 3: return RefinementSuccessPercent;
		case 4: return GetRefinementFailureVnum();
		default: return 0;
		}
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing", meta=(DisplayName="Fishing Power (Legacy Value0)"))
	int32 FishingDelayTenths = 0;
	// Legacy Value0 is fishing power, not delay; retain the historical property for saved assets.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing")
	int32 PracticeChanceDenominator = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing")
	TSoftClassPtr<UMT2ItemRodTemplate> RefinementFailureTemplate;
	UPROPERTY()
	int32 RefinementFailureVnum = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing")
	int32 ImprovementPointsRequired = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing")
	int32 RefinementSuccessPercent = 0;
};
