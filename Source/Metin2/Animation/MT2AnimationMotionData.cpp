#include "Animation/MT2AnimationMotionData.h"
#include "String/LexFromString.h"

namespace
{
	bool ReadHittingType(const FString& Token,EMT2MotionHittingType& Out)
	{
		int32 Raw=0;
		if (!LexTryParseString(Raw,*Token)) { return false; }
		if (Raw<0 || Raw>2)
		{
			UE_LOG(LogTemp,Warning,TEXT("Unsupported motion HittingType %d; retaining the preceding hit kind."),Raw);
			return false;
		}
		Out=static_cast<EMT2MotionHittingType>(Raw); return true;
	}
}

void UMT2AnimationMotionData::ReadAttackEvents(const FString& Script, TArray<FMT2MotionAttackEvent>& OutEvents)
{
	struct FScope
	{
		FString Name;
		FMT2MotionAttackEvent Event;
		int32 EventType = -1;
		float Start = -1.f, End = -1.f, During = 0.f;
		bool bNormal = false;
	};
	OutEvents.Reset();
	TArray<FScope> Scopes;
	Scopes.AddDefaulted();
	FString PendingName;
	TArray<FString> Lines; Script.ParseIntoArrayLines(Lines);
	for (FString Line : Lines)
	{
		Line.TrimStartAndEndInline();
		TArray<FString> Tokens; Line.ParseIntoArrayWS(Tokens);
		if (Tokens.IsEmpty()) { continue; }
		if (Tokens[0] == TEXT("Group") || Tokens[0] == TEXT("List"))
		{
			PendingName = Tokens.Num() > 1 ? Tokens[1] : FString();
			continue;
		}
		if (Tokens[0] == TEXT("{"))
		{
			FScope Scope; Scope.Name = PendingName; PendingName.Reset();
			Scope.bNormal = Scope.Name == TEXT("AttackingData") || Scope.Name.StartsWith(TEXT("HitData"));
			if (Scope.bNormal) { Scope.Event = Scopes.Last().Event; }
			Scopes.Add(MoveTemp(Scope));
			continue;
		}
		if (Tokens[0] == TEXT("}"))
		{
			if (Scopes.Num() <= 1) { continue; }
			const FScope Scope = Scopes.Pop();
			if (Scope.Event.HittingType != EMT2MotionHittingType::None && Scope.Start >= 0.f &&
				(Scope.EventType == 4 || (Scope.bNormal && Scope.End > Scope.Start)))
			{
				FMT2MotionAttackEvent Event = Scope.Event;
				Event.TimeSeconds = Scope.Start;
				Event.EndTimeSeconds = Scope.EventType == 4 ? Scope.Start + Scope.During : Scope.End;
				OutEvents.Add(Event);
			}
			continue;
		}
		if (Tokens.Num() < 2) { continue; }
		FScope& Scope = Scopes.Last();
		float Value = 0.f;
		if (!LexTryParseString(Value, *Tokens[1]) || !FMath::IsFinite(Value)) { continue; }
		const FString& Key = Tokens[0];
		if (Key == TEXT("HittingType")) { ReadHittingType(Tokens[1],Scope.Event.HittingType); }
		else if (Key == TEXT("ExternalForce")) { Scope.Event.ExternalForce = FMath::Max(Value, 0.f); }
		else if (Key == TEXT("MotionEventType")) { LexTryParseString(Scope.EventType, *Tokens[1]); }
		else if (Key == TEXT("StartingTime") || Key == TEXT("AttackingStartTime")) { Scope.Start = Value; }
		else if (Key == TEXT("AttackingEndTime")) { Scope.End = Value; }
		else if (Key == TEXT("DuringTime")) { Scope.During = FMath::Max(Value, 0.f); }
	}
	OutEvents.Sort([](const FMT2MotionAttackEvent& A, const FMT2MotionAttackEvent& B) { return A.TimeSeconds < B.TimeSeconds; });
}

void UMT2AnimationMotionData::ReadKnockbackMetadata(const FString& Script, float& OutForce, EMT2MotionHittingType& OutHittingType)
{
	OutForce = 0; OutHittingType = EMT2MotionHittingType::None;
	EMT2MotionHittingType CurrentHitType = EMT2MotionHittingType::None;
	TArray<EMT2MotionHittingType> HitTypeStack;
	TArray<FString> Lines; Script.ParseIntoArrayLines(Lines);
	for (FString Line : Lines)
	{
		Line.TrimStartAndEndInline();
		if (Line == TEXT("{")) { HitTypeStack.Add(CurrentHitType); continue; }
		if (Line == TEXT("}")) { CurrentHitType = HitTypeStack.IsEmpty() ? EMT2MotionHittingType::None : HitTypeStack.Pop(); continue; }
		TArray<FString> Tokens; Line.ParseIntoArrayWS(Tokens);
		if (Tokens.Num() < 2) { continue; }
		if (Tokens[0] == TEXT("HittingType")) { ReadHittingType(Tokens[1],CurrentHitType); }
		else if (Tokens[0] == TEXT("ExternalForce"))
		{
			float Force;
			if (LexTryParseString(Force, *Tokens[1]) && FMath::IsFinite(Force) && Force >= 0 && CurrentHitType != EMT2MotionHittingType::None &&
				(Force > OutForce || OutHittingType == EMT2MotionHittingType::None))
			{ OutForce = Force; OutHittingType = CurrentHitType; }
		}
	}
}
