#pragma once

#include "Commandlets/Commandlet.h"
#include "MT2RepairParticleOwnershipCommandlet.generated.h"

UCLASS()
class UMT2RepairParticleOwnershipCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UMT2RepairParticleOwnershipCommandlet();
	virtual int32 Main(const FString& Params) override;
};
