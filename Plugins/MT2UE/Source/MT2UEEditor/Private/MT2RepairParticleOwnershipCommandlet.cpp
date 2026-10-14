#include "MT2RepairParticleOwnershipCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Importers/MT2EffectImporter.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Particles/ParticleModule.h"
#include "Particles/ParticleSystem.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectHash.h"

UMT2RepairParticleOwnershipCommandlet::UMT2RepairParticleOwnershipCommandlet()
{
	IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true;
}

int32 UMT2RepairParticleOwnershipCommandlet::Main(const FString& Params)
{
	const bool bRepair = FParse::Param(*Params, TEXT("Repair"));
	FString Root = TEXT("/Game/ymir_work");
	FParse::Value(*Params, TEXT("Root="), Root);
	if (!Root.StartsWith(TEXT("/Game/")) || !FPackageName::IsValidLongPackageName(Root))
	{
		UE_LOG(LogTemp, Error, TEXT("[ParticleOwnership] Root must be a project content folder."));
		return 1;
	}
	auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.SearchAllAssets(true);
	FARFilter Filter;
	Filter.PackagePaths.Add(*Root); Filter.bRecursivePaths = true;
	Filter.ClassPaths.Add(UParticleSystem::StaticClass()->GetClassPathName());
	TArray<FAssetData> Assets;
	Registry.GetAssets(Filter, Assets);
	const FString BackupRoot = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ParticleOwnershipBackups"),
		FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S")) + TEXT("-") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
	int32 Invalid = 0, Repaired = 0, Failed = 0;
	for (int32 Index = 0; Index < Assets.Num(); ++Index)
	{
		UParticleSystem* System = Cast<UParticleSystem>(Assets[Index].GetAsset());
		if (!System) { ++Failed; continue; }
		TArray<UObject*> Objects;
		GetObjectsWithOuter(System, Objects, true);
		bool bInvalid = false;
		for (UObject* Object : Objects)
		{
			if (const UParticleModule* Module = Cast<UParticleModule>(Object))
				bInvalid |= Module->GetOuter() != System;
		}
		if (bInvalid)
		{
			++Invalid;
			UE_LOG(LogTemp, Display, TEXT("[ParticleOwnership] Invalid: %s"), *System->GetPathName());
			if (bRepair)
			{
				UPackage* Package = System->GetOutermost();
				const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
				const FString Backup = FPaths::Combine(BackupRoot, Package->GetName().Mid(6) + FPackageName::GetAssetPackageExtension());
				bool bBackupSucceeded = IFileManager::Get().MakeDirectory(*FPaths::GetPath(Backup), true);
				for (const TCHAR* Extension : { TEXT("uasset"), TEXT("uexp"), TEXT("ubulk") })
				{
					const FString Source = FPaths::ChangeExtension(Filename, Extension);
					if (IFileManager::Get().FileExists(*Source))
						bBackupSucceeded &= IFileManager::Get().Copy(*FPaths::ChangeExtension(Backup, Extension), *Source, false) == COPY_OK;
					else if (FCString::Strcmp(Extension, TEXT("uasset")) == 0) bBackupSucceeded = false;
				}
				FString Error;
				if (!bBackupSucceeded || !FMT2EffectImporter::RepairModuleOwnership(System, Error))
				{
					UE_LOG(LogTemp, Error, TEXT("[ParticleOwnership] Repair aborted for %s: %s"), *Filename,
						bBackupSucceeded ? *Error : TEXT("Backup failed; asset was not changed."));
					++Failed;
				}
				else
				{
					Package->MarkPackageDirty();
					FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
					if (UPackage::SavePackage(Package, System, *Filename, Args)) ++Repaired;
					else { ++Failed; UE_LOG(LogTemp, Error, TEXT("[ParticleOwnership] Could not save %s; backup: %s"), *Filename, *Backup); }
				}
			}
		}
		if ((Index + 1) % 25 == 0) CollectGarbage(RF_NoFlags);
	}
	UE_LOG(LogTemp, Display, TEXT("[ParticleOwnership] Scanned=%d Invalid=%d Repaired=%d Failed=%d Backup=%s"),
		Assets.Num(), Invalid, Repaired, Failed, *BackupRoot);
	return Failed || (!bRepair && Invalid) ? 1 : 0;
}
