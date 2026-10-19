#include "Items/MT2ItemTemplateResolver.h"
#include "Items/Templates/MT2ItemVariantTemplates.h"

UClass* ResolveMT2ItemTemplateClass(EMT2ItemType ItemType, int32 LegacySubtype)
{
	switch (ItemType)
	{
	case EMT2ItemType::Armor:
			switch (LegacySubtype)
			{
			case 0: return UMT2ItemArmorBodyTemplate::StaticClass();
			case 1: return UMT2ItemArmorHeadTemplate::StaticClass();
			case 2: return UMT2ItemArmorShieldTemplate::StaticClass();
			case 3: return UMT2ItemArmorWristTemplate::StaticClass();
			case 4: return UMT2ItemArmorFootwearTemplate::StaticClass();
			case 5: return UMT2ItemArmorNeckTemplate::StaticClass();
			case 6: return UMT2ItemArmorEarTemplate::StaticClass();
			default: return nullptr;
			}
	case EMT2ItemType::AutoUse:
			switch (LegacySubtype)
			{
			case 0: return UMT2ItemAutoUsePotionTemplate::StaticClass();
			case 1: return UMT2ItemAutoUseAbilityUpTemplate::StaticClass();
			case 2: return UMT2ItemAutoUseBombTemplate::StaticClass();
			case 3: return UMT2ItemAutoUseGoldTemplate::StaticClass();
			case 4: return UMT2ItemAutoUseMoneybagTemplate::StaticClass();
			case 5: return UMT2ItemAutoUseTreasureBoxTemplate::StaticClass();
			default: return nullptr;
			}
	case EMT2ItemType::Costume:
			switch (LegacySubtype)
			{
			case 0: return UMT2ItemCostumeBodyTemplate::StaticClass();
			case 1: return UMT2ItemCostumeHairTemplate::StaticClass();
			default: return nullptr;
			}
	case EMT2ItemType::DragonSoul:
			switch (LegacySubtype)
			{
			case 0: return UMT2ItemDragonSoulSlot1Template::StaticClass();
			case 1: return UMT2ItemDragonSoulSlot2Template::StaticClass();
			case 2: return UMT2ItemDragonSoulSlot3Template::StaticClass();
			case 3: return UMT2ItemDragonSoulSlot4Template::StaticClass();
			case 4: return UMT2ItemDragonSoulSlot5Template::StaticClass();
			case 5: return UMT2ItemDragonSoulSlot6Template::StaticClass();
			default: return nullptr;
			}
	case EMT2ItemType::Extract:
			switch (LegacySubtype)
			{
			case 0: return UMT2ItemExtractDragonSoulTemplate::StaticClass();
			case 1: return UMT2ItemExtractDragonHeartTemplate::StaticClass();
			default: return nullptr;
			}
	case EMT2ItemType::Fish:
			switch (LegacySubtype)
			{
			case 0: return UMT2ItemFishAliveTemplate::StaticClass();
			case 1: return UMT2ItemFishDeadTemplate::StaticClass();
			default: return nullptr;
			}
	case EMT2ItemType::Lottery:
			switch (LegacySubtype)
			{
			case 0: return UMT2ItemLotteryTicketTemplate::StaticClass();
			case 1: return UMT2ItemLotteryInstantTemplate::StaticClass();
			default: return nullptr;
			}
	case EMT2ItemType::Material:
			switch (LegacySubtype)
			{
			case 0: return UMT2ItemMaterialLeatherTemplate::StaticClass();
			case 1: return UMT2ItemMaterialBloodTemplate::StaticClass();
			case 2: return UMT2ItemMaterialRootTemplate::StaticClass();
			case 3: return UMT2ItemMaterialNeedleTemplate::StaticClass();
			case 4: return UMT2ItemMaterialJewelTemplate::StaticClass();
			case 5: return UMT2ItemMaterialDragonSoulRefineNormalTemplate::StaticClass();
			case 6: return UMT2ItemMaterialDragonSoulRefineBlessedTemplate::StaticClass();
			case 7: return UMT2ItemMaterialDragonSoulRefineHolyTemplate::StaticClass();
			default: return nullptr;
			}
	case EMT2ItemType::Metin:
			switch (LegacySubtype)
			{
			case 0: return UMT2ItemMetinNormalTemplate::StaticClass();
			case 1: return UMT2ItemMetinGoldTemplate::StaticClass();
			default: return nullptr;
			}
	case EMT2ItemType::Resource:
			switch (LegacySubtype)
			{
			case 0: return UMT2ItemResourceFishboneTemplate::StaticClass();
			case 1: return UMT2ItemResourceWaterstonePieceTemplate::StaticClass();
			case 2: return UMT2ItemResourceWaterstoneTemplate::StaticClass();
			case 3: return UMT2ItemResourceBloodPearlTemplate::StaticClass();
			case 4: return UMT2ItemResourceBluePearlTemplate::StaticClass();
			case 5: return UMT2ItemResourceWhitePearlTemplate::StaticClass();
			case 6: return UMT2ItemResourceBucketTemplate::StaticClass();
			case 7: return UMT2ItemResourceCrystalTemplate::StaticClass();
			case 8: return UMT2ItemResourceGemTemplate::StaticClass();
			case 9: return UMT2ItemResourceStoneTemplate::StaticClass();
			case 10: return UMT2ItemResourceMetinTemplate::StaticClass();
			case 11: return UMT2ItemResourceOreTemplate::StaticClass();
			default: return nullptr;
			}
	case EMT2ItemType::Special:
			switch (LegacySubtype)
			{
			case 0: return UMT2ItemSpecialMapTemplate::StaticClass();
			case 1: return UMT2ItemSpecialKeyTemplate::StaticClass();
			case 2: return UMT2ItemSpecialDocumentTemplate::StaticClass();
			case 3: return UMT2ItemSpecialSpiritTemplate::StaticClass();
			default: return nullptr;
			}
	case EMT2ItemType::SpecialDragonSoul:
			switch (LegacySubtype)
			{
			case 0: return UMT2ItemSpecialDragonSoulSlot1Template::StaticClass();
			case 1: return UMT2ItemSpecialDragonSoulSlot2Template::StaticClass();
			case 2: return UMT2ItemSpecialDragonSoulSlot3Template::StaticClass();
			case 3: return UMT2ItemSpecialDragonSoulSlot4Template::StaticClass();
			case 4: return UMT2ItemSpecialDragonSoulSlot5Template::StaticClass();
			case 5: return UMT2ItemSpecialDragonSoulSlot6Template::StaticClass();
			default: return nullptr;
			}
	case EMT2ItemType::Tool:
			switch (LegacySubtype)
			{
			case 0: return UMT2ItemToolFishingRodTemplate::StaticClass();
			default: return nullptr;
			}
	case EMT2ItemType::Unique:
			switch (LegacySubtype)
			{
			case 0: return UMT2ItemUniqueNoneTemplate::StaticClass();
			case 1: return UMT2ItemUniqueBookTemplate::StaticClass();
			case 2: return UMT2ItemUniqueSpecialRideTemplate::StaticClass();
			case 3: return UMT2ItemUniqueSpecialMountRideTemplate::StaticClass();
			case 4: return UMT2ItemUniqueLegacySubtype4Template::StaticClass();
			case 5: return UMT2ItemUniqueLegacySubtype5Template::StaticClass();
			case 6: return UMT2ItemUniqueLegacySubtype6Template::StaticClass();
			case 7: return UMT2ItemUniqueLegacySubtype7Template::StaticClass();
			case 8: return UMT2ItemUniqueLegacySubtype8Template::StaticClass();
			case 9: return UMT2ItemUniqueLegacySubtype9Template::StaticClass();
			case 10: return UMT2ItemUniqueSpecialUseTemplate::StaticClass();
			default: return nullptr;
			}
	case EMT2ItemType::Use:
			switch (LegacySubtype)
			{
			case 0: return UMT2ItemUsePotionTemplate::StaticClass();
			case 1: return UMT2ItemUseTalismanTemplate::StaticClass();
			case 2: return UMT2ItemUseTuningTemplate::StaticClass();
			case 3: return UMT2ItemUseMoveTemplate::StaticClass();
			case 4: return UMT2ItemUseTreasureBoxTemplate::StaticClass();
			case 5: return UMT2ItemUseMoneybagTemplate::StaticClass();
			case 6: return UMT2ItemUseBaitTemplate::StaticClass();
			case 7: return UMT2ItemUseAbilityUpTemplate::StaticClass();
			case 8: return UMT2ItemUseAffectTemplate::StaticClass();
			case 9: return UMT2ItemUseCreateStoneTemplate::StaticClass();
			case 10: return UMT2ItemUseSpecialTemplate::StaticClass();
			case 11: return UMT2ItemUsePotionNoDelayTemplate::StaticClass();
			case 12: return UMT2ItemUseClearTemplate::StaticClass();
			case 13: return UMT2ItemUseInvisibilityTemplate::StaticClass();
			case 14: return UMT2ItemUseDetachmentTemplate::StaticClass();
			case 15: return UMT2ItemUseBucketTemplate::StaticClass();
			case 16: return UMT2ItemUsePotionContinueTemplate::StaticClass();
			case 17: return UMT2ItemUseCleanSocketTemplate::StaticClass();
			case 18: return UMT2ItemUseChangeAttributeTemplate::StaticClass();
			case 19: return UMT2ItemUseAddAttributeTemplate::StaticClass();
			case 20: return UMT2ItemUseAddAccessorySocketTemplate::StaticClass();
			case 21: return UMT2ItemUsePutIntoAccessorySocketTemplate::StaticClass();
			case 22: return UMT2ItemUseAddAttribute2Template::StaticClass();
			case 23: return UMT2ItemUseRecipeTemplate::StaticClass();
			case 24: return UMT2ItemUseChangeAttribute2Template::StaticClass();
			case 25: return UMT2ItemUseBindTemplate::StaticClass();
			case 26: return UMT2ItemUseUnbindTemplate::StaticClass();
			case 27: return UMT2ItemUseTimeChargePercentTemplate::StaticClass();
			case 28: return UMT2ItemUseTimeChargeFixedTemplate::StaticClass();
			case 29: return UMT2ItemUsePutIntoBeltSocketTemplate::StaticClass();
			case 30: return UMT2ItemUsePutIntoRingSocketTemplate::StaticClass();
			default: return nullptr;
			}
	case EMT2ItemType::Weapon:
			switch (LegacySubtype)
			{
			case 0: return UMT2ItemWeaponSwordTemplate::StaticClass();
			case 1: return UMT2ItemWeaponDaggerTemplate::StaticClass();
			case 2: return UMT2ItemWeaponBowTemplate::StaticClass();
			case 3: return UMT2ItemWeaponTwoHandedTemplate::StaticClass();
			case 4: return UMT2ItemWeaponBellTemplate::StaticClass();
			case 5: return UMT2ItemWeaponFanTemplate::StaticClass();
			case 6: return UMT2ItemWeaponArrowTemplate::StaticClass();
			case 7: return UMT2ItemWeaponMountSpearTemplate::StaticClass();
			default: return nullptr;
			}
	case EMT2ItemType::Yang: return UMT2ItemGoldTemplate::StaticClass();
		case EMT2ItemType::Container: return UMT2ItemContainerTemplate::StaticClass();
		case EMT2ItemType::Rod: return UMT2ItemRodTemplate::StaticClass();
		case EMT2ItemType::Campfire: return UMT2ItemCampfireTemplate::StaticClass();
		case EMT2ItemType::SkillBook: return UMT2ItemSkillBookTemplate::StaticClass();
		case EMT2ItemType::Quest: return UMT2ItemQuestTemplate::StaticClass();
		case EMT2ItemType::Polymorph: return UMT2ItemPolymorphTemplate::StaticClass();
		case EMT2ItemType::TreasureBox: return UMT2ItemTreasureBoxTemplate::StaticClass();
		case EMT2ItemType::TreasureKey: return UMT2ItemTreasureKeyTemplate::StaticClass();
		case EMT2ItemType::SkillForget: return UMT2ItemSkillForgetTemplate::StaticClass();
		case EMT2ItemType::GiftBox: return UMT2ItemGiftBoxTemplate::StaticClass();
		case EMT2ItemType::Pick: return UMT2ItemPickTemplate::StaticClass();
		case EMT2ItemType::Hair: return UMT2ItemHairTemplate::StaticClass();
		case EMT2ItemType::Totem: return UMT2ItemTotemTemplate::StaticClass();
		case EMT2ItemType::Blend: return UMT2ItemBlendTemplate::StaticClass();
		case EMT2ItemType::SecondaryCoin: return UMT2ItemSecondaryCoinTemplate::StaticClass();
		case EMT2ItemType::Ring: return UMT2ItemRingTemplate::StaticClass();
		case EMT2ItemType::Belt: return UMT2ItemBeltTemplate::StaticClass();
		case EMT2ItemType::None: return UMT2ItemSpecialTemplate::StaticClass();
		default: return nullptr;
	}
}

