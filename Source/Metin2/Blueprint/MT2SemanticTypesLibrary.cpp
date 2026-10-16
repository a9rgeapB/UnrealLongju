#include "Blueprint/MT2SemanticTypesLibrary.h"

FMT2ItemSubtype UMT2SemanticTypesLibrary::MakeWeaponSubtype(EMT2WeaponSubType Choice)
{
	return FMT2ItemSubtype::FromLegacy(EMT2ItemType::Weapon,static_cast<int32>(Choice));
}

FMT2ItemSubtype UMT2SemanticTypesLibrary::MakeArmorSubtype(EMT2ArmorSubType Choice)
{
	return FMT2ItemSubtype::FromLegacy(EMT2ItemType::Armor,static_cast<int32>(Choice));
}

FMT2ItemSubtype UMT2SemanticTypesLibrary::MakeUseSubtype(EMT2UseSubType Choice)
{
	return FMT2ItemSubtype::FromLegacy(EMT2ItemType::Use,static_cast<int32>(Choice));
}

FMT2ItemSubtype UMT2SemanticTypesLibrary::MakeAutoUseSubtype(EMT2AutoUseSubType Choice)
{
	return FMT2ItemSubtype::FromLegacy(EMT2ItemType::AutoUse,static_cast<int32>(Choice));
}

FMT2ItemSubtype UMT2SemanticTypesLibrary::MakeMaterialSubtype(EMT2MaterialSubType Choice)
{
	return FMT2ItemSubtype::FromLegacy(EMT2ItemType::Material,static_cast<int32>(Choice));
}

FMT2ItemSubtype UMT2SemanticTypesLibrary::MakeMetinSubtype(EMT2MetinSubType Choice)
{
	return FMT2ItemSubtype::FromLegacy(EMT2ItemType::Metin,static_cast<int32>(Choice));
}

FMT2ItemSubtype UMT2SemanticTypesLibrary::MakeCostumeSubtype(EMT2CostumeSubType Choice)
{
	return FMT2ItemSubtype::FromLegacy(EMT2ItemType::Costume,static_cast<int32>(Choice));
}

FMT2ItemSubtype UMT2SemanticTypesLibrary::MakeUniqueSubtype(EMT2UniqueSubType Choice)
{
	return FMT2ItemSubtype::FromLegacy(EMT2ItemType::Unique,static_cast<int32>(Choice));
}

FMT2ItemSubtype UMT2SemanticTypesLibrary::MakeSpecialSubtype(EMT2SpecialSubType Choice)
{
	return FMT2ItemSubtype::FromLegacy(EMT2ItemType::Special,static_cast<int32>(Choice));
}

FMT2ItemSubtype UMT2SemanticTypesLibrary::MakeToolSubtype(EMT2ToolSubType Choice)
{
	return FMT2ItemSubtype::FromLegacy(EMT2ItemType::Tool,static_cast<int32>(Choice));
}

FMT2ItemSubtype UMT2SemanticTypesLibrary::MakeLotterySubtype(EMT2LotterySubType Choice)
{
	return FMT2ItemSubtype::FromLegacy(EMT2ItemType::Lottery,static_cast<int32>(Choice));
}

FMT2ItemSubtype UMT2SemanticTypesLibrary::MakeFishSubtype(EMT2FishSubType Choice)
{
	return FMT2ItemSubtype::FromLegacy(EMT2ItemType::Fish,static_cast<int32>(Choice));
}

FMT2ItemSubtype UMT2SemanticTypesLibrary::MakeResourceSubtype(EMT2ResourceSubType Choice)
{
	return FMT2ItemSubtype::FromLegacy(EMT2ItemType::Resource,static_cast<int32>(Choice));
}

FMT2ItemSubtype UMT2SemanticTypesLibrary::MakeExtractSubtype(EMT2ExtractSubType Choice)
{
	return FMT2ItemSubtype::FromLegacy(EMT2ItemType::Extract,static_cast<int32>(Choice));
}

FMT2ItemSubtype UMT2SemanticTypesLibrary::MakeDragonSoulSubtype(EMT2DragonSoulSubType Choice)
{
	return FMT2ItemSubtype::FromLegacy(EMT2ItemType::DragonSoul,static_cast<int32>(Choice));
}

bool UMT2SemanticTypesLibrary::IsItemSubtypeForFamily(const FMT2ItemSubtype& Subtype,EMT2ItemType Family)
{
	return Subtype.IsForFamily(Family);
}
void UMT2SemanticTypesLibrary::SetItemSubtype(FMT2ItemDefinition& Item,const FMT2ItemSubtype& Subtype)
{
	Item.ItemType=Subtype.Family; Item.SubType=Subtype;
}

namespace
{
template<typename T> int32 PositionMask(T Flag)
{
	const uint8 Position=static_cast<uint8>(Flag);
	return Position<32 ? static_cast<int32>(uint32(1)<<Position) : 0;
}
}
int32 UMT2SemanticTypesLibrary::MakeItemFlag(EMT2ItemFlag Flag) { return PositionMask(Flag); }
bool UMT2SemanticTypesLibrary::HasItemFlag(int32 Flags,EMT2ItemFlag Flag) { return (Flags & MakeItemFlag(Flag))!=0; }
int32 UMT2SemanticTypesLibrary::MakeItemAntiFlag(EMT2ItemAntiFlag Flag) { return PositionMask(Flag); }
bool UMT2SemanticTypesLibrary::HasItemAntiFlag(int32 Flags,EMT2ItemAntiFlag Flag) { return (Flags & MakeItemAntiFlag(Flag))!=0; }
int32 UMT2SemanticTypesLibrary::MakeItemImmuneFlag(EMT2ItemImmuneFlag Flag) { return PositionMask(Flag); }
bool UMT2SemanticTypesLibrary::HasItemImmuneFlag(int32 Flags,EMT2ItemImmuneFlag Flag) { return (Flags & MakeItemImmuneFlag(Flag))!=0; }
int32 UMT2SemanticTypesLibrary::MakeItemWearFlag(EMT2ItemWearFlag Flag) { return PositionMask(Flag); }
bool UMT2SemanticTypesLibrary::HasItemWearFlag(int32 Flags,EMT2ItemWearFlag Flag) { return (Flags & MakeItemWearFlag(Flag))!=0; }
int32 UMT2SemanticTypesLibrary::MakeItemBonusTargetFlag(EMT2ItemBonusTarget Flag) { return static_cast<int32>(Flag); }
bool UMT2SemanticTypesLibrary::HasItemBonusTargetFlag(int32 Flags,EMT2ItemBonusTarget Flag) { return (Flags & MakeItemBonusTargetFlag(Flag))!=0; }

FMT2AffectIdentifier UMT2SemanticTypesLibrary::MakeBuiltinAffect(EMT2BuiltinAffect Affect) { return FMT2AffectIdentifier::FromBuiltin(Affect); }
FMT2AffectIdentifier UMT2SemanticTypesLibrary::MakeSkillAffect(int32 SkillVnum) { return FMT2AffectIdentifier::FromSkillVnum(SkillVnum); }
FMT2AffectIdentifier UMT2SemanticTypesLibrary::MakeCustomAffect(int32 CustomId)
{
	FMT2AffectIdentifier Result; Result.Kind=EMT2AffectIdentityKind::Custom; Result.CustomId=CustomId; return Result;
}
int32 UMT2SemanticTypesLibrary::GetLegacyAffectId(const FMT2AffectIdentifier& Identifier) { return Identifier.ToLegacyId(); }
int32 UMT2SemanticTypesLibrary::MakeEffectFlag(EMT2EffectFlag Flag) { return Flag==EMT2EffectFlag::None ? 0 : PositionMask(Flag); }
int32 UMT2SemanticTypesLibrary::MakeSkillFlag(EMT2SkillFlag Flag) { return PositionMask(Flag); }

int32 UMT2SemanticTypesLibrary::MakeMobAIFlag(EMT2MobAIFlag Flag) { return PositionMask(Flag); }
bool UMT2SemanticTypesLibrary::HasMobAIFlag(int32 Flags,EMT2MobAIFlag Flag) { return (Flags & MakeMobAIFlag(Flag))!=0; }

int32 UMT2SemanticTypesLibrary::MakeMobRaceFlag(EMT2MobRaceFlag Flag) { return PositionMask(Flag); }
bool UMT2SemanticTypesLibrary::HasMobRaceFlag(int32 Flags,EMT2MobRaceFlag Flag) { return (Flags & MakeMobRaceFlag(Flag))!=0; }

int32 UMT2SemanticTypesLibrary::MakeMobImmuneFlag(EMT2MobImmuneFlag Flag) { return PositionMask(Flag); }
bool UMT2SemanticTypesLibrary::HasMobImmuneFlag(int32 Flags,EMT2MobImmuneFlag Flag) { return (Flags & MakeMobImmuneFlag(Flag))!=0; }
