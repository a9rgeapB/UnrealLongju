#include "Items/MT2InventoryComponent.h"
#include "Items/MT2AccessorySocketRules.h"
#include "Items/MT2ItemTemplate.h"
#include "Items/MT2ItemUtils.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Player/MT2PlayerState.h"
#include "Trade/MT2TradeComponent.h"
#include "Components/MT2HealthComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

FMT2AccessorySocketState UMT2InventoryComponent::GetEffectiveAccessoryState(int32 WearPosition) const
{
	if (!Equipment.IsValidIndex(WearPosition) || !MT2AccessorySockets::IsAccessory(ResolveTemplate(Equipment[WearPosition].Vnum)))
	{ return {}; }
	const double UpdatedAt = GetOwner() && !GetOwner()->HasAuthority() && ReceivedAccessoryTimes.IsValidIndex(WearPosition)
		? ReceivedAccessoryTimes[WearPosition] : AccessoryWearUpdatedAt;
	const double Elapsed = GetWorld() ? FMath::Max(0.0, GetWorld()->GetTimeSeconds() - UpdatedAt) : 0;
	return MT2AccessorySockets::Advance(Equipment[WearPosition].AccessorySockets, Elapsed);
}

void UMT2InventoryComponent::TrackReplicatedAccessoryWear()
{
	ReceivedAccessoryStates.SetNum(Equipment.Num()); ReceivedAccessoryVnums.SetNum(Equipment.Num()); ReceivedAccessoryTimes.SetNum(Equipment.Num());
	for (int32 Index=0; Index<Equipment.Num(); ++Index)
	{
		if (ReceivedAccessoryVnums[Index] != Equipment[Index].Vnum || !(ReceivedAccessoryStates[Index] == Equipment[Index].AccessorySockets))
		{
			ReceivedAccessoryTimes[Index] = GetWorld() ? GetWorld()->GetTimeSeconds() : 0;
			ReceivedAccessoryStates[Index] = Equipment[Index].AccessorySockets;
			ReceivedAccessoryVnums[Index] = Equipment[Index].Vnum;
		}
	}
}

void UMT2InventoryComponent::SettleAccessoryWear()
{
	if (bSettlingAccessoryWear || !GetOwner() || !GetOwner()->HasAuthority() || !GetWorld()) { return; }
	TGuardValue<bool> Guard(bSettlingAccessoryWear, true);
	bool bGradeChanged = false;
	for (int32 Index = 0; Index < Equipment.Num(); ++Index)
	{
		if (!MT2AccessorySockets::IsAccessory(ResolveTemplate(Equipment[Index].Vnum))) { continue; }
		const auto Effective = GetEffectiveAccessoryState(Index);
		bGradeChanged |= Effective.CurrentGrade != Equipment[Index].AccessorySockets.CurrentGrade;
		Equipment[Index].AccessorySockets = Effective;
	}
	AccessoryWearUpdatedAt = GetWorld()->GetTimeSeconds();
	if (bGradeChanged) { OnEquipmentChanged.Broadcast(); GetOwner()->ForceNetUpdate(); }
}

void UMT2InventoryComponent::ScheduleAccessoryWear()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !GetWorld()) { return; }
	auto& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(AccessoryWearTimer);
	double Delay = MAX_dbl;
	for (const auto& Item : Equipment)
	{
		if (MT2AccessorySockets::IsAccessory(ResolveTemplate(Item.Vnum)) && Item.AccessorySockets.CurrentGrade > 0)
		{ Delay = FMath::Min(Delay, Item.AccessorySockets.RemainingSeconds - Item.AccessorySockets.WearFraction); }
	}
	if (Delay != MAX_dbl) { Timers.SetTimer(AccessoryWearTimer, this, &ThisClass::ProcessAccessoryWear, FMath::Max(0.01f, static_cast<float>(Delay)), false); }
}

void UMT2InventoryComponent::ProcessAccessoryWear()
{
	SettleAccessoryWear();
	ScheduleAccessoryWear();
}
EMT2AccessorySocketApplyResult UMT2InventoryComponent::ApplyAccessoryMaterial(int32 SourceSlot, int32 TargetSlot, bool bTargetEquipped,
	const FMT2ItemSlot& ExpectedSource, const FMT2ItemSlot& ExpectedTarget)
{
	return ApplyAccessoryMaterialWithRoll(SourceSlot, TargetSlot, bTargetEquipped, ExpectedSource, ExpectedTarget, FMath::RandRange(1, 100));
}
EMT2AccessorySocketApplyResult UMT2InventoryComponent::ApplyAccessoryMaterialWithRoll(int32 SourceSlot, int32 TargetSlot, bool bTargetEquipped,
	FMT2ItemSlot ExpectedSource, FMT2ItemSlot ExpectedTarget, int32 Roll)
{
	using namespace MT2AccessorySockets;
	auto* Player = Cast<AMT2PlayerCharacter>(GetOwner());
	auto* State = Player ? Player->GetPlayerState<AMT2PlayerState>() : nullptr;
	SettleAccessoryWear();
	auto& Targets = bTargetEquipped ? Equipment : Slots;
	// Countdown changes are not item replacement; grade/capacity and all other instance data still match.
	if (Targets.IsValidIndex(TargetSlot))
	{
		if (bTargetEquipped) { ExpectedTarget.AccessorySockets.RemainingSeconds = Targets[TargetSlot].AccessorySockets.RemainingSeconds; }
		ExpectedTarget.AccessorySockets.WearFraction = Targets[TargetSlot].AccessorySockets.WearFraction;
	}
	if (bResolvingAccessory || !IsValid(Player) || !Player->HasAuthority() || !IsValid(State) ||
		Player->GetHealthComponent()->IsDead() || (State->GetTradeComponent() && State->GetTradeComponent()->IsTrading()) ||
		!Slots.IsValidIndex(SourceSlot) || !Targets.IsValidIndex(TargetSlot) || (!bTargetEquipped && SourceSlot == TargetSlot) ||
		ExpectedSource.IsEmpty() || ExpectedTarget.IsEmpty() || Slots[SourceSlot].Count != ExpectedSource.Count ||
		Targets[TargetSlot].Count != ExpectedTarget.Count || !MT2ItemUtils::HaveSameInstanceData(Slots[SourceSlot], ExpectedSource) ||
		!MT2ItemUtils::HaveSameInstanceData(Targets[TargetSlot], ExpectedTarget) || Roll < 1 || Roll > 100)
	{ return EMT2AccessorySocketApplyResult::Invalid; }
	TGuardValue<bool> Guard(bResolvingAccessory, true);
	const auto* Material = Cast<UMT2ItemUseTemplate>(ResolveTemplate(ExpectedSource.Vnum));
	if (!IsMaterial(Material) || !Compatible(Material, ResolveTemplate(ExpectedTarget.Vnum)))
	{ return EMT2AccessorySocketApplyResult::Incompatible; }
	auto SocketState = Normalize(Targets[TargetSlot].AccessorySockets);
	const bool bOpening = Material->UseSubType == EMT2UseSubType::AddAccessorySocket;
	if ((bOpening && SocketState.MaximumGrade == 3) || (!bOpening && SocketState.CurrentGrade >= SocketState.MaximumGrade))
	{ return EMT2AccessorySocketApplyResult::Full; }
	const bool bSuccess = Roll <= (bOpening ? 50 : InsertionChance(SocketState.CurrentGrade));
	if (bSuccess)
	{
		if (bOpening) { ++SocketState.MaximumGrade; }
		else { SocketState.RemainingSeconds = Duration(++SocketState.CurrentGrade); SocketState.WearFraction = 0; }
		Targets[TargetSlot].AccessorySockets = SocketState;
	}
	if (--Slots[SourceSlot].Count == 0) { Slots[SourceSlot] = FMT2ItemSlot(); }
	ScheduleAccessoryWear();
	OnInventoryChanged.Broadcast();
	if (bTargetEquipped && bSuccess) { OnEquipmentChanged.Broadcast(); }
	Player->ForceNetUpdate();
	return bSuccess ? EMT2AccessorySocketApplyResult::Success : EMT2AccessorySocketApplyResult::Failed;
}
