#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Equipment/MT2EquipmentComponent.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Combat/MT2CombatComponent.h"
#include "Items/MT2InventoryComponent.h"
#include "Items/MT2ItemTemplate.h"
#include "Quests/MT2QuestCondition.h"
#include "Quests/MT2QuestNode.h"
#include "Animation/MT2AnimationMotionData.h"
#include "Skills/MT2SkillComponent.h"
#include "Skills/MT2SkillDefinition.h"
#include "Skills/MT2SkillSet.h"
#include "Guild/MT2GuildComponent.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticApiTest,"Metin2.SemanticTypes.BlueprintFunctionPinsUseSemanticTypes",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticApiTest::RunTest(const FString&)
{
	struct FCase { UClass* Owner; const TCHAR* Function; const TCHAR* Pin; UEnum* Domain; };
	for (const auto& Case : TArray<FCase>{
		{UMT2EquipmentComponent::StaticClass(),TEXT("EquipWeapon"),TEXT("WeaponSubType"),StaticEnum<EMT2WeaponSubType>()},
		{UMT2EquipmentComponent::StaticClass(),TEXT("GetEquippedWeaponSubType"),TEXT("ReturnValue"),StaticEnum<EMT2WeaponSubType>()},
		{AMT2CharacterBase::StaticClass(),TEXT("SetWeaponAnimationSet"),TEXT("WeaponSubType"),StaticEnum<EMT2WeaponSubType>()},
		{AMT2PlayerCharacter::StaticClass(),TEXT("GetItemApplyBonus"),TEXT("ApplyType"),StaticEnum<EMT2ItemBonusType>()}})
	{
		UFunction* Function=Case.Owner->FindFunctionByName(Case.Function);
		if (!TestNotNull(Case.Function,Function)) { continue; }
		const auto* Pin=FindFProperty<FEnumProperty>(Function,Case.Pin);
		if (TestNotNull(*FString::Printf(TEXT("%s.%s is a named pin"),Case.Function,Case.Pin),Pin))
		{ TestEqual(TEXT("Pin uses its dedicated shared domain"),Pin->GetEnum(),Case.Domain); }
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticQuestSelectorsTest,"Metin2.SemanticTypes.QuestSelectorsUseSharedEnums",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticQuestSelectorsTest::RunTest(const FString&)
{
	const auto* Empire=FindFProperty<FEnumProperty>(UMT2QuestCondition_Empire::StaticClass(),TEXT("Empire"));
	const auto* Apply=FindFProperty<FEnumProperty>(UMT2QuestNode_ModifyAffect::StaticClass(),TEXT("ApplyType"));
	if (TestNotNull(TEXT("Quest empire is named"),Empire)) { TestEqual(TEXT("Quest empire reuses identity domain"),Empire->GetEnum(),StaticEnum<EMT2Empire>()); }
	if (TestNotNull(TEXT("Quest apply is named"),Apply)) { TestEqual(TEXT("Quest apply reuses item/effect domain"),Apply->GetEnum(),StaticEnum<EMT2ItemBonusType>()); }
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticHittingTest,"Metin2.SemanticTypes.HittingTypeMetadataRoundTrip",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticHittingTest::RunTest(const FString&)
{
	const auto* Event=FindFProperty<FEnumProperty>(FMT2MotionAttackEvent::StaticStruct(),TEXT("HittingType"));
	const auto* Motion=FindFProperty<FEnumProperty>(UMT2AnimationMotionData::StaticClass(),TEXT("HittingType"));
	if (!TestNotNull(TEXT("Attack-event hit kind is named"),Event) || !TestNotNull(TEXT("Motion hit kind is named"),Motion)) { return false; }
	TestEqual(TEXT("Motion and event share one verified domain"),Event->GetEnum(),Motion->GetEnum());
	TestEqual(TEXT("Legacy Great hit remains knockdown"),Event->GetEnum()->GetNameStringByValue(1),FString(TEXT("Knockdown")));
	TestEqual(TEXT("Legacy Good hit remains normal"),Event->GetEnum()->GetNameStringByValue(2),FString(TEXT("Normal")));
	float Force=0; decltype(FMT2MotionAttackEvent::HittingType) Kind{};
	UMT2AnimationMotionData::ReadKnockbackMetadata(TEXT("Group AttackingData\n{\nHittingType 2\nExternalForce 3\n}\nGroup Event00\n{\nHittingType 1\nExternalForce 17\n}\n"),Force,Kind);
	TestEqual(TEXT("Importer keeps strongest hit kind"),static_cast<int32>(Kind),1);
	TestEqual(TEXT("Force parsing is unchanged"),Force,17.f);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticSkillPinsTest,"Metin2.SemanticTypes.SkillGroupAndMasteryPinsAreNamed",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticSkillPinsTest::RunTest(const FString&)
{
	UEnum* Group=FindObject<UEnum>(nullptr,TEXT("/Script/Metin2.EMT2SkillGroup"));
	TestNotNull(TEXT("Two specializations have a dedicated named domain"),Group);
	for (const auto& Pair : TArray<TPair<const TCHAR*,const TCHAR*>>{
		{TEXT("GetSkillGroup"),TEXT("ReturnValue")},{TEXT("SelectSkillGroup"),TEXT("NewSkillGroup")},{TEXT("ServerSelectSkillGroup"),TEXT("NewSkillGroup")}})
	{
		const auto* Pin=FindFProperty<FEnumProperty>(UMT2SkillComponent::StaticClass()->FindFunctionByName(Pair.Key),Pair.Value);
		if (TestNotNull(TEXT("Skill-group pins are named"),Pin)) { TestEqual(TEXT("One group domain is shared"),Pin->GetEnum(),Group); }
	}
	TestNotNull(TEXT("Skill-set group authoring is named"),FindFProperty<FEnumProperty>(UMT2SkillSet::StaticClass(),TEXT("GroupIndex")));
	for (const TCHAR* Name : {TEXT("GetCastAnimation"),TEXT("GetGradeDisplayName"),TEXT("GetIconRegion")})
	{
		const auto* Pin=FindFProperty<FEnumProperty>(UMT2SkillDefinition::StaticClass()->FindFunctionByName(Name),TEXT("MasteryGrade"));
		if (TestNotNull(TEXT("Mastery input is named"),Pin)) { TestEqual(TEXT("Mastery reuses the existing domain"),Pin->GetEnum(),StaticEnum<EMT2SkillMastery>()); }
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticRemainingMasksTest,"Metin2.SemanticTypes.RemainingFlagInputsAreNamed",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticRemainingMasksTest::RunTest(const FString&)
{
	struct FCase { UClass* Owner; const TCHAR* Function; const TCHAR* Pin; const TCHAR* Enum; };
	for (const auto& Case : TArray<FCase>{
		{UMT2ItemTemplate::StaticClass(),TEXT("HasItemFlag"),TEXT("ItemFlag"),TEXT("/Script/Metin2.EMT2ItemFlag")},
		{UMT2GuildComponent::StaticClass(),TEXT("ChangeRank"),TEXT("Permissions"),TEXT("/Script/Metin2.EMT2GuildPermission")}})
	{
		const auto* Pin=FindFProperty<FIntProperty>(Case.Owner->FindFunctionByName(Case.Function),Case.Pin);
		if (!TestNotNull(Case.Pin,Pin)) { continue; }
		TestTrue(TEXT("Combined-mask input has named checkboxes"),Pin->HasMetaData(TEXT("Bitmask")));
		TestEqual(TEXT("Input uses its dedicated flag domain"),Pin->GetMetaData(TEXT("BitmaskEnum")),FString(Case.Enum));
	}
	return true;
}
#endif
