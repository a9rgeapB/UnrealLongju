#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Items/MT2ItemTypes.h"
#include "Items/MT2ItemBonusSettings.h"
#include "Mobs/MT2MobTypes.h"
#include "Components/MT2AffectTypes.h"
#include "Skills/MT2SkillDefinition.h"
#include "MT2SemanticTypesLibrary.generated.h"

UCLASS()
class METIN2_API UMT2SemanticTypesLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category="MT2|Affects")
	static FMT2AffectIdentifier MakeBuiltinAffect(EMT2BuiltinAffect Affect);
	UFUNCTION(BlueprintPure, Category="MT2|Affects")
	static FMT2AffectIdentifier MakeSkillAffect(int32 SkillVnum);
	UFUNCTION(BlueprintPure, Category="MT2|Affects")
	static FMT2AffectIdentifier MakeCustomAffect(int32 CustomId);
	UFUNCTION(BlueprintPure, Category="MT2|Affects")
	static int32 GetLegacyAffectId(const FMT2AffectIdentifier& Identifier);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static int32 MakeEffectFlag(EMT2EffectFlag Flag);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static int32 MakeSkillFlag(EMT2SkillFlag Flag);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static int32 MakeMobAIFlag(EMT2MobAIFlag Flag);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static bool HasMobAIFlag(UPARAM(meta=(Bitmask,BitmaskEnum="/Script/Metin2.EMT2MobAIFlag")) int32 Flags, EMT2MobAIFlag Flag);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static int32 MakeMobRaceFlag(EMT2MobRaceFlag Flag);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static bool HasMobRaceFlag(UPARAM(meta=(Bitmask,BitmaskEnum="/Script/Metin2.EMT2MobRaceFlag")) int32 Flags, EMT2MobRaceFlag Flag);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static int32 MakeMobImmuneFlag(EMT2MobImmuneFlag Flag);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static bool HasMobImmuneFlag(UPARAM(meta=(Bitmask,BitmaskEnum="/Script/Metin2.EMT2MobImmuneFlag")) int32 Flags, EMT2MobImmuneFlag Flag);
	UFUNCTION(BlueprintPure, Category="MT2|Items")
	static FMT2ItemSubtype MakeWeaponSubtype(EMT2WeaponSubType Choice);
	UFUNCTION(BlueprintPure, Category="MT2|Items")
	static FMT2ItemSubtype MakeArmorSubtype(EMT2ArmorSubType Choice);
	UFUNCTION(BlueprintPure, Category="MT2|Items")
	static FMT2ItemSubtype MakeUseSubtype(EMT2UseSubType Choice);
	UFUNCTION(BlueprintPure, Category="MT2|Items")
	static FMT2ItemSubtype MakeAutoUseSubtype(EMT2AutoUseSubType Choice);
	UFUNCTION(BlueprintPure, Category="MT2|Items")
	static FMT2ItemSubtype MakeMaterialSubtype(EMT2MaterialSubType Choice);
	UFUNCTION(BlueprintPure, Category="MT2|Items")
	static FMT2ItemSubtype MakeMetinSubtype(EMT2MetinSubType Choice);
	UFUNCTION(BlueprintPure, Category="MT2|Items")
	static FMT2ItemSubtype MakeCostumeSubtype(EMT2CostumeSubType Choice);
	UFUNCTION(BlueprintPure, Category="MT2|Items")
	static FMT2ItemSubtype MakeUniqueSubtype(EMT2UniqueSubType Choice);
	UFUNCTION(BlueprintPure, Category="MT2|Items")
	static FMT2ItemSubtype MakeSpecialSubtype(EMT2SpecialSubType Choice);
	UFUNCTION(BlueprintPure, Category="MT2|Items")
	static FMT2ItemSubtype MakeToolSubtype(EMT2ToolSubType Choice);
	UFUNCTION(BlueprintPure, Category="MT2|Items")
	static FMT2ItemSubtype MakeLotterySubtype(EMT2LotterySubType Choice);
	UFUNCTION(BlueprintPure, Category="MT2|Items")
	static FMT2ItemSubtype MakeFishSubtype(EMT2FishSubType Choice);
	UFUNCTION(BlueprintPure, Category="MT2|Items")
	static FMT2ItemSubtype MakeResourceSubtype(EMT2ResourceSubType Choice);
	UFUNCTION(BlueprintPure, Category="MT2|Items")
	static FMT2ItemSubtype MakeExtractSubtype(EMT2ExtractSubType Choice);
	UFUNCTION(BlueprintPure, Category="MT2|Items")
	static FMT2ItemSubtype MakeDragonSoulSubtype(EMT2DragonSoulSubType Choice);
	UFUNCTION(BlueprintPure, Category="MT2|Items")
	static bool IsItemSubtypeForFamily(const FMT2ItemSubtype& Subtype,EMT2ItemType Family);
	UFUNCTION(BlueprintCallable, Category="MT2|Items")
	static void SetItemSubtype(UPARAM(ref) FMT2ItemDefinition& Item,const FMT2ItemSubtype& Subtype);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static int32 MakeItemFlag(EMT2ItemFlag Flag);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static bool HasItemFlag(UPARAM(meta=(Bitmask,BitmaskEnum="/Script/Metin2.EMT2ItemFlag")) int32 Flags, EMT2ItemFlag Flag);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static int32 MakeItemAntiFlag(EMT2ItemAntiFlag Flag);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static bool HasItemAntiFlag(UPARAM(meta=(Bitmask,BitmaskEnum="/Script/Metin2.EMT2ItemAntiFlag")) int32 Flags, EMT2ItemAntiFlag Flag);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static int32 MakeItemImmuneFlag(EMT2ItemImmuneFlag Flag);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static bool HasItemImmuneFlag(UPARAM(meta=(Bitmask,BitmaskEnum="/Script/Metin2.EMT2ItemImmuneFlag")) int32 Flags, EMT2ItemImmuneFlag Flag);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static int32 MakeItemWearFlag(EMT2ItemWearFlag Flag);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static bool HasItemWearFlag(UPARAM(meta=(Bitmask,BitmaskEnum="/Script/Metin2.EMT2ItemWearFlag")) int32 Flags, EMT2ItemWearFlag Flag);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static int32 MakeItemBonusTargetFlag(EMT2ItemBonusTarget Flag);
	UFUNCTION(BlueprintPure, Category="MT2|Flags")
	static bool HasItemBonusTargetFlag(UPARAM(meta=(Bitmask,BitmaskEnum="/Script/Metin2.EMT2ItemBonusTarget")) int32 Flags, EMT2ItemBonusTarget Flag);
};
