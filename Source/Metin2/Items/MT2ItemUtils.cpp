/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/MT2ItemUtils.h"
#include "Items/Templates/MT2ItemAutoRecoveryTemplate.h"
#include "Items/Templates/MT2ItemCostumeTemplate.h"
#include "Items/Templates/MT2ItemEquipmentTemplate.h"
#include "Items/Templates/MT2ItemFishTemplate.h"
#include "Items/Templates/MT2ItemHairTemplate.h"
#include "Items/Templates/MT2ItemPickTemplate.h"
#include "Items/Templates/MT2ItemRodTemplate.h"
#include "Items/Templates/MT2ItemSkillBookTemplate.h"
#include "Items/Templates/MT2ItemSkillForgetTemplate.h"
#include "Items/Templates/MT2ItemPolymorphTemplate.h"


#include "Core/MT2VnumRegistrySubsystem.h"
#include "Core/MT2WorldUtils.h"
#include "Engine/GameInstance.h"
#include "Items/MT2ItemTemplate.h"
#include "Items/MT2ItemTypes.h"
#include "Items/Legacy/MT2LegacyItemRecord.h"
#include "Items/State/MT2LegacyQuestItemState.h"
#include "Items/State/MT2MetinEquipmentItemState.h"
#include "Items/State/MT2AccessoryItemState.h"
#include "Items/State/MT2PolymorphItemState.h"
#include "Items/Templates/MT2ItemBlendTemplate.h"
#include "Items/MT2AccessorySocketRules.h"
#include "Items/Templates/MT2ItemMetinStoneTemplate.h"
#include "Mobs/MT2Mob.h"

const UMT2ItemTemplate* MT2ItemUtils::ResolveTemplate(const UObject* WorldContextObject, int32 Vnum)
{
	if (Vnum <= 0)
	{
		return nullptr;
	}
	UGameInstance* GameInstance = MT2WorldUtils::GetGameInstance(WorldContextObject);
	UMT2VnumRegistrySubsystem* Registry = GameInstance
		? GameInstance->GetSubsystem<UMT2VnumRegistrySubsystem>() : nullptr;
	const TSubclassOf<UMT2ItemTemplate> TemplateClass = Registry
		? Registry->ResolveItemTemplateClass(Vnum) : nullptr;
	return TemplateClass.GetDefaultObject();
}

bool MT2ItemUtils::IsConsumableHairLook(const UMT2ItemTemplate* Template)
{
	if (Cast<UMT2ItemHairTemplate>(Template)) return true;
	const UMT2ItemCostumeTemplate* Costume = Cast<UMT2ItemCostumeTemplate>(Template);
	// The client proto classifies classic cosmetic/bonus hairstyles as costumes. They are
	// consumed looks in this project; separate, newer hair costumes remain wearable.
	return Costume && Costume->CostumeSubType == EMT2CostumeSubType::Hair && Costume->Vnum >= 73001 && Costume->Vnum <= 75619;
}

int32 MT2ItemUtils::GetInventorySize(const UObject* WorldContextObject, int32 Vnum)
{
	const UMT2ItemTemplate* Template = ResolveTemplate(WorldContextObject, Vnum);
	return Template ? FMath::Clamp(Template->InventorySize, 1, 3) : 1;
}

FText MT2ItemUtils::GetDisplayName(const UMT2ItemTemplate* Template, int32 FallbackVnum)
{
	if (!Template)
	{
		return FallbackVnum > 0 ? FText::AsNumber(FallbackVnum) : FText::GetEmpty();
	}
	return Template->DisplayName.IsEmpty()
		? FText::FromString(Template->InternalName) : Template->DisplayName;
}

bool MT2ItemUtils::HaveSameInstanceData(const FMT2ItemSlot& Left, const FMT2ItemSlot& Right)
{
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const int32 A = GetNumericSocket(Left, Index);
		const int32 B = GetNumericSocket(Right, Index);
		if (A != B) return false;
	}
	if (Left.Vnum != Right.Vnum ||
		Left.GetSkillVnum() != Right.GetSkillVnum() ||
		Left.RealtimeExpiresAt != Right.RealtimeExpiresAt ||
		Left.GetAutoRecoveryState().RemainingAmount != Right.GetAutoRecoveryState().RemainingAmount ||
		Left.GetAutoRecoveryState().MaximumAmount != Right.GetAutoRecoveryState().MaximumAmount ||
		Left.GetAutoRecoveryState().bActive != Right.GetAutoRecoveryState().bActive ||
		Left.GetMetinSockets().Num() != Right.GetMetinSockets().Num() ||
		!(Left.GetAccessorySockets() == Right.GetAccessorySockets()) ||
		Left.GetBonuses().Num() != Right.GetBonuses().Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Left.GetMetinSockets().Num(); ++Index)
	{
		const FMT2MetinSocket& A = Left.GetMetinSockets()[Index];
		const FMT2MetinSocket& B = Right.GetMetinSockets()[Index];
		if (A.Type != B.Type || A.Stone.Get() != B.Stone.Get() || A.Value != B.Value)
		{
			return false;
		}
	}
	for (int32 Index = 0; Index < Left.GetBonuses().Num(); ++Index)
	{
		const FMT2ItemBonus& A = Left.GetBonuses()[Index];
		const FMT2ItemBonus& B = Right.GetBonuses()[Index];
		if (A.Type != B.Type || A.Value != B.Value || A.Kind != B.Kind)
		{
			return false;
		}
	}
	return true;
}

bool MT2ItemUtils::UsesNumericSockets(const UMT2ItemTemplate* Template)
{
	return Template && !Cast<UMT2ItemEquipmentTemplate>(Template) && !Cast<UMT2ItemRodTemplate>(Template) &&
		!Cast<UMT2ItemPickTemplate>(Template) && !Cast<UMT2ItemFishTemplate>(Template) &&
		!Cast<UMT2ItemSkillBookTemplate>(Template) && !Cast<UMT2ItemSkillForgetTemplate>(Template) &&
		!Cast<UMT2ItemAutoRecoveryTemplate>(Template) && !Cast<UMT2ItemPolymorphTemplate>(Template);
}

int32 MT2ItemUtils::GetNumericSocket(const FMT2ItemInstanceData& Item, int32 Index)
{
	if (Index < 0 || Index >= 3) return 0;
	if (const auto* Legacy = Item.FindState<FMT2LegacyItemRecord>(); Legacy && Legacy->LegacySockets.IsValidIndex(Index)) return Legacy->LegacySockets[Index];
	if (const auto* State = Item.FindState<FMT2LegacyQuestItemState>()) return State->GetValue(Index);
	if (const auto* State = Item.FindState<FMT2FishingRodItemState>()) return Index == 0 ? State->Proficiency : Index == 2 ? State->BaitPower : 0;
	if (const auto* State = Item.FindState<FMT2PickaxeItemState>()) return Index == 0 ? State->Proficiency : 0;
	if (const auto* State = Item.FindState<FMT2FishItemState>()) return Index == 0 ? State->LengthTenths : 0;
	if (Item.FindState<FMT2SkillBookItemState>()) return Index == 0 ? Item.GetSkillVnum() : 0;
	if (Item.FindState<FMT2SkillForgetItemState>()) return Index == 0 ? Item.GetSkillVnum() : 0;
	if (const auto* State = Item.FindState<FMT2PolymorphItemState>())
	{
		if (Index == 0)
		{
			const AMT2Mob* Target = State->TargetMob ? State->TargetMob->GetDefaultObject<AMT2Mob>() : nullptr;
			return Target ? Target->GetMobVnum() : 0;
		}
		return Index == 1 ? State->RemainingPractice : State->SuccessBonus;
	}
	if (const auto* State = Item.FindState<FMT2AutoRecoveryItemState>()) return Index == 0 ? (State->bActive ? 1 : 0)
		: Index == 1 ? State->MaximumAmount - State->RemainingAmount : State->MaximumAmount;
	if (const auto* State = Item.FindState<FMT2AccessoryItemState>()) return Index == 0 ? State->Sockets.CurrentGrade
		: Index == 1 ? State->Sockets.MaximumGrade : State->Sockets.RemainingSeconds;
	if (const auto* State = Item.FindState<FMT2BlendItemState>()) return Index == 0
		? (State->UnresolvedLegacyBonus ? State->UnresolvedLegacyBonus : static_cast<int32>(State->BonusType))
		: Index == 1 ? State->Magnitude : State->DurationSeconds;
	// Existing Blueprint defaults and in-memory quest items can still use scratch Value fields.
	if (!Item.GetMetinSockets().IsValidIndex(Index)) return 0;
	const auto& Socket = Item.GetMetinSockets()[Index];
	const auto* Stone = Socket.Stone.GetDefaultObject();
	return Stone ? Stone->Vnum : Socket.Value;
}

bool MT2ItemUtils::SetNumericSocket(FMT2ItemInstanceData& Item, int32 Index, int32 Value)
{
	if (Index < 0 || Index >= 3) return false;
	if (auto* Legacy = Item.FindMutableState<FMT2LegacyItemRecord>())
	{
		if (Legacy->LegacySockets.Num() <= Index) Legacy->LegacySockets.SetNum(Index + 1);
		Legacy->LegacySockets[Index] = Value;
		return true;
	}
	if (auto* State = Item.FindMutableState<FMT2FishingRodItemState>())
	{ if (Index == 0) State->Proficiency = Value; else if (Index == 2) State->BaitPower = Value; else return false; return true; }
	if (auto* State = Item.FindMutableState<FMT2PickaxeItemState>()) { if (Index != 0) return false; State->Proficiency = Value; return true; }
	if (auto* State = Item.FindMutableState<FMT2FishItemState>()) { if (Index != 0) return false; State->LengthTenths = Value; return true; }
	if (Item.FindState<FMT2SkillBookItemState>()) { if (Index != 0) return false; Item.SetSkillVnum(Value); return true; }
	if (Item.FindState<FMT2SkillForgetItemState>()) { if (Index != 0) return false; Item.SetSkillVnum(Value); return true; }
	if (auto* State = Item.FindMutableState<FMT2PolymorphItemState>())
	{
		if (Index == 1) State->RemainingPractice = FMath::Max(0, Value);
		else if (Index == 2) State->SuccessBonus = Value;
		else
		{
			const int32 ExistingTargetVnum = GetNumericSocket(Item, 0);
			if (Value == ExistingTargetVnum) return true;

			// Legacy import/save codecs set socket zero as an integer VNUM and do not have a world
			// context to resolve that id into a mob class. Keep that compatibility payload opaque;
			// authoritative inventory edits resolve it to TargetMob before reaching this fallback.
			const int32 LegacyPractice = State->RemainingPractice;
			const int32 LegacySuccessBonus = State->SuccessBonus;
			Item.FamilyState.InitializeAs<FMT2LegacyQuestItemState>();
			auto& LegacyState = Item.EditState<FMT2LegacyQuestItemState>();
			LegacyState.SetValue(0, Value);
			LegacyState.SetValue(1, LegacyPractice);
			LegacyState.SetValue(2, LegacySuccessBonus);
		}
		return true;
	}
	if (auto* State = Item.FindMutableState<FMT2AutoRecoveryItemState>())
	{
		if (Index == 0) State->bActive = Value != 0 && State->RemainingAmount > 0;
		else if (Index == 1) State->RemainingAmount = static_cast<int32>(FMath::Clamp<int64>(int64(State->MaximumAmount) - Value, 0, State->MaximumAmount));
		else
		{
			const int64 Used = int64(State->MaximumAmount) - State->RemainingAmount;
			State->MaximumAmount = FMath::Max(Value, 0);
			State->RemainingAmount = static_cast<int32>(FMath::Clamp<int64>(int64(State->MaximumAmount) - Used, 0, State->MaximumAmount));
		}
		if (State->RemainingAmount == 0) State->bActive = false;
		return true;
	}
	if (auto* State = Item.FindMutableState<FMT2AccessoryItemState>())
	{
		if (Index == 0) State->Sockets.CurrentGrade = Value;
		else if (Index == 1) State->Sockets.MaximumGrade = Value;
		else State->Sockets.RemainingSeconds = Value;
		State->Sockets = MT2AccessorySockets::Normalize(State->Sockets);
		return true;
	}
	if (auto* State = Item.FindMutableState<FMT2BlendItemState>())
	{
		if (Index == 0)
		{
			State->UnresolvedLegacyBonus = MT2ItemSemanticTypes::TryDecodeBonus(Value, State->BonusType) ? 0 : Value;
		}
		else if (Index == 1) State->Magnitude = Value;
		else State->DurationSeconds = Value;
		return true;
	}
	if (Item.FindState<FMT2MetinEquipmentItemState>())
	{
		if (Item.GetMetinSockets().Num() <= Index) Item.EditMetinSockets().SetNum(Index + 1);
		Item.EditMetinSockets()[Index].Value = Value;
		return true;
	}
	if (!Item.FindState<FMT2LegacyQuestItemState>())
	{
		const auto Bonuses = Item.GetBonuses();
		const int32 Values[] = {GetNumericSocket(Item, 0), GetNumericSocket(Item, 1), GetNumericSocket(Item, 2)};
		Item.FamilyState.InitializeAs<FMT2LegacyQuestItemState>();
		auto& State = Item.EditState<FMT2LegacyQuestItemState>();
		State.Bonuses = Bonuses;
		for (int32 I = 0; I < 3; ++I) State.SetValue(I, Values[I]);
	}
	Item.EditState<FMT2LegacyQuestItemState>().SetValue(Index, Value);
	return true;
}

void MT2ItemUtils::NormalizeNumericSockets(FMT2ItemInstanceData& Item, const UMT2ItemTemplate* Template)
{
	if (!Template) return; // Keep opaque records intact until their definition becomes available.
	const bool bLegacy = Item.FindState<FMT2LegacyItemRecord>() != nullptr;
	const int32 Values[] = {GetNumericSocket(Item, 0), GetNumericSocket(Item, 1), GetNumericSocket(Item, 2)};
	if (Cast<UMT2ItemRodTemplate>(Template))
	{
		if (!Item.FindState<FMT2FishingRodItemState>())
		{ Item.FamilyState.InitializeAs<FMT2FishingRodItemState>(); Item.EditRodState().Proficiency = Values[0]; Item.EditRodState().BaitPower = Values[2]; }
	}
	else if (Cast<UMT2ItemPickTemplate>(Template))
	{
		if (!Item.FindState<FMT2PickaxeItemState>())
		{ Item.FamilyState.InitializeAs<FMT2PickaxeItemState>(); Item.EditPickaxeState().Proficiency = Values[0]; }
	}
	else if (Cast<UMT2ItemFishTemplate>(Template))
	{
		if (!Item.FindState<FMT2FishItemState>())
		{ Item.FamilyState.InitializeAs<FMT2FishItemState>(); Item.EditFishState().LengthTenths = Values[0]; }
	}
	else if (Cast<UMT2ItemBlendTemplate>(Template))
	{
		if (!Item.FindState<FMT2BlendItemState>())
		{ Item.FamilyState.InitializeAs<FMT2BlendItemState>(); for (int32 I = 0; I < 3; ++I) SetNumericSocket(Item, I, Values[I]); }
	}
	else if (Cast<UMT2ItemSkillBookTemplate>(Template))
	{
		if (!Item.FindState<FMT2SkillBookItemState>())
		{ const int32 Skill = Item.GetSkillVnum(); Item.FamilyState.InitializeAs<FMT2SkillBookItemState>(); Item.SetSkillVnum(Skill); }
	}
	else if (Cast<UMT2ItemSkillForgetTemplate>(Template))
	{
		if (!Item.FindState<FMT2SkillForgetItemState>())
		{ const int32 Skill = Item.GetSkillVnum(); Item.FamilyState.InitializeAs<FMT2SkillForgetItemState>(); Item.SetSkillVnum(Skill); }
	}
	else if (Cast<UMT2ItemAutoRecoveryTemplate>(Template))
	{
		if (!Item.FindState<FMT2AutoRecoveryItemState>())
		{ const auto State = Item.GetAutoRecoveryState(); Item.FamilyState.InitializeAs<FMT2AutoRecoveryItemState>(State); }
	}
	else if (Cast<UMT2ItemEquipmentTemplate>(Template))
	{
		if (bLegacy || !Item.FamilyState.IsValid() || Item.FamilyState.GetScriptStruct() == FMT2EquipmentItemState::StaticStruct())
		{
			const auto Bonuses = Item.GetBonuses(); const auto Sockets = Item.GetMetinSockets(); const auto Accessory = Item.GetAccessorySockets();
			if (MT2AccessorySockets::IsAccessory(Template))
			{ Item.FamilyState.InitializeAs<FMT2AccessoryItemState>(); Item.EditAccessorySockets() = Accessory; }
			else { Item.FamilyState.InitializeAs<FMT2MetinEquipmentItemState>(); Item.EditMetinSockets() = Sockets; }
			Item.EditBonuses() = Bonuses;
		}
	}
	else if (UsesNumericSockets(Template) && (bLegacy || Item.FindState<FMT2MetinEquipmentItemState>()))
	{
		const auto Bonuses = Item.GetBonuses();
		Item.FamilyState.InitializeAs<FMT2LegacyQuestItemState>();
		Item.EditBonuses() = Bonuses;
		for (int32 I = 0; I < 3; ++I) SetNumericSocket(Item, I, Values[I]);
	}
}

TMap<EMT2ItemBonusType, int32> MT2ItemUtils::GetAmplifiedEquipmentAttributes(
	const TArray<FMT2ItemSlot>& Equipment, const TArray<EMT2ItemWearFlag>& WearSlots, int32 Percent)
{
	TMap<EMT2ItemBonusType, int64> Totals;
	TMap<EMT2ItemBonusType, int32> Result;
	if (Percent <= 0) return Result;
	TSet<EMT2ItemWearFlag> Visited;
	for (const auto WearSlot : WearSlots)
	{
		const int32 Index = static_cast<int32>(WearSlot);
		if (Visited.Contains(WearSlot) || !Equipment.IsValidIndex(Index) || Equipment[Index].IsEmpty()) continue;
		Visited.Add(WearSlot);
		for (const auto& Bonus : Equipment[Index].GetBonuses())
			if (Bonus.IsValid()) Totals.FindOrAdd(Bonus.Type) += Bonus.Value;
	}
	for (const auto& Total : Totals)
	{
		// Legacy CBuffOnAttributes rounds after summing each APPLY across the eligible equipment.
		const int64 Value = Total.Value * FMath::Clamp(Percent, 0, 255) / 100;
		if (Value != 0) Result.Add(Total.Key, static_cast<int32>(FMath::Clamp<int64>(Value, MIN_int32, MAX_int32)));
	}
	return Result;
}
