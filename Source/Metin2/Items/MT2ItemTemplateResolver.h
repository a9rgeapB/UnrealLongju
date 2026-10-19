#pragma once

#include "CoreMinimal.h"
#include "Items/MT2ItemSemanticTypes.h"

class UClass;

/** Maps a parsed legacy item type/subtype pair to its concrete native template class. */
METIN2_API UClass* ResolveMT2ItemTemplateClass(EMT2ItemType ItemType, int32 LegacySubtype);

