#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Config/MT2PathSettings.h"
#include "Misc/FileHelper.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2BlendTableImportTest, "Metin2.Items.Import.BlendTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2BlendTableImportTest::RunTest(const FString&)
{
	const FString Row = TEXT("section\nitem_vnum 50821\napply_type CRITICAL_PCT\napply_value 8 10 12 15 20\napply_duration 60 120 180 300 600\nend\n");
	TMap<int32, FMT2BlendDefinition> Definitions;
	FString Error;
	TestTrue(TEXT("Legacy blend section parses"), FMT2ItemImporter::ParseBlendTable(Row, Definitions, Error));
	if (TestTrue(TEXT("One valid definition"), Definitions.Contains(50821)))
	{
		TestEqual(TEXT("Apply token decoded"), Definitions[50821].BonusType, EMT2ItemBonusType::CriticalChance);
		TestEqual(TEXT("Exact maximum duration"), Definitions[50821].Durations.Last(), 600);
	}
	TestFalse(TEXT("Duplicate vnum rejects whole table"), FMT2ItemImporter::ParseBlendTable(Row + Row, Definitions, Error));
	TestTrue(TEXT("Failure returns no partial definitions"), Definitions.IsEmpty());
	TestFalse(TEXT("Missing end rejected"), FMT2ItemImporter::ParseBlendTable(Row.LeftChop(4), Definitions, Error));
	TestFalse(TEXT("Unknown apply token rejected"), FMT2ItemImporter::ParseBlendTable(Row.Replace(TEXT("CRITICAL_PCT"), TEXT("UNKNOWN")), Definitions, Error));
	FString Legacy;
	const FString Path = FString(UMT2PathSettings::Path(TEXT("LegacyServerLocaleRoot"))) / TEXT("blend.txt");
	if (TestTrue(TEXT("Reference blend table is available"), FFileHelper::LoadFileToString(Legacy, *Path)))
	{
		TestTrue(TEXT("All source definitions parse"), FMT2ItemImporter::ParseBlendTable(Legacy, Definitions, Error));
		TestEqual(TEXT("Seven configured blends"), Definitions.Num(), 7);
		if (TestTrue(TEXT("Energy crystal exists"), Definitions.Contains(51002)))
		{
			TestEqual(TEXT("Energy first weight"), Definitions[51002].Weights[0], 5);
			TestEqual(TEXT("Energy final weight"), Definitions[51002].Weights[4], 15);
		}
	}
	return true;
}
#endif
