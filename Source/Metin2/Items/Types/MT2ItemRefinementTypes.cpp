#include "Items/Types/MT2ItemRefinementTypes.h"
#include "Items/MT2ItemTemplate.h"

int32 FMT2RefinementMaterial::GetItemVnum() const
{
	const auto Class = ItemTemplate.LoadSynchronous();
	return Class ? Class->GetDefaultObject<UMT2ItemTemplate>()->Vnum : bTypedReference ? 0 : ItemVnum;
}
