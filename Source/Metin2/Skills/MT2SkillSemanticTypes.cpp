#include "Skills/MT2SkillSemanticTypes.h"
#include "Serialization/MT2SemanticSerialization.h"
#include "UObject/PropertyTag.h"
FMT2LegacySkillClassSelection FMT2LegacySkillClassSelection::FromLegacy(int32 Raw)
{
	FMT2LegacySkillClassSelection Result; Result.LegacyValue=Raw;
	Result.Type=Raw>=-1 && Raw<=6 ? static_cast<EMT2LegacySkillClass>(Raw+1) : EMT2LegacySkillClass::Unsupported;
	return Result;
}
int32 FMT2LegacySkillClassSelection::ToLegacy() const
{
	return static_cast<uint8>(Type)<=7 ? static_cast<int32>(Type)-1 : LegacyValue;
}
bool FMT2LegacySkillClassSelection::SerializeFromMismatchedTag(const FPropertyTag& Tag,FStructuredArchive::FSlot Slot)
{
	if (Tag.GetType().GetName()!=NAME_IntProperty) { return false; }
	int32 Raw=0; Slot<<Raw; *this=FromLegacy(Raw); return true;
}
bool FMT2LegacySkillClassSelection::ImportTextItem(const TCHAR*& Buffer,int32 PortFlags,UObject* Parent,FOutputDevice* ErrorText)
{
	int32 Raw=0;
	if (!MT2SemanticSerialization::ReadInt32Token(Buffer,Raw)) { return false; }
	*this=FromLegacy(Raw); return true;
}
