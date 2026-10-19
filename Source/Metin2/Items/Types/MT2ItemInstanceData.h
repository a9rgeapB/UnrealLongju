/*

- Copyright © 2026 Castellese Brian Vincenzo.
- Licensed under the PolyForm Noncommercial License 1.0.0.
- Commercial use and resale require separate written permission.
- See LICENSE for the complete terms.

*/

#pragma once
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "Items/State/MT2ItemFamilyState.h"
#include "Items/State/MT2AutoRecoveryItemState.h"
#include "Items/State/MT2FishingRodItemState.h"
#include "Items/State/MT2PickaxeItemState.h"
#include "Items/State/MT2FishItemState.h"
#include "Items/State/MT2BlendItemState.h"
#include "Items/State/MT2SkillBookItemState.h"
#include "Items/State/MT2PolymorphItemState.h"
#include "Items/Types/MT2ItemBonusTypes.h"
#include "Items/Types/MT2MetinSocketTypes.h"
#include "Items/Types/MT2AccessorySocketState.h"
#include "MT2ItemInstanceData.generated.h"

class UMT2ItemTemplate;
class UMT2SkillDefinition;
struct FMT2AutoRecoveryItemState;
struct FMT2FishingRodItemState;
struct FMT2PickaxeItemState;
struct FMT2FishItemState;
struct FMT2BlendItemState;
struct FMT2SkillBookItemState;
struct FMT2PolymorphItemState;

USTRUCT(BlueprintType)
struct METIN2_API FMT2ItemInstanceData
{
	GENERATED_BODY()
	bool Serialize(FArchive& Ar);
	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Owner, FOutputDevice* Errors);
	// Only the actual family's named state exists. Raw legacy values are confined to codecs.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item|State", meta=(BaseStruct="/Script/Metin2.MT2ItemFamilyState"))
	FInstancedStruct FamilyState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Expiry")
	int64 RealtimeExpiresAt = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "0"))
	int32 Count = 1;

	template<typename T> const T* FindState() const { return FamilyState.GetPtr<T>(); }
	template<typename T> T* FindMutableState() { return FamilyState.GetMutablePtr<T>(); }
	template<typename T> const T& ReadState() const
	{
		if (const T* State = FindState<T>()) return *State;
		static const T Empty;
		return Empty;
	}
	template<typename T> T& EditState()
	{
		if (!FamilyState.IsValid()) FamilyState.InitializeAs<T>();
		checkf(FamilyState.GetPtr<T>(), TEXT("Editing the wrong item-family state"));
		return FamilyState.GetMutable<T>();
	}
	const TArray<FMT2ItemBonus>& GetBonuses() const;
	bool SupportsBonuses() const;
	TArray<FMT2ItemBonus>& EditBonuses();
	const TArray<FMT2MetinSocket>& GetMetinSockets() const;
	TArray<FMT2MetinSocket>& EditMetinSockets();
	const FMT2AccessorySocketState& GetAccessorySockets() const;
	FMT2AccessorySocketState& EditAccessorySockets();
	FMT2AutoRecoveryItemState GetAutoRecoveryState() const;
	FMT2AutoRecoveryItemState& EditAutoRecoveryState();
	const FMT2FishingRodItemState& GetRodState() const;
	FMT2FishingRodItemState& EditRodState();
	const FMT2PickaxeItemState& GetPickaxeState() const;
	FMT2PickaxeItemState& EditPickaxeState();
	const FMT2FishItemState& GetFishState() const;
	FMT2FishItemState& EditFishState();
	const FMT2BlendItemState& GetBlendState() const;
	FMT2BlendItemState& EditBlendState();
	const FMT2SkillBookItemState& GetSkillBookState() const;
	FMT2SkillBookItemState& EditSkillBookState();
	const FMT2PolymorphItemState& GetPolymorphState() const;
	FMT2PolymorphItemState& EditPolymorphState();
	int32 GetSkillVnum() const;
	void SetSkillVnum(int32 Vnum);

	// Transient gameplay marker: set only when an item is freshly received and cleared when its
	// tooltip is first shown. Persistence intentionally does not serialize it.
	UPROPERTY(BlueprintReadWrite, Category = "Item|Presentation")
	bool bNewlyAcquired = false;
};

USTRUCT(BlueprintType)
struct METIN2_API FMT2ItemSlot : public FMT2ItemInstanceData
{
	GENERATED_BODY()
	bool Serialize(FArchive& Ar);
	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Owner, FOutputDevice* Errors);

	FMT2ItemSlot() = default;
	FMT2ItemSlot(int32 InVnum, const FMT2ItemInstanceData& InInstanceData)
		: FMT2ItemInstanceData(InInstanceData), Vnum(InVnum)
	{
	}

	// Internal legacy identity retained for transport, imported ranges and unresolved saves.
	UPROPERTY()
	int32 Vnum = 0;
	UPROPERTY(BlueprintReadOnly, Category="Item")
	TSubclassOf<UMT2ItemTemplate> Template;

	bool IsEmpty() const { return Vnum <= 0 || Count <= 0; }
};

template<> struct TStructOpsTypeTraits<FMT2ItemInstanceData> : TStructOpsTypeTraitsBase2<FMT2ItemInstanceData>
{
	enum { WithSerializer = true, WithImportTextItem = true };
};
template<> struct TStructOpsTypeTraits<FMT2ItemSlot> : TStructOpsTypeTraitsBase2<FMT2ItemSlot>
{
	enum { WithSerializer = true, WithImportTextItem = true };
};

