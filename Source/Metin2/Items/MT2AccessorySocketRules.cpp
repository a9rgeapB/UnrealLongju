#include "Items/MT2AccessorySocketRules.h"
#include "Items/Templates/MT2ItemArmorTemplate.h"
#include "Items/Templates/MT2ItemBeltTemplate.h"
#include "Items/Templates/MT2ItemUseTemplate.h"
#include "Items/MT2ItemTemplate.h"
bool MT2AccessorySockets::IsAccessory(const UMT2ItemTemplate* Item)
{
	const auto* Armor = Cast<UMT2ItemArmorTemplate>(Item);
	return Cast<UMT2ItemBeltTemplate>(Item) || (Armor &&
		(Armor->ArmorSubType == EMT2ArmorSubType::Wrist || Armor->ArmorSubType == EMT2ArmorSubType::Neck || Armor->ArmorSubType == EMT2ArmorSubType::Ear));
}

bool MT2AccessorySockets::IsMaterial(const UMT2ItemTemplate* Item)
{
	const auto* Use = Cast<UMT2ItemUseTemplate>(Item);
	return Use && (Use->UseSubType == EMT2UseSubType::AddAccessorySocket || Use->UseSubType == EMT2UseSubType::PutIntoAccessorySocket || Use->UseSubType == EMT2UseSubType::PutIntoBeltSocket);
}

int32 MT2AccessorySockets::RequiredMineral(int32 Vnum, EMT2ArmorSubType ArmorSubType)
{
	const int32 Column = ArmorSubType == EMT2ArmorSubType::Wrist ? 0 : ArmorSubType == EMT2ArmorSubType::Neck ? 1 : ArmorSubType == EMT2ArmorSubType::Ear ? 2 : INDEX_NONE;
	if (Column == INDEX_NONE) { return 0; }
	static constexpr int32 Special[][3] = {{14420,16220,17220},{14500,16500,17500},
		{14520,16520,17520},{14540,16540,17540},{14560,16560,17560}};
	for (int32 Row = 0; Row < UE_ARRAY_COUNT(Special); ++Row)
	{
		if (Vnum / 10 * 10 == Special[Row][Column]) { return 50634 + Row; }
	}
	const int32 Offset = Vnum - (Column == 0 ? 14000 : Column == 1 ? 16000 : 17000);
	if (Offset < 0 || Offset >= 1000) { return 0; }
	int32 Type = Offset / 20;
	if (Type > 11) { Type = (Offset - 170) / 20; }
	else if (Vnum >= 16210 && Vnum <= 16219) { return 50625; }
	else if (Vnum >= 16230 && Vnum <= 16239) { return 50626; }
	const int32 Material = 50623 + Type;
	return Material <= 50638 ? Material : 0;
}

bool MT2AccessorySockets::Compatible(const UMT2ItemTemplate* Material, const UMT2ItemTemplate* Target)
{
	const auto* Use = Cast<UMT2ItemUseTemplate>(Material);
	if (!Use || !IsAccessory(Target)) { return false; }
	if (Use->UseSubType == EMT2UseSubType::AddAccessorySocket) { return true; }
	if (Cast<UMT2ItemBeltTemplate>(Target)) { return Use->UseSubType == EMT2UseSubType::PutIntoBeltSocket; }
	const auto* Armor = Cast<UMT2ItemArmorTemplate>(Target);
	return Use->UseSubType == EMT2UseSubType::PutIntoAccessorySocket && Material->Vnum == RequiredMineral(Target->Vnum, Armor->ArmorSubType);
}

int32 MT2AccessorySockets::Bonus(int32 BaseValue, int32 Grade)
{
	static constexpr int32 Percent[] = {0,10,20,40};
	Grade = FMath::Clamp(Grade, 0, 3);
	return Grade ? static_cast<int32>(FMath::Max<int64>(Grade, int64(BaseValue) * Percent[Grade] / 100)) : 0;
}

int32 MT2AccessorySockets::Duration(int32 Grade)
{
	static constexpr int32 Seconds[] = {0,86400,43200,21600};
	return Seconds[FMath::Clamp(Grade, 0, 3)];
}

int32 MT2AccessorySockets::InsertionChance(int32 Grade)
{
	return Grade >= 0 && Grade < 3 ? 90 - Grade * 10 : 0;
}

FMT2AccessorySocketState MT2AccessorySockets::Normalize(FMT2AccessorySocketState State)
{
	State.MaximumGrade = FMath::Clamp(State.MaximumGrade, 0, 3);
	if (State.CurrentGrade < 0 || State.CurrentGrade > State.MaximumGrade || State.RemainingSeconds < 0) { State.CurrentGrade = 0; }
	State.RemainingSeconds = FMath::Clamp(State.RemainingSeconds, 0, Duration(State.CurrentGrade));
	State.WearFraction = State.CurrentGrade && FMath::IsFinite(State.WearFraction) ? FMath::Clamp(State.WearFraction, 0.0, 0.999999) : 0;
	return State;
}

FMT2AccessorySocketState MT2AccessorySockets::Advance(FMT2AccessorySocketState State, double Elapsed)
{
	State = Normalize(State);
	if (!FMath::IsFinite(Elapsed) || Elapsed < 0) { return State; }
	Elapsed += State.WearFraction;
	while (State.CurrentGrade > 0 && Elapsed >= State.RemainingSeconds)
	{
		Elapsed -= State.RemainingSeconds;
		State.RemainingSeconds = Duration(--State.CurrentGrade);
	}
	State.RemainingSeconds -= FMath::Min(State.RemainingSeconds, static_cast<int32>(FMath::Min(Elapsed, double(MAX_int32))));
	State.WearFraction = State.CurrentGrade ? FMath::Frac(Elapsed) : 0;
	return State;
}
