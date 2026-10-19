#pragma once

#include "Commandlets/Commandlet.h"
#include "MT2ConfigureItemBehaviorsCommandlet.generated.h"

UCLASS()
class UMT2ConfigureItemBehaviorsCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UMT2ConfigureItemBehaviorsCommandlet();
	virtual int32 Main(const FString& Params) override;
};
