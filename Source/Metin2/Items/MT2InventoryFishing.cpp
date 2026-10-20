#include "Items/Templates/MT2ItemRodTemplate.h"
#include "Items/Templates/MT2ItemUseBaitTemplate.h"
#include "Items/MT2InventoryComponent.h"
#include "Items/MT2ItemTemplate.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Components/MT2HealthComponent.h"
#include "Fishing/MT2FishingComponent.h"
#include "Player/MT2PlayerController.h"

bool UMT2InventoryComponent::ApplyFishingBait(int32 InventorySlot, const UMT2ItemUseBaitTemplate& Bait)
{
	auto* Player = Cast<AMT2PlayerCharacter>(GetOwner());
	if (!Player || !Player->HasAuthority() || !Player->GetHealthComponent() || Player->GetHealthComponent()->IsDead() ||
		!Player->GetFishingComponent() || Player->GetFishingComponent()->IsFishing() ||
		!Slots.IsValidIndex(InventorySlot) || Slots[InventorySlot].IsEmpty() || !Equipment.IsValidIndex(4)) { return false; }
	const auto* ItemBait = Cast<UMT2ItemUseBaitTemplate>(ResolveTemplate(Slots[InventorySlot].Vnum));
	if (!ItemBait || ItemBait->GetClass() != Bait.GetClass() || ItemBait->BaitPower <= 0 ||
		!Cast<UMT2ItemRodTemplate>(ResolveTemplate(Equipment[4].Vnum))) { return false; }
	Equipment[4].EditRodState().BaitPower = ItemBait->BaitPower;
	if (--Slots[InventorySlot].Count == 0) { Slots[InventorySlot] = FMT2ItemSlot(); }
	OnInventoryChanged.Broadcast(); OnEquipmentChanged.Broadcast(); return true;
}
bool UMT2InventoryComponent::FinishFishingAttempt(bool bPractice)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Equipment.IsValidIndex(4)) { return false; }
	const auto* Rod = Cast<UMT2ItemRodTemplate>(ResolveTemplate(Equipment[4].Vnum));
	if (!Rod) { return false; }
	int32& Points = Equipment[4].EditRodState().Proficiency;
	if (bPractice && Rod->GetRefinedVnum() > 0 && Points < Rod->ImprovementPointsRequired && Rod->PracticeChanceDenominator > 0 &&
		FMath::RandRange(1, Rod->PracticeChanceDenominator) == 1)
	{
		++Points;
		if (auto* Player = Cast<AMT2PlayerCharacter>(GetOwner()))
			if (auto* Controller = Cast<AMT2PlayerController>(Player->GetController()))
			{
				Controller->SendSystemChatMessage(FString::Printf(TEXT("Fishing rod proficiency increased (%d/%d)."), Points, Rod->ImprovementPointsRequired));
				if (Points == Rod->ImprovementPointsRequired) { Controller->SendSystemChatMessage(TEXT("Your rod is ready for upgrading at the fisherman.")); }
			}
	}
	Equipment[4].EditRodState().BaitPower = 0;
	OnInventoryChanged.Broadcast(); OnEquipmentChanged.Broadcast(); return true;
}
int32 UMT2InventoryComponent::RefineFishingRod(int32 InventorySlot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Slots.IsValidIndex(InventorySlot) || Slots[InventorySlot].IsEmpty()) { return 2; }
	const auto* Rod = Cast<UMT2ItemRodTemplate>(ResolveTemplate(Slots[InventorySlot].Vnum));
	if (!Rod || Rod->GetRefinedVnum() <= 0 ||
		Slots[InventorySlot].GetRodState().Proficiency != Rod->ImprovementPointsRequired) { return 2; }
	const bool Success = FMath::RandRange(1, 100) <= FMath::Clamp(Rod->RefinementSuccessPercent, 0, 100);
	const int32 Vnum = Success ? Rod->GetRefinedVnum() : Rod->GetRefinementFailureVnum();
	const auto* Replacement = Cast<UMT2ItemRodTemplate>(ResolveTemplate(Vnum));
	if (!Replacement || !CanPlaceAt(InventorySlot, GetItemSize(Vnum), InventorySlot)) { return 2; }
	FMT2ItemSlot NewRod(Vnum, Replacement->MakeInstanceData(1));
	NewRod.Template = Replacement->GetClass();
	NewRod.EditRodState() = FMT2FishingRodItemState();
	Slots[InventorySlot] = MoveTemp(NewRod); OnInventoryChanged.Broadcast(); return Success ? 1 : 0;
}
