/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "CoreMinimal.h"
#include "Player/MT2PlayerTypes.h"
#include "MT2ItemVisualTypes.generated.h"

class UMaterialInterface;
class USkeletalMesh;
class UTexture2D;

USTRUCT(BlueprintType)
struct METIN2_API FMT2ArmorMaterialOverride
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor", meta = (ClampMin = "0"))
	int32 MaterialSlotIndex = INDEX_NONE;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor")
	TSoftObjectPtr<UMaterialInterface> Material;
};

USTRUCT(BlueprintType)
struct METIN2_API FMT2ArmorMeshVariant
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor")
	EMT2CharacterRace Race = EMT2CharacterRace::Warrior;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor")
	EMT2CharacterSex Sex = EMT2CharacterSex::Male;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor")
	TArray<FMT2ArmorMaterialOverride> MaterialOverrides;
};

USTRUCT(BlueprintType)
struct METIN2_API FMT2HairMeshVariant
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hair")
	EMT2CharacterRace Race = EMT2CharacterRace::Warrior;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hair")
	EMT2CharacterSex Sex = EMT2CharacterSex::Male;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hair")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hair")
	TSoftObjectPtr<UTexture2D> DiffuseTexture;
};

