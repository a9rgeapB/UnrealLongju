/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once

#include "Items/State/MT2ItemFamilyState.h"
#include "MT2SkillForgetItemState.generated.h"

class UMT2SkillDefinition;

USTRUCT(BlueprintType)
struct METIN2_API FMT2SkillForgetItemState : public FMT2ItemFamilyState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Forget")
	TObjectPtr<UMT2SkillDefinition> Skill;

	// Retained only when an old save references a skill asset that is not currently imported.
	UPROPERTY()
	int32 UnresolvedLegacySkill = 0;
};
