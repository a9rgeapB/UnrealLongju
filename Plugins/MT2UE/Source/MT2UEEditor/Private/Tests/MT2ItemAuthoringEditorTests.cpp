#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Importers/MT2ItemReferenceResolver.h"
#include "Items/Templates/MT2ItemWeaponTemplate.h"
#include "Core/MT2VnumRegistry.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Kismet2/KismetEditorUtilities.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2PartialItemReferenceImportTest, "Metin2.Items.Authoring.PartialReferenceImport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2PartialItemReferenceImportTest::RunTest(const FString&)
{
	auto CreateItem = [](int32 Vnum)
	{
		const FName Name = MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("ItemReferenceTest"));
		auto* Blueprint = FKismetEditorUtilities::CreateBlueprint(UMT2ItemWeaponTemplate::StaticClass(), GetTransientPackage(),
			Name, BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		auto* Item = Blueprint->GeneratedClass->GetDefaultObject<UMT2ItemTemplate>(); Item->Vnum = Vnum;
		return Item;
	};
	auto* Source = CreateItem(900001);
	auto* First = CreateItem(900002);
	auto* Second = CreateItem(900003);
	Source->RefinedVnum = First->Vnum;
	TArray<FString> Warnings;
	TestTrue(TEXT("Initial links resolve"), ResolveMT2ItemReferences({Source, First, Second}, Warnings));
	TestEqual(TEXT("Initial typed target"), Source->GetRefinedVnum(), First->Vnum);
	Source->RefinedVnum = Second->Vnum;
	Source->ResetImportedItemReferences();
	TestFalse(TEXT("Reimport invalidates migrated flag"), Source->bTypedItemReferences);
	TestTrue(TEXT("Reimport clears stale typed target"), Source->RefinedItemTemplate.IsNull());
	auto* Registry = NewObject<UMT2VnumRegistry>();
	TMap<int32, TSoftClassPtr<UMT2ItemTemplate>> Definitions;
	Definitions.Add(Second->Vnum, Second->GetClass());
	Registry->SetEntries({}, MoveTemp(Definitions));
	TestTrue(TEXT("Partial import resolves existing registry targets"), ResolveMT2ItemReferences({Source}, Warnings, Registry));
	TestEqual(TEXT("Changed source link resolves to the new target"), Source->GetRefinedVnum(), Second->Vnum);
	TestTrue(TEXT("No references were silently omitted"), Warnings.IsEmpty());
	return true;
}
#endif
