#pragma once
#include "CoreMinimal.h"
#include "Serialization/StructuredArchive.h"
#include "MT2SkillSemanticTypes.generated.h"
struct FPropertyTag;

UENUM(BlueprintType)
enum class EMT2LegacySkillClass : uint8
{
	Unspecified=0, Support=1, Warrior=2, Assassin=3, Sura=4, Shaman=5, Horse=6, AntiSkill=7, Unsupported=254 UMETA(Hidden)
};
USTRUCT(BlueprintType, meta=(LegacyNumericSelector))
struct METIN2_API FMT2LegacySkillClassSelection
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill")
	EMT2LegacySkillClass Type=EMT2LegacySkillClass::Unspecified;
	UPROPERTY()
	int32 LegacyValue=INDEX_NONE;
	static FMT2LegacySkillClassSelection FromLegacy(int32 Raw);
	int32 ToLegacy() const;
	bool SerializeFromMismatchedTag(const FPropertyTag& Tag,FStructuredArchive::FSlot Slot);
	bool ImportTextItem(const TCHAR*& Buffer,int32 PortFlags,UObject* Parent,FOutputDevice* ErrorText);
};
template<> struct TStructOpsTypeTraits<FMT2LegacySkillClassSelection> : TStructOpsTypeTraitsBase2<FMT2LegacySkillClassSelection>
{
	enum { WithStructuredSerializeFromMismatchedTag=true,WithImportTextItem=true };
};
