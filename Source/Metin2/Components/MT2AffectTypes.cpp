#include "Components/MT2AffectTypes.h"
#include "Serialization/MT2SemanticSerialization.h"
#include "UObject/PropertyTag.h"
namespace
{
	struct FAffectMapping { EMT2BuiltinAffect Named; int32 Id; };
	constexpr FAffectMapping Builtins[]={
		{EMT2BuiltinAffect::None,0},
		{EMT2BuiltinAffect::MoveSpeed,200},
		{EMT2BuiltinAffect::AttackSpeedLegacy,201},
		{EMT2BuiltinAffect::AttackGrade,202},
		{EMT2BuiltinAffect::Invisibility,203},
		{EMT2BuiltinAffect::Strength,204},
		{EMT2BuiltinAffect::Dexterity,205},
		{EMT2BuiltinAffect::Constitution,206},
		{EMT2BuiltinAffect::Intelligence,207},
		{EMT2BuiltinAffect::FishMindPill,208},
		{EMT2BuiltinAffect::Poison,209},
		{EMT2BuiltinAffect::Stun,210},
		{EMT2BuiltinAffect::Slow,211},
		{EMT2BuiltinAffect::DungeonReady,212},
		{EMT2BuiltinAffect::DungeonUnique,213},
		{EMT2BuiltinAffect::Building,214},
		{EMT2BuiltinAffect::ReviveInvisible,215},
		{EMT2BuiltinAffect::Fire,216},
		{EMT2BuiltinAffect::CastSpeed,217},
		{EMT2BuiltinAffect::HealthRecoveryContinue,218},
		{EMT2BuiltinAffect::ManaRecoveryContinue,219},
		{EMT2BuiltinAffect::Polymorph,220},
		{EMT2BuiltinAffect::Mount,221},
		{EMT2BuiltinAffect::WarFlag,222},
		{EMT2BuiltinAffect::BlockChat,223},
		{EMT2BuiltinAffect::ChinaFirework,224},
		{EMT2BuiltinAffect::BowDistance,225},
		{EMT2BuiltinAffect::DefenseGrade,226},
		{EMT2BuiltinAffect::ExperienceBonus,500},
		{EMT2BuiltinAffect::ItemBonus,501},
		{EMT2BuiltinAffect::Safebox,502},
		{EMT2BuiltinAffect::AutoLoot,503},
		{EMT2BuiltinAffect::FishMind,504},
		{EMT2BuiltinAffect::MarriageFast,505},
		{EMT2BuiltinAffect::GoldBonus,506},
		{EMT2BuiltinAffect::Mall,510},
		{EMT2BuiltinAffect::NoDeathPenalty,511},
		{EMT2BuiltinAffect::SkillBookGuaranteedSuccess,512},
		{EMT2BuiltinAffect::SkillBookNoCooldown,513},
		{EMT2BuiltinAffect::Hair,514},
		{EMT2BuiltinAffect::Collect,515},
		{EMT2BuiltinAffect::ExperienceBonusEuropeFree,516},
		{EMT2BuiltinAffect::ExperienceBonusEuropeUnder15,517},
		{EMT2BuiltinAffect::UniqueAbility,518},
		{EMT2BuiltinAffect::Cube1,519},
		{EMT2BuiltinAffect::Cube2,520},
		{EMT2BuiltinAffect::Cube3,521},
		{EMT2BuiltinAffect::Cube4,522},
		{EMT2BuiltinAffect::Cube5,523},
		{EMT2BuiltinAffect::Cube6,524},
		{EMT2BuiltinAffect::Cube7,525},
		{EMT2BuiltinAffect::Cube8,526},
		{EMT2BuiltinAffect::Cube9,527},
		{EMT2BuiltinAffect::Cube10,528},
		{EMT2BuiltinAffect::Cube11,529},
		{EMT2BuiltinAffect::Cube12,530},
		{EMT2BuiltinAffect::Blend,531},
		{EMT2BuiltinAffect::HorseName,532},
		{EMT2BuiltinAffect::MountBonus,533},
		{EMT2BuiltinAffect::AutoHealthRecovery,534},
		{EMT2BuiltinAffect::AutoManaRecovery,535},
		{EMT2BuiltinAffect::DragonSoulQualified,540},
		{EMT2BuiltinAffect::DragonSoulDeck0,541},
		{EMT2BuiltinAffect::DragonSoulDeck1,542},
		{EMT2BuiltinAffect::RamadanAbility,300},
		{EMT2BuiltinAffect::RamadanRing,301},
		{EMT2BuiltinAffect::NoGoodAbility,302},
		{EMT2BuiltinAffect::HolyStonePower,303},
		{EMT2BuiltinAffect::HealthRecovery,30001},
		{EMT2BuiltinAffect::ManaRecovery,30002},
		{EMT2BuiltinAffect::AttackSpeed,30003},
		{EMT2BuiltinAffect::MovementSpeed,30004}
	};
}
FMT2AffectIdentifier FMT2AffectIdentifier::FromBuiltin(EMT2BuiltinAffect Value)
{
	FMT2AffectIdentifier Result; Result.Builtin=Value; return Result;
}
FMT2AffectIdentifier FMT2AffectIdentifier::FromSkillVnum(int32 Value)
{
	FMT2AffectIdentifier Result; Result.Kind=EMT2AffectIdentityKind::Skill; Result.SkillVnum=Value; return Result;
}
FMT2AffectIdentifier FMT2AffectIdentifier::FromLegacyId(int32 Raw)
{
	for (const auto& Pair : Builtins) { if (Pair.Id==Raw) { return FromBuiltin(Pair.Named); } }
	if (Raw>0 && Raw<256) { return FromSkillVnum(Raw); }
	FMT2AffectIdentifier Result; Result.Kind=EMT2AffectIdentityKind::Custom; Result.CustomId=Raw; return Result;
}
int32 FMT2AffectIdentifier::ToLegacyId() const
{
	if (Kind==EMT2AffectIdentityKind::Skill) { return SkillVnum; }
	if (Kind==EMT2AffectIdentityKind::Custom) { return CustomId; }
	for (const auto& Pair : Builtins) { if (Pair.Named==Builtin) { return Pair.Id; } }
	ensureMsgf(false,TEXT("Unsupported built-in affect selector")); return CustomId;
}
bool FMT2AffectIdentifier::SerializeFromMismatchedTag(const FPropertyTag& Tag,FStructuredArchive::FSlot Slot)
{
	if (Tag.GetType().GetName()!=NAME_IntProperty) { return false; }
	int32 Raw=0; Slot<<Raw; *this=FromLegacyId(Raw); return true;
}
bool FMT2AffectIdentifier::ImportTextItem(const TCHAR*& Buffer,int32 PortFlags,UObject* Parent,FOutputDevice* ErrorText)
{
	int32 Raw=0;
	if (!MT2SemanticSerialization::ReadInt32Token(Buffer,Raw)) { return false; }
	*this=FromLegacyId(Raw); return true;
}
bool FMT2AffectIdentifier::NetSerialize(FArchive& Ar,UPackageMap*,bool& bOutSuccess)
{
	int32 Raw=ToLegacyId(); Ar<<Raw;
	if (Ar.IsLoading() && !Ar.IsError()) { *this=FromLegacyId(Raw); }
	bOutSuccess=!Ar.IsError(); return true;
}
