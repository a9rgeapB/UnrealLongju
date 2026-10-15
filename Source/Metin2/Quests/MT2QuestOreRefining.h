#pragma once

#include "Quests/MT2QuestNode.h"
#include "MT2QuestOreRefining.generated.h"

// Shared imported definition; suspended choices are separate per-conversation instances.
UCLASS(DisplayName = "Independent Alchemist")
class METIN2_API UMT2QuestNode_OreRefining : public UMT2QuestNode
{
	GENERATED_BODY()
public:
	virtual EMT2QuestNodeResult Execute(const FMT2QuestContext& Context) override;
	UPROPERTY(EditAnywhere, Category = "Alchemist") TMap<FName, FText> Messages;
private:
	UPROPERTY(Transient) uint8 Phase = 0;
	UPROPERTY(Transient) int32 MaterialPage = 0;
	UPROPERTY(Transient) int32 StoneSlot = INDEX_NONE;
	UPROPERTY(Transient) FMT2ItemSlot ExpectedStone;
	bool bResolved = false;
	FText Message(const TCHAR* Key, const TCHAR* Fallback, const TArray<FString>& Arguments = {}) const;
};
