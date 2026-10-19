/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/Templates/MT2ItemEquipmentTemplate.h"
#include "Items/Templates/MT2ItemMetinStoneTemplate.h"
#include "Items/Templates/MT2ItemArmorTemplate.h"
#include "Items/Templates/MT2ItemCostumeTemplate.h"
#include "Items/Templates/MT2ItemWeaponTemplate.h"
#include "Items/Templates/MT2ItemMetinRemovalTemplate.h"
#include "Items/Templates/MT2ItemSocketCleanerTemplate.h"
#include "Items/Templates/MT2ItemUseTemplate.h"
#include "Items/Types/MT2ItemActionResults.h"

#include "Items/MT2InventoryComponent.h"
#include "Items/MT2ItemUtils.h"
#include "Items/MT2WorldItem.h"

#include "Items/MT2ItemTemplate.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Components/MT2HealthComponent.h"
#include "Config/MT2GameplaySettings.h"
#include "Items/MT2AccessorySocketRules.h"
#include "Player/MT2PlayerState.h"
#include "Trade/MT2TradeComponent.h"

EMT2MetinSocketApplyResult UMT2InventoryComponent::ApplyMetinStone(
	int32 SourceSlot, int32 TargetSlot, bool bTargetEquipped)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Slots.IsValidIndex(SourceSlot) ||
		Slots[SourceSlot].IsEmpty())
	{
		return EMT2MetinSocketApplyResult::Invalid;
	}

	TArray<FMT2ItemSlot>& TargetContainer = bTargetEquipped ? Equipment : Slots;
	if (!TargetContainer.IsValidIndex(TargetSlot) || TargetContainer[TargetSlot].IsEmpty() ||
		(!bTargetEquipped && SourceSlot == TargetSlot))
	{
		return EMT2MetinSocketApplyResult::Invalid;
	}

	const UMT2ItemMetinStoneTemplate* Stone =
		Cast<UMT2ItemMetinStoneTemplate>(ResolveTemplate(Slots[SourceSlot].Vnum));
	const UMT2ItemEquipmentTemplate* EquipmentTemplate =
		Cast<UMT2ItemEquipmentTemplate>(ResolveTemplate(TargetContainer[TargetSlot].Vnum));
	if (!Stone || !EquipmentTemplate)
	{
		return EMT2MetinSocketApplyResult::Incompatible;
	}
	if ((Stone->CompatibleWearFlags & EquipmentTemplate->WearFlags) == 0)
	{
		return EMT2MetinSocketApplyResult::Incompatible;
	}

	FMT2ItemSlot& Target = TargetContainer[TargetSlot];
	const auto DefaultData = EquipmentTemplate->MakeInstanceData();
	if (Target.GetMetinSockets().IsEmpty() &&
		!DefaultData.GetMetinSockets().IsEmpty())
	{
		Target.EditMetinSockets() = DefaultData.GetMetinSockets();
	}
	if (Target.GetMetinSockets().IsEmpty())
	{
		for (int32 Index = 0;
			Index < FMath::Clamp(EquipmentTemplate->DefaultSilverSocketCount, 0, 3); ++Index)
		{
			Target.EditMetinSockets().AddDefaulted_GetRef().Type = EMT2MetinSocketType::Silver;
		}
		for (int32 Index = 0;
			Index < FMath::Clamp(EquipmentTemplate->DefaultGoldSocketCount, 0,
				3 - Target.GetMetinSockets().Num()); ++Index)
		{
			Target.EditMetinSockets().AddDefaulted_GetRef().Type = EMT2MetinSocketType::Gold;
		}
	}
	const bool bHasCategory = Target.GetMetinSockets().ContainsByPredicate(
		[Stone](const FMT2MetinSocket& Socket)
	{
		const UMT2ItemMetinStoneTemplate* Existing = Socket.Stone.GetDefaultObject();
		return Existing && Existing->Vnum != 28960 &&
			Existing->ConflictsWith(*Stone);
	});
	if (bHasCategory)
	{
		return EMT2MetinSocketApplyResult::DuplicateStone;
	}

	FMT2MetinSocket* OpenSocket = Target.EditMetinSockets().FindByPredicate(
		[Stone](const FMT2MetinSocket& Socket)
		{
			const int32 SocketGrade = Socket.Type == EMT2MetinSocketType::Gold ? 2 : 1;
			return !Socket.Stone && SocketGrade >= static_cast<int32>(Stone->RequiredSocketGrade);
		});
	if (!OpenSocket)
	{
		return EMT2MetinSocketApplyResult::NoSocket;
	}

	const TSubclassOf<UMT2ItemMetinStoneTemplate> StoneClass(
		const_cast<UClass*>(Stone->GetClass()));
	if (--Slots[SourceSlot].Count <= 0)
	{
		Slots[SourceSlot] = FMT2ItemSlot();
	}

	const bool bSucceeded = FMath::FRandRange(0.0f, 100.0f) <=
		FMath::Clamp(Stone->AttachmentSuccessPercent, 0.0f, 100.0f);
	if (bSucceeded)
	{
		OpenSocket->Stone = StoneClass;
	}
	else
	{
		const UMT2ItemMetinStoneTemplate* Broken =
			Cast<UMT2ItemMetinStoneTemplate>(ResolveTemplate(28960));
		OpenSocket->Stone = Broken
			? TSubclassOf<UMT2ItemMetinStoneTemplate>(const_cast<UClass*>(Broken->GetClass()))
			: nullptr;
	}

	OnInventoryChanged.Broadcast();
	if (bTargetEquipped)
	{
		OnEquipmentChanged.Broadcast();
	}
	return bSucceeded
		? EMT2MetinSocketApplyResult::Success
		: EMT2MetinSocketApplyResult::Failed;
}

EMT2MetinSocketItemResult UMT2InventoryComponent::ApplyMetinSocketItem(
	int32 SourceSlot, int32 TargetSlot, bool bTargetEquipped,
	const FMT2ItemSlot& ExpectedSource, const FMT2ItemSlot& ExpectedTarget)
{
	AMT2PlayerCharacter* Character = Cast<AMT2PlayerCharacter>(GetOwner());
	AMT2PlayerState* PlayerState = Character
		? Character->GetPlayerState<AMT2PlayerState>() : nullptr;
	const UMT2HealthComponent* Health = Character ? Character->GetHealthComponent() : nullptr;
	const UMT2GameplaySettings& Settings = UMT2GameplaySettings::Get();
	UClass* BrokenStoneClass = Settings.BrokenMetinStoneTemplate.LoadSynchronous();
	const UMT2ItemMetinStoneTemplate* BrokenStone = BrokenStoneClass
		? Cast<UMT2ItemMetinStoneTemplate>(BrokenStoneClass->GetDefaultObject()) : nullptr;
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValid(Character) ||
		!Health || Health->IsDead() || !PlayerState || !IsValid(BrokenStone))
	{
		return BrokenStone ? EMT2MetinSocketItemResult::Invalid
			: EMT2MetinSocketItemResult::MissingBrokenStoneTemplate;
	}
	if (const UMT2TradeComponent* Trade = PlayerState->GetTradeComponent();
		Trade && Trade->IsTrading())
	{
		return EMT2MetinSocketItemResult::Invalid;
	}
	if (!Slots.IsValidIndex(SourceSlot) || Slots[SourceSlot].IsEmpty() ||
		Slots[SourceSlot].Count != ExpectedSource.Count ||
		!MT2ItemUtils::HaveSameInstanceData(Slots[SourceSlot], ExpectedSource))
	{
		return EMT2MetinSocketItemResult::Invalid;
	}
	TArray<FMT2ItemSlot>& TargetItems = bTargetEquipped ? Equipment : Slots;
	if (!TargetItems.IsValidIndex(TargetSlot) || TargetItems[TargetSlot].IsEmpty() ||
		(!bTargetEquipped && SourceSlot == TargetSlot) ||
		TargetItems[TargetSlot].Count != ExpectedTarget.Count ||
		!MT2ItemUtils::HaveSameInstanceData(TargetItems[TargetSlot], ExpectedTarget))
	{
		return EMT2MetinSocketItemResult::Invalid;
	}

	const UMT2ItemUseTemplate* UseItem = Cast<UMT2ItemUseTemplate>(
		ResolveTemplate(ExpectedSource.Vnum));
	if (!UseItem)
	{
		return EMT2MetinSocketItemResult::Incompatible;
	}
	const UMT2ItemMetinRemovalTemplate* RemovalTemplate =
		Cast<UMT2ItemMetinRemovalTemplate>(UseItem);
	const UMT2ItemSocketCleanerTemplate* CleanerTemplate =
		Cast<UMT2ItemSocketCleanerTemplate>(UseItem);
	const UClass* ConfiguredSingleRemovalClass =
		Settings.SingleMetinRemovalItemTemplate.LoadSynchronous();
	// The project setting is the authoritative single-scroll identity. It must override an
	// imported Detachment subtype, which otherwise represents the all-stones scroll family.
	const bool bSingleRemovalItem =
		ConfiguredSingleRemovalClass == UseItem->GetClass() ||
		(RemovalTemplate && RemovalTemplate->RemovalMode == EMT2MetinRemovalMode::SingleStone);
	const bool bRemoveAll = !bSingleRemovalItem &&
		(RemovalTemplate || UseItem->UseSubType == EMT2UseSubType::Detachment);
	const bool bCleanSockets = CleanerTemplate ||
		UseItem->UseSubType == EMT2UseSubType::CleanSocket;
	if (!bSingleRemovalItem && !bRemoveAll && !bCleanSockets)
	{
		return EMT2MetinSocketItemResult::Incompatible;
	}

	const UMT2ItemTemplate* TargetTemplate = ResolveTemplate(ExpectedTarget.Vnum);
	const UMT2ItemWeaponTemplate* Weapon = Cast<UMT2ItemWeaponTemplate>(TargetTemplate);
	const UMT2ItemArmorTemplate* Armor = Cast<UMT2ItemArmorTemplate>(TargetTemplate);
	const bool bAccessoryArmor = Armor &&
		(Armor->ArmorSubType == EMT2ArmorSubType::Ear ||
		 Armor->ArmorSubType == EMT2ArmorSubType::Neck ||
		 Armor->ArmorSubType == EMT2ArmorSubType::Wrist);
	const bool bMetinEquipment = (Weapon || (Armor && !bAccessoryArmor)) &&
		!Cast<UMT2ItemCostumeTemplate>(TargetTemplate) &&
		!MT2AccessorySockets::IsAccessory(TargetTemplate);
	if (!bMetinEquipment || TargetItems[TargetSlot].GetMetinSockets().IsEmpty())
	{
		return EMT2MetinSocketItemResult::Incompatible;
	}
	if (bRemoveAll && (!TargetTemplate->HasItemFlag(
			1 << static_cast<int32>(EMT2ItemFlag::Refineable))))
	{
		return EMT2MetinSocketItemResult::Incompatible;
	}

	// Work on a copy until all return items are safely in inventory or on the ground. Failed
	// operations must not partially rewrite the replicated target sockets.
	TArray<FMT2MetinSocket> Sockets = TargetItems[TargetSlot].GetMetinSockets();
	const TSubclassOf<UMT2ItemMetinStoneTemplate> BrokenClassRef(BrokenStoneClass);
	const int32 BrokenStoneVnum = BrokenStone->Vnum;
	const auto GetSocketVnum = [](const FMT2MetinSocket& Socket)
	{
		const UMT2ItemMetinStoneTemplate* Stone = Socket.Stone.GetDefaultObject();
		return Stone ? Stone->Vnum : Socket.Value;
	};
	const auto IsBroken = [&BrokenClassRef, BrokenStoneVnum, &GetSocketVnum](
		const FMT2MetinSocket& Socket)
	{
		return Socket.Stone == BrokenClassRef ||
			(!Socket.Stone && GetSocketVnum(Socket) == BrokenStoneVnum);
	};
	const auto IsRemovableStone = [BrokenStoneVnum, &GetSocketVnum](
		const FMT2MetinSocket& Socket)
	{
		const int32 SocketVnum = GetSocketVnum(Socket);
		return SocketVnum > 2 && SocketVnum != BrokenStoneVnum;
	};

	int32 SingleStoneIndex = INDEX_NONE;
	TArray<int32> StoneIndices;
	if (bSingleRemovalItem)
	{
		for (int32 Index = Sockets.Num() - 1; Index >= 0; --Index)
		{
			if (IsRemovableStone(Sockets[Index]))
			{
				SingleStoneIndex = Index;
				break;
			}
		}
		if (SingleStoneIndex == INDEX_NONE)
		{
			return EMT2MetinSocketItemResult::NoRemovableStones;
		}
		StoneIndices.Add(SingleStoneIndex);
	}
	else if (bRemoveAll)
	{
		for (int32 Index = 0; Index < Sockets.Num(); ++Index)
		{
			if (IsRemovableStone(Sockets[Index]))
			{
				StoneIndices.Add(Index);
			}
		}
		if (StoneIndices.IsEmpty())
		{
			return EMT2MetinSocketItemResult::NoRemovableStones;
		}
	}
	else if (!Sockets.ContainsByPredicate(IsBroken))
	{
		return EMT2MetinSocketItemResult::NoBrokenSockets;
	}

	TArray<FMT2ItemSlot> InventoryBefore = Slots;
	TArray<AMT2WorldItem*> OverflowDrops;
	TArray<TPair<FMT2ItemSlot, int32>> InventoryRewards;
	const auto RollBackRewards = [this, &InventoryBefore, &OverflowDrops, &InventoryRewards]()
	{
		Slots = MoveTemp(InventoryBefore);
		for (AMT2WorldItem* SpawnedDrop : OverflowDrops)
		{
			if (IsValid(SpawnedDrop)) { SpawnedDrop->Destroy(); }
		}
		if (!InventoryRewards.IsEmpty()) { OnInventoryChanged.Broadcast(); }
	};
	const FString OwnerId = AMT2WorldItem::ResolvePlayerIdentity(PlayerState);
	const FString OwnerName = PlayerState->GetCharacterName();
	for (const int32 SocketIndex : StoneIndices)
	{
		const int32 StoneVnum = GetSocketVnum(Sockets[SocketIndex]);
		const UMT2ItemTemplate* StoneItemTemplate = ResolveTemplate(StoneVnum);
		if (!StoneItemTemplate)
		{
			RollBackRewards();
			return EMT2MetinSocketItemResult::CannotReturnStone;
		}
		FMT2ItemSlot Reward(StoneVnum, StoneItemTemplate->MakeInstanceData());
		const int32 AddedCount = AddItemSlotPartialInternal(Reward, false);
		if (AddedCount > 0)
		{
			FMT2ItemSlot AddedReward = Reward;
			AddedReward.Count = AddedCount;
			InventoryRewards.Emplace(AddedReward, AddedCount);
		}
		Reward.Count -= AddedCount;
		if (Reward.Count > 0)
		{
			AMT2WorldItem* Drop = AMT2WorldItem::SpawnWorldItemInstance(
				GetWorld(), Character->GetActorLocation(), Reward, Character,
				OwnerId, OwnerName, Settings.LootCrateOverflowOwnershipSeconds);
			if (!Drop)
			{
				RollBackRewards();
				return EMT2MetinSocketItemResult::CannotReturnStone;
			}
			OverflowDrops.Add(Drop);
		}
	}

	for (const TPair<FMT2ItemSlot, int32>& Reward : InventoryRewards)
	{
		NotifyItemReceived(Reward.Key, Reward.Value);
	}
	if (bSingleRemovalItem)
	{
		Sockets[SingleStoneIndex].Stone = nullptr;
		Sockets[SingleStoneIndex].Value = 0;
	}
	else if (bRemoveAll)
	{
		for (const int32 SocketIndex : StoneIndices)
		{
			Sockets[SocketIndex].Stone = BrokenClassRef;
			Sockets[SocketIndex].Value = 0;
		}
	}
	else
	{
		int32 WriteIndex = 0;
		for (int32 ReadIndex = 0; ReadIndex < Sockets.Num(); ++ReadIndex)
		{
			const FMT2MetinSocket& SourceSocket = Sockets[ReadIndex];
			if (IsBroken(SourceSocket) ||
				(!SourceSocket.Stone && GetSocketVnum(SourceSocket) <= 2))
			{
				continue;
			}
			if (WriteIndex != ReadIndex)
			{
				// Socket grade belongs to its physical slot. Move only the contents so a
				// surviving stone cannot accidentally be demoted by compaction.
				Sockets[WriteIndex].Stone = SourceSocket.Stone;
				Sockets[WriteIndex].Value = SourceSocket.Value;
			}
			++WriteIndex;
		}
		for (int32 Index = WriteIndex; Index < Sockets.Num(); ++Index)
		{
			Sockets[Index].Stone = nullptr;
			Sockets[Index].Value = 0;
		}
	}

	if (--Slots[SourceSlot].Count <= 0)
	{
		Slots[SourceSlot] = FMT2ItemSlot();
	}
	TargetItems[TargetSlot].EditMetinSockets() = MoveTemp(Sockets);
	OnInventoryChanged.Broadcast();
	if (bTargetEquipped) { OnEquipmentChanged.Broadcast(); }
	GetOwner()->ForceNetUpdate();
	return EMT2MetinSocketItemResult::Success;
}
