#pragma once
#include "Npcs/MT2Npc.h"
#include "MT2Campfire.generated.h"

class UStaticMeshComponent;
class UParticleSystemComponent;
class AMT2PlayerCharacter;

// A stationary, replicated NPC: give live/dead fish to roast its entire stack.
UCLASS(Blueprintable)
class METIN2_API AMT2Campfire : public AMT2Npc
{
	GENERATED_BODY()
public:
	AMT2Campfire();
	virtual void OnConstruction(const FTransform& Transform) override;
	bool Roast(AMT2PlayerCharacter& Player, int32 InventorySlot);
protected:
	virtual void BeginPlay() override;
private:
	void ConfigureCollision();
	UPROPERTY(VisibleAnywhere, Category="Campfire") TObjectPtr<UStaticMeshComponent> Logs;
	UPROPERTY(VisibleAnywhere, Category="Campfire") TObjectPtr<UParticleSystemComponent> Fire;
};
