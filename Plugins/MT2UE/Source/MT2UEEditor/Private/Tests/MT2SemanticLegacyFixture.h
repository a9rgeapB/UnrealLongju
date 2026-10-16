#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MT2SemanticLegacyFixture.generated.h"

// Editor test utility: preserves the pre-refactor class/tag shape without gameplay-only helpers.
UCLASS()
class UMT2SemanticLegacyItemFixture : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY() int32 WeaponSubType=0;
	UPROPERTY() int32 ArmorSubType=0;
	UPROPERTY() int32 UseSubType=0;
	UPROPERTY() int32 AutoUseSubType=0;
	UPROPERTY() int32 MaterialSubType=0;
	UPROPERTY() int32 MetinSubType=0;
	UPROPERTY() int32 CostumeSubType=0;
	UPROPERTY() int32 RequiredSocketGrade=0;
	UPROPERTY() int32 UniqueSubType=0;
	UPROPERTY() int32 LegacySkillType=INDEX_NONE;
};
