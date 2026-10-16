#pragma once

#include "CoreMinimal.h"
#include "Serialization/StructuredArchive.h"
#include "MT2ItemSemanticTypes.generated.h"

struct FPropertyTag;
enum class EMT2ItemBonusType : uint8;

UENUM(BlueprintType)
enum class EMT2WeaponSubType : uint8
{
	Sword=0, Dagger=1, Bow=2, TwoHanded=3, Bell=4, Fan=5, Arrow=6, MountSpear=7, None=9, Unsupported=254 UMETA(Hidden)
};
UENUM(BlueprintType)
enum class EMT2ArmorSubType : uint8
{
	Body=0, Head=1, Shield=2, Wrist=3, Foots=4, Neck=5, Ear=6, Unsupported=254 UMETA(Hidden)
};
UENUM(BlueprintType)
enum class EMT2UseSubType : uint8
{
	Potion=0, Talisman=1, Tuning=2, Move=3, TreasureBox=4, Moneybag=5, Bait=6, AbilityUp=7, Affect=8,
	CreateStone=9, Special=10, PotionNoDelay=11, Clear=12, Invisibility=13, Detachment=14, Bucket=15,
	PotionContinue=16, CleanSocket=17, ChangeAttribute=18, AddAttribute=19, AddAccessorySocket=20,
	PutIntoAccessorySocket=21, AddAttribute2=22, Recipe=23, ChangeAttribute2=24, Bind=25, Unbind=26,
	TimeChargePercent=27, TimeChargeFixed=28, PutIntoBeltSocket=29, PutIntoRingSocket=30, Unsupported=254 UMETA(Hidden)
};
UENUM(BlueprintType)
enum class EMT2AutoUseSubType : uint8
{
	Potion=0, AbilityUp=1, Bomb=2, Gold=3, Moneybag=4, TreasureBox=5, Unsupported=254 UMETA(Hidden)
};
UENUM(BlueprintType)
enum class EMT2MaterialSubType : uint8
{
	Leather=0, Blood=1, Root=2, Needle=3, Jewel=4, DragonSoulRefineNormal=5, DragonSoulRefineBlessed=6, DragonSoulRefineHoly=7, Unsupported=254 UMETA(Hidden)
};
UENUM(BlueprintType)
enum class EMT2MetinSubType : uint8
{
	Normal=0, Gold=1, Unsupported=254 UMETA(Hidden)
};
// Required item_proto socket grades are 1/2, unlike serialized slot kinds (Silver=0, Gold=1).
UENUM(BlueprintType)
enum class EMT2RequiredMetinSocketGrade : uint8
{
	Invalid=0 UMETA(Hidden), Silver=1, Gold=2
};
UENUM(BlueprintType)
enum class EMT2CostumeSubType : uint8
{
	Body=0, Hair=1, Unsupported=254 UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMT2ItemAddonType : uint8
{
	None = 0,
	AverageAndSkillDamage = 1 UMETA(DisplayName="Average and Skill Damage"),
	Unsupported = 2 UMETA(Hidden)
};

namespace MT2ItemSemanticTypes
{
	METIN2_API bool TryDecodeBonus(int32 Raw,EMT2ItemBonusType& Out);
	METIN2_API bool TryDecodeAddon(int32 Raw,EMT2ItemAddonType& Out);
	METIN2_API int32 EncodeAddon(EMT2ItemAddonType Type);
}

// A byte enum alone cannot preserve the signed legacy sentinel or unknown future addon IDs.
USTRUCT(BlueprintType, meta=(LegacyNumericSelector))
struct METIN2_API FMT2ItemAddonSelection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Addon")
	EMT2ItemAddonType Type=EMT2ItemAddonType::None;

	UPROPERTY()
	int32 LegacyValue=0;

	static FMT2ItemAddonSelection FromLegacy(int32 Raw);
	int32 ToLegacy() const;
	bool SerializeFromMismatchedTag(const FPropertyTag& Tag,FStructuredArchive::FSlot Slot);
	bool ImportTextItem(const TCHAR*& Buffer,int32 PortFlags,UObject* Parent,FOutputDevice* ErrorText);
	bool operator==(const FMT2ItemAddonSelection& Other) const { return ToLegacy()==Other.ToLegacy(); }
};

template<> struct TStructOpsTypeTraits<FMT2ItemAddonSelection> : TStructOpsTypeTraitsBase2<FMT2ItemAddonSelection>
{
	enum { WithStructuredSerializeFromMismatchedTag=true,WithImportTextItem=true,WithIdenticalViaEquality=true };
};


UENUM(BlueprintType)
enum class EMT2ItemType : uint8
{
	None=0, Weapon=1, Armor=2, Use=3, AutoUse=4, Material=5, Special=6, Tool=7, Lottery=8, Yang=9, Metin=10, Container=11, Fish=12, Rod=13, Resource=14, Campfire=15, Unique=16, SkillBook=17, Quest=18, Polymorph=19, TreasureBox=20, TreasureKey=21, SkillForget=22, GiftBox=23, Pick=24, Hair=25, Totem=26, Blend=27, Costume=28, DragonSoul=29, SpecialDragonSoul=30, Extract=31, SecondaryCoin=32, Ring=33, Belt=34
};

UENUM(BlueprintType)
enum class EMT2UniqueSubType : uint8
{
	None=0, Book=1, SpecialRide=2, SpecialMountRide=3, Unsupported=254 UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMT2SpecialSubType : uint8
{
	Map=0, Key=1, Document=2, Spirit=3, Unsupported=254 UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMT2ToolSubType : uint8
{
	FishingRod=0, Unsupported=254 UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMT2LotterySubType : uint8
{
	Ticket=0, Instant=1, Unsupported=254 UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMT2FishSubType : uint8
{
	Alive=0, Dead=1, Unsupported=254 UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMT2ResourceSubType : uint8
{
	Fishbone=0, WaterstonePiece=1, Waterstone=2, BloodPearl=3, BluePearl=4, WhitePearl=5, Bucket=6, Crystal=7, Gem=8, Stone=9, Metin=10, Ore=11, Unsupported=254 UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMT2ExtractSubType : uint8
{
	DragonSoul=0, DragonHeart=1, Unsupported=254 UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMT2DragonSoulSubType : uint8
{
	Slot1=0, Slot2=1, Slot3=2, Slot4=3, Slot5=4, Slot6=5, Unsupported=254 UMETA(Hidden)
};

USTRUCT(BlueprintType, meta=(LegacyNumericSelector))
struct METIN2_API FMT2ItemUniqueSubtypeSelection
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subtype")
	EMT2UniqueSubType Type=EMT2UniqueSubType::None;
	UPROPERTY()
	int32 LegacyValue=0;
	static FMT2ItemUniqueSubtypeSelection FromLegacy(int32 Raw);
	int32 ToLegacy() const;
	bool SerializeFromMismatchedTag(const FPropertyTag& Tag,FStructuredArchive::FSlot Slot);
	bool ImportTextItem(const TCHAR*& Buffer,int32 PortFlags,UObject* Parent,FOutputDevice* ErrorText);
};
template<> struct TStructOpsTypeTraits<FMT2ItemUniqueSubtypeSelection> : TStructOpsTypeTraitsBase2<FMT2ItemUniqueSubtypeSelection>
{
	enum { WithStructuredSerializeFromMismatchedTag=true, WithImportTextItem=true };
};

// A subtype ordinal has meaning only within its item family.
USTRUCT(BlueprintType, meta=(LegacyNumericSelector))
struct METIN2_API FMT2ItemSubtype
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subtype")
	EMT2ItemType Family=EMT2ItemType::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subtype", meta=(EditCondition="Family == EMT2ItemType::Weapon", EditConditionHides))
	EMT2WeaponSubType Weapon=EMT2WeaponSubType::Sword;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subtype", meta=(EditCondition="Family == EMT2ItemType::Armor", EditConditionHides))
	EMT2ArmorSubType Armor=EMT2ArmorSubType::Body;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subtype", meta=(EditCondition="Family == EMT2ItemType::Use", EditConditionHides))
	EMT2UseSubType Use=EMT2UseSubType::Potion;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subtype", meta=(EditCondition="Family == EMT2ItemType::AutoUse", EditConditionHides))
	EMT2AutoUseSubType AutoUse=EMT2AutoUseSubType::Potion;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subtype", meta=(EditCondition="Family == EMT2ItemType::Material", EditConditionHides))
	EMT2MaterialSubType Material=EMT2MaterialSubType::Leather;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subtype", meta=(EditCondition="Family == EMT2ItemType::Metin", EditConditionHides))
	EMT2MetinSubType Metin=EMT2MetinSubType::Normal;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subtype", meta=(EditCondition="Family == EMT2ItemType::Costume", EditConditionHides))
	EMT2CostumeSubType Costume=EMT2CostumeSubType::Body;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subtype", meta=(EditCondition="Family == EMT2ItemType::Unique", EditConditionHides))
	EMT2UniqueSubType Unique=EMT2UniqueSubType::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subtype", meta=(EditCondition="Family == EMT2ItemType::Special", EditConditionHides))
	EMT2SpecialSubType Special=EMT2SpecialSubType::Map;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subtype", meta=(EditCondition="Family == EMT2ItemType::Tool", EditConditionHides))
	EMT2ToolSubType Tool=EMT2ToolSubType::FishingRod;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subtype", meta=(EditCondition="Family == EMT2ItemType::Lottery", EditConditionHides))
	EMT2LotterySubType Lottery=EMT2LotterySubType::Ticket;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subtype", meta=(EditCondition="Family == EMT2ItemType::Fish", EditConditionHides))
	EMT2FishSubType Fish=EMT2FishSubType::Alive;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subtype", meta=(EditCondition="Family == EMT2ItemType::Resource", EditConditionHides))
	EMT2ResourceSubType Resource=EMT2ResourceSubType::Fishbone;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subtype", meta=(EditCondition="Family == EMT2ItemType::Extract", EditConditionHides))
	EMT2ExtractSubType Extract=EMT2ExtractSubType::DragonSoul;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subtype", meta=(EditCondition="Family == EMT2ItemType::DragonSoul || Family == EMT2ItemType::SpecialDragonSoul", EditConditionHides))
	EMT2DragonSoulSubType DragonSoul=EMT2DragonSoulSubType::Slot1;

	// Unclassified imported IDs are retained, never reinterpreted as a known choice.
	UPROPERTY()
	int32 LegacyValue=0;
	UPROPERTY()
	bool bNeedsLegacyFamily=false;
	static FMT2ItemSubtype FromLegacy(EMT2ItemType InFamily,int32 Raw);
	int32 ToLegacy() const;
	bool IsForFamily(EMT2ItemType InFamily) const { return !bNeedsLegacyFamily && Family==InFamily; }
	void ResolveLegacyFamily(EMT2ItemType InFamily);
	bool SerializeFromMismatchedTag(const FPropertyTag& Tag,FStructuredArchive::FSlot Slot);
	bool ImportTextItem(const TCHAR*& Buffer,int32 PortFlags,UObject* Parent,FOutputDevice* ErrorText);
};
template<> struct TStructOpsTypeTraits<FMT2ItemSubtype> : TStructOpsTypeTraitsBase2<FMT2ItemSubtype>
{
	enum { WithStructuredSerializeFromMismatchedTag=true, WithImportTextItem=true };
};
