#include "Items/Templates/MT2ItemArmorTemplate.h"
#include "Items/Templates/MT2ItemAutoUseTemplate.h"
#include "Items/Templates/MT2ItemCostumeTemplate.h"
#include "Items/Templates/MT2ItemMaterialTemplate.h"
#include "Items/Templates/MT2ItemMetinStoneTemplate.h"
#include "Items/Templates/MT2ItemUniqueTemplate.h"
#include "Items/Templates/MT2ItemUseTemplate.h"
#include "Items/Templates/MT2ItemWeaponTemplate.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Items/MT2ItemBonusSettings.h"
#include "Items/MT2ItemTemplate.h"
#include "Mobs/MT2Mob.h"
#include "Mobs/MT2MobAIComponent.h"
#include "Npcs/MT2NpcTypes.h"
#include "Components/MT2StatusEffectComponent.h"
#include "Skills/MT2SkillDefinition.h"
#include "UObject/UnrealType.h"
#include "UObject/StructOnScope.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticAppliesTest,"Metin2.SemanticTypes.AppliesUseExistingEnum",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticAppliesTest::RunTest(const FString&)
{
	for (const auto& Pair : TArray<TPair<UStruct*,FName>>{{FMT2ItemBonusDefinition::StaticStruct(),TEXT("ApplyType")},{FMT2DamageAddonFormula::StaticStruct(),TEXT("SkillDamageApplyType")},{FMT2DamageAddonFormula::StaticStruct(),TEXT("AverageDamageApplyType")}})
	{
		auto* Property=FindFProperty<FEnumProperty>(Pair.Key,Pair.Value);
		if (TestNotNull(*FString::Printf(TEXT("%s.%s is an enum selector"),*Pair.Key->GetName(),*Pair.Value.ToString()),Property))
		{
			TestEqual(TEXT("Apply selector uses the shared apply domain"),Property->GetEnum(),StaticEnum<EMT2ItemBonusType>());
			int32 LegacyValue=Pair.Value==TEXT("ApplyType") ? 58 : Pair.Value==TEXT("SkillDamageApplyType") ? 71 : 72;
			TStrongObjectPtr<UScriptStruct> Legacy(NewObject<UScriptStruct>());
			auto* OldProperty=new FIntProperty(Legacy.Get(),Pair.Value,RF_Public); Legacy->AddCppProperty(OldProperty); Legacy->Bind(); Legacy->StaticLink(true);
			FStructOnScope OldInstance(Legacy.Get()); OldProperty->SetPropertyValue_InContainer(OldInstance.GetStructMemory(),LegacyValue);
			TArray<uint8> Bytes; FMemoryWriter Writer(Bytes,true); FObjectAndNameAsStringProxyArchive Saving(Writer,false); Saving.ArNoDelta=true;
			Legacy->SerializeTaggedProperties(Saving,OldInstance.GetStructMemory(),Legacy.Get(),nullptr);
			FMemoryReader Reader(Bytes,true); FObjectAndNameAsStringProxyArchive Loading(Reader,false);
			FStructOnScope Instance(Pair.Key); Property->GetUnderlyingProperty()->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Instance.GetStructMemory()),uint64(0));
			Pair.Key->SerializeTaggedProperties(Loading,Instance.GetStructMemory(),Pair.Key,nullptr);
			TestFalse(TEXT("An old int32 tag loads without an archive error"),Loading.IsError());
			TestEqual(TEXT("Old integer apply retains its meaning"),Property->GetUnderlyingProperty()->GetSignedIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Instance.GetStructMemory())),static_cast<int64>(LegacyValue));
		}
	}
	const auto* Stamina=GetDefault<UMT2ItemBonusSettings>()->FindDefinition(58,EMT2ItemBonusKind::Normal);
	TestNull(TEXT("Adding an enum selector does not make stamina a rollable bonus"),Stamina);
	TestEqual(TEXT("Configured attack-speed rule retains its value"),GetDefault<UMT2ItemBonusSettings>()->FormatBonus(7,8),FString(TEXT("Attack Speed +8%")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticItemMasksTest,"Metin2.SemanticTypes.ItemFlagBits",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticItemMasksTest::RunTest(const FString&)
{
	UClass* Library=FindObject<UClass>(nullptr,TEXT("/Script/Metin2.MT2SemanticTypesLibrary"));
	if (!TestNotNull(TEXT("Named flag operations are available to Blueprint authors"),Library)) { return false; }
	UFunction* Make=Library->FindFunctionByName(TEXT("MakeItemWearFlag"));
	if (!TestNotNull(TEXT("Wear-mask constructor accepts a named flag"),Make)) { return false; }
	FStructOnScope Parameters(Make);
	const auto* Flag=FindFProperty<FEnumProperty>(Make,TEXT("Flag"));
	const auto* Result=FindFProperty<FIntProperty>(Make,TEXT("ReturnValue"));
	if (!TestNotNull(TEXT("Flag input is an enum"),Flag) || !TestNotNull(TEXT("Combined masks remain full-width"),Result)) { return false; }
	Flag->GetUnderlyingProperty()->SetIntPropertyValue(Flag->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()),static_cast<uint64>(23));
	Library->GetDefaultObject()->ProcessEvent(Make,Parameters.GetStructMemory());
	TestEqual(TEXT("Named belt flag makes legacy bit 23, not truncated byte or ordinal 23"),Result->GetPropertyValue_InContainer(Parameters.GetStructMemory()),1<<23);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticBonusTargetTest,"Metin2.SemanticTypes.BonusTargetUsesMaskValues",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticBonusTargetTest::RunTest(const FString&)
{
	const UEnum* Enum=StaticEnum<EMT2ItemBonusTarget>();
	TestTrue(TEXT("Actual bonus-target mask values are not interpreted as bit positions"),Enum->GetBoolMetaData(TEXT("UseEnumValuesAsMaskValuesInEditor")));
	TestEqual(TEXT("Weapon bonus target remains bit zero"),static_cast<int32>(EMT2ItemBonusTarget::Weapon),1);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticAddonTest,"Metin2.SemanticTypes.AddonSentinelRoundTrip",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticAddonTest::RunTest(const FString&)
{
	auto* Addon=FindFProperty<FStructProperty>(FMT2ItemDefinition::StaticStruct(),TEXT("BonusAddonType"));
	if (!TestNotNull(TEXT("Signed addon selector has a lossless typed representation"),Addon)) { return false; }
	const auto* Type=FindFProperty<FEnumProperty>(Addon->Struct,TEXT("Type"));
	const auto* Raw=FindFProperty<FIntProperty>(Addon->Struct,TEXT("LegacyValue"));
	if (!TestNotNull(TEXT("Addon choices have a dedicated enum"),Type) || !TestNotNull(TEXT("Opaque unsupported values retain their original storage"),Raw)) { return false; }
	for (int32 Number : {-1,0,7})
	{
		TStrongObjectPtr<UScriptStruct> Legacy(NewObject<UScriptStruct>());
		auto* OldProperty=new FIntProperty(Legacy.Get(),TEXT("BonusAddonType"),RF_Public); Legacy->AddCppProperty(OldProperty); Legacy->Bind(); Legacy->StaticLink(true);
		FStructOnScope OldInstance(Legacy.Get()); OldProperty->SetPropertyValue_InContainer(OldInstance.GetStructMemory(),Number);
		TArray<uint8> Bytes; FMemoryWriter Writer(Bytes,true); FObjectAndNameAsStringProxyArchive Saving(Writer,false); Saving.ArNoDelta=true;
		Legacy->SerializeTaggedProperties(Saving,OldInstance.GetStructMemory(),Legacy.Get(),nullptr);
		FMemoryReader Reader(Bytes,true); FObjectAndNameAsStringProxyArchive Loading(Reader,false);
		FStructOnScope NewInstance(FMT2ItemDefinition::StaticStruct());
		FMT2ItemDefinition::StaticStruct()->SerializeTaggedProperties(Loading,NewInstance.GetStructMemory(),FMT2ItemDefinition::StaticStruct(),nullptr);
		const void* Value=Addon->ContainerPtrToValuePtr<void>(NewInstance.GetStructMemory());
		TestFalse(TEXT("Legacy signed addon loads through tagged serialization"),Loading.IsError());
		TestEqual(TEXT("Known sentinel maps to its named choice; unsupported data stays opaque"),Type->GetUnderlyingProperty()->GetSignedIntPropertyValue(Type->ContainerPtrToValuePtr<void>(Value)),int64(Number==-1 ? 1 : Number==0 ? 0 : 2));
		TestEqual(TEXT("Original signed value is never truncated"),Raw->GetPropertyValue_InContainer(Value),Number);
	}
	FStructOnScope Imported(FMT2ItemDefinition::StaticStruct());
	void* Value=Addon->ContainerPtrToValuePtr<void>(Imported.GetStructMemory());
	TestNotNull(TEXT("Existing numeric config addon syntax remains importable"),Addon->ImportText_Direct(TEXT("-1"),Value,nullptr,PPF_None,GWarn));
	TestEqual(TEXT("Numeric config -1 selects paired damage addon"),Type->GetUnderlyingProperty()->GetSignedIntPropertyValue(Type->ContainerPtrToValuePtr<void>(Value)),int64(1));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticItemSubtypesTest,"Metin2.SemanticTypes.ConcreteItemSubtypeSelectors",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticItemSubtypesTest::RunTest(const FString&)
{
	const struct { UClass* Class; const TCHAR* Field; const TCHAR* EnumName; int32 Value; } Cases[]={
		{UMT2ItemWeaponTemplate::StaticClass(),TEXT("WeaponSubType"),TEXT("EMT2WeaponSubType"),2},
		{UMT2ItemArmorTemplate::StaticClass(),TEXT("ArmorSubType"),TEXT("EMT2ArmorSubType"),3},
		{UMT2ItemUseTemplate::StaticClass(),TEXT("UseSubType"),TEXT("EMT2UseSubType"),21},
		{UMT2ItemAutoUseTemplate::StaticClass(),TEXT("AutoUseSubType"),TEXT("EMT2AutoUseSubType"),2},
		{UMT2ItemMaterialTemplate::StaticClass(),TEXT("MaterialSubType"),TEXT("EMT2MaterialSubType"),4},
		{UMT2ItemMetinStoneTemplate::StaticClass(),TEXT("MetinSubType"),TEXT("EMT2MetinSubType"),1},
		{UMT2ItemCostumeTemplate::StaticClass(),TEXT("CostumeSubType"),TEXT("EMT2CostumeSubType"),1},
		{UMT2ItemMetinStoneTemplate::StaticClass(),TEXT("RequiredSocketGrade"),TEXT("EMT2RequiredMetinSocketGrade"),2}
	};
	for (const auto& Case : Cases)
	{
		auto* Property=FindFProperty<FEnumProperty>(Case.Class,Case.Field);
		if (!TestNotNull(*FString::Printf(TEXT("%s.%s exposes a named subtype"),*Case.Class->GetName(),Case.Field),Property)) { continue; }
		TestEqual(TEXT("Each item family uses its own domain"),Property->GetEnum()->GetName(),FString(Case.EnumName));
		UClass* LegacyClass=FindObject<UClass>(nullptr,TEXT("/Script/MT2UEEditor.MT2SemanticLegacyItemFixture"));
		if (!TestNotNull(TEXT("Old object-format test fixture is available"),LegacyClass)) { return false; }
		TStrongObjectPtr<UObject> OldInstance(NewObject<UObject>(GetTransientPackage(),LegacyClass));
		auto* OldProperty=FindFProperty<FIntProperty>(LegacyClass,Case.Field); OldProperty->SetPropertyValue_InContainer(OldInstance.Get(),Case.Value);
		TArray<uint8> Bytes; FMemoryWriter Writer(Bytes,true); FObjectAndNameAsStringProxyArchive Saving(Writer,false);
		LegacyClass->SerializeTaggedProperties(Saving,reinterpret_cast<uint8*>(OldInstance.Get()),LegacyClass,reinterpret_cast<const uint8*>(LegacyClass->GetDefaultObject()));
		auto* Target=NewObject<UObject>(GetTransientPackage(),Case.Class);
		FMemoryReader Reader(Bytes,true); FObjectAndNameAsStringProxyArchive Loading(Reader,false);
		Case.Class->SerializeTaggedProperties(Loading,reinterpret_cast<uint8*>(Target),Case.Class,nullptr);
		TestFalse(TEXT("Old non-default subtype loads without an archive error"),Loading.IsError());
		TestEqual(TEXT("Existing asset subtype is not reset"),Property->GetUnderlyingProperty()->GetSignedIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Target)),static_cast<int64>(Case.Value));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticVisualTypesTest,"Metin2.SemanticTypes.VisualRaceAndSexAreTyped",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticVisualTypesTest::RunTest(const FString&)
{
	for (UStruct* Struct : {FMT2ItemArmorVisualDefinition::StaticStruct(),FMT2ItemHairVisualDefinition::StaticStruct()})
	{
		for (FName Field : {FName(TEXT("Race")),FName(TEXT("Sex"))})
		{
			const auto* Property=FindFProperty<FEnumProperty>(Struct,Field);
			if (TestNotNull(*FString::Printf(TEXT("%s.%s has named choices"),*Struct->GetName(),*Field.ToString()),Property))
			{ TestEqual(TEXT("Visual selector reuses the character domain"),Property->GetEnum(),Field==TEXT("Race") ? StaticEnum<EMT2CharacterRace>() : StaticEnum<EMT2CharacterSex>()); }
		}
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticGenericSubtypeTest,"Metin2.SemanticTypes.GenericSubtypeUsesItsItemFamily",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticGenericSubtypeTest::RunTest(const FString&)
{
	auto* ItemType=FindFProperty<FEnumProperty>(FMT2ItemDefinition::StaticStruct(),TEXT("ItemType"));
	auto* Subtype=FindFProperty<FStructProperty>(FMT2ItemDefinition::StaticStruct(),TEXT("SubType"));
	if (!TestNotNull(TEXT("Generic item kind uses an enum"),ItemType) || !TestNotNull(TEXT("Generic subtype carries its family"),Subtype)) { return false; }
	const auto* Family=FindFProperty<FEnumProperty>(Subtype->Struct,TEXT("Family"));
	const auto* Weapon=FindFProperty<FEnumProperty>(Subtype->Struct,TEXT("Weapon"));
	const auto* Armor=FindFProperty<FEnumProperty>(Subtype->Struct,TEXT("Armor"));
	const auto* Raw=FindFProperty<FIntProperty>(Subtype->Struct,TEXT("LegacyValue"));
	if (!TestNotNull(TEXT("Subtype family is named"),Family) || !TestNotNull(TEXT("Weapon choices are distinct"),Weapon) || !TestNotNull(TEXT("Armor choices are distinct"),Armor) || !TestNotNull(TEXT("Unknown subtype storage is retained"),Raw)) { return false; }
	for (const auto& Input : TArray<TPair<int32,int32>>{{1,1},{2,1},{1,177}})
	{
		TStrongObjectPtr<UScriptStruct> Legacy(NewObject<UScriptStruct>());
		auto* OldKind=new FIntProperty(Legacy.Get(),TEXT("ItemType"),RF_Public); auto* OldSubtype=new FIntProperty(Legacy.Get(),TEXT("SubType"),RF_Public);
		Legacy->AddCppProperty(OldKind); Legacy->AddCppProperty(OldSubtype); Legacy->Bind(); Legacy->StaticLink(true);
		FStructOnScope OldInstance(Legacy.Get()); OldKind->SetPropertyValue_InContainer(OldInstance.GetStructMemory(),Input.Key); OldSubtype->SetPropertyValue_InContainer(OldInstance.GetStructMemory(),Input.Value);
		TArray<uint8> Bytes; FMemoryWriter Writer(Bytes,true); FObjectAndNameAsStringProxyArchive Saving(Writer,false); Saving.ArNoDelta=true;
		Legacy->SerializeTaggedProperties(Saving,OldInstance.GetStructMemory(),Legacy.Get(),nullptr);
		FMemoryReader Reader(Bytes,true); FObjectAndNameAsStringProxyArchive Loading(Reader,false); FStructOnScope NewInstance(FMT2ItemDefinition::StaticStruct());
		FMT2ItemDefinition::StaticStruct()->SerializeItem(Loading,NewInstance.GetStructMemory(),nullptr);
		const void* Value=Subtype->ContainerPtrToValuePtr<void>(NewInstance.GetStructMemory());
		TestFalse(TEXT("Old generic item tagged data loads"),Loading.IsError());
		TestEqual(TEXT("Subtype resolves against its owning item family after loading"),Family->GetUnderlyingProperty()->GetSignedIntPropertyValue(Family->ContainerPtrToValuePtr<void>(Value)),int64(Input.Key));
		if (Input.Value==177)
		{
			TestEqual(TEXT("Unknown subtype is not replaced by a known choice"),Raw->GetPropertyValue_InContainer(Value),177);
			TestEqual(TEXT("Unknown weapon subtype is explicitly opaque"),Weapon->GetUnderlyingProperty()->GetSignedIntPropertyValue(Weapon->ContainerPtrToValuePtr<void>(Value)),int64(254));
		}
		else
		{
			const auto* Choice=Input.Key==1 ? Weapon : Armor;
			TestEqual(TEXT("Equal ordinals keep different family meanings"),Choice->GetEnum()->GetNameStringByValue(1),Input.Key==1 ? FString(TEXT("Dagger")) : FString(TEXT("Head")));
		}
	}
	UClass* Library=FindObject<UClass>(nullptr,TEXT("/Script/Metin2.MT2SemanticTypesLibrary"));
	UFunction* Check=Library ? Library->FindFunctionByName(TEXT("IsItemSubtypeForFamily")) : nullptr;
	if (!TestNotNull(TEXT("Cross-family subtype validation is available to Blueprint authors"),Check)) { return false; }
	FStructOnScope Parameters(Check);
	const auto* Parameter=FindFProperty<FStructProperty>(Check,TEXT("Subtype"));
	const auto* Expected=FindFProperty<FEnumProperty>(Check,TEXT("Family"));
	const auto* Result=FindFProperty<FBoolProperty>(Check,TEXT("ReturnValue"));
	if (!TestNotNull(TEXT("Subtype argument retains its discriminator"),Parameter) || !TestNotNull(TEXT("Expected family is an enum"),Expected) || !TestNotNull(TEXT("Validation returns a boolean"),Result)) { return false; }
	void* Value=Parameter->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory());
	Family->GetUnderlyingProperty()->SetIntPropertyValue(Family->ContainerPtrToValuePtr<void>(Value),uint64(1));
	Expected->GetUnderlyingProperty()->SetIntPropertyValue(Expected->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()),uint64(2));
	Library->GetDefaultObject()->ProcessEvent(Check,Parameters.GetStructMemory());
	TestFalse(TEXT("A weapon subtype is rejected for armor instead of reinterpreted"),Result->GetPropertyValue_InContainer(Parameters.GetStructMemory()));
	Expected->GetUnderlyingProperty()->SetIntPropertyValue(Expected->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()),uint64(1));
	Library->GetDefaultObject()->ProcessEvent(Check,Parameters.GetStructMemory());
	TestTrue(TEXT("A matching weapon family is accepted"),Result->GetPropertyValue_InContainer(Parameters.GetStructMemory()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticUniqueSubtypeTest,"Metin2.SemanticTypes.UnknownImportedSubtypeIsPreserved",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticUniqueSubtypeTest::RunTest(const FString&)
{
	const auto* Property=FindFProperty<FStructProperty>(UMT2ItemUniqueTemplate::StaticClass(),TEXT("UniqueSubType"));
	if (!TestNotNull(TEXT("Unique subtype has named choices with opaque retention"),Property)) { return false; }
	const auto* Type=FindFProperty<FEnumProperty>(Property->Struct,TEXT("Type")); const auto* Raw=FindFProperty<FIntProperty>(Property->Struct,TEXT("LegacyValue"));
	if (!TestNotNull(TEXT("Unique subtype uses a dedicated enum"),Type) || !TestNotNull(TEXT("Unknown unique subtype retains its raw ID"),Raw)) { return false; }
	UClass* OldClass=FindObject<UClass>(nullptr,TEXT("/Script/MT2UEEditor.MT2SemanticLegacyItemFixture"));
	if (!TestNotNull(TEXT("Old class-tag fixture exists"),OldClass)) { return false; }
	TStrongObjectPtr<UObject> Old(NewObject<UObject>(GetTransientPackage(),OldClass)); FindFProperty<FIntProperty>(OldClass,TEXT("UniqueSubType"))->SetPropertyValue_InContainer(Old.Get(),10);
	TArray<uint8> Bytes; FMemoryWriter Writer(Bytes,true); FObjectAndNameAsStringProxyArchive Saving(Writer,false);
	OldClass->SerializeTaggedProperties(Saving,reinterpret_cast<uint8*>(Old.Get()),OldClass,reinterpret_cast<const uint8*>(OldClass->GetDefaultObject()));
	auto* Target=NewObject<UMT2ItemUniqueTemplate>(); FMemoryReader Reader(Bytes,true); FObjectAndNameAsStringProxyArchive Loading(Reader,false);
	Target->GetClass()->SerializeTaggedProperties(Loading,reinterpret_cast<uint8*>(Target),Target->GetClass(),nullptr);
	const void* Value=Property->ContainerPtrToValuePtr<void>(Target);
	TestFalse(TEXT("Old unique subtype loads without archive errors"),Loading.IsError());
	TestEqual(TEXT("Undocumented imported unique subtype is not renamed or reset"),Raw->GetPropertyValue_InContainer(Value),10);
	TestEqual(TEXT("An unsupported subtype is explicitly opaque"),Type->GetUnderlyingProperty()->GetSignedIntPropertyValue(Type->ContainerPtrToValuePtr<void>(Value)),int64(254));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticMobSelectorsTest,"Metin2.SemanticTypes.MobEmpireSelectors",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticMobSelectorsTest::RunTest(const FString&)
{
	TestNotNull(TEXT("Imported empire retains opaque legacy IDs"),FindFProperty<FStructProperty>(FMT2MobDefinition::StaticStruct(),TEXT("Empire")));
	for (UStruct* Owner : TArray<UStruct*>{AMT2Mob::StaticClass(),UMT2MobAIComponent::StaticClass()})
	{
		const auto* Property=FindFProperty<FEnumProperty>(Owner,TEXT("Empire"));
		if (TestNotNull(*FString::Printf(TEXT("%s empire is named"),*Owner->GetName()),Property))
		{ TestEqual(TEXT("Mob empire reuses player empire"),Property->GetEnum(),StaticEnum<EMT2Empire>()); }
	}
	const auto* Click=FindFProperty<FEnumProperty>(FMT2MobDefinition::StaticStruct(),TEXT("OnClickType"));
	if (TestNotNull(TEXT("On-click behavior is named"),Click))
	{ TestEqual(TEXT("Mob and NPC click behavior share the verified domain"),Click->GetEnum(),StaticEnum<EMT2NpcOnClickType>()); }
	const auto* Reserved=FindFProperty<FIntProperty>(FMT2MobDefinition::StaticStruct(),TEXT("ExtendedAIFlags"));
	if (TestNotNull(TEXT("Reserved extension data is preserved"),Reserved))
	{ TestFalse(TEXT("Undefined extended bits are not offered as raw flag authoring"),Reserved->HasAnyPropertyFlags(CPF_Edit|CPF_BlueprintVisible)); }
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticMobMasksTest,"Metin2.SemanticTypes.MobFlagBitMapping",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticMobMasksTest::RunTest(const FString&)
{
	UClass* Library=FindObject<UClass>(nullptr,TEXT("/Script/Metin2.MT2SemanticTypesLibrary"));
	for (const auto& Case : TArray<TTuple<FString,FString,int32>>{{TEXT("AI"),TEXT("NoMove"),1},{TEXT("Race"),TEXT("AttackDark"),16},{TEXT("Immune"),TEXT("Reflect"),6}})
	{
		const FString Domain=Case.Get<0>(), EnumName=TEXT("/Script/Metin2.EMT2Mob")+Domain+TEXT("Flag");
		UEnum* Enum=FindObject<UEnum>(nullptr,*EnumName);
		if (!TestNotNull(*EnumName,Enum)) { continue; }
		TestFalse(TEXT("Mob flags use bit positions, not mask values"),Enum->GetBoolMetaData(TEXT("UseEnumValuesAsMaskValuesInEditor")));
		TestEqual(TEXT("Verified legacy bit position"),Enum->GetValueByNameString(Case.Get<1>()),int64(Case.Get<2>()));
		for (UStruct* Owner : TArray<UStruct*>{FMT2MobDefinition::StaticStruct(),Domain==TEXT("AI") ? static_cast<UStruct*>(UMT2MobAIComponent::StaticClass()) : static_cast<UStruct*>(AMT2Mob::StaticClass())})
		{
			const auto* Property=FindFProperty<FIntProperty>(Owner,*(Domain+TEXT("Flags")));
			if (TestNotNull(TEXT("Combined mask retains int32 storage"),Property))
			{ TestEqual(TEXT("Mask editor binds the correct domain"),Property->GetMetaData(TEXT("BitmaskEnum")),EnumName); }
		}
		UFunction* Make=Library ? Library->FindFunctionByName(*(TEXT("MakeMob")+Domain+TEXT("Flag"))) : nullptr;
		if (!TestNotNull(TEXT("Blueprint can make named mob flags"),Make)) { continue; }
		FStructOnScope Params(Make);
		const auto* Flag=FindFProperty<FEnumProperty>(Make,TEXT("Flag"));
		if (!TestNotNull(TEXT("Flag input is named"),Flag)) { continue; }
		Flag->GetUnderlyingProperty()->SetIntPropertyValue(Flag->ContainerPtrToValuePtr<void>(Params.GetStructMemory()),uint64(Case.Get<2>()));
		Library->GetDefaultObject()->ProcessEvent(Make,Params.GetStructMemory());
		TestEqual(TEXT("Named flag makes the exact legacy mask"),FindFProperty<FIntProperty>(Make,TEXT("ReturnValue"))->GetPropertyValue_InContainer(Params.GetStructMemory()),int32(uint32(1)<<Case.Get<2>()));
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticMobLegacyTest,"Metin2.SemanticTypes.OldMobDefaultsSurviveLoad",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticMobLegacyTest::RunTest(const FString&)
{
	TStrongObjectPtr<UScriptStruct> Legacy(NewObject<UScriptStruct>());
	const TMap<FName,int32> Values={{TEXT("Empire"),3},{TEXT("OnClickType"),2},{TEXT("AIFlags"),2},{TEXT("RaceFlags"),int32(0x80010001u)},{TEXT("ImmuneFlags"),64},{TEXT("ExtendedAIFlags"),int32(0x84000000u)}};
	for (const auto& Pair : Values) { Legacy->AddCppProperty(new FIntProperty(Legacy.Get(),Pair.Key,RF_Public)); }
	Legacy->Bind(); Legacy->StaticLink(true); FStructOnScope Old(Legacy.Get());
	for (const auto& Pair : Values) { FindFProperty<FIntProperty>(Legacy.Get(),Pair.Key)->SetPropertyValue_InContainer(Old.GetStructMemory(),Pair.Value); }
	TArray<uint8> Bytes; FMemoryWriter Writer(Bytes,true); FObjectAndNameAsStringProxyArchive Saving(Writer,false); Saving.ArNoDelta=true;
	Legacy->SerializeTaggedProperties(Saving,Old.GetStructMemory(),Legacy.Get(),nullptr);
	FMemoryReader Reader(Bytes,true); FObjectAndNameAsStringProxyArchive Loading(Reader,false); FMT2MobDefinition Target;
	FMT2MobDefinition::StaticStruct()->SerializeItem(Loading,&Target,nullptr);
	TestFalse(TEXT("Old mob data loads"),Loading.IsError());
	TestEqual(TEXT("Non-neutral empire remains Jinno"),Target.Empire.ToLegacy(),3);
	TestEqual(TEXT("Talk remains click ordinal two"),static_cast<int32>(Target.OnClickType),2);
	TestEqual(TEXT("No-move remains bit one"),Target.AIFlags,2);
	TestEqual(TEXT("Unknown high race bits survive"),Target.RaceFlags,int32(0x80010001u));
	TestEqual(TEXT("Reflect retains its mask"),Target.ImmuneFlags,64);
	TestEqual(TEXT("Reserved extension bits are retained without invented meanings"),Target.ExtendedAIFlags,int32(0x84000000u));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticEffectTypesTest,"Metin2.SemanticTypes.EffectApplyAndFlagsAreTyped",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticEffectTypesTest::RunTest(const FString&)
{
	for (UStruct* Owner : TArray<UStruct*>{FMT2StatusEffect::StaticStruct(),FMT2SkillApplyDefinition::StaticStruct()})
	{
		const auto* Apply=FindFProperty<FEnumProperty>(Owner,TEXT("ApplyType"));
		if (TestNotNull(TEXT("Effects and skills use named apply selectors"),Apply))
		{ TestEqual(TEXT("Apply domain is shared with equipment"),Apply->GetEnum(),StaticEnum<EMT2ItemBonusType>()); }
		const auto* Flags=FindFProperty<FIntProperty>(Owner,Owner==FMT2StatusEffect::StaticStruct() ? TEXT("Flags") : TEXT("AffectFlags"));
		if (TestNotNull(TEXT("Effect flags retain their mask storage"),Flags))
		{ TestEqual(TEXT("Flags have named bit positions"),Flags->GetMetaData(TEXT("BitmaskEnum")),FString(TEXT("/Script/Metin2.EMT2EffectFlag"))); }
	}
	UFunction* Query=UMT2StatusEffectComponent::StaticClass()->FindFunctionByName(TEXT("HasEffectFlag"));
	TestNotNull(TEXT("Effect flag query takes one enum choice"),FindFProperty<FEnumProperty>(Query,TEXT("Flag")));
	for (const TCHAR* Name : {TEXT("RemoveEffectsByType"),TEXT("HasEffectType"),TEXT("GetEffectValueByType")})
	{
		const auto* Type=FindFProperty<FStructProperty>(UMT2StatusEffectComponent::StaticClass()->FindFunctionByName(Name),TEXT("Type"));
		if (TestNotNull(TEXT("Blueprint affect queries use the non-byte identifier"),Type))
		{ TestEqual(TEXT("Identifier domain is shared"),Type->Struct->GetName(),FString(TEXT("MT2AffectIdentifier"))); }
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticEffectLoadTest,"Metin2.SemanticTypes.EffectIdentityTaggedLoad",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticEffectLoadTest::RunTest(const FString&)
{
	const auto* Type=FindFProperty<FStructProperty>(FMT2StatusEffect::StaticStruct(),TEXT("Type"));
	if (!TestNotNull(TEXT("Effect identity is not a narrowed byte enum"),Type)) { return false; }
	UFunction* Export=FindObject<UClass>(nullptr,TEXT("/Script/Metin2.MT2SemanticTypesLibrary"))->FindFunctionByName(TEXT("GetLegacyAffectId"));
	if (!TestNotNull(TEXT("Explicit identifier export boundary exists"),Export)) { return false; }
	for (int32 Id : {0,1,512,534,30001,123456789,MIN_int32,MAX_int32})
	{
		TStrongObjectPtr<UScriptStruct> OldType(NewObject<UScriptStruct>());
		auto* OldField=new FIntProperty(OldType.Get(),TEXT("Type"),RF_Public); OldType->AddCppProperty(OldField); OldType->Bind(); OldType->StaticLink(true);
		FStructOnScope Old(OldType.Get()); OldField->SetPropertyValue_InContainer(Old.GetStructMemory(),Id);
		TArray<uint8> Bytes; FMemoryWriter Writer(Bytes,true); FObjectAndNameAsStringProxyArchive Saving(Writer,false); Saving.ArNoDelta=true;
		OldType->SerializeTaggedProperties(Saving,Old.GetStructMemory(),OldType.Get(),nullptr);
		FMT2StatusEffect Effect; FMemoryReader Reader(Bytes,true); FObjectAndNameAsStringProxyArchive Loading(Reader,false);
		FMT2StatusEffect::StaticStruct()->SerializeItem(Loading,&Effect,nullptr);
		TestFalse(TEXT("Old affect tag loads"),Loading.IsError());
		FStructOnScope Params(Export); auto* Input=FindFProperty<FStructProperty>(Export,TEXT("Identifier"));
		if (!TestNotNull(TEXT("Export accepts typed identity"),Input)) { return false; }
		Input->CopyCompleteValue(Input->ContainerPtrToValuePtr<void>(Params.GetStructMemory()),Type->ContainerPtrToValuePtr<void>(&Effect));
		Export->GetOuterUClass()->GetDefaultObject()->ProcessEvent(Export,Params.GetStructMemory());
		TestEqual(TEXT("Every old ID survives without truncation"),FindFProperty<FIntProperty>(Export,TEXT("ReturnValue"))->GetPropertyValue_InContainer(Params.GetStructMemory()),Id);
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticSkillFlagsTest,"Metin2.SemanticTypes.SkillFlagHighBit",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticSkillFlagsTest::RunTest(const FString&)
{
	TestFalse(TEXT("Skill editor uses positions rather than treating 26 as a mask"),StaticEnum<EMT2SkillFlag>()->GetBoolMetaData(TEXT("UseEnumValuesAsMaskValuesInEditor")));
	auto* Skill=NewObject<UMT2SkillDefinition>(); Skill->Flags=int32(uint32(1)<<26);
	TestTrue(TEXT("Fire bit retains exact runtime meaning"),Skill->HasSkillFlag(EMT2SkillFlag::AttributeFire));
	TestFalse(TEXT("High bit does not set Attack"),Skill->HasSkillFlag(EMT2SkillFlag::Attack));
	const auto* Class=FindFProperty<FStructProperty>(UMT2SkillDefinition::StaticClass(),TEXT("LegacySkillType"));
	TestNotNull(TEXT("Signed absent class uses a lossless named selector"),Class);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticUnknownEmpireTest,"Metin2.SemanticTypes.UnknownMobEmpireSurvivesResave",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticUnknownEmpireTest::RunTest(const FString&)
{
	const FProperty* Empire=FindFProperty<FProperty>(FMT2MobDefinition::StaticStruct(),TEXT("Empire"));
	auto RawValue=[Empire](const FMT2MobDefinition& Value)->int64
	{
		const void* Data=Empire->ContainerPtrToValuePtr<void>(&Value);
		if (const auto* Enum=CastField<FEnumProperty>(Empire)) { return Enum->GetUnderlyingProperty()->GetSignedIntPropertyValue(Data); }
		const auto* Struct=CastField<FStructProperty>(Empire);
		const auto* Raw=Struct ? FindFProperty<FIntProperty>(Struct->Struct,TEXT("LegacyValue")) : nullptr;
		return Raw ? Raw->GetPropertyValue_InContainer(Data) : MIN_int64;
	};
	for (int32 Raw : {7,-1,MAX_int32})
	{
		TStrongObjectPtr<UScriptStruct> Legacy(NewObject<UScriptStruct>());
		auto* Property=new FIntProperty(Legacy.Get(),TEXT("Empire"),RF_Public);
		Legacy->AddCppProperty(Property); Legacy->Bind(); Legacy->StaticLink(true);
		FStructOnScope Old(Legacy.Get()); Property->SetPropertyValue_InContainer(Old.GetStructMemory(),Raw);
		TArray<uint8> Bytes; FMemoryWriter Writer(Bytes,true); FObjectAndNameAsStringProxyArchive Saving(Writer,false); Saving.ArNoDelta=true;
		Legacy->SerializeTaggedProperties(Saving,Old.GetStructMemory(),Legacy.Get(),nullptr);
		FMT2MobDefinition Loaded; FMemoryReader Reader(Bytes,true); FObjectAndNameAsStringProxyArchive Loading(Reader,false);
		FMT2MobDefinition::StaticStruct()->SerializeItem(Loading,&Loaded,nullptr);
		TestEqual(TEXT("Unsupported old integer retains its exact ID"),RawValue(Loaded),int64(Raw));
		EMT2Empire RuntimeEmpire=EMT2Empire::Jinno;
		TestFalse(TEXT("Opaque ID cannot become a runtime identity"),Loaded.Empire.TryGetEmpire(RuntimeEmpire));
		TestEqual(TEXT("Failed conversion leaves the destination unchanged"),RuntimeEmpire,EMT2Empire::Jinno);
		TArray<uint8> Resaved; FMemoryWriter SaveWriter(Resaved,true); FObjectAndNameAsStringProxyArchive SaveAgain(SaveWriter,false); SaveAgain.ArNoDelta=true;
		FMT2MobDefinition::StaticStruct()->SerializeItem(SaveAgain,&Loaded,nullptr);
		FMT2MobDefinition Reloaded; FMemoryReader ReadAgain(Resaved,true); FObjectAndNameAsStringProxyArchive Reload(ReadAgain,false);
		FMT2MobDefinition::StaticStruct()->SerializeItem(Reload,&Reloaded,nullptr);
		TestFalse(TEXT("Resave does not corrupt the archive"),Reload.IsError());
		TestEqual(TEXT("Unsupported ID remains recoverable after resave"),RawValue(Reloaded),int64(Raw));
	}
	// The immediately preceding schema stored a named empire enum rather than an integer.
	TStrongObjectPtr<UScriptStruct> EnumLayout(NewObject<UScriptStruct>());
	auto* OldEnum=new FEnumProperty(EnumLayout.Get(),TEXT("Empire"),RF_Public);
	OldEnum->SetEnum(StaticEnum<EMT2Empire>());
	OldEnum->AddCppProperty(new FByteProperty(OldEnum,TEXT("UnderlyingType"),RF_Public));
	EnumLayout->AddCppProperty(OldEnum); EnumLayout->Bind(); EnumLayout->StaticLink(true);
	FStructOnScope OldEnumValue(EnumLayout.Get());
	OldEnum->GetUnderlyingProperty()->SetIntPropertyValue(OldEnum->ContainerPtrToValuePtr<void>(OldEnumValue.GetStructMemory()),uint64(3));
	TArray<uint8> EnumBytes; FMemoryWriter EnumWriter(EnumBytes,true); FObjectAndNameAsStringProxyArchive EnumSave(EnumWriter,false); EnumSave.ArNoDelta=true;
	EnumLayout->SerializeTaggedProperties(EnumSave,OldEnumValue.GetStructMemory(),EnumLayout.Get(),nullptr);
	FMT2MobDefinition FromEnum; FMemoryReader EnumReader(EnumBytes,true); FObjectAndNameAsStringProxyArchive EnumLoad(EnumReader,false);
	FMT2MobDefinition::StaticStruct()->SerializeItem(EnumLoad,&FromEnum,nullptr);
	TestEqual(TEXT("Existing named enum assets retain Jinno"),RawValue(FromEnum),int64(3));
	return true;
}
#endif
