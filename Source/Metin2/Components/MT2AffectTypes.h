#pragma once
#include "CoreMinimal.h"
#include "Serialization/StructuredArchive.h"
#include "MT2AffectTypes.generated.h"
struct FPropertyTag;
class UPackageMap;

UENUM(BlueprintType)
enum class EMT2AffectIdentityKind : uint8 { Builtin=0, Skill=1, Custom=2 };

// Semantic choices map explicitly to legacy IDs; they are not the serialized IDs.
UENUM(BlueprintType)
enum class EMT2BuiltinAffect : uint8
{
	None=0, MoveSpeed=1, AttackSpeedLegacy=2, AttackGrade=3, Invisibility=4, Strength=5, Dexterity=6, Constitution=7, Intelligence=8, FishMindPill=9, Poison=10, Stun=11, Slow=12, DungeonReady=13, DungeonUnique=14, Building=15, ReviveInvisible=16, Fire=17, CastSpeed=18, HealthRecoveryContinue=19, ManaRecoveryContinue=20, Polymorph=21, Mount=22, WarFlag=23, BlockChat=24, ChinaFirework=25, BowDistance=26, DefenseGrade=27, ExperienceBonus=28, ItemBonus=29, Safebox=30, AutoLoot=31, FishMind=32, MarriageFast=33, GoldBonus=34, Mall=35, NoDeathPenalty=36, SkillBookGuaranteedSuccess=37, SkillBookNoCooldown=38, Hair=39, Collect=40, ExperienceBonusEuropeFree=41, ExperienceBonusEuropeUnder15=42, UniqueAbility=43, Cube1=44, Cube2=45, Cube3=46, Cube4=47, Cube5=48, Cube6=49, Cube7=50, Cube8=51, Cube9=52, Cube10=53, Cube11=54, Cube12=55, Blend=56, HorseName=57, MountBonus=58, AutoHealthRecovery=59, AutoManaRecovery=60, DragonSoulQualified=61, DragonSoulDeck0=62, DragonSoulDeck1=63, RamadanAbility=64, RamadanRing=65, NoGoodAbility=66, HolyStonePower=67, HealthRecovery=68, ManaRecovery=69, AttackSpeed=70, MovementSpeed=71
};

// The existing 32-bit affect mask can author these legacy EAffectBits positions.
UENUM(BlueprintType, meta=(Bitflags))
enum class EMT2EffectFlag : uint8
{
	None=0 UMETA(Hidden), Ymir=1, Invisibility=2, Spawn=3, Poison=4, Slow=5, Stun=6, DungeonReady=7, DungeonUnique=8, BuildingConstructionSmall=9, BuildingConstructionLarge=10, BuildingUpgrade=11, MoveSpeedPotion=12, AttackSpeedPotion=13, FishMind=14, Jeongwihon=15, Geomgyeong=16, Cheongeun=17, Gyeonggong=18, Eunhyung=19, Gwigum=20, Terror=21, Jumagap=22, Hosin=23, Boho=24, Kwaesok=25, ManaShield=26, Muyeong=27, ReviveInvisible=28, Fire=29, Gicheon=30, JeungRyeok=31
};

USTRUCT(BlueprintType, meta=(LegacyNumericSelector))
struct METIN2_API FMT2AffectIdentifier
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affect")
	EMT2AffectIdentityKind Kind=EMT2AffectIdentityKind::Builtin;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affect", meta=(EditCondition="Kind == EMT2AffectIdentityKind::Builtin", EditConditionHides))
	EMT2BuiltinAffect Builtin=EMT2BuiltinAffect::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affect", meta=(EditCondition="Kind == EMT2AffectIdentityKind::Skill", EditConditionHides, DisplayName="Skill VNUM"))
	int32 SkillVnum=0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affect", meta=(EditCondition="Kind == EMT2AffectIdentityKind::Custom", EditConditionHides, DisplayName="Custom Affect ID"))
	int32 CustomId=0;
	static FMT2AffectIdentifier FromLegacyId(int32 Raw);
	static FMT2AffectIdentifier FromBuiltin(EMT2BuiltinAffect Value);
	static FMT2AffectIdentifier FromSkillVnum(int32 Value);
	int32 ToLegacyId() const;
	bool operator==(const FMT2AffectIdentifier& Other) const { return ToLegacyId()==Other.ToLegacyId(); }
	bool SerializeFromMismatchedTag(const FPropertyTag& Tag,FStructuredArchive::FSlot Slot);
	bool ImportTextItem(const TCHAR*& Buffer,int32 PortFlags,UObject* Parent,FOutputDevice* ErrorText);
	bool NetSerialize(FArchive& Ar,UPackageMap* Map,bool& bOutSuccess);
};
template<> struct TStructOpsTypeTraits<FMT2AffectIdentifier> : TStructOpsTypeTraitsBase2<FMT2AffectIdentifier>
{
	enum { WithStructuredSerializeFromMismatchedTag=true,WithImportTextItem=true,WithIdenticalViaEquality=true,WithNetSerializer=true };
};
