#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Config/MT2PathSettings.h"
#include "Misc/FileHelper.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2FishTableImportTest, "Metin2.Items.Import.FishTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2FishTableImportTest::RunTest(const FString&)
{
	const FString Row = TEXT("Fish\t27803\t27833\t27863\t1950\t1600\t800\t700\t13\t4\t1000\t2500\t2800\t0\t1\t0\t0\t2\t3\t4\t0\t0\t0\n");
	TMap<int32, FMT2FishDefinition> Definitions;
	FString Error;
	TestTrue(TEXT("Reference format parses"), FMT2ItemImporter::ParseFishTable(Row, Definitions, Error));
	if (TestTrue(TEXT("Live and dead fish configured"), Definitions.Contains(27803) && Definitions.Contains(27833)))
	{
		const auto& Fish = Definitions[27803];
		TestEqual(TEXT("Empty outcome"), Fish.ResolveUseRoll(1), 0);
		TestEqual(TEXT("Shellfish outcome"), Fish.ResolveUseRoll(1001), 27987);
		TestEqual(TEXT("Earthworm outcome"), Fish.ResolveUseRoll(1981), 27801);
		TestEqual(TEXT("Fishbone lower bound"), Fish.ResolveUseRoll(2001), 27799);
		TestEqual(TEXT("Dead fish lower bound"), Fish.ResolveUseRoll(4001), 27833);
		TestEqual(TEXT("Dead fish upper bound"), Fish.ResolveUseRoll(10000), 27833);
		TestEqual(TEXT("Out of range roll rejected"), Fish.ResolveUseRoll(0), INDEX_NONE);
		TestEqual(TEXT("Roasting result"), Definitions[27833].GrilledItemVnum, 27863);
	}
	TestFalse(TEXT("Duplicate input rejected"), FMT2ItemImporter::ParseFishTable(Row + Row, Definitions, Error));
	TestTrue(TEXT("Bad input returns no partial definitions"), Definitions.IsEmpty());
	TestFalse(TEXT("Truncated input rejected"), FMT2ItemImporter::ParseFishTable(TEXT("Fish\t27803"), Definitions, Error));
	TestTrue(TEXT("Spreadsheet totals are not fish rows"), FMT2ItemImporter::ParseFishTable(Row + TEXT("\t\t\t\t9950\t9950\t9800\t9900\n"), Definitions, Error));
	const FString Path = FString(UMT2PathSettings::Path(TEXT("LegacyServerLocaleRoot"))) / TEXT("fishing.txt");
	TestTrue(TEXT("Complete CP949 legacy table loads without losing column separators"), FMT2ItemImporter::LoadFishTable(Path, Definitions, Error));
	return true;
}
#endif
