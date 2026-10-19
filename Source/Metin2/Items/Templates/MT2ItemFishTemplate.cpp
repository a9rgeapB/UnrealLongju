/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/Templates/MT2ItemFishTemplate.h"
#include "Items/MT2ItemTemplate.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/MT2HealthComponent.h"
#include "Engine/World.h"
#include "Fishing/MT2FishingSettings.h"
#include "Items/MT2InventoryComponent.h"
#include "Npcs/MT2Campfire.h"
#include "World/MT2MapAttributes.h"

EMT2ItemUseExecution UMT2ItemFishTemplate::ExecuteUse(AMT2PlayerCharacter& Character,
	UMT2InventoryComponent& Inventory, int32 InventorySlot) const
{
	if (!Character.HasAuthority() || FishSubType != EMT2FishSubType::Alive || !Fish.IsValid() ||
		!Inventory.GetSlots().IsValidIndex(InventorySlot) || Inventory.GetSlots()[InventorySlot].Vnum != Vnum)
		return EMT2ItemUseExecution::Rejected;
	const int32 Result = Fish.ResolveUseRoll(FMath::RandRange(1, 10000));
	return Result >= 0 && Inventory.ExchangeItemAtSlot(InventorySlot, 1, Result, Result > 0 ? 1 : 0)
		? EMT2ItemUseExecution::Accepted : EMT2ItemUseExecution::Rejected;
}

void UMT2ItemFishTemplate::InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const
{
	Data.FamilyState.InitializeAs<FMT2FishItemState>(DefaultFishState);
}

void UMT2ItemFishTemplate::CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data)
{
	DefaultFishState = Data.GetFishState();
}
