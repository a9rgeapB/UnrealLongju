#include "Items/Types/MT2ItemInstanceData.h"

#include "Items/State/MT2EquipmentItemState.h"
#include "Items/State/MT2MetinEquipmentItemState.h"
#include "Items/State/MT2AccessoryItemState.h"
#include "Items/State/MT2AutoRecoveryItemState.h"
#include "Items/State/MT2FishingRodItemState.h"
#include "Items/State/MT2PickaxeItemState.h"
#include "Items/State/MT2FishItemState.h"
#include "Items/State/MT2BlendItemState.h"
#include "Items/State/MT2SkillBookItemState.h"
#include "Items/State/MT2PolymorphItemState.h"
#include "Skills/MT2SkillDefinition.h"
#include "Skills/MT2SkillSet.h"
#include "Items/Legacy/MT2LegacyItemRecord.h"
#include "Serialization/CustomVersion.h"

namespace
{
	const FGuid ItemStateVersion(0x59AE0BBC, 0xCDF14E98, 0x81D83AAF, 0x59FDD57B);
	FCustomVersionRegistration RegisterItemStateVersion(ItemStateVersion, 1, TEXT("MT2ItemFamilyState"));

	void ReadLegacyRecord(FMT2ItemInstanceData& Data, const FMT2LegacyItemRecord& Legacy)
	{
		Data.Count = Legacy.Count;
		Data.RealtimeExpiresAt = Legacy.RealtimeExpiresAt;
		Data.bNewlyAcquired = Legacy.bNewlyAcquired;
		Data.FamilyState.InitializeAs<FMT2LegacyItemRecord>(Legacy);
	}

	template<typename Item, typename Legacy>
	bool SerializeItem(Item& Data, FArchive& Ar)
	{
		Ar.UsingCustomVersion(ItemStateVersion);
		if (Ar.IsLoading() && Ar.IsPersistent() && Ar.CustomVer(ItemStateVersion) < 1)
		{
			Legacy Record;
			if (const auto* Existing = Data.template FindState<FMT2LegacyItemRecord>())
				static_cast<FMT2LegacyItemRecord&>(Record) = *Existing;
			Legacy::StaticStruct()->SerializeItem(Ar, &Record, nullptr);
			ReadLegacyRecord(Data, Record);
			if constexpr (std::is_same_v<Item, FMT2ItemSlot>) Data.Vnum = Record.Vnum;
		}
		else Item::StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(&Data), nullptr, nullptr);
		return true;
	}

	template<typename Item, typename Legacy>
	bool ImportLegacyText(Item& Data, const TCHAR*& Buffer, int32 Flags, UObject* Owner, FOutputDevice* Errors)
	{
		const FString Text(Buffer);
		if (Text.Contains(TEXT("FamilyState=")) ||
			!(Text.Contains(TEXT("LegacySockets=")) || Text.Contains(TEXT("MetinSockets=")) ||
			  Text.Contains(TEXT("SkillVnum=")) || Text.Contains(TEXT("Bonuses=")))) return false;
		Legacy Record;
		const TCHAR* End = Legacy::StaticStruct()->ImportText(Buffer, &Record, Owner, Flags, Errors, TEXT("legacy item"));
		if (!End) return false;
		ReadLegacyRecord(Data, Record);
		if constexpr (std::is_same_v<Item, FMT2ItemSlot>) Data.Vnum = Record.Vnum;
		Buffer = End;
		return true;
	}
}

bool FMT2ItemInstanceData::Serialize(FArchive& Ar) { return SerializeItem<FMT2ItemInstanceData, FMT2LegacyItemRecord>(*this, Ar); }
bool FMT2ItemSlot::Serialize(FArchive& Ar) { return SerializeItem<FMT2ItemSlot, FMT2LegacyItemSlotRecord>(*this, Ar); }
bool FMT2ItemInstanceData::ImportTextItem(const TCHAR*& Buffer, int32 Flags, UObject* Owner, FOutputDevice* Errors)
{ return ImportLegacyText<FMT2ItemInstanceData, FMT2LegacyItemRecord>(*this, Buffer, Flags, Owner, Errors); }
bool FMT2ItemSlot::ImportTextItem(const TCHAR*& Buffer, int32 Flags, UObject* Owner, FOutputDevice* Errors)
{ return ImportLegacyText<FMT2ItemSlot, FMT2LegacyItemSlotRecord>(*this, Buffer, Flags, Owner, Errors); }

const TArray<FMT2ItemBonus>& FMT2ItemInstanceData::GetBonuses() const
{
	if (const auto* Legacy = FindState<FMT2LegacyItemRecord>()) return Legacy->Bonuses;
	return ReadState<FMT2EquipmentItemState>().Bonuses;
}

bool FMT2ItemInstanceData::SupportsBonuses() const
{
	return !FamilyState.IsValid() || FindState<FMT2EquipmentItemState>() || FindState<FMT2LegacyItemRecord>();
}

TArray<FMT2ItemBonus>& FMT2ItemInstanceData::EditBonuses()
{
	if (auto* Legacy = FindMutableState<FMT2LegacyItemRecord>()) return Legacy->Bonuses;
	return EditState<FMT2EquipmentItemState>().Bonuses;
}

const TArray<FMT2MetinSocket>& FMT2ItemInstanceData::GetMetinSockets() const
{
	if (const auto* Legacy = FindState<FMT2LegacyItemRecord>()) return Legacy->MetinSockets;
	return ReadState<FMT2MetinEquipmentItemState>().Sockets;
}

TArray<FMT2MetinSocket>& FMT2ItemInstanceData::EditMetinSockets()
{
	if (auto* Legacy = FindMutableState<FMT2LegacyItemRecord>()) return Legacy->MetinSockets;
	if (!FindState<FMT2MetinEquipmentItemState>())
	{
		const auto Bonuses = GetBonuses();
		FamilyState.InitializeAs<FMT2MetinEquipmentItemState>();
		FamilyState.GetMutable<FMT2MetinEquipmentItemState>().Bonuses = Bonuses;
	}
	return FamilyState.GetMutable<FMT2MetinEquipmentItemState>().Sockets;
}

const FMT2AccessorySocketState& FMT2ItemInstanceData::GetAccessorySockets() const
{
	if (const auto* Legacy = FindState<FMT2LegacyItemRecord>()) return Legacy->AccessorySockets;
	return ReadState<FMT2AccessoryItemState>().Sockets;
}

FMT2AccessorySocketState& FMT2ItemInstanceData::EditAccessorySockets()
{
	if (auto* Legacy = FindMutableState<FMT2LegacyItemRecord>()) return Legacy->AccessorySockets;
	if (!FindState<FMT2AccessoryItemState>())
	{
		const auto Bonuses = GetBonuses();
		FamilyState.InitializeAs<FMT2AccessoryItemState>();
		FamilyState.GetMutable<FMT2AccessoryItemState>().Bonuses = Bonuses;
	}
	return FamilyState.GetMutable<FMT2AccessoryItemState>().Sockets;
}

#define MT2_ITEM_STATE_ACCESSORS(Name, Type) \
const Type& FMT2ItemInstanceData::Get##Name() const { return ReadState<Type>(); } \
Type& FMT2ItemInstanceData::Edit##Name() { return EditState<Type>(); }

MT2_ITEM_STATE_ACCESSORS(RodState, FMT2FishingRodItemState)
MT2_ITEM_STATE_ACCESSORS(PickaxeState, FMT2PickaxeItemState)
MT2_ITEM_STATE_ACCESSORS(FishState, FMT2FishItemState)
MT2_ITEM_STATE_ACCESSORS(BlendState, FMT2BlendItemState)
MT2_ITEM_STATE_ACCESSORS(SkillBookState, FMT2SkillBookItemState)
MT2_ITEM_STATE_ACCESSORS(SkillForgetState, FMT2SkillForgetItemState)
MT2_ITEM_STATE_ACCESSORS(PolymorphState, FMT2PolymorphItemState)
#undef MT2_ITEM_STATE_ACCESSORS

FMT2AutoRecoveryItemState FMT2ItemInstanceData::GetAutoRecoveryState() const
{
	if (const auto* Legacy = FindState<FMT2LegacyItemRecord>())
	{
		FMT2AutoRecoveryItemState Result;
		Result.RemainingAmount = Legacy->AutoRecoveryRemainingAmount;
		Result.MaximumAmount = Legacy->AutoRecoveryMaximumAmount;
		Result.bActive = Legacy->bAutoRecoveryActive;
		return Result;
	}
	return ReadState<FMT2AutoRecoveryItemState>();
}

FMT2AutoRecoveryItemState& FMT2ItemInstanceData::EditAutoRecoveryState()
{
	if (FindState<FMT2LegacyItemRecord>())
	{
		const auto State = GetAutoRecoveryState();
		FamilyState.InitializeAs<FMT2AutoRecoveryItemState>(State);
	}
	return EditState<FMT2AutoRecoveryItemState>();
}

int32 FMT2ItemInstanceData::GetSkillVnum() const
{
	if (const auto* Legacy = FindState<FMT2LegacyItemRecord>()) return Legacy->SkillVnum;
	if (const auto* State = FindState<FMT2SkillForgetItemState>())
	{
		return State->Skill ? State->Skill->Vnum : State->UnresolvedLegacySkill;
	}
	const auto* Skill = GetSkillBookState().Skill.Get();
	return Skill ? Skill->Vnum : GetSkillBookState().UnresolvedLegacySkill;
}

void FMT2ItemInstanceData::SetSkillVnum(int32 Vnum)
{
	if (auto* Legacy = FindMutableState<FMT2LegacyItemRecord>()) { Legacy->SkillVnum = Vnum; return; }
	if (auto* SkillForget = FindMutableState<FMT2SkillForgetItemState>())
	{
		SkillForget->Skill = UMT2SkillSet::FindSkillAcrossSets(Vnum);
		SkillForget->UnresolvedLegacySkill = SkillForget->Skill ? 0 : Vnum;
		return;
	}
	auto& State = EditSkillBookState();
	State.Skill = UMT2SkillSet::FindSkillAcrossSets(Vnum);
	State.UnresolvedLegacySkill = State.Skill ? 0 : Vnum;
}
