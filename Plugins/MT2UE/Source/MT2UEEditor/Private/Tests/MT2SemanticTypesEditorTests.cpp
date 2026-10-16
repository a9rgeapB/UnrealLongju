#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Commandlets/Commandlet.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Items/MT2ItemTemplate.h"
#include "Mobs/MT2Mob.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_VariableGet.h"
#include "K2Node_Knot.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Tests/MT2SemanticLegacyFixture.h"

namespace
{
UCommandlet* AuditCommandlet(FAutomationTestBase& Test)
{
	UClass* Class=FindObject<UClass>(nullptr,TEXT("/Script/MT2UEEditor.MT2AuditSemanticTypesCommandlet"));
	Test.TestNotNull(TEXT("Read-only semantic audit commandlet is registered"),Class);
	return Class ? NewObject<UCommandlet>(GetTransientPackage(),Class) : nullptr;
}
TSharedPtr<FJsonObject> ReadManifest(const FString& Path)
{
	FString Json; TSharedPtr<FJsonObject> Result;
	if (FFileHelper::LoadFileToString(Json,*Path)) { FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Result); }
	return Result;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticAuditReadOnlyTest,"Metin2.SemanticTypes.Editor.AuditIsReadOnly",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticAuditReadOnlyTest::RunTest(const FString&)
{
	UCommandlet* Audit=AuditCommandlet(*this); if (!Audit) { return false; }
	auto* Item=NewObject<UMT2ItemWeaponTemplate>(GetTransientPackage(),NAME_None,RF_Transient); Item->WeaponSubType=EMT2WeaponSubType::Bow;
	const bool WasDirty=Item->GetOutermost()->IsDirty();
	const FString Path=FPaths::ProjectSavedDir()/TEXT("SemanticTypes")/(TEXT("TestReadOnly-")+FGuid::NewGuid().ToString()+TEXT(".json"));
	TestEqual(TEXT("Capture succeeds"),Audit->Main(FString::Printf(TEXT("-Mode=Capture -Object=\"%s\" -Manifest=\"%s\""),*Item->GetPathName(),*Path)),0);
	TestEqual(TEXT("Capture preserves original non-default value"),static_cast<int32>(Item->WeaponSubType),2);
	TestEqual(TEXT("Capture does not dirty its package"),Item->GetOutermost()->IsDirty(),WasDirty);
	const auto Manifest=ReadManifest(Path); if (!TestTrue(TEXT("Capture emits a readable manifest"),Manifest.IsValid())) { return false; }
	const auto& Objects=Manifest->GetArrayField(TEXT("Objects")); if (!TestEqual(TEXT("Explicit capture is scoped to one object"),Objects.Num(),1)) { return false; }
	const auto Object=Objects[0]->AsObject();
	TestFalse(TEXT("Pre-migration tagged serialization is captured"),Object->GetStringField(TEXT("TaggedData")).IsEmpty());
	bool FoundSubtype=false;
	for (const auto& Value : Object->GetArrayField(TEXT("Properties")))
	{
		const auto Property=Value->AsObject();
		if (Property->GetStringField(TEXT("Path"))==TEXT("WeaponSubType")) { FoundSubtype=Property->GetStringField(TEXT("RawValue"))==TEXT("2"); }
	}
	TestTrue(TEXT("Manifest captures the real old subtype value"),FoundSubtype);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticAuditPinsTest,"Metin2.SemanticTypes.Editor.AuditRecordsBlueprintPinDefaults",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticAuditPinsTest::RunTest(const FString&)
{
	UCommandlet* Audit=AuditCommandlet(*this); if (!Audit) { return false; }
	const FName Name=MakeUniqueObjectName(GetTransientPackage(),UBlueprint::StaticClass(),TEXT("SemanticAuditPins"));
	auto* Blueprint=FKismetEditorUtilities::CreateBlueprint(AMT2Mob::StaticClass(),GetTransientPackage(),Name,BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass());
	if (!TestNotNull(TEXT("Real Blueprint fixture"),Blueprint)) { return false; }
	auto* Graph=FBlueprintEditorUtils::CreateNewGraph(Blueprint,TEXT("AuditGraph"),UEdGraph::StaticClass(),UEdGraphSchema_K2::StaticClass());
	FBlueprintEditorUtils::AddUbergraphPage(Blueprint,Graph);
	auto* Getter=NewObject<UK2Node_VariableGet>(Graph); Getter->VariableReference.SetSelfMember(TEXT("Empire")); Graph->AddNode(Getter); Getter->CreateNewGuid(); Getter->AllocateDefaultPins();
	auto* Knot=NewObject<UK2Node_Knot>(Graph); Graph->AddNode(Knot); Knot->CreateNewGuid(); Knot->AllocateDefaultPins();
	auto* Output=Getter->FindPin(TEXT("Empire")); if (!TestNotNull(TEXT("Inherited numeric variable pin"),Output)) { return false; }
	Knot->GetInputPin()->PinType=Output->PinType; Knot->GetInputPin()->DefaultValue=TEXT("2"); Output->MakeLinkTo(Knot->GetInputPin());
	const FString Path=FPaths::ProjectSavedDir()/TEXT("SemanticTypes")/(TEXT("TestPins-")+FGuid::NewGuid().ToString()+TEXT(".json"));
	TestEqual(TEXT("Blueprint capture succeeds"),Audit->Main(FString::Printf(TEXT("-Mode=Capture -Object=\"%s\" -Manifest=\"%s\""),*Blueprint->GetPathName(),*Path)),0);
	const auto Manifest=ReadManifest(Path); if (!TestTrue(TEXT("Blueprint manifest is readable"),Manifest.IsValid())) { return false; }
	bool Found=false;
	for (const auto& Value : Manifest->GetArrayField(TEXT("BlueprintPins")))
	{
		const auto Pin=Value->AsObject();
		if (Pin->GetStringField(TEXT("NodeGuid"))==Knot->NodeGuid.ToString() && Pin->GetStringField(TEXT("PinId"))==Knot->GetInputPin()->PinId.ToString())
		{ Found=Pin->GetStringField(TEXT("DefaultValue"))==TEXT("2") && Pin->GetArrayField(TEXT("Links")).Num()==1; }
	}
	TestTrue(TEXT("Real Blueprint pin default and connection are preserved in capture"),Found);
	return true;
}
#endif
