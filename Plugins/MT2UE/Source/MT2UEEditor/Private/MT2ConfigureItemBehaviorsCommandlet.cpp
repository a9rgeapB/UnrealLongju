#include "MT2ConfigureItemBehaviorsCommandlet.h"

#include "Config/MT2PathSettings.h"
#include "Importers/MT2ItemImporter.h"
#include "Misc/Parse.h"

UMT2ConfigureItemBehaviorsCommandlet::UMT2ConfigureItemBehaviorsCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UMT2ConfigureItemBehaviorsCommandlet::Main(const FString& Params)
{
	FString SourceRoot = UMT2PathSettings::Path(TEXT("LegacyDumpRoot"));
	FString DestinationRoot = TEXT("/Game");
	FParse::Value(*Params, TEXT("SourceRoot="), SourceRoot);
	FParse::Value(*Params, TEXT("DestinationRoot="), DestinationRoot);
	TArray<FMT2ItemImportRecord> Records;
	TArray<FString> Warnings;
	FString Error;
	if (!FMT2ItemImporter::Discover(SourceRoot, DestinationRoot, Records, Warnings, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("Item behavior discovery failed: %s"), *Error);
		return 1;
	}
	for (const auto& Warning : Warnings) UE_LOG(LogTemp, Display, TEXT("%s"), *Warning);
	Records.RemoveAll([](const FMT2ItemImportRecord& Record)
	{
		if (Record.Definition.ItemType == EMT2ItemType::Blend || Record.Definition.ItemType == EMT2ItemType::Fish ||
			Record.Definition.ItemType == EMT2ItemType::Campfire) return false;
		if (Record.Definition.ItemType != EMT2ItemType::Use) return true;
		const auto Type = Record.Definition.SubType.Use;
		return Type != EMT2UseSubType::AbilityUp && Type != EMT2UseSubType::PotionContinue && Type != EMT2UseSubType::Clear;
	});
	if (Records.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("No item behavior definitions matched the source proto."));
		return 1;
	}
	UE_LOG(LogTemp, Display, TEXT("Item behavior definitions: %d"), Records.Num());
	if (FParse::Param(*Params, TEXT("Preview"))) return 0;
	FMT2ItemImportResult Result;
	const bool bSuccess = FMT2ItemImporter::Import(Records, DestinationRoot, [] { return false; }, Result);
	for (const auto& ImportError : Result.Errors) UE_LOG(LogTemp, Error, TEXT("%s"), *ImportError);
	UE_LOG(LogTemp, Display, TEXT("%s"), *Result.BuildSummary());
	return bSuccess ? 0 : 1;
}
