#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "MT2AuditSemanticTypesCommandlet.h"
#include "Mobs/MT2Mob.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_VariableGet.h"
#include "K2Node_Knot.h"
#include "K2Node_CallFunction.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
struct FMigrationFixture
{
	TStrongObjectPtr<UBlueprint> Blueprint;
	UK2Node_VariableGet* Getter=nullptr;
	UK2Node_Knot* Knot=nullptr;
	FString Manifest,Report;
	bool Create(FAutomationTestBase& Test)
	{
		const FName Name=MakeUniqueObjectName(GetTransientPackage(),UBlueprint::StaticClass(),TEXT("SemanticMigration"));
		Blueprint.Reset(FKismetEditorUtilities::CreateBlueprint(AMT2Mob::StaticClass(),GetTransientPackage(),Name,BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass()));
		if (!Test.TestNotNull(TEXT("Blueprint"),Blueprint.Get())) { return false; }
		auto* Graph=FBlueprintEditorUtils::CreateNewGraph(Blueprint.Get(),TEXT("MigrationGraph"),UEdGraph::StaticClass(),UEdGraphSchema_K2::StaticClass());
		FBlueprintEditorUtils::AddUbergraphPage(Blueprint.Get(),Graph);
		Getter=NewObject<UK2Node_VariableGet>(Graph); Getter->VariableReference.SetSelfMember(TEXT("Empire"));
		Graph->AddNode(Getter); Getter->CreateNewGuid(); Getter->AllocateDefaultPins();
		Knot=NewObject<UK2Node_Knot>(Graph); Graph->AddNode(Knot); Knot->CreateNewGuid(); Knot->AllocateDefaultPins();
		// Preserve the pre-refactor graph shape explicitly; native Empire is now an enum.
		FEdGraphPinType OldInteger; OldInteger.PinCategory=UEdGraphSchema_K2::PC_Int;
		Getter->FindPin(TEXT("Empire"))->PinType=OldInteger;
		Knot->GetInputPin()->PinType=OldInteger; Knot->GetOutputPin()->PinType=OldInteger;
		Knot->GetInputPin()->DefaultValue=TEXT("2");
		Getter->FindPin(TEXT("Empire"))->MakeLinkTo(Knot->GetInputPin());
		const FString Stem=FPaths::ProjectSavedDir()/TEXT("SemanticTypes")/(TEXT("Migration-")+FGuid::NewGuid().ToString());
		Manifest=Stem+TEXT("-before.json"); Report=Stem+TEXT("-report.json"); return true;
	}
	int32 Capture()
	{
		return NewObject<UMT2AuditSemanticTypesCommandlet>()->Main(FString::Printf(TEXT("-Mode=Capture -Object=\"%s\" -Manifest=\"%s\""),*Blueprint->GetPathName(),*Manifest));
	}
	int32 Migrate(bool Apply=false)
	{
		return NewObject<UMT2AuditSemanticTypesCommandlet>()->Main(FString::Printf(TEXT("-Mode=Migrate -Object=\"%s\" -Manifest=\"%s\" -Report=\"%s\" %s"),*Blueprint->GetPathName(),*Manifest,*Report,Apply ? TEXT("-Apply") : TEXT("")));
	}
	TSharedPtr<FJsonObject> ReadReport()
	{
		FString Text; TSharedPtr<FJsonObject> Result;
		if (FFileHelper::LoadFileToString(Text,*Report)) { FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Result); }
		return Result;
	}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2MigrationDryRunTest,"Metin2.SemanticTypes.Editor.MigrationDryRunDoesNotSave",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2MigrationDryRunTest::RunTest(const FString&)
{
	FMigrationFixture Fixture; if (!Fixture.Create(*this)) { return false; }
	TestEqual(TEXT("Old graph snapshot captures"),Fixture.Capture(),0);
	const bool Dirty=Fixture.Blueprint->GetOutermost()->IsDirty();
	TestEqual(TEXT("Migration defaults to a successful read-only dry run"),Fixture.Migrate(),0);
	TestEqual(TEXT("Dry run leaves old graph pins untouched"),Fixture.Getter->FindPin(TEXT("Empire"))->PinType.PinCategory,UEdGraphSchema_K2::PC_Int);
	TestEqual(TEXT("Dry run preserves links"),Fixture.Knot->GetInputPin()->LinkedTo.Num(),1);
	TestEqual(TEXT("Dry run preserves dirty state"),Fixture.Blueprint->GetOutermost()->IsDirty(),Dirty);
	const auto Report=Fixture.ReadReport(); if (!TestTrue(TEXT("Dry-run report exists"),Report.IsValid())) { return false; }
	TestEqual(TEXT("Dry run identifies affected Blueprint"),Report->GetIntegerField(TEXT("ChangedBlueprints")),1);
	TestEqual(TEXT("Dry run writes no package"),Report->GetIntegerField(TEXT("SavedPackages")),0); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2MigrationPinsTest,"Metin2.SemanticTypes.Editor.MigrationPreservesPinDefaultsAndLinks",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2MigrationPinsTest::RunTest(const FString&)
{
	FMigrationFixture Fixture; if (!Fixture.Create(*this)) { return false; }
	const FGuid GetterId=Fixture.Getter->FindPin(TEXT("Empire"))->PinId, KnotId=Fixture.Knot->GetInputPin()->PinId;
	TestEqual(TEXT("Old graph snapshot captures"),Fixture.Capture(),0);
	TestEqual(TEXT("Validated reconstruction succeeds"),Fixture.Migrate(true),0);
	const auto* Getter=Fixture.Getter->FindPin(TEXT("Empire")); const auto* Input=Fixture.Knot->GetInputPin();
	TestEqual(TEXT("Named enum pin replaces the integer"),Getter->PinType.PinSubCategoryObject.Get(),static_cast<UObject*>(StaticEnum<EMT2Empire>()));
	TestEqual(TEXT("Reroute carries the same enum"),Input->PinType.PinSubCategoryObject.Get(),static_cast<UObject*>(StaticEnum<EMT2Empire>()));
	TestEqual(TEXT("Non-default ordinal two becomes named Chunjo"),StaticEnum<EMT2Empire>()->GetValueByNameString(Input->DefaultValue),int64(2));
	TestEqual(TEXT("Getter pin identity is stable"),Getter->PinId,GetterId); TestEqual(TEXT("Reroute identity is stable"),Input->PinId,KnotId);
	TestTrue(TEXT("Original connection is retained"),Getter->LinkedTo.Contains(Fixture.Knot->GetInputPin()));
	TestTrue(TEXT("Blueprint compiles after reconstruction"),Fixture.Blueprint->Status!=BS_Error); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2MigrationIdempotenceTest,"Metin2.SemanticTypes.Editor.MigrationIsIdempotent",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2MigrationIdempotenceTest::RunTest(const FString&)
{
	FMigrationFixture Fixture; if (!Fixture.Create(*this)) { return false; }
	TestEqual(TEXT("Capture"),Fixture.Capture(),0); TestEqual(TEXT("First migration"),Fixture.Migrate(true),0);
	TestEqual(TEXT("Second migration"),Fixture.Migrate(true),0);
	const auto Report=Fixture.ReadReport(); if (!TestTrue(TEXT("Repeat report exists"),Report.IsValid())) { return false; }
	TestEqual(TEXT("Second run changes no Blueprint"),Report->GetIntegerField(TEXT("ChangedBlueprints")),0);
	TestEqual(TEXT("Second run writes no package"),Report->GetIntegerField(TEXT("SavedPackages")),0); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2MigrationUnmappableTest,"Metin2.SemanticTypes.Editor.UnmappableAssetIsReported",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2MigrationUnmappableTest::RunTest(const FString&)
{
	FMigrationFixture Fixture; if (!Fixture.Create(*this)) { return false; }
	Fixture.Getter->FindPin(TEXT("Empire"))->BreakAllPinLinks();
	auto* Graph=Fixture.Getter->GetGraph(); auto* Math=NewObject<UK2Node_CallFunction>(Graph);
	Math->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(TEXT("Abs_Int")));
	Graph->AddNode(Math); Math->CreateNewGuid(); Math->AllocateDefaultPins();
	Fixture.Getter->FindPin(TEXT("Empire"))->MakeLinkTo(Math->FindPin(TEXT("A")));
	TestEqual(TEXT("Unsupported graph snapshot captures"),Fixture.Capture(),0);
	AddExpectedError(TEXT("Cannot migrate link"),EAutomationExpectedErrorFlags::Contains,1);
	TestTrue(TEXT("Unproven numeric graph conversion is reported"),Fixture.Migrate(true)!=0);
	TestEqual(TEXT("Rejected graph is not partially converted"),Fixture.Getter->FindPin(TEXT("Empire"))->PinType.PinCategory,UEdGraphSchema_K2::PC_Int);
	TestTrue(TEXT("Rejected link is retained"),Fixture.Getter->FindPin(TEXT("Empire"))->LinkedTo.Contains(Math->FindPin(TEXT("A")))); return true;
}
#endif
