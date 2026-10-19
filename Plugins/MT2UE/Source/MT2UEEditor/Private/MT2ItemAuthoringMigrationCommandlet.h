#pragma once
#include "Commandlets/Commandlet.h"
#include "MT2ItemAuthoringMigrationCommandlet.generated.h"

UCLASS()
class UMT2ItemAuthoringMigrationCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UMT2ItemAuthoringMigrationCommandlet();
	virtual int32 Main(const FString& Params) override;
};
