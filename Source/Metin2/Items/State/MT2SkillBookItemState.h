#pragma once
#include "Items/State/MT2ItemFamilyState.h"
#include "MT2SkillBookItemState.generated.h"

class UMT2SkillDefinition;

USTRUCT(BlueprintType)
struct METIN2_API FMT2SkillBookItemState : public FMT2ItemFamilyState
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill Book") TObjectPtr<UMT2SkillDefinition> Skill;
	// Preserves missing imported definitions at the persistence boundary, never an authoring input.
	UPROPERTY() int32 UnresolvedLegacySkill = 0;
};
