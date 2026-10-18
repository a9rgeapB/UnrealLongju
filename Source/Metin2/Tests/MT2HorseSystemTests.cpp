#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Player/MT2PlayerState.h"
#include "Mounts/MT2HorseProgressionComponent.h"
#include "Config/MT2GameplaySettings.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace MT2HorseTests
{
	struct FRig
	{
		TStrongObjectPtr<UGameInstance> Instance{NewObject<UGameInstance>(GEngine)};
		UWorld* World = nullptr;
		UWorld* OriginalWorld = GWorld;
		FRig() { Instance->InitializeStandalone(); World = Instance->GetWorld(); World->InitializeActorsForPlay(FURL()); World->GetWorldSettings()->NotifyBeginPlay(); }
		~FRig() { World->EndPlay(EEndPlayReason::Quit); Instance->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); GWorld = OriginalWorld; }
		AMT2PlayerState* Player() { auto* Controller = World->SpawnActor<APlayerController>(); auto* State = World->SpawnActor<AMT2PlayerState>(); Controller->SetPlayerState(State); auto* Pawn = World->SpawnActor<APawn>(); if (!Pawn->GetRootComponent()) { auto* Root = NewObject<USceneComponent>(Pawn); Pawn->SetRootComponent(Root); Root->RegisterComponent(); } Controller->Possess(Pawn); return State; }
	};
	static TSharedPtr<FJsonObject> Parse(const FString& Json)
	{
		TSharedPtr<FJsonObject> Result;
		FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Result);
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2GameplayProgressionSettingsTest,
	"Metin2.GameplaySettings.ProgressionIsConfigurable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMT2GameplayProgressionSettingsTest::RunTest(const FString&)
{
	const FIntProperty* CharacterMaximumProperty = FindFProperty<FIntProperty>(
		UMT2GameplaySettings::StaticClass(), TEXT("MaximumCharacterLevel"));
	const FIntProperty* HorseMaximumProperty = FindFProperty<FIntProperty>(
		UMT2GameplaySettings::StaticClass(), TEXT("MaximumHorseLevel"));
	TestNotNull(TEXT("Character maximum level is an exposed Project Settings property"), CharacterMaximumProperty);
	TestNotNull(TEXT("Horse maximum level is an exposed Project Settings property"), HorseMaximumProperty);
	if (!CharacterMaximumProperty || !HorseMaximumProperty)
	{
		return false;
	}

	TestTrue(TEXT("Character level cap is stored in config"), CharacterMaximumProperty->HasAnyPropertyFlags(CPF_Config));
	TestTrue(TEXT("Horse level cap is stored in config"), HorseMaximumProperty->HasAnyPropertyFlags(CPF_Config));

	UMT2GameplaySettings* Settings = GetMutableDefault<UMT2GameplaySettings>();
	TestEqual(TEXT("Legacy horse table is available from project config"), Settings->HorseLevelSettings.Num(), 30);
	TestEqual(TEXT("Legacy horse riding drain interval is configurable"), Settings->HorseStaminaDrainIntervalSeconds, 360);
	TestEqual(TEXT("Legacy horse offline recovery interval is configurable"), Settings->HorseStaminaRegenerationIntervalSeconds, 720);
	TestEqual(TEXT("Legacy horse health decay interval is configurable"), Settings->HorseHealthDecayIntervalSeconds, 259200);
	if (Settings->HorseLevelSettings.Num() == 30)
	{
		TestEqual(TEXT("Horse level 1 health comes from project config"), Settings->HorseLevelSettings[0].MaximumHealth, 3);
		TestEqual(TEXT("Horse level 21 rider minimum comes from project config"), Settings->HorseLevelSettings[20].MinimumRiderLevel, 50);
	}
	const int32 OriginalCharacterMaximum = CharacterMaximumProperty->GetPropertyValue_InContainer(Settings);
	const int32 OriginalHorseMaximum = HorseMaximumProperty->GetPropertyValue_InContainer(Settings);
	CharacterMaximumProperty->SetPropertyValue_InContainer(Settings, 42);
	HorseMaximumProperty->SetPropertyValue_InContainer(Settings, 12);

	MT2HorseTests::FRig Rig;
	AMT2PlayerState* State = Rig.Player();
	TestEqual(TEXT("Configured character cap overrides the XP curve maximum"), State->GetMaximumCharacterLevel(), 42);
	TestTrue(TEXT("Setting a level above the configured character cap is accepted as a capped level"), State->SetCharacterLevel(50));
	TestEqual(TEXT("Character progression clamps to the configured cap"), State->GetCharacterLevel(), 42);
	TestTrue(TEXT("Configured horse cap controls progression"), State->GetHorseProgressionComponent()->SetLevel(100));
	TestEqual(TEXT("Horse progression clamps to configured maximum"), State->GetHorseProgressionComponent()->GetLevel(), 12);

	CharacterMaximumProperty->SetPropertyValue_InContainer(Settings, OriginalCharacterMaximum);
	HorseMaximumProperty->SetPropertyValue_InContainer(Settings, OriginalHorseMaximum);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2HorseProgressionPersistenceTest,
	"Metin2.Horse.Progression.PersistenceRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMT2HorseProgressionPersistenceTest::RunTest(const FString&)
{
	MT2HorseTests::FRig Rig;
	AMT2PlayerState* State = Rig.Player();
	TestNotNull(TEXT("Player state created"), State);
	if (!State)
	{
		return false;
	}

	const FString InitialJson = State->CapturePersistentStateJson_Implementation();
	TSharedPtr<FJsonObject> Initial = MT2HorseTests::Parse(InitialJson);
	TestTrue(TEXT("Initial character snapshot parses"), Initial.IsValid());
	if (!Initial.IsValid())
	{
		return false;
	}
	TestTrue(TEXT("New character snapshot includes horse progression"),
		Initial->HasTypedField<EJson::Object>(TEXT("horse")));
	TestEqual(TEXT("Horse starts unowned"), State->GetHorseProgressionComponent()->GetLevel(), 0);
	TestEqual(TEXT("Horse grade starts at zero"), State->GetHorseProgressionComponent()->GetGrade(), 0);
	const FString MalformedHorseJson = TEXT("{\"version\":5,\"horse\":{\"level\":15}} ");
	TestTrue(TEXT("Character snapshot with incomplete optional horse fields loads"),
		State->ApplyPersistentStateJson_Implementation(MalformedHorseJson));
	TestEqual(TEXT("Incomplete horse data does not create/overwrite a horse"),
		State->GetHorseProgressionComponent()->GetLevel(), 0);

	Initial->SetObjectField(TEXT("horse"), MakeShared<FJsonObject>());
	TSharedPtr<FJsonObject> Horse = Initial->GetObjectField(TEXT("horse"));
	Horse->SetNumberField(TEXT("level"), 21.0);
	Horse->SetNumberField(TEXT("health"), 35.0);
	Horse->SetNumberField(TEXT("stamina"), 120.0);

	FString InputJson;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&InputJson);
	TestTrue(TEXT("Horse snapshot serializes"), FJsonSerializer::Serialize(Initial.ToSharedRef(), Writer));
	TestTrue(TEXT("Horse snapshot applies"), State->ApplyPersistentStateJson_Implementation(InputJson));

	const TSharedPtr<FJsonObject> Saved = MT2HorseTests::Parse(State->CapturePersistentStateJson_Implementation());
	TestTrue(TEXT("Saved character snapshot includes horse progression"),
		Saved.IsValid() && Saved->HasTypedField<EJson::Object>(TEXT("horse")));
	if (Saved.IsValid() && Saved->HasTypedField<EJson::Object>(TEXT("horse")))
	{
		const TSharedPtr<FJsonObject> SavedHorse = Saved->GetObjectField(TEXT("horse"));
		TestEqual(TEXT("Horse level round-trips"), SavedHorse->GetIntegerField(TEXT("level")), 21);
		TestEqual(TEXT("Horse grade is derived from level"), SavedHorse->GetIntegerField(TEXT("grade")), 3);
		TestEqual(TEXT("Horse health round-trips"), SavedHorse->GetIntegerField(TEXT("health")), 35);
		TestEqual(TEXT("Horse stamina round-trips"), SavedHorse->GetIntegerField(TEXT("stamina")), 120);
	}
	TestTrue(TEXT("Horse level clamps to legacy maximum"), State->GetHorseProgressionComponent()->SetLevel(100));
	TestEqual(TEXT("Level clamps to 30"), State->GetHorseProgressionComponent()->GetLevel(), 30);
	TestEqual(TEXT("Grade 3 permission boundary starts at level 21"), State->GetHorseProgressionComponent()->GetGrade(), 3);
	TestTrue(TEXT("Legacy grade 2 boundary can be set"), State->GetHorseProgressionComponent()->SetLevel(11));
	TestEqual(TEXT("Level 11 is armed horse grade"), State->GetHorseProgressionComponent()->GetGrade(), 2);
	TestTrue(TEXT("Legacy basic-grade boundary can be set"), State->GetHorseProgressionComponent()->SetLevel(10));
	TestEqual(TEXT("Level 10 remains basic horse grade"), State->GetHorseProgressionComponent()->GetGrade(), 1);
	State->GetHorseProgressionComponent()->RestoreProgression(10, 10, 0, 0,
		FDateTime::UtcNow().ToUnixTimestamp() - 3600);
	TestEqual(TEXT("Offline recovery grants one stamina per twelve minutes"),
		State->GetHorseProgressionComponent()->GetStamina(), 5);
	State->GetHorseProgressionComponent()->RestoreProgression(10, 0, 0, 0, 0);
	TestTrue(TEXT("Horse with zero saved health is dead"), State->GetHorseProgressionComponent()->IsDead());
	TestTrue(TEXT("Dead horse is revived"), State->GetHorseProgressionComponent()->Revive());
	TestEqual(TEXT("Revive restores legacy full health"), State->GetHorseProgressionComponent()->GetHealth(), 15);
	TestEqual(TEXT("Revive restores legacy full stamina"), State->GetHorseProgressionComponent()->GetStamina(), 10);
	TestTrue(TEXT("Living horse can be fed"), State->GetHorseProgressionComponent()->Feed());
	TestTrue(TEXT("Horse level zero removes horse ownership"), State->GetHorseProgressionComponent()->SetLevel(0));
	TestFalse(TEXT("Horse without ownership cannot be revived"), State->GetHorseProgressionComponent()->Revive());

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
