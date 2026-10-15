#include "Items/MT2InventoryComponent.h"
#include "Items/MT2ItemTemplate.h"

bool UMT2InventoryComponent::FinishMiningAttempt()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Equipment.IsValidIndex(4)) { return false; }
	const auto* Pick=Cast<UMT2ItemPickTemplate>(ResolveTemplate(Equipment[4].Vnum));
	if (!Pick || Pick->RefinedVnum<=0 || Pick->PracticeChanceDenominator<=0 || Pick->ImprovementPointsRequired<=0) { return false; }
	const int32 Points=Equipment[4].MetinSockets.IsValidIndex(0) ? Equipment[4].MetinSockets[0].Value : 0;
	if (Points>=Pick->ImprovementPointsRequired || FMath::RandRange(1,Pick->PracticeChanceDenominator)!=1) { return false; }
	if (Equipment[4].MetinSockets.IsEmpty()) { Equipment[4].MetinSockets.SetNum(1); }
	Equipment[4].MetinSockets[0].Value=Points+1;
	OnInventoryChanged.Broadcast(); OnEquipmentChanged.Broadcast();
	return true;
}
