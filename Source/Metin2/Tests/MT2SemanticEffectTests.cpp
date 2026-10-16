#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Components/MT2StatusEffectComponent.h"
#include "Skills/MT2SkillDefinition.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Player/MT2PlayerState.h"
#include "Engine/World.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2AffectWireTest,"Metin2.SemanticTypes.EffectIdentityPreservesHighIds",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2AffectWireTest::RunTest(const FString&)
{
	for (int32 Id : {0,1,512,534,30001,123456789,MIN_int32,MAX_int32})
	{
		FMT2AffectIdentifier Original=FMT2AffectIdentifier::FromLegacyId(Id), Restored;
		TestEqual(TEXT("Native identity codec is lossless"),Original.ToLegacyId(),Id);
		TArray<uint8> Bytes; FMemoryWriter Writer(Bytes,true); bool bSuccess=false;
		auto* Ops=FMT2AffectIdentifier::StaticStruct()->GetCppStructOps();
		if (!TestTrue(TEXT("Replication uses the explicit raw-ID codec"),Ops && Ops->HasNetSerializer())) { return false; }
		TestTrue(TEXT("Net serializer handles the record"),Ops->NetSerialize(Writer,nullptr,bSuccess,&Original));
		TestTrue(TEXT("Wire write succeeds"),bSuccess); TestEqual(TEXT("Affect ID stays four bytes, not the authoring struct layout"),Bytes.Num(),4);
		FMemoryReader Reader(Bytes,true); bSuccess=false; Ops->NetSerialize(Reader,nullptr,bSuccess,&Restored);
		TestTrue(TEXT("Wire read succeeds"),bSuccess); TestEqual(TEXT("Initial/late snapshot identity remains exact"),Restored.ToLegacyId(),Id);
	}
	TestEqual(TEXT("Built-in health pool retains project ID"),FMT2AffectIdentifier::FromBuiltin(EMT2BuiltinAffect::HealthRecovery).ToLegacyId(),30001);
	TestEqual(TEXT("Built-in book affect retains legacy ID"),FMT2AffectIdentifier::FromBuiltin(EMT2BuiltinAffect::SkillBookGuaranteedSuccess).ToLegacyId(),512);
	TestEqual(TEXT("Arbitrary skill VNUM stays authorable"),FMT2AffectIdentifier::FromSkillVnum(12345).ToLegacyId(),12345);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2AffectSaveTest,"Metin2.SemanticTypes.EffectSaveRoundTrip",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2AffectSaveTest::RunTest(const FString&)
{
	UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
	if (!TestNotNull(TEXT("World"),World)) { return false; }
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	auto* State=World->SpawnActor<AMT2PlayerState>();
	if (!TestNotNull(TEXT("Player state"),State)) { return false; }
	const TArray<int32> Ids={0,1,512,534,30001,123456789,MIN_int32,MAX_int32};
	auto Root=MakeShared<FJsonObject>(); Root->SetNumberField(TEXT("version"),5);
	TArray<TSharedPtr<FJsonValue>> Rows;
	for (int32 Id : Ids)
	{
		auto Row=MakeShared<FJsonObject>(); Row->SetNumberField(TEXT("type"),Id);
		Row->SetNumberField(TEXT("apply_on"),58); Row->SetNumberField(TEXT("value"),17);
		Row->SetNumberField(TEXT("flag"),int32(uint32(1)<<26)); Row->SetNumberField(TEXT("duration"),60);
		Rows.Add(MakeShared<FJsonValueObject>(Row));
	}
	Root->SetArrayField(TEXT("affects"),Rows); FString OldJson;
	FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&OldJson));
	TestTrue(TEXT("Old numeric JSON loads"),State->ApplyPersistentStateJson_Implementation(OldJson));
	TArray<FMT2StatusEffect> Effects; TestTrue(TEXT("Restored effects stage before pawn creation"),State->ConsumePendingStatusEffects(Effects));
	if (!TestEqual(TEXT("Every saved identity is retained"),Effects.Num(),Ids.Num())) { return false; }
	for (int32 Index=0;Index<Ids.Num();++Index)
	{
		TestEqual(TEXT("ID restores exactly"),Effects[Index].Type.ToLegacyId(),Ids[Index]);
		TestEqual(TEXT("Apply 58 restores as stamina"),Effects[Index].ApplyType,EMT2ItemBonusType::MaxStamina);
		TestEqual(TEXT("High flag restores exactly"),Effects[Index].Flags,int32(uint32(1)<<26));
	}
	auto* Pawn=World->SpawnActor<AMT2PlayerCharacter>(); if (!TestNotNull(TEXT("Pawn"),Pawn)) { return false; }
	Pawn->SetPlayerState(State); Pawn->GetStatusEffectComponent()->RestoreEffects(Effects);
	TSharedPtr<FJsonObject> Saved;
	TestTrue(TEXT("Current save parses"),FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(State->CapturePersistentStateJson_Implementation()),Saved));
	const TArray<TSharedPtr<FJsonValue>>* SavedRows=nullptr;
	if (!TestTrue(TEXT("Current JSON retains the affects array"),Saved.IsValid() && Saved->TryGetArrayField(TEXT("affects"),SavedRows))) { return false; }
	if (!TestEqual(TEXT("No affect is lost on resave"),SavedRows->Num(),Ids.Num())) { return false; }
	for (int32 Index=0;Index<Ids.Num();++Index)
	{
		const auto Row=(*SavedRows)[Index]->AsObject();
		TestEqual(TEXT("Saved ID remains numeric and exact"),Row->GetNumberField(TEXT("type")),double(Ids[Index]));
		TestEqual(TEXT("Saved apply remains the old ordinal"),Row->GetNumberField(TEXT("apply_on")),58.0);
		TestEqual(TEXT("Saved flag remains the old mask"),Row->GetNumberField(TEXT("flag")),double(uint32(1)<<26));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2LegacySkillClassLoadTest,"Metin2.SemanticTypes.LegacySkillClassNoneRoundTrip",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2LegacySkillClassLoadTest::RunTest(const FString&)
{
	UClass* Legacy=FindObject<UClass>(nullptr,TEXT("/Script/MT2UEEditor.MT2SemanticLegacyItemFixture"));
	if (!TestNotNull(TEXT("Old integer fixture"),Legacy)) { return false; }
	for (int32 Raw : {-1,0,1,5,6,17})
	{
		TStrongObjectPtr<UObject> Old(NewObject<UObject>(GetTransientPackage(),Legacy));
		FindFProperty<FIntProperty>(Legacy,TEXT("LegacySkillType"))->SetPropertyValue_InContainer(Old.Get(),Raw);
		TArray<uint8> Bytes; FMemoryWriter Writer(Bytes,true); FObjectAndNameAsStringProxyArchive Saving(Writer,false);
		Legacy->SerializeTaggedProperties(Saving,reinterpret_cast<uint8*>(Old.Get()),Legacy,reinterpret_cast<const uint8*>(Legacy->GetDefaultObject()));
		auto* Target=NewObject<UMT2SkillDefinition>(); FMemoryReader Reader(Bytes,true); FObjectAndNameAsStringProxyArchive Loading(Reader,false);
		Target->GetClass()->SerializeTaggedProperties(Loading,reinterpret_cast<uint8*>(Target),Target->GetClass(),nullptr);
		TestFalse(TEXT("Old skill class loads"),Loading.IsError()); TestEqual(TEXT("Class sentinel and unknown values survive"),Target->LegacySkillType.ToLegacy(),Raw);
	}
	TestEqual(TEXT("Missing metadata is explicitly unspecified"),FMT2LegacySkillClassSelection::FromLegacy(-1).Type,EMT2LegacySkillClass::Unspecified);
	TestEqual(TEXT("Horse is not conflated with presentation categories"),FMT2LegacySkillClassSelection::FromLegacy(5).Type,EMT2LegacySkillClass::Horse);
	return true;
}
#endif
