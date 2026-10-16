#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MT2AuditSemanticTypesCommandlet.generated.h"

UCLASS()
class UMT2AuditSemanticTypesCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UMT2AuditSemanticTypesCommandlet();
	virtual int32 Main(const FString& Params) override;
};
