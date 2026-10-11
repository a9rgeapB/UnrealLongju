#include "Npcs/MT2StaticNpc.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

AMT2StaticNpc::AMT2StaticNpc()
{
	StaticVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticVisual"));
	StaticVisual->SetupAttachment(GetRootComponent());
	StaticVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticVisual->SetGenerateOverlapEvents(false);
	StaticVisual->PrimaryComponentTick.bCanEverTick = false;
}

void AMT2StaticNpc::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (StaticVisual->GetStaticMesh())
	{
		float Radius, HalfHeight, Offset; CalculateVisualCollision(Radius, HalfHeight, Offset);
		GetCapsuleComponent()->SetCapsuleSize(Radius, HalfHeight);
		StaticVisual->SetRelativeLocation(FVector(0, 0, Offset));
	}
}

void AMT2StaticNpc::CalculateVisualCollision(float& Radius, float& HalfHeight, float& MeshRelativeZ) const
{
	if (const UStaticMesh* Asset = StaticVisual->GetStaticMesh())
	{
		const float Scale = FMath::Max(ScalePercent, 1) / 100.f;
		const auto Box = Asset->GetBoundingBox();
		Radius = FMath::Max(20.f, FMath::Max(Box.GetExtent().X, Box.GetExtent().Y) * Scale);
		HalfHeight = FMath::Max(Radius, Box.GetExtent().Z * Scale);
		MeshRelativeZ = -HalfHeight - Box.Min.Z * Scale;
	}
	else Super::CalculateVisualCollision(Radius, HalfHeight, MeshRelativeZ);
}

void AMT2StaticNpc::BeginPlay()
{
	Super::BeginPlay();
	float Radius, HalfHeight, Offset; CalculateVisualCollision(Radius, HalfHeight, Offset);
	StaticVisual->SetRelativeScale3D(FVector(FMath::Max(ScalePercent, 1) / 100.f));
	StaticVisual->SetRelativeLocation(FVector(0, 0, Offset));
}
