/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/MT2ItemTemplate.h"
#include "MT2ItemMetinStoneTemplate.generated.h"

UCLASS(Blueprintable)
class METIN2_API UMT2ItemMetinStoneTemplate : public UMT2ItemTemplate
{
	GENERATED_BODY()

public:
	bool ConflictsWith(const UMT2ItemMetinStoneTemplate& Other) const;
	// Select one representative stone shared by every grade in this conflict family.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Metin Stone")
	TSoftClassPtr<UMT2ItemMetinStoneTemplate> ConflictFamily;
	// METIN_NORMAL=0 may enter silver or gold; METIN_GOLD=1 requires a gold socket.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Metin Stone")
	EMT2MetinSubType MetinSubType = EMT2MetinSubType::Normal;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Metin Stone",
		meta = (Bitmask, BitmaskEnum = "/Script/Metin2.EMT2ItemWearFlag"))
	int32 CompatibleWearFlags = 0;

	// Value5 in the original item_proto. Equipment cannot contain two stones in one category.
	UPROPERTY()
	int32 StoneCategory = 0;

	// Minimum socket grade accepted by the original comparison (silver=1, gold=2).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Metin Stone")
	EMT2RequiredMetinSocketGrade RequiredSocketGrade = EMT2RequiredMetinSocketGrade::Silver;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Metin Stone",
		meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float AttachmentSuccessPercent = 30.0f;
};
