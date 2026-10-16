#include "MT2AuditSemanticTypesCommandlet.h"
#include "MT2SemanticBlueprintMigration.h"

#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "Engine/DataAsset.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Items/MT2ItemSemanticTypes.h"
#include "Components/MT2AffectTypes.h"
#include "Skills/MT2SkillSemanticTypes.h"
#include "Mobs/MT2MobEmpireSelection.h"
#include "Misc/Base64.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

namespace
{
using FRows=TArray<TSharedPtr<FJsonValue>>;
bool IsProjectType(const UStruct* Type)
{
	const FString Package=Type->GetOutermost()->GetName();
	return Package==TEXT("/Script/Metin2") || Package==TEXT("/Script/MT2UEEditor");
}
bool IsVisible(const FProperty* Property)
{
	return Property->HasMetaData(TEXT("SemanticReservedMask")) || Property->HasAnyPropertyFlags(CPF_Edit|CPF_BlueprintVisible) ||
		(Property->HasAnyPropertyFlags(CPF_Parm) && Cast<UFunction>(Property->GetOwnerStruct()) &&
		CastChecked<UFunction>(Property->GetOwnerStruct())->HasAnyFunctionFlags(FUNC_BlueprintCallable|FUNC_BlueprintEvent|FUNC_Delegate));
}
FString Classify(const FProperty* Property)
{
	if (Property->HasMetaData(TEXT("SemanticReservedMask"))) { return TEXT("ReservedOpaqueMask"); }
	if (const auto* Struct=CastField<FStructProperty>(Property); Struct && Struct->Struct->HasMetaData(TEXT("LegacyNumericSelector"))) { return TEXT("NamedSelector"); }
	if (CastField<FEnumProperty>(Property) || (CastField<FByteProperty>(Property) && CastFieldChecked<FByteProperty>(Property)->Enum)) { return TEXT("NamedEnum"); }
	if (Property->HasMetaData(TEXT("BitmaskEnum"))) { return TEXT("NamedMask"); }
	if (Property->HasMetaData(TEXT("SemanticOpenId"))) { return TEXT("OpenIdentifier"); }
	if (Property->HasMetaData(TEXT("SemanticQuantityMap"))) { return TEXT("EnumKeyQuantityMap"); }
	if (!CastField<FNumericProperty>(Property) && !CastField<FArrayProperty>(Property) &&
		!CastField<FMapProperty>(Property) && !CastField<FSetProperty>(Property)) { return TEXT("NonNumeric"); }
	const FString Name=Property->GetName();
	const auto* Function=Cast<UFunction>(Property->GetOwnerStruct());
	const bool ComputedOutput=Property->HasAnyPropertyFlags(CPF_ReturnParm) ||
		(Property->HasAnyPropertyFlags(CPF_OutParm) && !Property->HasAnyPropertyFlags(CPF_ReferenceParm));
	if (Function && ComputedOutput) { return TEXT("NumericQuantityOrIdentifier"); }
	if (Name.Contains(TEXT("Type")) || Name.Contains(TEXT("Flags")) || Name.EndsWith(TEXT("Flag")) || Name.Contains(TEXT("Permission")) || Name.EndsWith(TEXT("Mask")) ||
		Name==TEXT("Empire") || Name==TEXT("Race") || Name==TEXT("Sex") || Name==TEXT("StoneCategory") || Name==TEXT("RequiredSocketGrade"))
	{ return TEXT("UnresolvedSelector"); }
	return TEXT("NumericQuantityOrIdentifier");
}
void CaptureValue(const FProperty* Property,const void* Value,const FString& Path,FRows& Rows,int32 Depth)
{
	if (Depth>32) { return; }
	const FNumericProperty* Numeric=CastField<FNumericProperty>(Property);
	const FEnumProperty* Enum=CastField<FEnumProperty>(Property);
	if (Enum) { Numeric=Enum->GetUnderlyingProperty(); }
	if (Numeric && Numeric->IsInteger())
	{
		auto Row=MakeShared<FJsonObject>();
		Row->SetStringField(TEXT("Path"),Path);
		Row->SetStringField(TEXT("Owner"),Property->GetOwnerStruct()->GetPathName());
		const bool bUnsigned=CastField<FByteProperty>(Numeric) || CastField<FUInt16Property>(Numeric) || CastField<FUInt32Property>(Numeric) || CastField<FUInt64Property>(Numeric);
		Row->SetStringField(TEXT("RawValue"),bUnsigned ? LexToString(Numeric->GetUnsignedIntPropertyValue(Value)) : LexToString(Numeric->GetSignedIntPropertyValue(Value)));
		Row->SetStringField(TEXT("StorageType"),Property->GetCPPType());
		Row->SetStringField(TEXT("Classification"),Classify(Property));
		Row->SetStringField(TEXT("BitmaskEnum"),Property->GetMetaData(TEXT("BitmaskEnum")));
		if (Enum) { Row->SetStringField(TEXT("Enum"),Enum->GetEnum()->GetPathName()); }
		else if (const auto* Byte=CastField<FByteProperty>(Property); Byte && Byte->Enum) { Row->SetStringField(TEXT("Enum"),Byte->Enum->GetPathName()); }
		Rows.Add(MakeShared<FJsonValueObject>(Row)); return;
	}
	if (const auto* Struct=CastField<FStructProperty>(Property))
	{
		if (Struct->Struct==FMT2ItemAddonSelection::StaticStruct() || Struct->Struct==FMT2ItemSubtype::StaticStruct() ||
			Struct->Struct==FMT2ItemUniqueSubtypeSelection::StaticStruct() || Struct->Struct==FMT2AffectIdentifier::StaticStruct() ||
			Struct->Struct==FMT2LegacySkillClassSelection::StaticStruct() || Struct->Struct==FMT2MobEmpireSelection::StaticStruct())
		{
			auto Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("Path"),Path); Row->SetStringField(TEXT("Owner"),Property->GetOwnerStruct()->GetPathName());
			const int32 Raw=Struct->Struct==FMT2ItemSubtype::StaticStruct() ? static_cast<const FMT2ItemSubtype*>(Value)->ToLegacy()
				: Struct->Struct==FMT2AffectIdentifier::StaticStruct() ? static_cast<const FMT2AffectIdentifier*>(Value)->ToLegacyId()
				: Struct->Struct==FMT2LegacySkillClassSelection::StaticStruct() ? static_cast<const FMT2LegacySkillClassSelection*>(Value)->ToLegacy()
				: Struct->Struct==FMT2ItemUniqueSubtypeSelection::StaticStruct() ? static_cast<const FMT2ItemUniqueSubtypeSelection*>(Value)->ToLegacy()
				: Struct->Struct==FMT2MobEmpireSelection::StaticStruct() ? static_cast<const FMT2MobEmpireSelection*>(Value)->ToLegacy()
				: static_cast<const FMT2ItemAddonSelection*>(Value)->ToLegacy();
			Row->SetStringField(TEXT("RawValue"),LexToString(Raw));
			Row->SetStringField(TEXT("StorageType"),Property->GetCPPType()); Row->SetStringField(TEXT("Classification"),TEXT("NamedSelector"));
			Rows.Add(MakeShared<FJsonValueObject>(Row)); return;
		}
		for (TFieldIterator<FProperty> It(Struct->Struct); It; ++It)
		{
			if (!IsVisible(*It)) { continue; }
			for (int32 Index=0;Index<It->ArrayDim;++Index)
			{ CaptureValue(*It,It->ContainerPtrToValuePtr<void>(Value,Index),Path+TEXT(".")+It->GetName()+(It->ArrayDim>1 ? FString::Printf(TEXT("[%d]"),Index) : TEXT("")),Rows,Depth+1); }
		}
	}
	else if (const auto* Array=CastField<FArrayProperty>(Property))
	{
		FScriptArrayHelper Helper(Array,Value);
		for (int32 Index=0;Index<Helper.Num();++Index) { CaptureValue(Array->Inner,Helper.GetRawPtr(Index),FString::Printf(TEXT("%s[%d]"),*Path,Index),Rows,Depth+1); }
	}
	else if (const auto* Map=CastField<FMapProperty>(Property))
	{
		FScriptMapHelper Helper(Map,Value);
		for (int32 Index=0;Index<Helper.GetMaxIndex();++Index)
		{
			if (!Helper.IsValidIndex(Index)) { continue; }
			FString Key; Map->KeyProp->ExportTextItem_Direct(Key,Helper.GetKeyPtr(Index),nullptr,nullptr,PPF_None);
			CaptureValue(Map->KeyProp,Helper.GetKeyPtr(Index),Path+TEXT("{Key:")+Key+TEXT("}"),Rows,Depth+1);
			CaptureValue(Map->ValueProp,Helper.GetValuePtr(Index),Path+TEXT("{")+Key+TEXT("}"),Rows,Depth+1);
		}
	}
	else if (const auto* Set=CastField<FSetProperty>(Property))
	{
		FScriptSetHelper Helper(Set,Value);
		for (int32 Index=0;Index<Helper.GetMaxIndex();++Index)
		{
			if (!Helper.IsValidIndex(Index)) { continue; }
			FString Key; Set->ElementProp->ExportTextItem_Direct(Key,Helper.GetElementPtr(Index),nullptr,nullptr,PPF_None);
			CaptureValue(Set->ElementProp,Helper.GetElementPtr(Index),Path+TEXT("{")+Key+TEXT("}"),Rows,Depth+1);
		}
	}
}
void CaptureObject(UObject* Object,const FString& AssetPath,FRows& Objects)
{
	FRows Properties;
	for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
	{
		const bool BlueprintOwned=It->GetOwnerStruct()->GetOutermost()->GetName().StartsWith(TEXT("/Game/")) ||
			(Object->GetClass()->ClassGeneratedBy && It->GetOwnerStruct()==Object->GetClass());
		if (!IsVisible(*It) || (!IsProjectType(It->GetOwnerStruct()) && !BlueprintOwned)) { continue; }
		for (int32 Index=0;Index<It->ArrayDim;++Index)
		{ CaptureValue(*It,It->ContainerPtrToValuePtr<void>(Object,Index),It->GetName()+(It->ArrayDim>1 ? FString::Printf(TEXT("[%d]"),Index) : TEXT("")),Properties,0); }
	}
	if (Properties.IsEmpty()) { return; }
	auto Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("ObjectPath"),Object->GetPathName());
	Row->SetStringField(TEXT("AssetPath"),AssetPath); Row->SetStringField(TEXT("ClassPath"),Object->GetClass()->GetPathName());
	Row->SetArrayField(TEXT("Properties"),Properties);
	TArray<uint8> Bytes; FMemoryWriter Writer(Bytes,true); FObjectAndNameAsStringProxyArchive Archive(Writer,false); Archive.ArNoDelta=true;
	Object->GetClass()->SerializeTaggedProperties(Archive,reinterpret_cast<uint8*>(Object),Object->GetClass(),nullptr);
	Row->SetStringField(TEXT("TaggedData"),FBase64::Encode(Bytes));
	Objects.Add(MakeShared<FJsonValueObject>(Row));
}
void CaptureAsset(UObject* Asset,FRows& Objects,FRows& Pins,FRows* BlueprintSchema=nullptr)
{
	const FString AssetPath=Asset->GetPathName();
	TArray<UObject*> Children; GetObjectsWithOuter(Asset,Children,true); Children.Insert(Asset,0);
	if (auto* Blueprint=Cast<UBlueprint>(Asset))
	{
		if (Blueprint->GeneratedClass) { Children.AddUnique(Blueprint->GeneratedClass->GetDefaultObject()); }
		if (BlueprintSchema && Blueprint->GeneratedClass)
		{
			for (TFieldIterator<FProperty> It(Blueprint->GeneratedClass,EFieldIterationFlags::None);It;++It) if (IsVisible(*It))
			{
				auto Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("Owner"),Blueprint->GeneratedClass->GetPathName());
				Row->SetStringField(TEXT("Property"),It->GetName()); Row->SetStringField(TEXT("StorageType"),It->GetCPPType());
				Row->SetStringField(TEXT("Classification"),Classify(*It)); Row->SetStringField(TEXT("BitmaskEnum"),It->GetMetaData(TEXT("BitmaskEnum")));
				Row->SetStringField(TEXT("Reason"),TEXT("Project Blueprint declaration; enum/mask bindings or numeric identifier/quantity contract."));
				BlueprintSchema->Add(MakeShared<FJsonValueObject>(Row));
			}
		}
		TArray<UEdGraph*> Graphs; Blueprint->GetAllGraphs(Graphs);
		for (const auto* Graph : Graphs) for (const UEdGraphNode* Node : Graph->Nodes) if (Node) for (const auto* Pin : Node->Pins) if (Pin)
		{
			auto Row=MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("AssetPath"),AssetPath); Row->SetStringField(TEXT("GraphPath"),Graph->GetPathName());
			Row->SetStringField(TEXT("NodeGuid"),Node->NodeGuid.ToString()); Row->SetStringField(TEXT("PinId"),Pin->PinId.ToString());
			Row->SetStringField(TEXT("NodeClass"),Node->GetClass()->GetPathName()); Row->SetNumberField(TEXT("Direction"),Pin->Direction);
			Row->SetStringField(TEXT("PinName"),Pin->PinName.ToString()); Row->SetStringField(TEXT("Category"),Pin->PinType.PinCategory.ToString());
			Row->SetStringField(TEXT("SubCategory"),Pin->PinType.PinSubCategory.ToString());
			Row->SetStringField(TEXT("TypeObject"),GetPathNameSafe(Pin->PinType.PinSubCategoryObject.Get()));
			Row->SetStringField(TEXT("DefaultValue"),Pin->DefaultValue); Row->SetStringField(TEXT("DefaultObject"),GetPathNameSafe(Pin->DefaultObject));
			Row->SetStringField(TEXT("DefaultText"),Pin->DefaultTextValue.ToString());
			FRows Links; for (const auto* Linked : Pin->LinkedTo) if (Linked) { Links.Add(MakeShared<FJsonValueString>(Linked->GetOwningNode()->NodeGuid.ToString()+TEXT(":")+Linked->PinId.ToString())); }
			Row->SetArrayField(TEXT("Links"),Links); Pins.Add(MakeShared<FJsonValueObject>(Row));
		}
	}
	for (UObject* Child : Children) { CaptureObject(Child,AssetPath,Objects); }
}
bool SaveJson(const FString& Path,const TSharedRef<FJsonObject>& Root)
{
	FString Text;
	return FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Text)) &&
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true) && FFileHelper::SaveStringToFile(Text,*Path);
}
TSet<FString> ChangedNativeAssets(const TSharedPtr<FJsonObject>& Baseline)
{
	TSet<FString> Result;
	if (!Baseline) { return Result; }
	for (const auto& Value : Baseline->GetArrayField(TEXT("Objects")))
	{
		const auto Object=Value->AsObject();
		for (const auto& P : Object->GetArrayField(TEXT("Properties")))
		{
			const auto Row=P->AsObject(); FString Name=Row->GetStringField(TEXT("Path"));
			int32 Dot; if (Name.FindLastChar(TEXT('.'),Dot)) { Name=Name.Mid(Dot+1); }
			int32 Bracket; if (Name.FindChar(TEXT('['),Bracket)) { Name.LeftInline(Bracket); }
			const auto* Owner=FindObject<UStruct>(nullptr,*Row->GetStringField(TEXT("Owner")));
			const FProperty* Property=Owner ? FindFProperty<FProperty>(Owner,*Name) : nullptr;
			if (Property && Property->GetCPPType()!=Row->GetStringField(TEXT("StorageType")))
			{ Result.Add(Object->GetStringField(TEXT("AssetPath"))); break; }
		}
	}
	return Result;
}
int32 ComparePins(UBlueprint* Blueprint,const FRows& Before)
{
	TMap<FString,const UEdGraphPin*> Pins;
	TArray<UEdGraph*> Graphs; Blueprint->GetAllGraphs(Graphs);
	for (const auto* Graph : Graphs) for (const UEdGraphNode* Node : Graph->Nodes) if (Node) for (const auto* Pin : Node->Pins) if (Pin)
	{ Pins.Add(Graph->GetPathName()+TEXT(":")+Node->NodeGuid.ToString()+TEXT(":")+Pin->PinId.ToString(),Pin); }
	int32 Failed=0;
	for (const auto& Value : Before)
	{
		const auto Row=Value->AsObject();
		const FString Key=Row->GetStringField(TEXT("GraphPath"))+TEXT(":")+Row->GetStringField(TEXT("NodeGuid"))+TEXT(":")+Row->GetStringField(TEXT("PinId"));
		const auto* Found=Pins.Find(Key);
		if (!Found) { UE_LOG(LogTemp,Error,TEXT("[SemanticTypes] Pin identity lost: %s"),*Key); ++Failed; continue; }
		const UEdGraphPin* Pin=*Found;
		TArray<FString> OldLinks,NewLinks;
		for (const auto& Link : Row->GetArrayField(TEXT("Links"))) { OldLinks.Add(Link->AsString()); }
		for (const auto* Link : Pin->LinkedTo) if (Link) { NewLinks.Add(Link->GetOwningNode()->NodeGuid.ToString()+TEXT(":")+Link->PinId.ToString()); }
		OldLinks.Sort(); NewLinks.Sort();
		const bool Default=MT2SemanticBlueprintMigration::EquivalentDefault(Pin,Row->GetStringField(TEXT("DefaultValue")));
		const bool SameType=Pin->PinType.PinCategory.ToString()==Row->GetStringField(TEXT("Category")) &&
			GetPathNameSafe(Pin->PinType.PinSubCategoryObject.Get())==Row->GetStringField(TEXT("TypeObject"));
		const FString OldCategory=Row->GetStringField(TEXT("Category"));
		const auto* NewObject=Pin->PinType.PinSubCategoryObject.Get();
		const bool TypedNumeric=(OldCategory==TEXT("int") || OldCategory==TEXT("int64") || OldCategory==TEXT("byte")) && NewObject &&
			NewObject->GetOutermost()->GetName()==TEXT("/Script/Metin2") &&
			(Cast<UEnum>(NewObject) || (Cast<UScriptStruct>(NewObject) && CastChecked<UScriptStruct>(NewObject)->HasMetaData(TEXT("LegacyNumericSelector"))));
		if (OldLinks!=NewLinks || !Default || (!SameType && !TypedNumeric) ||
			GetPathNameSafe(Pin->DefaultObject)!=Row->GetStringField(TEXT("DefaultObject")) ||
			Pin->DefaultTextValue.ToString()!=Row->GetStringField(TEXT("DefaultText")))
		{ UE_LOG(LogTemp,Error,TEXT("[SemanticTypes] Pin meaning/default/link changed: %s"),*Key); ++Failed; }
	}
	return Failed;
}
struct FPackageBackup { FString Filename,Backup; };
bool RestorePackages(const TArray<FPackageBackup>& Backups)
{
	bool Success=true;
	for (const auto& Item : Backups)
	{
		if (!FPlatformFileManager::Get().GetPlatformFile().CopyFile(*Item.Filename,*Item.Backup))
		{ UE_LOG(LogTemp,Error,TEXT("[SemanticTypes] Restore failed; original remains at %s"),*Item.Backup); Success=false; }
	}
	return Success;
}
bool SaveChangedPackages(const TArray<TStrongObjectPtr<UBlueprint>>& Blueprints,TArray<FPackageBackup>& Backups,int32& Saved)
{
	const FString BackupRoot=FPaths::ProjectSavedDir()/TEXT("SemanticTypes/Backups")/FGuid::NewGuid().ToString();
	TArray<UBlueprint*> Persistent;
	for (const auto& Blueprint : Blueprints)
	{
		UPackage* Package=Blueprint->GetOutermost();
		if (Package==GetTransientPackage()) { continue; }
		if (!Package->GetName().StartsWith(TEXT("/Game/")))
		{ UE_LOG(LogTemp,Error,TEXT("[SemanticTypes] Refusing non-project package %s"),*Package->GetName()); return false; }
		const FString Filename=FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension());
		const FString Backup=BackupRoot/Package->GetName().Mid(6)+FPackageName::GetAssetPackageExtension();
		if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(Backup),true) ||
			!FPlatformFileManager::Get().GetPlatformFile().CopyFile(*Backup,*Filename))
		{ UE_LOG(LogTemp,Error,TEXT("[SemanticTypes] Cannot back up %s; no package writes started."),*Filename); return false; }
		Backups.Add({Filename,Backup}); Persistent.Add(Blueprint.Get());
	}
	if (!Backups.IsEmpty()) { UE_LOG(LogTemp,Display,TEXT("[SemanticTypes] Original package backups: %s"),*BackupRoot); }
	for (int32 Index=0;Index<Persistent.Num();++Index)
	{
		auto* Blueprint=Persistent[Index]; FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
		if (!UPackage::SavePackage(Blueprint->GetOutermost(),Blueprint,*Backups[Index].Filename,Args))
		{ UE_LOG(LogTemp,Error,TEXT("[SemanticTypes] Package save failed; restoring original packages.")); RestorePackages(Backups); return false; }
		++Saved;
	}
	return true;
}
}

UMT2AuditSemanticTypesCommandlet::UMT2AuditSemanticTypesCommandlet()
{
	IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true;
}
int32 UMT2AuditSemanticTypesCommandlet::Main(const FString& Params)
{
	check(IsInGameThread());
	FString Mode,ManifestPath,ObjectPath,ReportPath;
	FParse::Value(*Params,TEXT("Mode="),Mode); FParse::Value(*Params,TEXT("Manifest="),ManifestPath);
	FParse::Value(*Params,TEXT("Object="),ObjectPath); FParse::Value(*Params,TEXT("Report="),ReportPath);
	const bool Migrate=Mode==TEXT("Migrate"),Apply=Migrate && FParse::Param(*Params,TEXT("Apply"));
	if ((Mode!=TEXT("Capture") && Mode!=TEXT("Validate") && !Migrate) || ManifestPath.IsEmpty())
	{ UE_LOG(LogTemp,Error,TEXT("[SemanticTypes] Specify -Mode=Capture|Validate|Migrate and -Manifest=<path>.")); return 1; }
	TSharedPtr<FJsonObject> Baseline;
	if (Mode!=TEXT("Capture"))
	{
		FString Json; const TArray<TSharedPtr<FJsonValue>>* Array=nullptr;
		if (!FFileHelper::LoadFileToString(Json,*ManifestPath) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Baseline) ||
			!Baseline.IsValid() || !Baseline->HasTypedField<EJson::Number>(TEXT("Version")) || Baseline->GetIntegerField(TEXT("Version"))!=1 ||
			!Baseline->TryGetArrayField(TEXT("AssetRoots"),Array) || !Baseline->TryGetArrayField(TEXT("Objects"),Array) ||
			!Baseline->TryGetArrayField(TEXT("BlueprintPins"),Array) || !Baseline->TryGetArrayField(TEXT("Schema"),Array))
		{ UE_LOG(LogTemp,Error,TEXT("[SemanticTypes] Invalid or unsupported baseline manifest.")); return 1; }
		if (ReportPath.IsEmpty() || FPaths::IsSamePath(ReportPath,ManifestPath))
		{ UE_LOG(LogTemp,Error,TEXT("[SemanticTypes] Validation requires a distinct -Report path; baseline is never overwritten.")); return 1; }
	}
	else if (IFileManager::Get().FileExists(*ManifestPath) && !FParse::Param(*Params,TEXT("Overwrite")))
	{ UE_LOG(LogTemp,Error,TEXT("[SemanticTypes] Baseline exists; use a new path or explicit -Overwrite.")); return 1; }
	FRows Objects,Pins,AssetRoots,BlueprintSchema,Proposed;
	TArray<FString> Paths;
	if (Baseline)
	{ for (const auto& Value : Baseline->GetArrayField(TEXT("AssetRoots"))) { Paths.AddUnique(Value->AsString()); } }
	else if (!ObjectPath.IsEmpty()) { Paths.Add(ObjectPath); }
	else
	{
		auto& Registry=FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get(); Registry.SearchAllAssets(true);
		FARFilter Filter; Filter.PackagePaths.Add(TEXT("/Game")); Filter.bRecursivePaths=true; Filter.bRecursiveClasses=true;
		for (const UClass* Class : {UBlueprint::StaticClass(),UDataAsset::StaticClass(),UWorld::StaticClass(),UAnimSequence::StaticClass()}) { Filter.ClassPaths.Add(Class->GetClassPathName()); }
		TArray<FAssetData> Assets; Registry.GetAssets(Filter,Assets); Assets.Sort([](const auto& A,const auto& B){return A.PackageName.LexicalLess(B.PackageName);});
		for (const auto& Asset : Assets) { Paths.Add(Asset.GetSoftObjectPath().ToString()); }
	}
	TMap<FString,FRows> BeforePins;
	if (Baseline) for (const auto& Row : Baseline->GetArrayField(TEXT("BlueprintPins"))) { BeforePins.FindOrAdd(Row->AsObject()->GetStringField(TEXT("AssetPath"))).Add(Row); }
	const TSet<FString> NativeChanged=ChangedNativeAssets(Baseline);
	TSet<FString> ChangedBlueprints; TArray<FString> CompilePaths;
	int32 Failed=0,Compiled=0,Saved=0;
	auto Load=[&](const FString& Path)->UObject*
	{
		UObject* Asset=FindObject<UObject>(nullptr,*Path); if (!Asset) { Asset=LoadObject<UObject>(nullptr,*Path); }
		if (!Asset) { UE_LOG(LogTemp,Error,TEXT("[SemanticTypes] Cannot load %s"),*Path); ++Failed; }
		return Asset;
	};
	auto ReportIssues=[&](const TArray<FString>& Issues)
	{ for (const auto& Issue : Issues) { UE_LOG(LogTemp,Error,TEXT("[SemanticTypes] %s"),*Issue); ++Failed; } };
	for (int32 Index=0;Index<Paths.Num();++Index)
	{
		UObject* Asset=Load(Paths[Index]); if (!Asset) { continue; }
		if (auto* Blueprint=Cast<UBlueprint>(Asset))
		{
			if (Baseline) { Failed+=ComparePins(Blueprint,BeforePins.FindOrAdd(Paths[Index])); }
			if (Migrate)
			{
				MT2SemanticBlueprintMigration::FPlan Plan; TArray<FString> Issues;
				MT2SemanticBlueprintMigration::Plan(Blueprint,Plan,Issues); ReportIssues(Issues);
				CompilePaths.Add(Paths[Index]);
				if (!Plan.Pins.IsEmpty()) { ChangedBlueprints.Add(Paths[Index]); }
				for (const auto& Change : Plan.Pins)
				{
					auto Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("AssetPath"),Paths[Index]);
					Row->SetStringField(TEXT("NodeGuid"),Change.Pin->GetOwningNode()->NodeGuid.ToString());
					Row->SetStringField(TEXT("PinId"),Change.Pin->PinId.ToString()); Row->SetStringField(TEXT("PinName"),Change.Pin->PinName.ToString());
					Row->SetStringField(TEXT("OldDefault"),Change.Pin->DefaultValue); Row->SetStringField(TEXT("NewDefault"),Change.DefaultValue);
					Row->SetStringField(TEXT("NewType"),GetPathNameSafe(Change.Type.PinSubCategoryObject.Get())); Proposed.Add(MakeShared<FJsonValueObject>(Row));
				}
			}
		}
		CaptureAsset(Asset,Objects,Pins,&BlueprintSchema); AssetRoots.Add(MakeShared<FJsonValueString>(Paths[Index]));
		if (ObjectPath.IsEmpty() && (Index+1)%25==0)
		{ UE_LOG(LogTemp,Display,TEXT("[SemanticTypes] Preflight %d/%d assets"),Index+1,Paths.Num()); CollectGarbage(RF_NoFlags); }
	}
	auto CompareValues=[&]()
	{
		if (!Baseline) { return; }
		TMap<FString,FString> Current;
		for (const auto& Value : Objects) for (const auto& P : Value->AsObject()->GetArrayField(TEXT("Properties")))
		{ Current.Add(Value->AsObject()->GetStringField(TEXT("ObjectPath"))+TEXT(":")+P->AsObject()->GetStringField(TEXT("Path")),P->AsObject()->GetStringField(TEXT("RawValue"))); }
		for (const auto& Value : Baseline->GetArrayField(TEXT("Objects"))) for (const auto& P : Value->AsObject()->GetArrayField(TEXT("Properties")))
		{
			const FString Key=Value->AsObject()->GetStringField(TEXT("ObjectPath"))+TEXT(":")+P->AsObject()->GetStringField(TEXT("Path"));
			const FString* Raw=Current.Find(Key);
			if (!Raw || *Raw!=P->AsObject()->GetStringField(TEXT("RawValue"))) { ++Failed; UE_LOG(LogTemp,Error,TEXT("[SemanticTypes] Value changed or missing: %s"),*Key); }
		}
	};
	CompareValues();
	auto Root=MakeShared<FJsonObject>(); Root->SetNumberField(TEXT("Version"),1); Root->SetStringField(TEXT("Engine"),FEngineVersion::Current().ToString());
	Root->SetArrayField(TEXT("AssetRoots"),AssetRoots);
	FRows Schema=BlueprintSchema;
	for (TObjectIterator<UStruct> Type;Type;++Type) if (IsProjectType(*Type))
	{
		for (TFieldIterator<FProperty> It(*Type,EFieldIterationFlags::None);It;++It) if (IsVisible(*It))
		{
			auto Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("Owner"),Type->GetPathName()); Row->SetStringField(TEXT("Property"),It->GetName());
			Row->SetStringField(TEXT("StorageType"),It->GetCPPType()); Row->SetStringField(TEXT("Classification"),Classify(*It));
			Row->SetStringField(TEXT("BitmaskEnum"),It->GetMetaData(TEXT("BitmaskEnum")));
			Row->SetStringField(TEXT("Reason"),Classify(*It)==TEXT("UnresolvedSelector") ? TEXT("Finite-looking numeric field needs a domain or explicit open-ID contract.") :
				TEXT("Named reflected domain/mask/codec, or a numerical quantity, editable-table index or open identity."));
			Schema.Add(MakeShared<FJsonValueObject>(Row));
		}
	}
	Root->SetArrayField(TEXT("Schema"),Schema); Root->SetArrayField(TEXT("ProposedPinChanges"),Proposed);
	Root->SetNumberField(TEXT("ChangedBlueprints"),ChangedBlueprints.Num()); Root->SetNumberField(TEXT("BlueprintsToCompile"),CompilePaths.Num());
	Root->SetNumberField(TEXT("NativeChangedAssets"),NativeChanged.Num()); Root->SetBoolField(TEXT("ApplyRequested"),Apply);
	auto UpdateReport=[&]()
	{
		Root->SetArrayField(TEXT("Objects"),Objects); Root->SetArrayField(TEXT("BlueprintPins"),Pins);
		Root->SetNumberField(TEXT("Failures"),Failed); Root->SetNumberField(TEXT("CompiledBlueprints"),Compiled); Root->SetNumberField(TEXT("SavedPackages"),Saved);
	};
	UpdateReport();
	const FString Output=Baseline ? ReportPath : ManifestPath;
	if (!SaveJson(Output,Root)) { UE_LOG(LogTemp,Error,TEXT("[SemanticTypes] Cannot write report; no packages saved.")); return 1; }
	TArray<TStrongObjectPtr<UBlueprint>> ToSave; TArray<FPackageBackup> Backups;
	if (Apply && Failed==0)
	{
		// Every package/value/link has passed preflight before any graph mutation or disk write.
		for (int32 Index=0;Index<CompilePaths.Num();++Index)
		{
			auto* Blueprint=Cast<UBlueprint>(Load(CompilePaths[Index])); if (!Blueprint) { continue; }
			TStrongObjectPtr<UBlueprint> KeepAlive(Blueprint);
			MT2SemanticBlueprintMigration::FPlan Plan; TArray<FString> Issues;
			if (!MT2SemanticBlueprintMigration::Plan(Blueprint,Plan,Issues)) { ReportIssues(Issues); break; }
			const bool Changed=!Plan.Pins.IsEmpty();
			if (!MT2SemanticBlueprintMigration::Apply(Blueprint,Plan,Issues)) { ReportIssues(Issues); break; }
			++Compiled; Failed+=ComparePins(Blueprint,BeforePins.FindOrAdd(CompilePaths[Index]));
			Objects.RemoveAll([&](const auto& Value){ return Value->AsObject()->GetStringField(TEXT("AssetPath"))==CompilePaths[Index]; });
			Pins.RemoveAll([&](const auto& Value){ return Value->AsObject()->GetStringField(TEXT("AssetPath"))==CompilePaths[Index]; });
			CaptureAsset(Blueprint,Objects,Pins);
			if (Changed) { ToSave.Emplace(Blueprint); }
			if (Failed) { break; }
			if (ObjectPath.IsEmpty() && (Index+1)%25==0)
			{ UE_LOG(LogTemp,Display,TEXT("[SemanticTypes] Compiled %d/%d Blueprints"),Index+1,CompilePaths.Num()); CollectGarbage(RF_NoFlags); }
		}
		CompareValues();
		if (!Failed && !SaveChangedPackages(ToSave,Backups,Saved)) { ++Failed; }
		UpdateReport();
		if (!SaveJson(Output,Root))
		{ UE_LOG(LogTemp,Error,TEXT("[SemanticTypes] Final report write failed; restoring original packages.")); RestorePackages(Backups); return 1; }
	}
	UE_LOG(LogTemp,Display,TEXT("[SemanticTypes] Mode=%s Assets=%d Objects=%d Pins=%d Changes=%d Compiled=%d Saved=%d Failures=%d"),
		*Mode,Paths.Num(),Objects.Num(),Pins.Num(),ChangedBlueprints.Num(),Compiled,Saved,Failed);
	return Failed ? 1 : 0;
}
