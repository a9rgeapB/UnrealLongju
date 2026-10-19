#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Items/MT2ItemTemplate.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"
#include "Items/Legacy/MT2LegacyItemRecord.h"
#include "Items/MT2ItemUtils.h"
#include "Items/Templates/MT2ItemPickTemplate.h"
#include "Items/Templates/MT2ItemRodTemplate.h"
#include "Items/Templates/MT2ItemBlendTemplate.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2ItemTemplateFilesTest, "Metin2.Items.Authoring.TemplateFiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2ItemTemplateFilesTest::RunTest(const FString&)
{
	TArray<UClass*> Classes;
	GetDerivedClasses(UMT2ItemTemplate::StaticClass(), Classes, true);
	Classes.Add(UMT2ItemTemplate::StaticClass());
	for (const auto* Class : Classes)
	{
		if (!Class->HasAnyClassFlags(CLASS_Native)) continue;
		const FString Header = Class->GetMetaData(TEXT("ModuleRelativePath"));
		const FString Expected = Class->GetName() + TEXT(".h");
		TestTrue(Class->GetName() + TEXT(" owns a dedicated header"), FPaths::GetCleanFilename(Header) == Expected);
		const FString Implementation = FPaths::ProjectDir() / TEXT("Source/Metin2") / FPaths::ChangeExtension(Header, TEXT("cpp"));
		TestTrue(Class->GetName() + TEXT(" owns a dedicated implementation"), IFileManager::Get().FileExists(*Implementation));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2ItemAuthoringSchemaTest, "Metin2.Items.Authoring.TypedState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2ItemAuthoringSchemaTest::RunTest(const FString&)
{
	TestNull(TEXT("Common item state has no raw socket array"), FindFProperty<FProperty>(FMT2ItemInstanceData::StaticStruct(), TEXT("LegacySockets")));
	for (const FName Name : {FName(TEXT("SkillVnum")), FName(TEXT("MetinSockets")), FName(TEXT("AccessorySockets")),
		FName(TEXT("AutoRecoveryRemainingAmount")), FName(TEXT("AutoRecoveryMaximumAmount")), FName(TEXT("bAutoRecoveryActive"))})
		TestNull(Name.ToString() + TEXT(" is not owned by common item state"), FindFProperty<FProperty>(FMT2ItemInstanceData::StaticStruct(), Name));
	const FProperty* Refined = FindFProperty<FProperty>(UMT2ItemTemplate::StaticClass(), TEXT("RefinedItemTemplate"));
	TestTrue(TEXT("Refinement target is an item class reference"), Refined && (CastField<FClassProperty>(Refined) || CastField<FSoftClassProperty>(Refined)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2ItemFamilyPersistenceTest, "Metin2.Items.Authoring.FamilyPersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2ItemFamilyPersistenceTest::RunTest(const FString&)
{
	FMT2LegacyItemSlotRecord Old;
	Old.Vnum = 29101; Old.Count = 7; Old.RealtimeExpiresAt = 123456;
	Old.MetinSockets.SetNum(3); Old.MetinSockets[0].Value = 37;
	TArray<uint8> LegacyBytes;
	FMemoryWriter LegacyWriter(LegacyBytes, true);
	FObjectAndNameAsStringProxyArchive OldArchive(LegacyWriter, false);
	FMT2LegacyItemSlotRecord::StaticStruct()->SerializeItem(OldArchive, &Old, nullptr);
	FMemoryReader LegacyReader(LegacyBytes, true);
	FObjectAndNameAsStringProxyArchive LegacyArchive(LegacyReader, true);
	// Memory readers assume current registered versions unless supplied a package's version map.
	LegacyArchive.SetCustomVersions(OldArchive.GetCustomVersions());
	FMT2ItemSlot Loaded;
	Loaded.Serialize(LegacyArchive);
	TestFalse(TEXT("Legacy archive reads without error"), LegacyArchive.IsError());
	TestEqual(TEXT("Legacy slot retains identity"), Loaded.Vnum, 29101);
	TestEqual(TEXT("Legacy slot retains count"), Loaded.Count, 7);
	TestEqual(TEXT("Legacy slot retains expiry"), Loaded.RealtimeExpiresAt, int64(123456));
	TestNotNull(TEXT("Unresolved legacy state is retained"), Loaded.FindState<FMT2LegacyItemRecord>());
	auto* Pick = NewObject<UMT2ItemPickTemplate>(); Pick->Vnum = 29101;
	MT2ItemUtils::NormalizeNumericSockets(Loaded, Pick);
	TestEqual(TEXT("Legacy practice becomes named pickaxe proficiency"), Loaded.GetPickaxeState().Proficiency, 37);
	TestTrue(TEXT("Pickaxe no longer carries Metin sockets"), Loaded.GetMetinSockets().IsEmpty());
	TArray<uint8> CurrentBytes;
	FMemoryWriter CurrentWriter(CurrentBytes, true);
	FObjectAndNameAsStringProxyArchive CurrentArchive(CurrentWriter, false);
	Loaded.Serialize(CurrentArchive);
	FMemoryReader CurrentReader(CurrentBytes, true);
	FObjectAndNameAsStringProxyArchive ReadArchive(CurrentReader, true);
	ReadArchive.SetCustomVersions(CurrentArchive.GetCustomVersions());
	FMT2ItemSlot RoundTrip; RoundTrip.Serialize(ReadArchive);
	TestFalse(TEXT("Current archive reads without error"), ReadArchive.IsError());
	TestEqual(TEXT("Typed practice survives archive round trip"), RoundTrip.GetPickaxeState().Proficiency, 37);
	TestEqual(TEXT("Typed slot count survives"), RoundTrip.Count, 7);
	const TCHAR* OldText = TEXT("(Count=2,LegacySockets=(15,8,600))");
	FMT2ItemInstanceData BlendData;
	TestNotNull(TEXT("Legacy Blueprint default text imports"), FMT2ItemInstanceData::StaticStruct()->ImportText(OldText, &BlendData, nullptr, PPF_None, GWarn, TEXT("item migration test")));
	auto* Blend = NewObject<UMT2ItemBlendTemplate>();
	MT2ItemUtils::NormalizeNumericSockets(BlendData, Blend);
	TestEqual(TEXT("Legacy blend magnitude is preserved"), BlendData.GetBlendState().Magnitude, 8);
	TestEqual(TEXT("Legacy blend duration is preserved"), BlendData.GetBlendState().DurationSeconds, 600);
	FMT2ItemInstanceData Empty;
	Empty.GetAutoRecoveryState(); Empty.GetBonuses(); Empty.GetMetinSockets();
	TestFalse(TEXT("Reading absent family state never creates unrelated state"), Empty.FamilyState.IsValid());
	auto* Rod = NewObject<UMT2ItemRodTemplate>();
	Rod->bTypedFamilyDefaults = true; Rod->DefaultRodState.Proficiency = 12;
	TestEqual(TEXT("Rod defaults come from the rod template"), Rod->MakeInstanceData().GetRodState().Proficiency, 12);
	return true;
}
#endif
