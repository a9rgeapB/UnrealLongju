#pragma once

#include "CoreMinimal.h"
#include "Npcs/MT2Npc.h"
#include "MT2OreVein.generated.h"

class UStaticMeshComponent;

// Vein race models are unskinned GR2 assets, unlike ordinary animated NPCs.
UCLASS(Blueprintable)
class METIN2_API AMT2OreVein : public AMT2Npc
{
	GENERATED_BODY()
public:
	AMT2OreVein();
	UStaticMeshComponent* GetVeinMesh() const { return VeinMesh; }
	virtual void CalculateVisualCollision(float& OutRadius, float& OutHalfHeight, float& OutMeshRelativeZ) const override;
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mining", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> VeinMesh;
};
