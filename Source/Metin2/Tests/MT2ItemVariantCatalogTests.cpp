#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Items/MT2ItemTemplateResolver.h"
#include "Items/Templates/MT2ItemVariantTemplates.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2ItemVariantCatalogTest, "Metin2.Items.Authoring.VariantCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2ItemVariantCatalogTest::RunTest(const FString&)
{
	struct FCase { EMT2ItemType Type; int32 RawSubtype; UClass* Expected; UClass* Family; const TCHAR* LegacyName; };
	auto TestResolvedClass = [this](const TCHAR* Label, UClass* Actual, UClass* Expected)
	{
		TestTrue(Label, Actual == Expected);
	};
	const FCase Cases[] = {
		{ EMT2ItemType::Armor, 0, UMT2ItemArmorBodyTemplate::StaticClass(), UMT2ItemArmorTemplate::StaticClass(), TEXT("ARMOR_BODY") },
		{ EMT2ItemType::Armor, 6, UMT2ItemArmorEarTemplate::StaticClass(), UMT2ItemArmorTemplate::StaticClass(), TEXT("ARMOR_EAR") },
		{ EMT2ItemType::Armor, 4, UMT2ItemArmorFootwearTemplate::StaticClass(), UMT2ItemArmorTemplate::StaticClass(), TEXT("ARMOR_FOOTS") },
		{ EMT2ItemType::Armor, 1, UMT2ItemArmorHeadTemplate::StaticClass(), UMT2ItemArmorTemplate::StaticClass(), TEXT("ARMOR_HEAD") },
		{ EMT2ItemType::Armor, 5, UMT2ItemArmorNeckTemplate::StaticClass(), UMT2ItemArmorTemplate::StaticClass(), TEXT("ARMOR_NECK") },
		{ EMT2ItemType::Armor, 2, UMT2ItemArmorShieldTemplate::StaticClass(), UMT2ItemArmorTemplate::StaticClass(), TEXT("ARMOR_SHIELD") },
		{ EMT2ItemType::Armor, 3, UMT2ItemArmorWristTemplate::StaticClass(), UMT2ItemArmorTemplate::StaticClass(), TEXT("ARMOR_WRIST") },
		{ EMT2ItemType::AutoUse, 1, UMT2ItemAutoUseAbilityUpTemplate::StaticClass(), UMT2ItemAutoUseTemplate::StaticClass(), TEXT("AUTOUSE_ABILITY_UP") },
		{ EMT2ItemType::AutoUse, 2, UMT2ItemAutoUseBombTemplate::StaticClass(), UMT2ItemAutoUseTemplate::StaticClass(), TEXT("AUTOUSE_BOMB") },
		{ EMT2ItemType::AutoUse, 3, UMT2ItemAutoUseGoldTemplate::StaticClass(), UMT2ItemAutoUseTemplate::StaticClass(), TEXT("AUTOUSE_GOLD") },
		{ EMT2ItemType::AutoUse, 4, UMT2ItemAutoUseMoneybagTemplate::StaticClass(), UMT2ItemAutoUseTemplate::StaticClass(), TEXT("AUTOUSE_MONEYBAG") },
		{ EMT2ItemType::AutoUse, 0, UMT2ItemAutoUsePotionTemplate::StaticClass(), UMT2ItemAutoUseTemplate::StaticClass(), TEXT("AUTOUSE_POTION") },
		{ EMT2ItemType::AutoUse, 5, UMT2ItemAutoUseTreasureBoxTemplate::StaticClass(), UMT2ItemAutoUseTemplate::StaticClass(), TEXT("AUTOUSE_TREASURE_BOX") },
		{ EMT2ItemType::Costume, 0, UMT2ItemCostumeBodyTemplate::StaticClass(), UMT2ItemCostumeTemplate::StaticClass(), TEXT("COSTUME_BODY") },
		{ EMT2ItemType::Costume, 1, UMT2ItemCostumeHairTemplate::StaticClass(), UMT2ItemCostumeTemplate::StaticClass(), TEXT("COSTUME_HAIR") },
		{ EMT2ItemType::DragonSoul, 0, UMT2ItemDragonSoulSlot1Template::StaticClass(), UMT2ItemDragonSoulTemplate::StaticClass(), TEXT("DS_SLOT1") },
		{ EMT2ItemType::DragonSoul, 1, UMT2ItemDragonSoulSlot2Template::StaticClass(), UMT2ItemDragonSoulTemplate::StaticClass(), TEXT("DS_SLOT2") },
		{ EMT2ItemType::DragonSoul, 2, UMT2ItemDragonSoulSlot3Template::StaticClass(), UMT2ItemDragonSoulTemplate::StaticClass(), TEXT("DS_SLOT3") },
		{ EMT2ItemType::DragonSoul, 3, UMT2ItemDragonSoulSlot4Template::StaticClass(), UMT2ItemDragonSoulTemplate::StaticClass(), TEXT("DS_SLOT4") },
		{ EMT2ItemType::DragonSoul, 4, UMT2ItemDragonSoulSlot5Template::StaticClass(), UMT2ItemDragonSoulTemplate::StaticClass(), TEXT("DS_SLOT5") },
		{ EMT2ItemType::DragonSoul, 5, UMT2ItemDragonSoulSlot6Template::StaticClass(), UMT2ItemDragonSoulTemplate::StaticClass(), TEXT("DS_SLOT6") },
		{ EMT2ItemType::Extract, 1, UMT2ItemExtractDragonHeartTemplate::StaticClass(), UMT2ItemExtractTemplate::StaticClass(), TEXT("EXTRACT_DRAGON_HEART") },
		{ EMT2ItemType::Extract, 0, UMT2ItemExtractDragonSoulTemplate::StaticClass(), UMT2ItemExtractTemplate::StaticClass(), TEXT("EXTRACT_DRAGON_SOUL") },
		{ EMT2ItemType::Fish, 0, UMT2ItemFishAliveTemplate::StaticClass(), UMT2ItemFishTemplate::StaticClass(), TEXT("FISH_ALIVE") },
		{ EMT2ItemType::Fish, 1, UMT2ItemFishDeadTemplate::StaticClass(), UMT2ItemFishTemplate::StaticClass(), TEXT("FISH_DEAD") },
		{ EMT2ItemType::Lottery, 1, UMT2ItemLotteryInstantTemplate::StaticClass(), UMT2ItemLotteryTemplate::StaticClass(), TEXT("LOTTERY_INSTANT") },
		{ EMT2ItemType::Lottery, 0, UMT2ItemLotteryTicketTemplate::StaticClass(), UMT2ItemLotteryTemplate::StaticClass(), TEXT("LOTTERY_TICKET") },
		{ EMT2ItemType::Material, 1, UMT2ItemMaterialBloodTemplate::StaticClass(), UMT2ItemMaterialTemplate::StaticClass(), TEXT("MATERIAL_BLOOD") },
		{ EMT2ItemType::Material, 6, UMT2ItemMaterialDragonSoulRefineBlessedTemplate::StaticClass(), UMT2ItemMaterialTemplate::StaticClass(), TEXT("MATERIAL_DS_REFINE_BLESSED") },
		{ EMT2ItemType::Material, 7, UMT2ItemMaterialDragonSoulRefineHolyTemplate::StaticClass(), UMT2ItemMaterialTemplate::StaticClass(), TEXT("MATERIAL_DS_REFINE_HOLLY") },
		{ EMT2ItemType::Material, 5, UMT2ItemMaterialDragonSoulRefineNormalTemplate::StaticClass(), UMT2ItemMaterialTemplate::StaticClass(), TEXT("MATERIAL_DS_REFINE_NORMAL") },
		{ EMT2ItemType::Material, 4, UMT2ItemMaterialJewelTemplate::StaticClass(), UMT2ItemMaterialTemplate::StaticClass(), TEXT("MATERIAL_JEWEL") },
		{ EMT2ItemType::Material, 0, UMT2ItemMaterialLeatherTemplate::StaticClass(), UMT2ItemMaterialTemplate::StaticClass(), TEXT("MATERIAL_LEATHER") },
		{ EMT2ItemType::Material, 3, UMT2ItemMaterialNeedleTemplate::StaticClass(), UMT2ItemMaterialTemplate::StaticClass(), TEXT("MATERIAL_NEEDLE") },
		{ EMT2ItemType::Material, 2, UMT2ItemMaterialRootTemplate::StaticClass(), UMT2ItemMaterialTemplate::StaticClass(), TEXT("MATERIAL_ROOT") },
		{ EMT2ItemType::Metin, 1, UMT2ItemMetinGoldTemplate::StaticClass(), UMT2ItemMetinStoneTemplate::StaticClass(), TEXT("METIN_GOLD") },
		{ EMT2ItemType::Metin, 0, UMT2ItemMetinNormalTemplate::StaticClass(), UMT2ItemMetinStoneTemplate::StaticClass(), TEXT("METIN_NORMAL") },
		{ EMT2ItemType::Resource, 3, UMT2ItemResourceBloodPearlTemplate::StaticClass(), UMT2ItemResourceTemplate::StaticClass(), TEXT("RESOURCE_BLOOD_PEARL") },
		{ EMT2ItemType::Resource, 4, UMT2ItemResourceBluePearlTemplate::StaticClass(), UMT2ItemResourceTemplate::StaticClass(), TEXT("RESOURCE_BLUE_PEARL") },
		{ EMT2ItemType::Resource, 6, UMT2ItemResourceBucketTemplate::StaticClass(), UMT2ItemResourceTemplate::StaticClass(), TEXT("RESOURCE_BUCKET") },
		{ EMT2ItemType::Resource, 7, UMT2ItemResourceCrystalTemplate::StaticClass(), UMT2ItemResourceTemplate::StaticClass(), TEXT("RESOURCE_CRYSTAL") },
		{ EMT2ItemType::Resource, 0, UMT2ItemResourceFishboneTemplate::StaticClass(), UMT2ItemResourceTemplate::StaticClass(), TEXT("RESOURCE_FISHBONE") },
		{ EMT2ItemType::Resource, 8, UMT2ItemResourceGemTemplate::StaticClass(), UMT2ItemResourceTemplate::StaticClass(), TEXT("RESOURCE_GEM") },
		{ EMT2ItemType::Resource, 10, UMT2ItemResourceMetinTemplate::StaticClass(), UMT2ItemResourceTemplate::StaticClass(), TEXT("RESOURCE_METIN") },
		{ EMT2ItemType::Resource, 11, UMT2ItemResourceOreTemplate::StaticClass(), UMT2ItemResourceTemplate::StaticClass(), TEXT("RESOURCE_ORE") },
		{ EMT2ItemType::Resource, 9, UMT2ItemResourceStoneTemplate::StaticClass(), UMT2ItemResourceTemplate::StaticClass(), TEXT("RESOURCE_STONE") },
		{ EMT2ItemType::Resource, 1, UMT2ItemResourceWaterstonePieceTemplate::StaticClass(), UMT2ItemResourceTemplate::StaticClass(), TEXT("RESOURCE_WATERSTONEPIECE") },
		{ EMT2ItemType::Resource, 2, UMT2ItemResourceWaterstoneTemplate::StaticClass(), UMT2ItemResourceTemplate::StaticClass(), TEXT("RESOURCE_WATERSTONE") },
		{ EMT2ItemType::Resource, 5, UMT2ItemResourceWhitePearlTemplate::StaticClass(), UMT2ItemResourceTemplate::StaticClass(), TEXT("RESOURCE_WHITE_PEARL") },
		{ EMT2ItemType::Special, 2, UMT2ItemSpecialDocumentTemplate::StaticClass(), UMT2ItemSpecialTemplate::StaticClass(), TEXT("SPECIAL_DOC") },
		{ EMT2ItemType::SpecialDragonSoul, 0, UMT2ItemSpecialDragonSoulSlot1Template::StaticClass(), UMT2ItemSpecialDragonSoulTemplate::StaticClass(), TEXT("DS_SLOT1") },
		{ EMT2ItemType::SpecialDragonSoul, 1, UMT2ItemSpecialDragonSoulSlot2Template::StaticClass(), UMT2ItemSpecialDragonSoulTemplate::StaticClass(), TEXT("DS_SLOT2") },
		{ EMT2ItemType::SpecialDragonSoul, 2, UMT2ItemSpecialDragonSoulSlot3Template::StaticClass(), UMT2ItemSpecialDragonSoulTemplate::StaticClass(), TEXT("DS_SLOT3") },
		{ EMT2ItemType::SpecialDragonSoul, 3, UMT2ItemSpecialDragonSoulSlot4Template::StaticClass(), UMT2ItemSpecialDragonSoulTemplate::StaticClass(), TEXT("DS_SLOT4") },
		{ EMT2ItemType::SpecialDragonSoul, 4, UMT2ItemSpecialDragonSoulSlot5Template::StaticClass(), UMT2ItemSpecialDragonSoulTemplate::StaticClass(), TEXT("DS_SLOT5") },
		{ EMT2ItemType::SpecialDragonSoul, 5, UMT2ItemSpecialDragonSoulSlot6Template::StaticClass(), UMT2ItemSpecialDragonSoulTemplate::StaticClass(), TEXT("DS_SLOT6") },
		{ EMT2ItemType::Special, 1, UMT2ItemSpecialKeyTemplate::StaticClass(), UMT2ItemSpecialTemplate::StaticClass(), TEXT("SPECIAL_KEY") },
		{ EMT2ItemType::Special, 0, UMT2ItemSpecialMapTemplate::StaticClass(), UMT2ItemSpecialTemplate::StaticClass(), TEXT("SPECIAL_MAP") },
		{ EMT2ItemType::Special, 3, UMT2ItemSpecialSpiritTemplate::StaticClass(), UMT2ItemSpecialTemplate::StaticClass(), TEXT("SPECIAL_SPIRIT") },
		{ EMT2ItemType::Tool, 0, UMT2ItemToolFishingRodTemplate::StaticClass(), UMT2ItemToolTemplate::StaticClass(), TEXT("TOOL_FISHING_ROD") },
		{ EMT2ItemType::Unique, 1, UMT2ItemUniqueBookTemplate::StaticClass(), UMT2ItemUniqueTemplate::StaticClass(), TEXT("UNIQUE_BOOK") },
		{ EMT2ItemType::Unique, 4, UMT2ItemUniqueLegacySubtype4Template::StaticClass(), UMT2ItemUniqueTemplate::StaticClass(), TEXT("UNIQUE_4") },
		{ EMT2ItemType::Unique, 5, UMT2ItemUniqueLegacySubtype5Template::StaticClass(), UMT2ItemUniqueTemplate::StaticClass(), TEXT("UNIQUE_5") },
		{ EMT2ItemType::Unique, 6, UMT2ItemUniqueLegacySubtype6Template::StaticClass(), UMT2ItemUniqueTemplate::StaticClass(), TEXT("UNIQUE_6") },
		{ EMT2ItemType::Unique, 7, UMT2ItemUniqueLegacySubtype7Template::StaticClass(), UMT2ItemUniqueTemplate::StaticClass(), TEXT("UNIQUE_7") },
		{ EMT2ItemType::Unique, 8, UMT2ItemUniqueLegacySubtype8Template::StaticClass(), UMT2ItemUniqueTemplate::StaticClass(), TEXT("UNIQUE_8") },
		{ EMT2ItemType::Unique, 9, UMT2ItemUniqueLegacySubtype9Template::StaticClass(), UMT2ItemUniqueTemplate::StaticClass(), TEXT("UNIQUE_9") },
		{ EMT2ItemType::Unique, 0, UMT2ItemUniqueNoneTemplate::StaticClass(), UMT2ItemUniqueTemplate::StaticClass(), TEXT("UNIQUE_NONE") },
		{ EMT2ItemType::Unique, 3, UMT2ItemUniqueSpecialMountRideTemplate::StaticClass(), UMT2ItemUniqueTemplate::StaticClass(), TEXT("UNIQUE_3 (legacy enum value UNIQUE_SPECIAL_MOUNT_RIDE)") },
		{ EMT2ItemType::Unique, 2, UMT2ItemUniqueSpecialRideTemplate::StaticClass(), UMT2ItemUniqueTemplate::StaticClass(), TEXT("UNIQUE_SPECIAL_RIDE") },
		{ EMT2ItemType::Unique, 10, UMT2ItemUniqueSpecialUseTemplate::StaticClass(), UMT2ItemUniqueTemplate::StaticClass(), TEXT("USE_SPECIAL accepted in ITEM_UNIQUE table") },
		{ EMT2ItemType::Use, 7, UMT2ItemUseAbilityUpTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_ABILITY_UP") },
		{ EMT2ItemType::Use, 20, UMT2ItemUseAddAccessorySocketTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_ADD_ACCESSORY_SOCKET") },
		{ EMT2ItemType::Use, 22, UMT2ItemUseAddAttribute2Template::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_ADD_ATTRIBUTE2") },
		{ EMT2ItemType::Use, 19, UMT2ItemUseAddAttributeTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_ADD_ATTRIBUTE") },
		{ EMT2ItemType::Use, 8, UMT2ItemUseAffectTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_AFFECT") },
		{ EMT2ItemType::Use, 6, UMT2ItemUseBaitTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_BAIT") },
		{ EMT2ItemType::Use, 25, UMT2ItemUseBindTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_BIND") },
		{ EMT2ItemType::Use, 15, UMT2ItemUseBucketTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_BUCKET") },
		{ EMT2ItemType::Use, 24, UMT2ItemUseChangeAttribute2Template::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_CHANGE_ATTRIBUTE2") },
		{ EMT2ItemType::Use, 18, UMT2ItemUseChangeAttributeTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_CHANGE_ATTRIBUTE") },
		{ EMT2ItemType::Use, 17, UMT2ItemUseCleanSocketTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_CLEAN_SOCKET") },
		{ EMT2ItemType::Use, 12, UMT2ItemUseClearTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_CLEAR") },
		{ EMT2ItemType::Use, 9, UMT2ItemUseCreateStoneTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_CREATE_STONE") },
		{ EMT2ItemType::Use, 14, UMT2ItemUseDetachmentTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_DETACHMENT") },
		{ EMT2ItemType::Use, 13, UMT2ItemUseInvisibilityTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_INVISIBILITY") },
		{ EMT2ItemType::Use, 5, UMT2ItemUseMoneybagTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_MONEYBAG") },
		{ EMT2ItemType::Use, 3, UMT2ItemUseMoveTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_MOVE") },
		{ EMT2ItemType::Use, 16, UMT2ItemUsePotionContinueTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_POTION_CONTINUE") },
		{ EMT2ItemType::Use, 11, UMT2ItemUsePotionNoDelayTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_POTION_NODELAY") },
		{ EMT2ItemType::Use, 0, UMT2ItemUsePotionTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_POTION") },
		{ EMT2ItemType::Use, 21, UMT2ItemUsePutIntoAccessorySocketTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_PUT_INTO_ACCESSORY_SOCKET") },
		{ EMT2ItemType::Use, 29, UMT2ItemUsePutIntoBeltSocketTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_PUT_INTO_BELT_SOCKET") },
		{ EMT2ItemType::Use, 30, UMT2ItemUsePutIntoRingSocketTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_PUT_INTO_RING_SOCKET") },
		{ EMT2ItemType::Use, 23, UMT2ItemUseRecipeTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_RECIPE") },
		{ EMT2ItemType::Use, 10, UMT2ItemUseSpecialTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_SPECIAL") },
		{ EMT2ItemType::Use, 1, UMT2ItemUseTalismanTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_TALISMAN") },
		{ EMT2ItemType::Use, 28, UMT2ItemUseTimeChargeFixedTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_TIME_CHARGE_FIX") },
		{ EMT2ItemType::Use, 27, UMT2ItemUseTimeChargePercentTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_TIME_CHARGE_PER") },
		{ EMT2ItemType::Use, 4, UMT2ItemUseTreasureBoxTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_TREASURE_BOX") },
		{ EMT2ItemType::Use, 2, UMT2ItemUseTuningTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_TUNING") },
		{ EMT2ItemType::Use, 26, UMT2ItemUseUnbindTemplate::StaticClass(), UMT2ItemUseTemplate::StaticClass(), TEXT("USE_UNBIND") },
		{ EMT2ItemType::Weapon, 6, UMT2ItemWeaponArrowTemplate::StaticClass(), UMT2ItemWeaponTemplate::StaticClass(), TEXT("WEAPON_ARROW") },
		{ EMT2ItemType::Weapon, 4, UMT2ItemWeaponBellTemplate::StaticClass(), UMT2ItemWeaponTemplate::StaticClass(), TEXT("WEAPON_BELL") },
		{ EMT2ItemType::Weapon, 2, UMT2ItemWeaponBowTemplate::StaticClass(), UMT2ItemWeaponTemplate::StaticClass(), TEXT("WEAPON_BOW") },
		{ EMT2ItemType::Weapon, 1, UMT2ItemWeaponDaggerTemplate::StaticClass(), UMT2ItemWeaponTemplate::StaticClass(), TEXT("WEAPON_DAGGER") },
		{ EMT2ItemType::Weapon, 5, UMT2ItemWeaponFanTemplate::StaticClass(), UMT2ItemWeaponTemplate::StaticClass(), TEXT("WEAPON_FAN") },
		{ EMT2ItemType::Weapon, 7, UMT2ItemWeaponMountSpearTemplate::StaticClass(), UMT2ItemWeaponTemplate::StaticClass(), TEXT("WEAPON_MOUNT_SPEAR") },
		{ EMT2ItemType::Weapon, 0, UMT2ItemWeaponSwordTemplate::StaticClass(), UMT2ItemWeaponTemplate::StaticClass(), TEXT("WEAPON_SWORD") },
		{ EMT2ItemType::Weapon, 3, UMT2ItemWeaponTwoHandedTemplate::StaticClass(), UMT2ItemWeaponTemplate::StaticClass(), TEXT("WEAPON_TWO_HANDED") },
	};
	for (const FCase& Case : Cases)
	{
		const FString Label = FString::Printf(TEXT("ItemType %d / subtype %d (%s)"), static_cast<int32>(Case.Type), Case.RawSubtype, Case.LegacyName);
		UClass* Actual = ResolveMT2ItemTemplateClass(Case.Type, Case.RawSubtype);
		TestTrue(Label, Actual == Case.Expected);
		TestTrue(Label + TEXT(" direct family parent"), Actual && Actual->GetSuperClass() == Case.Family);
	}
	TestResolvedClass(TEXT("Yang family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::Yang, 0), UMT2ItemGoldTemplate::StaticClass());
	TestResolvedClass(TEXT("Container family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::Container, 0), UMT2ItemContainerTemplate::StaticClass());
	TestResolvedClass(TEXT("Rod family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::Rod, 0), UMT2ItemRodTemplate::StaticClass());
	TestResolvedClass(TEXT("Campfire family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::Campfire, 0), UMT2ItemCampfireTemplate::StaticClass());
	TestResolvedClass(TEXT("SkillBook family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::SkillBook, 0), UMT2ItemSkillBookTemplate::StaticClass());
	TestResolvedClass(TEXT("Quest family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::Quest, 0), UMT2ItemQuestTemplate::StaticClass());
	TestResolvedClass(TEXT("Polymorph family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::Polymorph, 0), UMT2ItemPolymorphTemplate::StaticClass());
	TestResolvedClass(TEXT("TreasureBox family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::TreasureBox, 0), UMT2ItemTreasureBoxTemplate::StaticClass());
	TestResolvedClass(TEXT("TreasureKey family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::TreasureKey, 0), UMT2ItemTreasureKeyTemplate::StaticClass());
	TestResolvedClass(TEXT("SkillForget family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::SkillForget, 0), UMT2ItemSkillForgetTemplate::StaticClass());
	TestResolvedClass(TEXT("GiftBox family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::GiftBox, 0), UMT2ItemGiftBoxTemplate::StaticClass());
	TestResolvedClass(TEXT("Pick family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::Pick, 0), UMT2ItemPickTemplate::StaticClass());
	TestResolvedClass(TEXT("Hair family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::Hair, 0), UMT2ItemHairTemplate::StaticClass());
	TestResolvedClass(TEXT("Totem family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::Totem, 0), UMT2ItemTotemTemplate::StaticClass());
	TestResolvedClass(TEXT("Blend family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::Blend, 0), UMT2ItemBlendTemplate::StaticClass());
	TestResolvedClass(TEXT("SecondaryCoin family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::SecondaryCoin, 0), UMT2ItemSecondaryCoinTemplate::StaticClass());
	TestResolvedClass(TEXT("Ring family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::Ring, 0), UMT2ItemRingTemplate::StaticClass());
	TestResolvedClass(TEXT("Belt family fallback"), ResolveMT2ItemTemplateClass(EMT2ItemType::Belt, 0), UMT2ItemBeltTemplate::StaticClass());
	TestNull(TEXT("Weapon sentinel is not an item class"), ResolveMT2ItemTemplateClass(EMT2ItemType::Weapon, 8));
	TestNull(TEXT("Armor sentinel is not an item class"), ResolveMT2ItemTemplateClass(EMT2ItemType::Armor, 7));
	TestNull(TEXT("Unknown use subtype is rejected"), ResolveMT2ItemTemplateClass(EMT2ItemType::Use, 31));
	TestNull(TEXT("Unknown unique subtype is rejected"), ResolveMT2ItemTemplateClass(EMT2ItemType::Unique, 11));
	return true;
}
#endif
