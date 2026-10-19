/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/Templates/MT2ItemConsumableTemplate.h"
#include "MT2ItemSkillBookTemplate.generated.h"

class UMT2SkillDefinition;

UCLASS(Blueprintable)
class METIN2_API UMT2ItemSkillBookTemplate : public UMT2ItemConsumableTemplate
{
	GENERATED_BODY()

public:
	virtual void InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const override;
	virtual void CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skill Book|Initial State")
	FMT2SkillBookItemState DefaultSkillBookState;
	int32 GetSkillVnum() const;
	virtual void InitializeGeneratedInstance(FMT2ItemInstanceData& InOutData) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Book")
	TObjectPtr<UMT2SkillDefinition> Skill;
	UPROPERTY()
	int32 SkillVnum = 0;
};
