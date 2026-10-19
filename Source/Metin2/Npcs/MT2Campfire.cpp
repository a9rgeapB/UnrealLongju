#include "Npcs/MT2Campfire.h"
#include "Items/Templates/MT2ItemFishTemplate.h"

#include "Characters/MT2PlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Config/MT2GameplaySettings.h"
#include "Fishing/MT2FishingSettings.h"
#include "Items/MT2InventoryComponent.h"
#include "Items/MT2ItemTemplate.h"
#include "Items/MT2ItemUtils.h"
#include "Particles/ParticleSystemComponent.h"

AMT2Campfire::AMT2Campfire()
{
	MobVnum = 12000;
	MobType = EMT2MobType::NPC;
	InternalName = TEXT("campfire");
	DisplayName = TEXT("Campfire");
	Logs = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CampfireLogs"));
	Logs->SetupAttachment(GetRootComponent());
	Logs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Fire = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("CampfireEffect"));
	Fire->SetupAttachment(Logs);
}

void AMT2Campfire::ConfigureCollision()
{
	const auto& Settings = *GetDefault<UMT2FishingSettings>();
	const float Radius = FMath::Max(1.f, Settings.CampfireCollisionRadius);
	const float Height = FMath::Max(Radius, Settings.CampfireCollisionHalfHeight);
	GetCapsuleComponent()->SetCapsuleSize(Radius, Height);
	Logs->SetRelativeLocation(FVector(0, 0, -Height));
}

void AMT2Campfire::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConfigureCollision();
}

void AMT2Campfire::BeginPlay()
{
	Super::BeginPlay();
	ConfigureCollision(); // The mob base reapplies collision on client initialization.
	const auto& Settings = *GetDefault<UMT2FishingSettings>();
	if (GetNetMode() != NM_DedicatedServer)
	{
		Logs->SetStaticMesh(Settings.CampfireMesh.LoadSynchronous());
		Fire->SetTemplate(Settings.CampfireEffect.LoadSynchronous());
	}
	if (HasAuthority()) SetLifeSpan(FMath::Max(1.f, Settings.CampfireLifetime));
}

bool AMT2Campfire::Roast(AMT2PlayerCharacter& Player, int32 InventorySlot)
{
	if (!HasAuthority() || IsActorBeingDestroyed() || GetWorld() != Player.GetWorld() ||
		FVector::DistSquared2D(GetActorLocation(), Player.GetActorLocation()) >
		FMath::Square(UMT2GameplaySettings::Get().WorldItemInteractionRange)) return false;
	auto* Inventory = Player.GetInventoryComponent();
	if (!Inventory || !Inventory->GetSlots().IsValidIndex(InventorySlot)) return false;
	const auto Item = Inventory->GetSlots()[InventorySlot];
	const auto* Fish = MT2ItemUtils::ResolveTemplate<UMT2ItemFishTemplate>(&Player, Item.Vnum);
	return Fish && Fish->Fish.GetGrilledItemVnum() > 0 &&
		Inventory->ExchangeItemAtSlot(InventorySlot, Item.Count, Fish->Fish.GetGrilledItemVnum(), Item.Count);
}
