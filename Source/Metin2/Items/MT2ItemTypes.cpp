#include "Items/MT2ItemTypes.h"

bool FMT2ItemDefinition::ImportTextItem(const TCHAR*& Buffer,int32 PortFlags,UObject* Parent,FOutputDevice* ErrorText)
{
	const TCHAR* End=StaticStruct()->ImportText(Buffer,this,Parent,PortFlags,ErrorText,TEXT("MT2ItemDefinition"),false);
	if (!End) { return false; }
	Buffer=End; SubType.ResolveLegacyFamily(ItemType); return true;
}
