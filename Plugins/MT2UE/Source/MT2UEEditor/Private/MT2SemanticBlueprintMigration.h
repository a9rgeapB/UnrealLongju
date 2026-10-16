#pragma once
#include "CoreMinimal.h"
#include "EdGraph/EdGraphPin.h"

class UBlueprint;
namespace MT2SemanticBlueprintMigration
{
	struct FPinChange
	{
		UEdGraphPin* Pin=nullptr;
		FEdGraphPinType Type;
		FString DefaultValue;
	};
	struct FPlan { TArray<FPinChange> Pins; };
	bool Plan(UBlueprint* Blueprint,FPlan& Out,TArray<FString>& Issues);
	bool Apply(UBlueprint* Blueprint,const FPlan& Plan,TArray<FString>& Issues);
	bool Compile(UBlueprint* Blueprint,TArray<FString>& Issues);
	bool EquivalentDefault(const UEdGraphPin* Pin,const FString& OldDefault);
}
