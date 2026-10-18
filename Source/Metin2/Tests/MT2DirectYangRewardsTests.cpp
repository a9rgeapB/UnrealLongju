#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Mobs/MT2Mob.h"
#include "Mobs/MT2MobLootComponent.h"
#include "Mobs/MT2MobTypes.h"
#include "Player/MT2PlayerState.h"
#include "Party/MT2Party.h"
#include "Items/MT2WorldItem.h"
#include "Config/MT2GameplaySettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2DirectYangRewardsTest, "Metin2.Loot.DirectYangRewards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2DirectYangRewardsTest::RunTest(const FString&)
{
	TGuardValue<float> Multiplier(GetMutableDefault<UMT2GameplaySettings>()->MobGoldAmountMultiplier, 1.f);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("World"), World)) return false;
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	auto* First = World->SpawnActor<AMT2PlayerState>(); First->SetPlayerId(1);
	auto* Second = World->SpawnActor<AMT2PlayerState>(); Second->SetPlayerId(2);
	FMT2MobDefinition Definition; Definition.GoldMin = 101; Definition.GoldMax = 101;
	auto MakeLoot = [&](const FVector& Location = FVector::ZeroVector)
	{
		auto* Victim = World->SpawnActor<AMT2Mob>();
		Victim->SetActorLocation(Location);
		auto* Loot = Victim->GetLootComponent(); Loot->Configure(Definition);
		return Loot;
	};
	FMT2DamageShare FirstDamage; FirstDamage.Attacker = First; FirstDamage.TotalDamage = 100.f;
	FMT2DamageShare SecondDamage; SecondDamage.Attacker = Second; SecondDamage.TotalDamage = 1.f;
	TArray<FMT2DamageShare> Shares = { FirstDamage, SecondDamage };
	auto* SoloLoot = MakeLoot(); SoloLoot->GenerateRewards(Shares);
	TestEqual(TEXT("Solo winner receives Yang immediately"), First->GetYang(), int64(101));
	TestEqual(TEXT("Other attacker receives no winner's Yang"), Second->GetYang(), int64(0));
	SoloLoot->GenerateRewards(Shares);
	TestEqual(TEXT("Repeated reward cannot duplicate Yang"), First->GetYang(), int64(101));
	auto* Party = World->SpawnActor<AMT2Party>();
	if (!TestTrue(TEXT("Party initialized"), Party->InitializeParty(First, Second))) return false;
	MakeLoot()->GenerateRewards(Shares);
	TestEqual(TEXT("Yang is credited to the individual top damager"), First->GetYang(), int64(202));
	TestEqual(TEXT("Yang is not split across the top damager's party"), Second->GetYang(), int64(0));
	auto* Third = World->SpawnActor<AMT2PlayerState>(); Third->SetPlayerId(3);
	FMT2DamageShare FirstPartyDamage; FirstPartyDamage.Attacker = First; FirstPartyDamage.TotalDamage = 40.f;
	FMT2DamageShare SecondPartyDamage; SecondPartyDamage.Attacker = Second; SecondPartyDamage.TotalDamage = 40.f;
	FMT2DamageShare ThirdDamage; ThirdDamage.Attacker = Third; ThirdDamage.TotalDamage = 60.f;
	TArray<FMT2DamageShare> CompetingShares = { FirstPartyDamage, SecondPartyDamage, ThirdDamage };
	MakeLoot(FVector(0.0f, 5200.0f, 9000.0f))->GenerateRewards(CompetingShares);
	TestEqual(TEXT("Legacy XY approximate range includes the individual top damager despite height and diagonal distance"),
		Third->GetYang(), int64(101));
	TestEqual(TEXT("Higher-damage party does not take the individual killer's Yang"),
		First->GetYang() + Second->GetYang(), int64(202));
	MakeLoot()->GenerateRewards(Shares, false);
	TestEqual(TEXT("Disabled drops cannot grant Yang"), First->GetYang() + Second->GetYang(), int64(202));
	int32 Drops = 0; for (TActorIterator<AMT2WorldItem> It(World); It; ++It) ++Drops;
	TestEqual(TEXT("Yang awards create no world items"), Drops, 0);
	return true;
}
#endif
