#include "Mining/MT2MiningComponent.h"
#include "Items/Templates/MT2ItemPickTemplate.h"
#include "Mining/MT2MiningRules.h"
#include "Characters/MT2PlayerCharacter.h"
#include "Components/MT2HealthComponent.h"
#include "Components/MT2StatusEffectComponent.h"
#include "Combat/MT2CombatComponent.h"
#include "Fishing/MT2FishingComponent.h"
#include "Items/MT2InventoryComponent.h"
#include "Items/MT2ItemTemplate.h"
#include "Items/MT2WorldItem.h"
#include "Core/MT2VnumRegistrySubsystem.h"
#include "Mounts/MT2MountComponent.h"
#include "Npcs/MT2Npc.h"
#include "Npcs/MT2NpcInteractionComponent.h"
#include "Player/MT2PlayerController.h"
#include "Player/MT2PlayerState.h"
#include "Quests/MT2QuestManagerComponent.h"
#include "Skills/MT2SkillComponent.h"
#include "Abilities/MT2GameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Animation/MT2CharacterAnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

UMT2MiningComponent::UMT2MiningComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
	Random.GenerateNewSeed();
}
void UMT2MiningComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMT2MiningComponent, State);
}
void UMT2MiningComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (auto* Player=Cast<AMT2PlayerCharacter>(GetOwner()))
	{
		Player->GetInventoryComponent()->OnEquipmentChanged.RemoveDynamic(this, &UMT2MiningComponent::OnEquipmentChanged);
		Player->GetStatusEffectComponent()->OnStatusEffectsChanged.RemoveDynamic(this, &UMT2MiningComponent::CheckAttempt);
		Player->GetHealthComponent()->OnValueChanged.RemoveDynamic(this, &UMT2MiningComponent::OnHealthChanged);
	}
	StopPresentation();
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearAllTimersForObject(this); }
	Super::EndPlay(Reason);
}
const UMT2ItemPickTemplate* UMT2MiningComponent::EquippedPick() const
{
	const auto* Player=Cast<AMT2PlayerCharacter>(GetOwner());
	auto* GI=GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	auto* Registry=GI ? GI->GetSubsystem<UMT2VnumRegistrySubsystem>() : nullptr;
	if (!Player || !Registry) { return nullptr; }
	const auto& Worn=Player->GetInventoryComponent()->GetEquipment();
	return Worn.IsValidIndex(4) && !Worn[4].IsEmpty() ? Cast<UMT2ItemPickTemplate>(Registry->ResolveItemTemplateClass(Worn[4].Vnum).GetDefaultObject()) : nullptr;
}
bool UMT2MiningComponent::HasPickaxeEquipped() const { return EquippedPick() != nullptr; }
bool UMT2MiningComponent::CanMine(AMT2Npc* Vein) const
{
	const auto* Player=Cast<AMT2PlayerCharacter>(GetOwner());
	if (!Player || !Player->HasAuthority() || !IsValid(Vein) || Vein->IsActorBeingDestroyed() ||
		Vein->GetWorld()!=GetWorld() || !MT2Mining::RawOre(Vein->GetMobVnum()) ||
		Player->GetHealthComponent()->IsDead() || Player->GetMountComponent()->IsMounted() ||
		Player->GetFishingComponent()->IsFishing() || Player->GetStatusEffectComponent()->HasEffectFlag(EMT2EffectFlag::Stun) || !EquippedPick() ||
		!Player->GetCharacterMovement()->IsMovingOnGround() || Player->GetCharacterMovement()->HasRootMotionSources()) { return false; }
	const auto* ASC=Player->GetAbilitySystemComponent();
	if (ASC && ASC->HasMatchingGameplayTag(MT2GameplayTags::State_Attacking)) { return false; }
	const float Range=Vein->GetInteractionComponent()->GetInteractionRange()+100.f;
	return FVector::Dist(Player->GetActorLocation(),Vein->GetActorLocation())<=Range;
}
bool UMT2MiningComponent::StartMining(AMT2Npc* Vein)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bResolving) { return false; }
	if (IsMining()) { CancelMining(); return false; }
	if (!CanMine(Vein)) { return false; }
	auto* Player=CastChecked<AMT2PlayerCharacter>(GetOwner());
	Player->GetInventoryComponent()->OnEquipmentChanged.AddUniqueDynamic(this, &UMT2MiningComponent::OnEquipmentChanged);
	Player->GetStatusEffectComponent()->OnStatusEffectsChanged.AddUniqueDynamic(this, &UMT2MiningComponent::CheckAttempt);
	Player->GetHealthComponent()->OnValueChanged.AddUniqueDynamic(this, &UMT2MiningComponent::OnHealthChanged);
	Player->GetCharacterMovement()->StopMovementImmediately();
	Player->GetCombatComponent()->StopBasicAttackLoop();
	Origin=Player->GetActorLocation();
	Player->SetActorRotation(FRotator(0,(Vein->GetActorLocation()-Origin).Rotation().Yaw,0));
	if (++SessionCounter==0) { ++SessionCounter; }
	State.Session=SessionCounter; State.Vein=Vein; State.StartTime=ServerTime(); State.Swings=Random.RandRange(5,15);
	Player->ForceNetUpdate(); OnRep_State();
	const uint32 Session=State.Session;
	GetWorld()->GetTimerManager().SetTimer(CompletionTimer, FTimerDelegate::CreateWeakLambda(this,[this,Session]{Complete(Session);}), 2.f*State.Swings,false);
	GetWorld()->GetTimerManager().SetTimer(GuardTimer,this,&UMT2MiningComponent::CheckAttempt,.1f,true);
	return true;
}
void UMT2MiningComponent::CheckAttempt()
{
	if (IsMining() && (!CanMine(State.Vein) || FVector::Dist(GetOwner()->GetActorLocation(),Origin)>5.f)) { CancelMining(); }
}
void UMT2MiningComponent::OnEquipmentChanged() { CancelMining(); }
void UMT2MiningComponent::OnHealthChanged(float OldValue, float NewValue) { if (NewValue<OldValue) { CancelMining(); } }
void UMT2MiningComponent::RequestCancelMining()
{
	if (!IsMining()) { return; }
	if (GetOwner()->HasAuthority()) { CancelMining(); }
	else { ServerCancelMining(State.Session); }
}
void UMT2MiningComponent::ServerCancelMining_Implementation(uint32 ExpectedSession)
{
	if (ExpectedSession==State.Session) { CancelMining(); }
}
void UMT2MiningComponent::CancelMining()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsMining()) { return; }
	State.Swings=0; State.Vein=nullptr;
	GetWorld()->GetTimerManager().ClearTimer(CompletionTimer); GetWorld()->GetTimerManager().ClearTimer(GuardTimer);
	GetOwner()->ForceNetUpdate(); OnRep_State();
}
void UMT2MiningComponent::Complete(uint32 ExpectedSession)
{
	if (bResolving || !GetOwner() || !GetOwner()->HasAuthority() || !IsMining() || ExpectedSession!=State.Session) { return; }
	if (ServerTime()+.001 < State.StartTime+2.0*State.Swings) { return; }
	if (!CanMine(State.Vein) || FVector::Dist(GetOwner()->GetActorLocation(),Origin)>5.f) { CancelMining(); return; }
	TGuardValue<bool> Guard(bResolving,true);
	auto* Player=CastChecked<AMT2PlayerCharacter>(GetOwner());
	const auto* Pick=EquippedPick();
	const auto* PS=Player->GetPlayerState<AMT2PlayerState>();
	const int32 Chance=MT2Mining::SuccessChance(PS ? PS->GetSkillComponent()->GetSkillLevel(124) : 0,Pick->RefinementLevel);
	const int32 OreVnum=MT2Mining::RawOre(State.Vein->GetMobVnum());
	// Clear the attempt before inventory callbacks can re-enter gameplay.
	CancelMining();
	Player->GetInventoryComponent()->FinishMiningAttempt();
	if (Random.RandRange(1,100)>Chance) { Say(TEXT("Mining failed. You found no ore.")); return; }
	auto* Registry=GetWorld()->GetGameInstance()->GetSubsystem<UMT2VnumRegistrySubsystem>();
	const auto* Ore=Registry->ResolveItemTemplateClass(OreVnum).GetDefaultObject();
	if (!Ore) { UE_LOG(LogTemp,Error,TEXT("Mining ore %d has no imported template."),OreVnum); return; }
	const int32 Bucket=Random.RandRange(1,100);
	const int32 Quantity=MT2Mining::Quantity(Bucket,Random.RandRange(0,9));
	FMT2ItemSlot Reward(OreVnum,Ore->MakeInstanceData(Quantity));
	const int32 Added=Player->GetInventoryComponent()->AddItemSlotPartial(Reward);
	Reward.Count-=Added;
	if (Reward.Count>0 && !AMT2WorldItem::SpawnWorldItemInstance(GetWorld(),Origin,Reward,Player,
		AMT2WorldItem::ResolvePlayerIdentity(PS),PS ? PS->GetPlayerName() : Player->GetName(),15.f))
	{ UE_LOG(LogTemp,Error,TEXT("Mining could not deliver %d of ore %d to %s."),Reward.Count,OreVnum,*Player->GetName()); }
	if (Added>0 && PS)
	{
		FMT2ItemSlot Picked=Reward; Picked.Count=Added;
		PS->GetQuestManagerComponent()->NotifyItemPicked(Picked);
	}
	Say(FString::Printf(TEXT("You mined %d ore."),Quantity));
}
void UMT2MiningComponent::Say(const FString& Message) const
{
	if (const auto* Player=Cast<AMT2PlayerCharacter>(GetOwner()))
		if (auto* PC=Cast<AMT2PlayerController>(Player->GetController())) { PC->SendSystemChatMessage(Message); }
}
double UMT2MiningComponent::ServerTime() const
{
	const auto* GS=GetWorld()->GetGameState();
	return GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}
void UMT2MiningComponent::OnRep_State()
{
	if (GetNetMode()==NM_DedicatedServer) { return; }
	if (auto* Player=Cast<AMT2PlayerCharacter>(GetOwner())) { Player->UpdatePawnYawFollowsCamera(); }
	if (!IsMining()) { StopPresentation(); return; }
	if (PresentedSession!=State.Session) { StopPresentation(); PresentedSession=State.Session; }
	UpdatePresentation();
	GetWorld()->GetTimerManager().SetTimer(PresentationTimer,this,&UMT2MiningComponent::UpdatePresentation,.1f,true);
}
void UMT2MiningComponent::StopPresentation()
{
	if (const auto* Player=Cast<AMT2PlayerCharacter>(GetOwner()))
		if (auto* Anim=Player->GetMesh()->GetAnimInstance(); Anim && DigMontage) { Anim->Montage_Stop(.1f,DigMontage); }
	DigMontage=nullptr; PresentedSwing=INDEX_NONE; bFacingApplied=false;
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(PresentationTimer); }
}
void UMT2MiningComponent::UpdatePresentation()
{
	const double Elapsed=FMath::Max(0.0,ServerTime()-State.StartTime);
	const int32 Swing=FMath::FloorToInt(Elapsed/2.0);
	if (!IsMining() || Swing>=State.Swings) { StopPresentation(); return; }
	auto* Player=Cast<AMT2PlayerCharacter>(GetOwner());
	if (!bFacingApplied && Player && IsValid(State.Vein))
	{
		Player->SetActorRotation(FRotator(0,(State.Vein->GetActorLocation()-Player->GetActorLocation()).Rotation().Yaw,0));
		bFacingApplied=true;
	}
	if (Swing==PresentedSwing) { return; }
	auto* Anim=Player ? Cast<UMT2CharacterAnimInstance>(Player->GetMesh()->GetAnimInstance()) : nullptr;
	auto* Dig=Anim ? Anim->GetAnimation(TEXT("general"),TEXT("dig")) : nullptr;
	if (!Dig) { return; }
	const float Rate=Dig->GetPlayLength()/2.f;
	DigMontage=Anim->PlaySlotAnimationAsDynamicMontage(Dig,TEXT("DefaultSlot"),.05f,.05f,Rate,1,-1.f,float(FMath::Fmod(Elapsed,2.0))*Rate);
	PresentedSwing=Swing;
}
