/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "CoreMinimal.h"
#include "Items/MT2ItemSemanticTypes.h"
#include "Player/MT2PlayerTypes.h"
#include "Items/Types/MT2ItemFlags.h"
#include "Items/Types/MT2ItemRefinementTypes.h"
#include "Items/Types/MT2ItemBonusTypes.h"
#include "Items/Types/MT2ItemInstanceData.h"
#include "Items/Types/MT2ItemLimitTypes.h"
#include "MT2ItemImportTypes.generated.h"

USTRUCT(BlueprintType)
struct METIN2_API FMT2ItemArmorMaterialOverrideDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armor")
	FString SourceTextureObjectPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armor")
	FString TargetTextureObjectPath;
};

USTRUCT(BlueprintType)
struct METIN2_API FMT2ItemArmorVisualDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armor")
	EMT2CharacterRace Race = EMT2CharacterRace::Warrior;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armor")
	EMT2CharacterSex Sex = EMT2CharacterSex::Male;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armor")
	FString MeshObjectPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Armor")
	TArray<FMT2ItemArmorMaterialOverrideDefinition> MaterialOverrides;
};

USTRUCT(BlueprintType)
struct METIN2_API FMT2ItemHairVisualDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hair")
	EMT2CharacterRace Race = EMT2CharacterRace::Warrior;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hair")
	EMT2CharacterSex Sex = EMT2CharacterSex::Male;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hair")
	FString MeshObjectPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hair")
	FString TextureObjectPath;
};

USTRUCT(BlueprintType)
struct METIN2_API FMT2ItemDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	int32 Vnum = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FString InternalName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FString DisplayName;

	// locale itemdesc.txt third column (tooltip flavor text); empty when the item has no entry.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FString Description;

	// EItemTypes ordinal from the old client's common/enums.h (ITEM_TYPE_WEAPON=1, ITEM_TYPE_ARMOR=2, ...).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	EMT2ItemType ItemType = EMT2ItemType::None;

	// EWeaponSubTypes / EArmorSubTypes ordinal; meaning depends on ItemType.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FMT2ItemSubtype SubType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	int32 Size = 1;

	// item_proto WEIGHT; carried but unused (the old game never enforced weight either).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	int32 Weight = 0;

	// Vnums in [Vnum, Vnum+VnumRange] resolve to this same item (old client SelectItemData ranges).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	int32 VnumRange = 0;

	// EWearPositions bitmask (1 << WEAR_WEAPON, etc.); 0 if not equippable.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flags",
		meta = (Bitmask, BitmaskEnum = "/Script/Metin2.EMT2ItemWearFlag"))
	int32 WearFlags = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flags",
		meta = (Bitmask, BitmaskEnum = "/Script/Metin2.EMT2ItemAntiFlag"))
	int32 AntiFlags = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flags",
		meta = (Bitmask, BitmaskEnum = "/Script/Metin2.EMT2ItemFlag"))
	int32 Flags = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flags",
		meta = (Bitmask, BitmaskEnum = "/Script/Metin2.EMT2ItemImmuneFlag"))
	int32 ImmuneFlags = 0;

	// 40250 TItemTable dwIBuyItemPrice: what shops charge the player.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy")
	int64 BuyPrice = 0;

	// 40250 TItemTable dwISellItemPrice: what shops pay the player.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy")
	int64 SellPrice = 0;

	UPROPERTY()
	int32 RefinedVnum = 0;

	UPROPERTY()
	int32 RefineSet = 0;

	UPROPERTY()
	int32 PreviousRefinedVnum = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refine", meta = (ClampMin = "0", ClampMax = "9"))
	int32 RefinementLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refine")
	FMT2RefinementRecipe RefinementRecipe;

	// item_proto MAGIC_PCT: chance to roll magic attributes when acquired; not wired yet.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refine")
	int32 AlterToMagicItemPercent = 0;

	// item_proto GAIN_SOCKET_PCT: chance to gain a socket on refine; not wired yet.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refine")
	int32 GainSocketPercent = 0;

	// Old item_proto addon_type. -1 generates the paired average/skill-damage attributes.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	FMT2ItemAddonSelection BonusAddonType;

	// item_proto Specular strength (visual shine); not wired yet.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	int32 Specular = 0;

	// Raw Value0..5 from item_proto. Weapon slots are magic min/max at 1/2, physical min/max at 3/4,
	// and refinement power at 5.
	UPROPERTY()
	TArray<int32> Values;

	// aApplies[3] from item_proto - the equipped stat bonuses.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	TArray<FMT2ItemApply> Applies;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	TArray<FMT2ItemLimit> Limits;

	// Mutable defaults carried from item_proto into the generated item template.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	FMT2ItemInstanceData DefaultInstanceData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	FString IconObjectPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	FString WorldMeshObjectPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TArray<FMT2ItemArmorVisualDefinition> ArmorVisuals;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TArray<FMT2ItemHairVisualDefinition> HairVisuals;
	void PostSerialize(const FArchive& Archive) { if (Archive.IsLoading()) { SubType.ResolveLegacyFamily(ItemType); } }
	bool ImportTextItem(const TCHAR*& Buffer,int32 PortFlags,UObject* Parent,FOutputDevice* ErrorText);
};

template<> struct TStructOpsTypeTraits<FMT2ItemDefinition> : TStructOpsTypeTraitsBase2<FMT2ItemDefinition>
{
	enum { WithPostSerialize=true, WithImportTextItem=true };
};
