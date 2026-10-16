#pragma once

#include "CoreMinimal.h"
#include "Player/MT2PlayerTypes.h"
#include "Serialization/StructuredArchive.h"
#include "MT2MobEmpireSelection.generated.h"

struct FPropertyTag;

// Import selection includes opaque IDs; authoritative runtime identities remain EMT2Empire.
UENUM(BlueprintType)
enum class EMT2ImportedMobEmpire : uint8
{
	None=0, Shinsoo=1, Chunjo=2, Jinno=3, Unsupported=254 UMETA(Hidden)
};

USTRUCT(BlueprintType, meta=(LegacyNumericSelector))
struct METIN2_API FMT2MobEmpireSelection
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
	EMT2ImportedMobEmpire Type=EMT2ImportedMobEmpire::None;
	UPROPERTY()
	int32 LegacyValue=0;
	static FMT2MobEmpireSelection FromLegacy(int32 Raw);
	int32 ToLegacy() const;
	bool TryGetEmpire(EMT2Empire& Out) const;
	bool SerializeFromMismatchedTag(const FPropertyTag& Tag,FStructuredArchive::FSlot Slot);
	bool ImportTextItem(const TCHAR*& Buffer,int32 PortFlags,UObject* Parent,FOutputDevice* ErrorText);
};

template<> struct TStructOpsTypeTraits<FMT2MobEmpireSelection> : TStructOpsTypeTraitsBase2<FMT2MobEmpireSelection>
{
	enum { WithStructuredSerializeFromMismatchedTag=true, WithImportTextItem=true };
};
