/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/Templates/MT2ItemBonusConsumableTemplate.h"

#include "Items/MT2InventoryComponent.h"

#include "Items/MT2ItemBonusSettings.h"
#include "Items/MT2ItemTemplate.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Components/MT2HealthComponent.h"
#include "Player/MT2PlayerState.h"
#include "Trade/MT2TradeComponent.h"

EMT2ItemBonusApplyResult UMT2InventoryComponent::ApplyBonusConsumable(
	int32 SourceSlot, int32 TargetSlot, bool bTargetEquipped)
{
	const AMT2PlayerCharacter* Character = Cast<AMT2PlayerCharacter>(GetOwner());
	const UMT2HealthComponent* Health = Character ? Character->GetHealthComponent() : nullptr;
	const AMT2PlayerState* State = Character ? Character->GetPlayerState<AMT2PlayerState>() : nullptr;
	if (!Character || !Character->HasAuthority() || !Health || Health->IsDead() ||
		(State && State->GetTradeComponent() && State->GetTradeComponent()->IsTrading()) ||
		(!bTargetEquipped && SourceSlot == TargetSlot) ||
		!Slots.IsValidIndex(SourceSlot) || Slots[SourceSlot].IsEmpty())
	{
		return EMT2ItemBonusApplyResult::Invalid;
	}
	TArray<FMT2ItemSlot>& TargetContainer = bTargetEquipped ? Equipment : Slots;
	if (!TargetContainer.IsValidIndex(TargetSlot) || TargetContainer[TargetSlot].IsEmpty())
	{
		return EMT2ItemBonusApplyResult::Invalid;
	}

	const UMT2ItemBonusSettings* Settings = GetDefault<UMT2ItemBonusSettings>();
	const UMT2ItemBonusConsumableTemplate* SourceTemplate =
		Cast<UMT2ItemBonusConsumableTemplate>(ResolveTemplate(Slots[SourceSlot].Vnum));
	const UMT2ItemTemplate* TargetTemplate = ResolveTemplate(TargetContainer[TargetSlot].Vnum);
	if (!Settings || !SourceTemplate || !TargetTemplate)
	{
		return EMT2ItemBonusApplyResult::Invalid;
	}

	FMT2ItemSlot StagedTarget = TargetContainer[TargetSlot];
	const EMT2ItemBonusApplyResult Result = SourceTemplate->ApplyToEquipment(
		StagedTarget, *TargetTemplate, *Settings);
	if (Result == EMT2ItemBonusApplyResult::Invalid)
	{
		return Result;
	}
	if (Result == EMT2ItemBonusApplyResult::Success)
	{
		TargetContainer[TargetSlot] = MoveTemp(StagedTarget);
	}
	if (--Slots[SourceSlot].Count <= 0)
	{
		Slots[SourceSlot] = FMT2ItemSlot();
	}
	OnInventoryChanged.Broadcast();
	if (bTargetEquipped)
	{
		OnEquipmentChanged.Broadcast();
	}
	return Result;
}
