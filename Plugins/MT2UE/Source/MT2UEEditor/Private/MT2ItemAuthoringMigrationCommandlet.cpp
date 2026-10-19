#include "MT2ItemAuthoringMigrationCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Engine/Blueprint.h"
#include "HAL/FileManager.h"
#include "Items/MT2ItemTemplate.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Importers/MT2ItemReferenceResolver.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"
#include "Items/Legacy/MT2LegacyItemRecord.h"
#include "Items/MT2ItemUtils.h"

UMT2ItemAuthoringMigrationCommandlet::UMT2ItemAuthoringMigrationCommandlet()
{
	IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true;
}

int32 UMT2ItemAuthoringMigrationCommandlet::Main(const FString& Params)
{
	const FString Root = FPaths::ProjectSavedDir() / TEXT("ItemAuthoringMigration");
	const FString Manifest = Root / TEXT("Manifest.json");
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	if (bApply && !IFileManager::Get().FileExists(*Manifest))
	{
		UE_LOG(LogTemp, Error, TEXT("Create a preservation snapshot before applying the item migration.")); return 1;
	}
	if (!bApply && IFileManager::Get().FileExists(*Manifest))
	{
		UE_LOG(LogTemp, Error, TEXT("Refusing to overwrite item authoring snapshot: %s"), *Manifest);
		return 1;
	}
	auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.SearchAllAssets(true);
	TArray<FAssetData> Assets;
	Registry.GetAssetsByPath(TEXT("/Game/Items"), Assets, true);
	if (bApply)
	{
		FString ManifestText;
		TSharedPtr<FJsonObject> Snapshot;
		if (!FFileHelper::LoadFileToString(ManifestText, *Manifest) ||
			!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ManifestText), Snapshot) || !Snapshot)
		{ UE_LOG(LogTemp, Error, TEXT("Cannot read item preservation snapshot.")); return 1; }
		TMap<FString, TSharedPtr<FJsonObject>> SavedDefaults;
		for (const auto& Record : Snapshot->GetArrayField(TEXT("items")))
		{
			const auto Object = Record->AsObject();
			SavedDefaults.Add(Object->GetStringField(TEXT("asset")), Object->GetObjectField(TEXT("defaults")));
		}
		TArray<UBlueprint*> Blueprints;
		for (const auto& Asset : Assets)
		{
			if (Asset.AssetClassPath != UBlueprint::StaticClass()->GetClassPathName()) continue;
			auto* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
			if (!Blueprint || !Blueprint->GeneratedClass || !Blueprint->GeneratedClass->IsChildOf(UMT2ItemTemplate::StaticClass())) continue;
			FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipGarbageCollection);
			if (Blueprint->Status == BS_Error)
			{ UE_LOG(LogTemp, Error, TEXT("Cannot migrate invalid item Blueprint %s."), *Blueprint->GetPathName()); return 1; }
			Blueprints.Add(Blueprint);
		}
		TArray<UMT2ItemTemplate*> Items;
		for (auto* Blueprint : Blueprints)
		{
			auto* Item = Blueprint->GeneratedClass->GetDefaultObject<UMT2ItemTemplate>();
			const auto SnapshotDefaults = SavedDefaults.FindRef(Blueprint->GetPathName());
			const TSharedPtr<FJsonObject>* InstanceDefaults = nullptr;
			if (!SnapshotDefaults || !SnapshotDefaults->TryGetObjectField(TEXT("defaultInstanceData"), InstanceDefaults))
			{ UE_LOG(LogTemp, Error, TEXT("No original default record for %s."), *Blueprint->GetPathName()); return 1; }
			FMT2LegacyItemRecord Original;
			if (!FJsonObjectConverter::JsonObjectToUStruct((*InstanceDefaults).ToSharedRef(), &Original, 0, 0))
			{ UE_LOG(LogTemp, Error, TEXT("Cannot decode original defaults for %s."), *Blueprint->GetPathName()); return 1; }
			FMT2ItemSlot Expected; Expected.Vnum = Item->Vnum;
			Expected.Count = Original.Count; Expected.RealtimeExpiresAt = Original.RealtimeExpiresAt;
			Expected.FamilyState.InitializeAs<FMT2LegacyItemRecord>(Original);
			MT2ItemUtils::NormalizeNumericSockets(Expected, Item);
			Item->Modify(); Item->FinalizeFamilyDefaults();
			FMT2ItemSlot Actual(Item->Vnum, Item->DefaultInstanceData);
			Item->InitializeDefaultFamilyState(Actual);
			MT2ItemUtils::NormalizeNumericSockets(Actual, Item);
			if (Actual.Count != Expected.Count || !MT2ItemUtils::HaveSameInstanceData(Expected, Actual))
			{ UE_LOG(LogTemp, Error, TEXT("Refusing to save changed authored defaults for %s."), *Blueprint->GetPathName()); return 1; }
			Items.Add(Item);
		}
		TArray<FString> Warnings;
		ResolveMT2ItemReferences(Items, Warnings);
		for (const auto& Warning : Warnings) UE_LOG(LogTemp, Warning, TEXT("%s"), *Warning);
		int32 Saved = 0;
		for (auto* Blueprint : Blueprints)
		{
			auto* Package = Blueprint->GetOutermost();
			Blueprint->MarkPackageDirty();
			const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
			FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
			if (!UPackage::SavePackage(Package, Blueprint, *Filename, Args))
			{ UE_LOG(LogTemp, Error, TEXT("Failed to save migrated item %s."), *Filename); return 1; }
			++Saved;
		}
		UE_LOG(LogTemp, Display, TEXT("Item authoring migration: %d assets saved, %d unresolved-link warnings; original backups retained."), Saved, Warnings.Num());
		return Saved > 0 ? 0 : 1;
	}
	TArray<TSharedPtr<FJsonValue>> Records;
	for (const auto& Asset : Assets)
	{
		if (Asset.AssetClassPath != UBlueprint::StaticClass()->GetClassPathName()) continue;
		const auto* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
		const UClass* Class = Blueprint ? Blueprint->GeneratedClass.Get() : nullptr;
		if (!Class || !Class->IsChildOf(UMT2ItemTemplate::StaticClass())) continue;
		auto Record = MakeShared<FJsonObject>();
		Record->SetStringField(TEXT("asset"), Asset.GetObjectPathString());
		Record->SetStringField(TEXT("class"), Class->GetPathName());
		auto Defaults = MakeShared<FJsonObject>();
		if (!FJsonObjectConverter::UStructToJsonObject(Class, Class->GetDefaultObject(), Defaults, 0, CPF_Transient))
		{
			UE_LOG(LogTemp, Error, TEXT("Could not export defaults for %s."), *Class->GetPathName()); return 1;
		}
		Record->SetObjectField(TEXT("defaults"), Defaults);
		const FString Filename = FPackageName::LongPackageNameToFilename(Asset.PackageName.ToString(), FPackageName::GetAssetPackageExtension());
		const FString Backup = Root / TEXT("Assets") / Asset.PackageName.ToString().RightChop(6) + FPackageName::GetAssetPackageExtension();
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Backup), true);
		if (IFileManager::Get().Copy(*Backup, *Filename, false) != COPY_OK)
		{
			UE_LOG(LogTemp, Error, TEXT("Could not back up %s to %s."), *Filename, *Backup); return 1;
		}
		Records.Add(MakeShared<FJsonValueObject>(Record));
	}
	if (Records.IsEmpty()) { UE_LOG(LogTemp, Error, TEXT("No item assets found to snapshot.")); return 1; }
	auto Document = MakeShared<FJsonObject>();
	Document->SetNumberField(TEXT("version"), 1);
	Document->SetArrayField(TEXT("items"), Records);
	FString Text;
	FJsonSerializer::Serialize(Document, TJsonWriterFactory<>::Create(&Text));
	if (!FFileHelper::SaveStringToFile(Text, *Manifest)) { UE_LOG(LogTemp, Error, TEXT("Could not write %s."), *Manifest); return 1; }
	UE_LOG(LogTemp, Display, TEXT("Item authoring snapshot: %d assets, %s"), Records.Num(), *Manifest);
	return 0;
}
