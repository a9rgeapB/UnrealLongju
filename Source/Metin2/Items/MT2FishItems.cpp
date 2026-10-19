#include "Items/MT2ItemTemplate.h"

#include "Characters/MT2PlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/MT2HealthComponent.h"
#include "Engine/World.h"
#include "Fishing/MT2FishingSettings.h"
#include "Items/MT2InventoryComponent.h"
#include "Npcs/MT2Campfire.h"
#include "World/MT2MapAttributes.h"

int32 FMT2FishUseOutcome::GetItemVnum() const
{
	const auto Class = ItemTemplate.LoadSynchronous();
	return Class ? Class->GetDefaultObject<UMT2ItemTemplate>()->Vnum : bTypedReference ? 0 : ItemVnum;
}
int32 FMT2FishDefinition::GetDeadItemVnum() const
{
	const auto Class = DeadItemTemplate.LoadSynchronous();
	return Class ? Class->GetDefaultObject<UMT2ItemTemplate>()->Vnum : bTypedReferences ? 0 : DeadItemVnum;
}
int32 FMT2FishDefinition::GetGrilledItemVnum() const
{
	const auto Class = GrilledItemTemplate.LoadSynchronous();
	return Class ? Class->GetDefaultObject<UMT2ItemTemplate>()->Vnum : bTypedReferences ? 0 : GrilledItemVnum;
}

bool FMT2FishDefinition::IsValid() const
{
	int64 Total = 0;
	for (const auto& Outcome : UseOutcomes)
	{
		if (Outcome.Weight < 0 || Outcome.GetItemVnum() < 0) return false;
		Total += Outcome.Weight;
	}
	return Total == 10000;
}

int32 FMT2FishDefinition::ResolveUseRoll(int32 Roll) const
{
	if (!IsValid() || Roll < 1 || Roll > 10000) return INDEX_NONE;
	for (const auto& Outcome : UseOutcomes)
	{
		Roll -= Outcome.Weight;
		if (Roll <= 0) return Outcome.GetItemVnum();
	}
	return INDEX_NONE;
}




