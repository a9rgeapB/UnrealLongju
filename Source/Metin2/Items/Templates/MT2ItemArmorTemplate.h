/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "Items/Templates/MT2ItemEquipmentTemplate.h"
#include "Items/Types/MT2ItemVisualTypes.h"
#include "MT2ItemArmorTemplate.generated.h"

UCLASS(Blueprintable)
class METIN2_API UMT2ItemArmorTemplate : public UMT2ItemEquipmentTemplate
{
	GENERATED_BODY()

public:
	virtual void InitializeDefaultFamilyState(FMT2ItemInstanceData& Data) const override;
	virtual void CaptureDefaultFamilyState(const FMT2ItemInstanceData& Data) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Armor|Sockets")
	TArray<FMT2MetinSocket> DefaultMetinSockets;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Armor|Accessory Sockets")
	FMT2AccessorySocketState DefaultAccessorySockets;
	virtual TSubclassOf<UMT2ItemTooltipWidget> GetDefaultTooltipClass() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor")
	EMT2ArmorSubType ArmorSubType = EMT2ArmorSubType::Body;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor|Stats")
	int32 Defense = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor|Stats")
	int32 RefinementDefense = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor|Visual")
	int32 ArmorShapeId = 0;

	// Unlike weapons (attached as a separate static mesh), body armor replaces the character's whole
	// skeletal mesh - the old client does the same per-race/gender body swap on equip.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor")
	TSoftObjectPtr<USkeletalMesh> ArmorMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor")
	TArray<FMT2ArmorMeshVariant> ArmorMeshVariants;

	TSoftObjectPtr<USkeletalMesh> ResolveArmorMesh(const FMT2CharacterAppearance& Appearance) const
	{
		if (const FMT2ArmorMeshVariant* Variant = ArmorMeshVariants.FindByPredicate(
			[&Appearance](const FMT2ArmorMeshVariant& Candidate)
			{
				return Candidate.Race == Appearance.Race && Candidate.Sex == Appearance.Sex;
			}))
		{
			return Variant->Mesh;
		}
		return ArmorMesh;
	}

	const FMT2ArmorMeshVariant* ResolveArmorVariant(const FMT2CharacterAppearance& Appearance) const
	{
		return ArmorMeshVariants.FindByPredicate(
			[&Appearance](const FMT2ArmorMeshVariant& Candidate)
			{
				return Candidate.Race == Appearance.Race && Candidate.Sex == Appearance.Sex;
			});
	}
};
