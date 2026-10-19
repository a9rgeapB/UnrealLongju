/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Items/Templates/MT2ItemCampfireTemplate.h"
#include "Items/MT2ItemTemplate.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/MT2HealthComponent.h"
#include "Engine/World.h"
#include "Fishing/MT2FishingSettings.h"
#include "Items/MT2InventoryComponent.h"
#include "Npcs/MT2Campfire.h"
#include "World/MT2MapAttributes.h"

EMT2ItemUseExecution UMT2ItemCampfireTemplate::ExecuteUse(AMT2PlayerCharacter& Character,
	UMT2InventoryComponent& Inventory, int32 InventorySlot) const
{
	auto* World = Character.GetWorld();
	if (!Character.HasAuthority() || !World || Character.GetHealthComponent()->IsDead() ||
		!Inventory.GetSlots().IsValidIndex(InventorySlot) || Inventory.GetSlots()[InventorySlot].Vnum != Vnum)
		return EMT2ItemUseExecution::Rejected;
	const auto& Settings = *GetDefault<UMT2FishingSettings>();
	if (!FMath::IsFinite(Settings.CampfirePlacementDistance) || Settings.CampfirePlacementDistance <= 0 ||
		!FMath::IsFinite(Settings.CampfireGroundTraceHeight) || Settings.CampfireGroundTraceHeight <= 0 ||
		!FMath::IsFinite(Settings.CampfireGroundTraceDepth) || Settings.CampfireGroundTraceDepth <= 0)
		return EMT2ItemUseExecution::Rejected;
	const FVector Location = Character.GetActorLocation() + Character.GetActorForwardVector() * Settings.CampfirePlacementDistance;
	auto* Attributes = World->GetSubsystem<UMT2MapAttributeSubsystem>();
	uint8 MapFlags = 0;
	if (!Attributes || !Attributes->FindMapAt(Location, MapFlags) || (MapFlags & (MT2MapAttribute::Water | MT2MapAttribute::NoWalk)))
		return EMT2ItemUseExecution::Rejected;
	FHitResult Ground;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(CampfirePlacement), false, &Character);
	if (!World->LineTraceSingleByObjectType(Ground, Location + FVector(0, 0, Settings.CampfireGroundTraceHeight),
		Location - FVector(0, 0, Settings.CampfireGroundTraceDepth), FCollisionObjectQueryParams(ECC_WorldStatic), Query) ||
		Ground.ImpactNormal.Z < .5f) return EMT2ItemUseExecution::Rejected;
	const float Height = FMath::Max(FMath::Max(1.f, Settings.CampfireCollisionRadius), Settings.CampfireCollisionHalfHeight);
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
	return World->SpawnActor<AMT2Campfire>(Ground.ImpactPoint + FVector(0, 0, Height),
		FRotator(0, FMath::RandRange(0.f, 359.f), 0), Spawn)
		? EMT2ItemUseExecution::Consume : EMT2ItemUseExecution::Rejected;
}
