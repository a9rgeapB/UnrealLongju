#include "Mining/MT2OreVein.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Mobs/MT2MobRuntimeSettings.h"

AMT2OreVein::AMT2OreVein()
{
	VeinMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VeinMesh"));
	// Reuse mob grounding, scale and orientation; the capsule owns interaction/collision.
	VeinMesh->SetupAttachment(GetMesh());
	VeinMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VeinMesh->SetGenerateOverlapEvents(false);
	VeinMesh->PrimaryComponentTick.bCanEverTick=false;
}

void AMT2OreVein::CalculateVisualCollision(float& OutRadius, float& OutHalfHeight, float& OutMeshRelativeZ) const
{
	Super::CalculateVisualCollision(OutRadius,OutHalfHeight,OutMeshRelativeZ);
	const UStaticMesh* StaticAsset=VeinMesh ? VeinMesh->GetStaticMesh() : nullptr;
	const auto* Settings=GetDefault<UMT2MobRuntimeSettings>();
	if (!StaticAsset || !Settings->bFitCapsuleToSkeletalMesh) { return; }
	const float Scale=FMath::Max(ScalePercent,1)/100.f;
	const FBox Bounds=StaticAsset->GetBounds().GetBox().TransformBy(FTransform(GetMesh()->GetRelativeRotation(),FVector::ZeroVector,FVector(Scale)));
	if (!Bounds.IsValid || Bounds.GetSize().IsNearlyZero()) { return; }
	const float Extent=FMath::Max(FMath::Max(FMath::Abs(Bounds.Min.X),FMath::Abs(Bounds.Max.X)),
		FMath::Max(FMath::Abs(Bounds.Min.Y),FMath::Abs(Bounds.Max.Y)));
	OutRadius=FMath::Max(Extent*Settings->MeshCapsuleRadiusScale+Settings->MeshCapsuleRadiusPadding,Settings->MinimumMeshCapsuleRadius);
	OutHalfHeight=FMath::Max3(float(Bounds.GetSize().Z)*.5f*Settings->MeshCapsuleHeightScale+Settings->MeshCapsuleHalfHeightPadding,
		Settings->MinimumMeshCapsuleHalfHeight,OutRadius);
	OutMeshRelativeZ=-OutHalfHeight-Bounds.Min.Z;
}
