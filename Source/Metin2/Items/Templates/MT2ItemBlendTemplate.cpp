/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/Templates/MT2ItemBlendTemplate.h"
#include "Items/MT2ItemTemplate.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Components/MT2HealthComponent.h"
#include "Components/MT2StatusEffectComponent.h"
#include "Items/MT2InventoryComponent.h"
#include "Items/MT2ItemUtils.h"

void UMT2ItemBlendTemplate::InitializeGeneratedInstance(FMT2ItemInstanceData& Data) const
{
	if (!Blend.IsValid() || MT2ItemUtils::GetNumericSocket(Data, 0) != 0 ||
		MT2ItemUtils::GetNumericSocket(Data, 1) != 0 || MT2ItemUtils::GetNumericSocket(Data, 2) != 0) return;
	const int32 ValueIndex = Blend.GetRollIndex(FMath::RandRange(1, Blend.GetTotalWeight()));
	const int32 DurationIndex = Blend.GetRollIndex(FMath::RandRange(1, Blend.GetTotalWeight()));
	auto& State = Data.EditBlendState();
	State.BonusType = Blend.BonusType;
	State.Magnitude = Blend.Magnitudes[ValueIndex];
	State.DurationSeconds = Blend.Durations[DurationIndex];
}


EMT2ItemUseExecution UMT2ItemBlendTemplate::ExecuteUse(AMT2PlayerCharacter& Character,
	UMT2InventoryComponent& Inventory, int32 InventorySlot) const
{
	if (!Character.HasAuthority() || Character.GetHealthComponent()->IsDead() || !Blend.IsValid() ||
		!Inventory.GetSlots().IsValidIndex(InventorySlot)) return EMT2ItemUseExecution::Rejected;
	const FMT2ItemSlot& Item = Inventory.GetSlots()[InventorySlot];
	const int32 RawApply = MT2ItemUtils::GetNumericSocket(Item, 0);
	EMT2ItemBonusType Apply;
	if (Item.Vnum != Vnum || !MT2ItemSemanticTypes::TryDecodeBonus(RawApply, Apply) || Apply == EMT2ItemBonusType::None ||
		MT2ItemUtils::GetNumericSocket(Item, 1) <= 0 || MT2ItemUtils::GetNumericSocket(Item, 2) <= 0)
		return EMT2ItemUseExecution::Rejected;
	auto* Effects = Character.GetStatusEffectComponent();
	if (!Effects) return EMT2ItemUseExecution::Rejected;
	const auto BlendType = FMT2AffectIdentifier::FromBuiltin(EMT2BuiltinAffect::Blend);
	const auto PremiumType = FMT2AffectIdentifier::FromBuiltin(EMT2BuiltinAffect::ExperienceBonusEuropeFree);
	if (Effects->GetEffects().ContainsByPredicate([&](const FMT2StatusEffect& Effect)
		{
			return (Effect.Type == BlendType && Effect.ApplyType == Apply) ||
				(Effect.Type == PremiumType && Effect.ApplyType == EMT2ItemBonusType::MagicResistance);
		})) return EMT2ItemUseExecution::Rejected;
	FMT2StatusEffect Effect;
	Effect.Type = BlendType;
	Effect.SourceItemVnum = Item.Vnum;
	Effect.ApplyType = Apply;
	Effect.ApplyValue = MT2ItemUtils::GetNumericSocket(Item, 1);
	Effect.RemainingSeconds = MT2ItemUtils::GetNumericSocket(Item, 2);
	Effect.Icon = Icon;
	Effect.bRemoveOnDeath = true;
	return Effects->AddEffect(Effect, false) ? EMT2ItemUseExecution::Consume : EMT2ItemUseExecution::Rejected;
}

void UMT2ItemBlendTemplate::InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const
{
	Data.FamilyState.InitializeAs<FMT2BlendItemState>(DefaultBlendState);
}

void UMT2ItemBlendTemplate::CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data)
{
	DefaultBlendState = Data.GetBlendState();
}
