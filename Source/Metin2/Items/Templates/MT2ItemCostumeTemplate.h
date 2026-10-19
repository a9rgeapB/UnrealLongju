/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/Templates/MT2ItemEquipmentTemplate.h"
#include "Items/Types/MT2ItemVisualTypes.h"
#include "MT2ItemCostumeTemplate.generated.h"

UCLASS(Blueprintable)
class METIN2_API UMT2ItemCostumeTemplate : public UMT2ItemEquipmentTemplate
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Costume")
	EMT2CostumeSubType CostumeSubType = EMT2CostumeSubType::Body;

	// COSTUME_HAIR stores the old client's race-data HairIndex in item_proto Value3.
	UPROPERTY()
	int32 HairShapeId = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Costume|Hair")
	TArray<FMT2HairMeshVariant> HairMeshVariants;

	const FMT2HairMeshVariant* ResolveHairVariant(
		const FMT2CharacterAppearance& Appearance) const
	{
		return HairMeshVariants.FindByPredicate(
			[&Appearance](const FMT2HairMeshVariant& Candidate)
			{
				return Candidate.Race == Appearance.Race && Candidate.Sex == Appearance.Sex;
			});
	}
};
