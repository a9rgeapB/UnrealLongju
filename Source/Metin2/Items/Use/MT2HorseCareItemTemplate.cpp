#include "Items/Use/MT2HorseCareItemTemplate.h"

#include "Characters/MT2PlayerCharacter.h"
#include "Mounts/MT2HorseProgressionComponent.h"
#include "Player/MT2PlayerController.h"
#include "Player/MT2PlayerState.h"

EMT2ItemUseExecution UMT2HorseCareItemTemplate::ExecuteUse(
	AMT2PlayerCharacter& Character, UMT2InventoryComponent&, int32) const
{
	if (!Character.HasAuthority()) return EMT2ItemUseExecution::Rejected;
	AMT2PlayerState* State = Character.GetPlayerState<AMT2PlayerState>();
	UMT2HorseProgressionComponent* Horse = State ? State->GetHorseProgressionComponent() : nullptr;
	const bool bApplied = Horse && (Action == EMT2HorseCareAction::Feed ? Horse->Feed() : Horse->Revive());
	if (!bApplied)
	{
		if (AMT2PlayerController* Controller = Cast<AMT2PlayerController>(Character.GetController()))
		{
			Controller->SendSystemChatMessage(Action == EMT2HorseCareAction::Feed
				? TEXT("Your horse cannot use this food right now.")
				: TEXT("Your horse does not need to be revived."));
		}
		return EMT2ItemUseExecution::Rejected;
	}
	return EMT2ItemUseExecution::Consume;
}
