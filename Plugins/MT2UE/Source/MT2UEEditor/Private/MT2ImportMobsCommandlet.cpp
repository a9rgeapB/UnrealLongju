/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "MT2ImportMobsCommandlet.h"
#include "Config/MT2PathSettings.h"

#include "Importers/MT2MobImporter.h"
#include "Misc/Parse.h"
#include "Mining/MT2MiningRules.h"
#include "MT2VnumRegistryBuilder.h"

UMT2ImportMobsCommandlet::UMT2ImportMobsCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UMT2ImportMobsCommandlet::Main(const FString& Params)
{
	FString SourceRoot = UMT2PathSettings::Path(TEXT("LegacyDumpRoot"));
	FString DestinationRoot = UMT2PathSettings::Path(TEXT("ImportDestinationRoot"));
	FParse::Value(*Params, TEXT("Source="), SourceRoot);
	FParse::Value(*Params, TEXT("Destination="), DestinationRoot);
	if (FParse::Param(*Params, TEXT("LootOnly")))
	{
		TArray<FString> LootWarnings;
		const int32 ConfiguredMobs = FMT2MobImporter::ImportMobLootEntries(
			SourceRoot, DestinationRoot, LootWarnings);
		for (const FString& Warning : LootWarnings)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Warning);
		}
		UE_LOG(LogTemp, Display, TEXT("Generated loot tables for %d mob Blueprints."), ConfiguredMobs);
		return 0;
	}

	TArray<FMT2MobImportRecord> Records;
	TArray<FString> Warnings;
	FString Error;
	// Include already-imported mobs so a commandlet run refreshes their DataAsset stats too.
	if (!FMT2MobImporter::Discover(SourceRoot, DestinationRoot, Records, Warnings, Error, true))
	{
		UE_LOG(LogTemp, Error, TEXT("Mob discovery failed: %s"), *Error);
		return 1;
	}
	for (const FString& Warning : Warnings)
	{
		UE_LOG(LogTemp, Display, TEXT("%s"), *Warning);
	}
	if (FParse::Param(*Params,TEXT("OreVeinsOnly")))
	{
		Records.RemoveAll([](const FMT2MobImportRecord& Record) { return MT2Mining::RawOre(Record.Definition.Vnum)==0; });
		if (Records.Num()!=18)
		{
			UE_LOG(LogTemp,Error,TEXT("Expected all 18 ore veins, but discovered %d. Check imported static meshes and source race scripts."),Records.Num());
			return 1;
		}
	}
	UE_LOG(LogTemp, Display, TEXT("Discovered %d importable mobs."), Records.Num());
	if (!Records.IsEmpty())
	{
		const FMT2MobImportRecord& First = Records[0];
		UE_LOG(LogTemp, Display, TEXT("First mob: VNUM=%d Name=%s Resource=%s Level=%d HP=%d Mesh=%s"),
			First.Definition.Vnum,
			*First.Definition.DisplayName,
			*First.Definition.ResourceName,
			First.Definition.Level,
			First.Definition.MaxHealth,
			*First.MeshObjectPath);
	}

	if (FParse::Param(*Params, TEXT("DiscoverOnly")))
	{
		return 0;
	}

	int32 MaxRecords = 0;
	FParse::Value(*Params, TEXT("Max="), MaxRecords);
	if (MaxRecords > 0 && Records.Num() > MaxRecords)
	{
		Records.SetNum(MaxRecords);
	}
	FMT2MobImportResult Result;
	FMT2MobImporter::Import(Records, SourceRoot, DestinationRoot, []() { return false; }, Result);
	UE_LOG(LogTemp, Display, TEXT("%s"), *Result.BuildSummary());
	for (const FString& ImportError : Result.Errors)
	{
		UE_LOG(LogTemp, Error, TEXT("%s"), *ImportError);
	}
	if (Result.Errors.IsEmpty() && FParse::Param(*Params,TEXT("OreVeinsOnly")))
	{
		TArray<FString> RegistryErrors; int32 MobCount=0,ItemCount=0;
		if (!FMT2VnumRegistryBuilder::RebuildAndSave(RegistryErrors,MobCount,ItemCount))
		{
			for (const FString& RegistryError : RegistryErrors) { UE_LOG(LogTemp,Error,TEXT("%s"),*RegistryError); }
			return 1;
		}
		UE_LOG(LogTemp,Display,TEXT("VNUM registry saved: %d mobs, %d items."),MobCount,ItemCount);
	}
	return Result.Errors.IsEmpty() ? 0 : 1;
}
