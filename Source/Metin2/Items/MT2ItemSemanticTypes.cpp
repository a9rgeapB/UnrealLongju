#include "Items/MT2ItemSemanticTypes.h"
#include "Items/MT2ItemTypes.h"
#include "Serialization/MT2SemanticSerialization.h"
#include "UObject/PropertyTag.h"

bool MT2ItemSemanticTypes::TryDecodeBonus(int32 Raw,EMT2ItemBonusType& Out)
{
	const UEnum* Domain=StaticEnum<EMT2ItemBonusType>();
	if (Raw<0 || Raw>=Domain->GetMaxEnumValue() || !Domain->IsValidEnumValue(Raw)) { return false; }
	Out=static_cast<EMT2ItemBonusType>(Raw); return true;
}

namespace
{
	// Fixed subtype enums never substitute an arbitrary ordinal for an unsupported ID.
	template<typename T> T DecodeSubtype(int32 Raw)
	{
		return Raw>=0 && Raw<254 && StaticEnum<T>()->IsValidEnumValue(Raw) ? static_cast<T>(Raw) : T::Unsupported;
	}
	template<typename T> int32 EncodeSubtype(T Choice,int32 Raw)
	{
		return Choice==T::Unsupported ? Raw : static_cast<int32>(Choice);
	}
}
FMT2ItemUniqueSubtypeSelection FMT2ItemUniqueSubtypeSelection::FromLegacy(int32 Raw)
{
	FMT2ItemUniqueSubtypeSelection Result; Result.Type=DecodeSubtype<EMT2UniqueSubType>(Raw); Result.LegacyValue=Raw; return Result;
}
int32 FMT2ItemUniqueSubtypeSelection::ToLegacy() const { return EncodeSubtype(Type,LegacyValue); }
bool FMT2ItemUniqueSubtypeSelection::SerializeFromMismatchedTag(const FPropertyTag& Tag,FStructuredArchive::FSlot Slot)
{
	if (Tag.GetType().GetName()!=NAME_IntProperty) { return false; }
	int32 Raw=0; Slot<<Raw; *this=FromLegacy(Raw); return true;
}
bool FMT2ItemUniqueSubtypeSelection::ImportTextItem(const TCHAR*& Buffer,int32 PortFlags,UObject* Parent,FOutputDevice* ErrorText)
{
	int32 Raw=0;
	if (!MT2SemanticSerialization::ReadInt32Token(Buffer,Raw)) { return false; }
	*this=FromLegacy(Raw); return true;
}
FMT2ItemSubtype FMT2ItemSubtype::FromLegacy(EMT2ItemType InFamily,int32 Raw)
{
	FMT2ItemSubtype Result; Result.Family=InFamily; Result.LegacyValue=Raw;
	switch (InFamily)
	{
	case EMT2ItemType::Weapon: Result.Weapon=DecodeSubtype<EMT2WeaponSubType>(Raw); break;
	case EMT2ItemType::Armor: Result.Armor=DecodeSubtype<EMT2ArmorSubType>(Raw); break;
	case EMT2ItemType::Use: Result.Use=DecodeSubtype<EMT2UseSubType>(Raw); break;
	case EMT2ItemType::AutoUse: Result.AutoUse=DecodeSubtype<EMT2AutoUseSubType>(Raw); break;
	case EMT2ItemType::Material: Result.Material=DecodeSubtype<EMT2MaterialSubType>(Raw); break;
	case EMT2ItemType::Metin: Result.Metin=DecodeSubtype<EMT2MetinSubType>(Raw); break;
	case EMT2ItemType::Costume: Result.Costume=DecodeSubtype<EMT2CostumeSubType>(Raw); break;
	case EMT2ItemType::Unique: Result.Unique=DecodeSubtype<EMT2UniqueSubType>(Raw); break;
	case EMT2ItemType::Special: Result.Special=DecodeSubtype<EMT2SpecialSubType>(Raw); break;
	case EMT2ItemType::Tool: Result.Tool=DecodeSubtype<EMT2ToolSubType>(Raw); break;
	case EMT2ItemType::Lottery: Result.Lottery=DecodeSubtype<EMT2LotterySubType>(Raw); break;
	case EMT2ItemType::Fish: Result.Fish=DecodeSubtype<EMT2FishSubType>(Raw); break;
	case EMT2ItemType::Resource: Result.Resource=DecodeSubtype<EMT2ResourceSubType>(Raw); break;
	case EMT2ItemType::Extract: Result.Extract=DecodeSubtype<EMT2ExtractSubType>(Raw); break;
	case EMT2ItemType::SpecialDragonSoul:
	case EMT2ItemType::DragonSoul: Result.DragonSoul=DecodeSubtype<EMT2DragonSoulSubType>(Raw); break;
	default: break;
	}
	return Result;
}
int32 FMT2ItemSubtype::ToLegacy() const
{
	if (bNeedsLegacyFamily) { return LegacyValue; }
	switch (Family)
	{
	case EMT2ItemType::Weapon: return EncodeSubtype(Weapon,LegacyValue);
	case EMT2ItemType::Armor: return EncodeSubtype(Armor,LegacyValue);
	case EMT2ItemType::Use: return EncodeSubtype(Use,LegacyValue);
	case EMT2ItemType::AutoUse: return EncodeSubtype(AutoUse,LegacyValue);
	case EMT2ItemType::Material: return EncodeSubtype(Material,LegacyValue);
	case EMT2ItemType::Metin: return EncodeSubtype(Metin,LegacyValue);
	case EMT2ItemType::Costume: return EncodeSubtype(Costume,LegacyValue);
	case EMT2ItemType::Unique: return EncodeSubtype(Unique,LegacyValue);
	case EMT2ItemType::Special: return EncodeSubtype(Special,LegacyValue);
	case EMT2ItemType::Tool: return EncodeSubtype(Tool,LegacyValue);
	case EMT2ItemType::Lottery: return EncodeSubtype(Lottery,LegacyValue);
	case EMT2ItemType::Fish: return EncodeSubtype(Fish,LegacyValue);
	case EMT2ItemType::Resource: return EncodeSubtype(Resource,LegacyValue);
	case EMT2ItemType::Extract: return EncodeSubtype(Extract,LegacyValue);
	case EMT2ItemType::SpecialDragonSoul:
	case EMT2ItemType::DragonSoul: return EncodeSubtype(DragonSoul,LegacyValue);
	default: return LegacyValue;
	}
}
void FMT2ItemSubtype::ResolveLegacyFamily(EMT2ItemType InFamily)
{
	if (bNeedsLegacyFamily) { *this=FromLegacy(InFamily,LegacyValue); }
}
bool FMT2ItemSubtype::SerializeFromMismatchedTag(const FPropertyTag& Tag,FStructuredArchive::FSlot Slot)
{
	if (Tag.GetType().GetName()!=NAME_IntProperty) { return false; }
	Slot<<LegacyValue; bNeedsLegacyFamily=true; return true;
}
bool FMT2ItemSubtype::ImportTextItem(const TCHAR*& Buffer,int32 PortFlags,UObject* Parent,FOutputDevice* ErrorText)
{
	int32 Raw=0;
	if (!MT2SemanticSerialization::ReadInt32Token(Buffer,Raw)) { return false; }
	LegacyValue=Raw; bNeedsLegacyFamily=true; return true;
}

bool MT2ItemSemanticTypes::TryDecodeAddon(int32 Raw,EMT2ItemAddonType& Out)
{
	if (Raw==0) { Out=EMT2ItemAddonType::None; return true; }
	if (Raw==-1) { Out=EMT2ItemAddonType::AverageAndSkillDamage; return true; }
	return false;
}
int32 MT2ItemSemanticTypes::EncodeAddon(EMT2ItemAddonType Type)
{
	checkf(Type==EMT2ItemAddonType::None || Type==EMT2ItemAddonType::AverageAndSkillDamage,TEXT("Opaque addon IDs require their lossless selection wrapper."));
	return Type==EMT2ItemAddonType::AverageAndSkillDamage ? -1 : 0;
}
FMT2ItemAddonSelection FMT2ItemAddonSelection::FromLegacy(int32 Raw)
{
	FMT2ItemAddonSelection Result; Result.LegacyValue=Raw;
	if (!MT2ItemSemanticTypes::TryDecodeAddon(Raw,Result.Type)) { Result.Type=EMT2ItemAddonType::Unsupported; }
	return Result;
}
int32 FMT2ItemAddonSelection::ToLegacy() const
{
	if (Type!=EMT2ItemAddonType::None && Type!=EMT2ItemAddonType::AverageAndSkillDamage) { return LegacyValue; }
	return MT2ItemSemanticTypes::EncodeAddon(Type);
}
bool FMT2ItemAddonSelection::SerializeFromMismatchedTag(const FPropertyTag& Tag,FStructuredArchive::FSlot Slot)
{
	if (Tag.GetType().GetName()!=NAME_IntProperty) { return false; }
	int32 Raw=0; Slot<<Raw; *this=FromLegacy(Raw); return true;
}
bool FMT2ItemAddonSelection::ImportTextItem(const TCHAR*& Buffer,int32,UObject*,FOutputDevice*)
{
	int32 Raw=0;
	if (!MT2SemanticSerialization::ReadInt32Token(Buffer,Raw)) { return false; }
	*this=FromLegacy(Raw); return true;
}
