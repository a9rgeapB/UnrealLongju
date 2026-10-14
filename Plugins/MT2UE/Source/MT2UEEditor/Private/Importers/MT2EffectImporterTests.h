#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMT2ImportedParticleOwnershipTest,
	"Metin2.Editor.Effects.ImportedModuleOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMT2ImportedParticleOwnershipTest::RunTest(const FString&)
{
	const TCHAR* Paths[] = {
		TEXT("/Game/ymir_work/environment/PS_skipia_dungeon.PS_skipia_dungeon"),
		TEXT("/Game/ymir_work/environment/PS_a1.PS_a1")
	};
	for (const TCHAR* Path : Paths)
	{
		UParticleSystem* System = LoadObject<UParticleSystem>(nullptr, Path);
		if (!TestNotNull(Path, System)) continue;
		TArray<UObject*> Objects;
		GetObjectsWithOuter(System, Objects, true);
		int32 InvalidModules = 0;
		for (UObject* Object : Objects)
		{
			if (const UParticleModule* Module = Cast<UParticleModule>(Object))
				InvalidModules += Module->GetOuter() != System ? 1 : 0;
		}
		TestEqual(FString::Printf(TEXT("%s modules can be saved/cooked with valid ownership"), Path), InvalidModules, 0);
	}
	return true;
}
#endif
