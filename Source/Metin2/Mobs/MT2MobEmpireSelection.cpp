#include "Mobs/MT2MobEmpireSelection.h"
#include "Serialization/MT2SemanticSerialization.h"
#include "UObject/PropertyTag.h"

FMT2MobEmpireSelection FMT2MobEmpireSelection::FromLegacy(int32 Raw)
{
	FMT2MobEmpireSelection Result;
	Result.LegacyValue=Raw;
	Result.Type=Raw>=0 && Raw<=3 ? static_cast<EMT2ImportedMobEmpire>(Raw) : EMT2ImportedMobEmpire::Unsupported;
	return Result;
}
int32 FMT2MobEmpireSelection::ToLegacy() const
{
	return static_cast<uint8>(Type)<=3 ? static_cast<int32>(Type) : LegacyValue;
}
bool FMT2MobEmpireSelection::TryGetEmpire(EMT2Empire& Out) const
{
	if (static_cast<uint8>(Type)>3) { return false; }
	Out=static_cast<EMT2Empire>(Type);
	return true;
}
bool FMT2MobEmpireSelection::SerializeFromMismatchedTag(const FPropertyTag& Tag,FStructuredArchive::FSlot Slot)
{
	if (Tag.GetType().GetName()==NAME_IntProperty)
	{
		int32 Raw=0; Slot<<Raw; *this=FromLegacy(Raw); return true;
	}
	if (Tag.GetType().IsEnum(StaticEnum<EMT2Empire>()->GetFName()))
	{
		FName Name; Slot<<Name;
		const int64 Raw=StaticEnum<EMT2Empire>()->GetValueByName(Name);
		if (Raw==INDEX_NONE) { return false; }
		*this=FromLegacy(static_cast<int32>(Raw)); return true;
	}
	return false;
}
bool FMT2MobEmpireSelection::ImportTextItem(const TCHAR*& Buffer,int32,UObject*,FOutputDevice*)
{
	int32 Raw=0;
	if (!MT2SemanticSerialization::ReadInt32Token(Buffer,Raw)) { return false; }
	*this=FromLegacy(Raw); return true;
}
