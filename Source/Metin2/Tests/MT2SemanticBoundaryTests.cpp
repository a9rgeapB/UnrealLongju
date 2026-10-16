#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Player/MT2PlayerState.h"
#include "Components/MT2StatusEffectComponent.h"
#include "Quests/MT2QuestNode.h"
#include "Skills/MT2SkillSemanticTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2InvalidApplyBoundaryTest,"Metin2.SemanticTypes.InvalidApplyIdsDoNotAliasKnownBonuses",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2InvalidApplyBoundaryTest::RunTest(const FString&)
{
	UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
	if (!TestNotNull(TEXT("World"),World)) { return false; }
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	auto* Pawn=World->SpawnActor<AMT2PlayerCharacter>(); auto* State=World->SpawnActor<AMT2PlayerState>();
	if (!TestNotNull(TEXT("Pawn"),Pawn) || !TestNotNull(TEXT("State"),State)) { return false; }
	for (int32 Raw : {-1,92,264,MAX_int32})
	{
		auto* Node=NewObject<UMT2QuestNode_ModifyAffect>();
		Node->Operation=EMT2QuestAffectOperation::AddCollect;
		Node->ApplyTypeExpression=LexToString(Raw); Node->Value=50; Node->DurationSeconds=60;
		FMT2QuestContext Context; Context.Player=Pawn;
		Node->Execute(Context);
		TestEqual(TEXT("A raw script ID cannot wrap into a known byte enum bonus"),Pawn->GetStatusEffectComponent()->GetEffects().Num(),0);
		Pawn->GetStatusEffectComponent()->ClearAllEffects();
		const FString Json=FString::Printf(TEXT("{\"version\":5,\"affects\":[{\"type\":30001,\"apply_on\":%d,\"value\":50,\"duration\":60}]}"),Raw);
		TestTrue(TEXT("An unsupported affect does not corrupt the rest of the save"),State->ApplyPersistentStateJson_Implementation(Json));
		TArray<FMT2StatusEffect> Staged; State->ConsumePendingStatusEffects(Staged);
		TestEqual(TEXT("Unsupported saved applies are rejected, not reinterpreted"),Staged.Num(),0);
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2SemanticNumericTextTest,"Metin2.SemanticTypes.LegacyNumericTextRejectsOverflow",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMT2SemanticNumericTextTest::RunTest(const FString&)
{
	for (const TCHAR* Text : {TEXT("2147483648"),TEXT("-2147483649"),TEXT("4294967295"),TEXT("99999999999999999999999")})
	{
		const TCHAR* Cursor=Text; FMT2ItemAddonSelection Addon;
		TestFalse(TEXT("Addon text cannot overflow into signed sentinel"),Addon.ImportTextItem(Cursor,0,nullptr,nullptr));
		TestTrue(TEXT("Rejected text is not consumed"),Cursor==Text);
		Cursor=Text; FMT2AffectIdentifier Affect;
		TestFalse(TEXT("Affect text cannot truncate an ID"),Affect.ImportTextItem(Cursor,0,nullptr,nullptr));
		Cursor=Text; FMT2LegacySkillClassSelection Skill;
		TestFalse(TEXT("Skill-class text cannot wrap into a known class"),Skill.ImportTextItem(Cursor,0,nullptr,nullptr));
	}
	return true;
}
#endif
