/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/Templates/MT2ItemSkillForgetTemplate.h"

#include "Characters/MT2PlayerCharacter.h"
#include "Config/MT2GameplaySettings.h"
#include "Items/MT2InventoryComponent.h"
#include "Player/MT2PlayerState.h"
#include "Player/MT2PlayerController.h"
#include "Skills/MT2SkillComponent.h"
#include "Skills/MT2SkillDefinition.h"
#include "Skills/MT2SkillSet.h"

void UMT2ItemSkillForgetTemplate::InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const
{
	FMT2SkillForgetItemState State = DefaultSkillForgetState;
	if (Skill) State.Skill = Skill;
	Data.FamilyState.InitializeAs<FMT2SkillForgetItemState>(State);
}

void UMT2ItemSkillForgetTemplate::CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data)
{
	if (const FMT2SkillForgetItemState* State = Data.FindState<FMT2SkillForgetItemState>())
		DefaultSkillForgetState = *State;
}

void UMT2ItemSkillForgetTemplate::InitializeGeneratedInstance(FMT2ItemInstanceData& Data) const
{
	if (Data.GetSkillVnum() > 0) return;
	if (Skill)
	{
		Data.SetSkillVnum(Skill->Vnum);
		return;
	}

	// Legacy item_manager.cpp rolls 70037 from skills 1..111 and 70055 from 112..119.
	const UMT2GameplaySettings& Settings = UMT2GameplaySettings::Get();
	const UClass* ItemClass = GetClass();
	const bool bIsStandardSkillForgetItem = Settings.StandardSkillForgetItemTemplate.Get() == ItemClass;
	const bool bIsLegacyGeneralSkillForgetItem = Settings.LegacyGeneralSkillForgetItemTemplate.Get() == ItemClass;
	const bool bIsAntiSkillForgetItem = Settings.AntiSkillForgetItemTemplate.Get() == ItemClass;
	if (!bIsStandardSkillForgetItem && !bIsLegacyGeneralSkillForgetItem && !bIsAntiSkillForgetItem) return;
	int32 FirstSkillVnum = Settings.StandardSkillForgetMinimumSkillVnum;
	int32 LastSkillVnum = Settings.StandardSkillForgetMaximumSkillVnum;
	if (bIsAntiSkillForgetItem)
	{
		FirstSkillVnum = Settings.AntiSkillForgetMinimumSkillVnum;
		LastSkillVnum = Settings.AntiSkillForgetMaximumSkillVnum;
	}
	if (FirstSkillVnum <= 0 || LastSkillVnum < FirstSkillVnum) return;
	const TArray<int32> SkillVnums = UMT2SkillSet::GetSkillVnumsInRange(FirstSkillVnum, LastSkillVnum);
	if (SkillVnums.IsEmpty()) return;
	Data.SetSkillVnum(SkillVnums[FMath::RandHelper(SkillVnums.Num())]);
}

EMT2ItemUseExecution UMT2ItemSkillForgetTemplate::ExecuteUse(AMT2PlayerCharacter& Character,
	UMT2InventoryComponent& Inventory, int32 InventorySlot) const
{
	if (!Character.HasAuthority() || !Inventory.GetSlots().IsValidIndex(InventorySlot))
		return EMT2ItemUseExecution::Rejected;

	const FMT2ItemSlot& Item = Inventory.GetSlots()[InventorySlot];
	if (Item.Vnum != Vnum) return EMT2ItemUseExecution::Rejected;
	const int32 SkillVnum = Item.GetSkillVnum();
	if (SkillVnum <= 0)
	{
		// Legacy char_item.cpp destroys malformed skill-forget items.
		return EMT2ItemUseExecution::ConsumeAndReject;
	}

	AMT2PlayerState* PlayerState = Character.GetPlayerState<AMT2PlayerState>();
	UMT2SkillComponent* Skills = PlayerState ? PlayerState->GetSkillComponent() : nullptr;
	if (!Skills || !Skills->ForgetSkillByItem(SkillVnum))
	{
		if (AMT2PlayerController* Controller = Cast<AMT2PlayerController>(Character.GetController()))
			Controller->SendSystemChatMessage(TEXT("This skill cannot be forgotten right now."));
		return EMT2ItemUseExecution::Rejected;
	}

	if (AMT2PlayerController* Controller = Cast<AMT2PlayerController>(Character.GetController()))
		Controller->SendSystemChatMessage(TEXT("The skill level has been lowered."));
	return EMT2ItemUseExecution::Consume;
}


