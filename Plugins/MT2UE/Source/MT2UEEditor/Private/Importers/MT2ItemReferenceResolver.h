#pragma once

#include "CoreMinimal.h"

class UMT2ItemTemplate;
class UMT2VnumRegistry;

// Run only after all item classes exist, so forward refinement/reward references can resolve.
bool ResolveMT2ItemReferences(const TArray<UMT2ItemTemplate*>& Items, TArray<FString>& Warnings,
	const UMT2VnumRegistry* ExistingRegistry = nullptr);
