/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#include "Animation/AnimNotifies/AnimNotify_PlaySound.h"
#include "Animation/AnimSequence.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Combat/MT2CombatComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CapsuleComponent.h"
#include "Components/EditableTextBox.h"
#include "Components/MT2StatusEffectComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/MT2VnumRegistrySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Mobs/MT2Mob.h"
#include "Player/MT2PlayerState.h"
#include "UI/MT2ChatWidget.h"

namespace
{
	struct FMT2PolymorphPresentationWorld
	{
		TStrongObjectPtr<UGameInstance> Instance{NewObject<UGameInstance>(GEngine)};
		UWorld* World = nullptr;
		AMT2PlayerCharacter* Player = nullptr;
		AMT2PlayerState* PlayerState = nullptr;
		UMT2VnumRegistrySubsystem* Registry = nullptr;

		FMT2PolymorphPresentationWorld()
		{
			Instance->InitializeStandalone();
			World = Instance->GetWorld();
			if (!World) { return; }
			Player = World->SpawnActor<AMT2PlayerCharacter>();
			PlayerState = World->SpawnActor<AMT2PlayerState>();
			if (Player && PlayerState) { Player->SetPlayerState(PlayerState); }
			Registry = Instance->GetSubsystem<UMT2VnumRegistrySubsystem>();
		}

		~FMT2PolymorphPresentationWorld()
		{
			Instance->Shutdown();
			if (World)
			{
				World->DestroyWorld(false);
				GEngine->DestroyWorldContext(World);
			}
		}

		bool ApplyPolymorph(int32 MobVnum)
		{
			if (!Player) { return false; }
			FMT2StatusEffect Transform;
			Transform.Type = FMT2AffectIdentifier::FromBuiltin(EMT2BuiltinAffect::Polymorph);
			Transform.ApplyType = EMT2ItemBonusType::None;
			Transform.ApplyValue = MobVnum;
			Transform.RemainingSeconds = -1;
			return Player->GetStatusEffectComponent()->AddEffect(Transform);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2PolymorphAutocompleteTest,
	"Metin2.UI.PolymorphCommandAutocomplete",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMT2PolymorphAutocompleteTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>(GEngine));
	Instance->InitializeStandalone();
	UMT2ChatWidget* Widget = NewObject<UMT2ChatWidget>(Instance.Get());
	Widget->WidgetTree = NewObject<UWidgetTree>(Widget);
	UCanvasPanel* Canvas = Widget->WidgetTree->ConstructWidget<UCanvasPanel>();
	Widget->WidgetTree->RootWidget = Canvas;
	Widget->InputBox = Widget->WidgetTree->ConstructWidget<UEditableTextBox>();
	Canvas->AddChildToCanvas(Widget->InputBox);
	Widget->InputBox->SetText(FText::FromString(TEXT("/po")));
	Widget->bIsOpen = true;

	Widget->RefreshSuggestions();
	TestTrue(TEXT("Typing /po offers the /poly command"), Widget->CurrentSuggestions.Contains(TEXT("/poly")));
	TestTrue(TEXT("A matching command opens the suggestion popup"), Widget->AreSuggestionsVisible());
	Instance->Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2PolymorphGroundAlignmentTest,
	"Metin2.Items.Polymorph.GroundAlignment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMT2PolymorphGroundAlignmentTest::RunTest(const FString&)
{
	FMT2PolymorphPresentationWorld Fixture;
	if (!TestNotNull(TEXT("Player fixture"), Fixture.Player) ||
		!TestNotNull(TEXT("Mob registry fixture"), Fixture.Registry))
	{
		return false;
	}
	const TSubclassOf<AMT2Mob> MobClass = Fixture.Registry->ResolveMobClass(101);
	const AMT2Mob* MobDefaults = MobClass ? MobClass->GetDefaultObject<AMT2Mob>() : nullptr;
	const USkeletalMeshComponent* MobMesh = MobDefaults ? MobDefaults->GetMesh() : nullptr;
	if (!TestNotNull(TEXT("Imported mob defaults"), MobDefaults) ||
		!TestNotNull(TEXT("Imported mob mesh"), MobMesh))
	{
		return false;
	}
	const float MobHalfHeight = MobDefaults->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float PlayerHalfHeight = Fixture.Player->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	if (!TestTrue(TEXT("Fixture mob uses a different capsule height from the player"),
		!FMath::IsNearlyEqual(MobHalfHeight, PlayerHalfHeight)))
	{
		return false;
	}
	if (!Fixture.ApplyPolymorph(101)) { return false; }

	Fixture.Player->RefreshPolymorphAppearance();
	const float ExpectedMeshZ = MobMesh->GetRelativeLocation().Z + MobHalfHeight - PlayerHalfHeight;
	TestTrue(TEXT("Transformed mesh keeps its feet aligned with the player's capsule base"),
		FMath::IsNearlyEqual(Fixture.Player->GetMesh()->GetRelativeLocation().Z, ExpectedMeshZ, 0.1f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2PolymorphAttackMotionTest,
	"Metin2.Items.Polymorph.AttackUsesMobMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMT2PolymorphAttackMotionTest::RunTest(const FString&)
{
	FMT2PolymorphPresentationWorld Fixture;
	if (!TestNotNull(TEXT("Player fixture"), Fixture.Player) ||
		!TestNotNull(TEXT("Mob registry fixture"), Fixture.Registry))
	{
		return false;
	}
	const TSubclassOf<AMT2Mob> MobClass = Fixture.Registry->ResolveMobClass(101);
	const AMT2Mob* MobDefaults = MobClass ? MobClass->GetDefaultObject<AMT2Mob>() : nullptr;
	UAnimSequence* ExpectedMobAttack = MobDefaults
		? MobDefaults->ResolveMotionAnimation(EMT2MobMotion::NormalAttack, 0) : nullptr;
	if (!TestNotNull(TEXT("Imported mob attack sequence"), ExpectedMobAttack) ||
		!Fixture.ApplyPolymorph(101))
	{
		return false;
	}

	Fixture.Player->HandleBasicAttackPerformed(0);
	TestEqual(TEXT("Polymorphed player drives combat timing from the mob's attack sequence"),
		Fixture.Player->GetCombatComponent()->BasicAttackMotion.Get(), ExpectedMobAttack);
	const bool bHasMobMotionSound = ExpectedMobAttack->Notifies.ContainsByPredicate(
		[](const FAnimNotifyEvent& Notify)
		{
			const UAnimNotify_PlaySound* PlaySound = Cast<UAnimNotify_PlaySound>(Notify.Notify);
			return PlaySound && PlaySound->Sound;
		});
	TestTrue(TEXT("Selected mob attack sequence retains its mob-specific motion sound"), bHasMobMotionSound);
	return true;
}
