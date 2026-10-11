#pragma once
#include "Npcs/MT2Npc.h"
#include "MT2StaticNpc.generated.h"
class UStaticMeshComponent;

// Legacy seal props have static models rather than skeletons, but use normal NPC interactions.
UCLASS()
class METIN2_API AMT2StaticNpc : public AMT2Npc
{
	GENERATED_BODY()
public:
	AMT2StaticNpc();
	virtual void PostInitializeComponents() override;
	virtual void CalculateVisualCollision(float& Radius, float& HalfHeight, float& MeshRelativeZ) const override;
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> StaticVisual;
};
