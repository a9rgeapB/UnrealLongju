#include "MT2ConfigureDevilTowerCommandlet.h"
#include "Config/MT2PathSettings.h"
#include "Core/MT2VnumRegistry.h"
#include "Dungeons/MT2DevilTowerRoom.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshCompiler.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Internationalization/Regex.h"
#include "Importers/MT2MobProtoReader.h"
#include "Npcs/MT2StaticNpc.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/Blueprint.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "MT2VnumRegistryBuilder.h"
#include "Importers/MT2MapTerrainImporter.h"
#include "MT2MapTerrainBuilder.h"
#include "Mobs/MT2Mob.h"
#include "Mobs/MT2MobSpawnActor.h"
#include "Mobs/MT2MobSpawnComponent.h"
#include "World/MT2MapPresentationActor.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionHelpers.h"

namespace
{
	bool ImportSealProps()
	{
		FMT2MobProtoReadResult Proto; FString Error;
		if (!FMT2MobProtoReader::Read(FString(UMT2PathSettings::Path(TEXT("LegacyDumpRoot"))) / UMT2PathSettings::Path(TEXT("Part_locale_en_mob_proto")), Proto, Error))
		{ UE_LOG(LogTemp, Error, TEXT("%s"), *Error); return false; }
		TArray<UPackage*> Packages;
		for (int32 Vnum : {20073, 20366})
		{
			const auto* Definition = Proto.Definitions.FindByPredicate([Vnum](const auto& Row) { return Row.Vnum == Vnum; });
			const FString Resource = Vnum == 20073 ? TEXT("seal_stone") : TEXT("keyholestone");
			const FString MeshPath = FString::Printf(TEXT("/Game/ymir_work/npc/%s/SM_%s.SM_%s"), *Resource, *Resource, *Resource);
			auto* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
			if (!Definition || !Mesh) { UE_LOG(LogTemp, Error, TEXT("Seal %d needs its legacy proto and imported mesh %s."), Vnum, *MeshPath); return false; }
			const FString Name = FString::Printf(TEXT("BP_Npc_%05d_%s"), Vnum, *Resource);
			const FString Path = TEXT("/Game/Npcs/Blueprints/") + Name;
			auto* Blueprint = LoadObject<UBlueprint>(nullptr, *(Path + TEXT(".") + Name));
			if (!Blueprint)
			{
				auto* Package = CreatePackage(*Path);
				Blueprint = FKismetEditorUtilities::CreateBlueprint(AMT2StaticNpc::StaticClass(), Package, FName(Name), BPTYPE_Normal,
					UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
				if (!Blueprint) return false;
				FAssetRegistryModule::AssetCreated(Blueprint);
			}
			auto* Defaults = Blueprint->GeneratedClass ? Cast<AMT2StaticNpc>(Blueprint->GeneratedClass->GetDefaultObject()) : nullptr;
			if (!Defaults) return false;
			Blueprint->Modify(); Defaults->Modify(); Defaults->ConfigureFromDefinition(*Definition);
			Defaults->StaticVisual->SetStaticMesh(Mesh);
			const auto Box = Mesh->GetBoundingBox();
			const float Radius = FMath::Max(20.f, FMath::Max(Box.GetExtent().X, Box.GetExtent().Y));
			Defaults->GetCapsuleComponent()->SetCapsuleSize(Radius, FMath::Max(Radius, Box.GetExtent().Z));
			FKismetEditorUtilities::CompileBlueprint(Blueprint); Blueprint->MarkPackageDirty(); Packages.AddUnique(Blueprint->GetPackage());
		}
		if (!UEditorLoadingAndSavingUtils::SavePackages(Packages, true)) return false;
		TArray<FString> Errors; int32 Mobs = 0, Items = 0;
		if (!FMT2VnumRegistryBuilder::RebuildAndSave(Errors, Mobs, Items))
		{ for (const auto& Entry : Errors) UE_LOG(LogTemp, Error, TEXT("%s"), *Entry); return false; }
		return true;
	}
	struct FFloorPlan
	{
		UStaticMeshComponent* Mesh = nullptr;
		FVector Entrance;
		float FloorZ = MAX_flt;
		TArray<FMT2DungeonSpawn> Spawns;
		TArray<FMT2DungeonSpawn> Puzzle, Regen;
	};
	bool Surface(UStaticMeshComponent* Mesh, const FVector2D& XY, FVector& Out, float Radius = 45.f, float HalfHeight = 96.f, float MaxZ = MAX_flt)
	{
		FHitResult Hit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(MT2TowerBake), true);
		FVector Start(XY.X, XY.Y, Mesh->Bounds.GetBox().Max.Z + 1000);
		const FVector End(XY.X, XY.Y, Mesh->Bounds.GetBox().Min.Z - 1000);
		bool bFound = false;
		// Upper rooms have pillars/roof caps above some legacy points. Continue through those
		// surfaces to the actual arena rather than treating the first upward face as the floor.
		for (int32 Layer = 0; Layer < 32 && Start.Z > End.Z; ++Layer)
		{
			if (!Mesh->LineTraceComponent(Hit, Start, End, Query)) break;
			FCollisionQueryParams Clearance(SCENE_QUERY_STAT(MT2TowerSurfaceClearance), false);
			if (Hit.ImpactNormal.Z >= .5f && Hit.ImpactPoint.Z <= MaxZ &&
				!Mesh->GetWorld()->OverlapBlockingTestByChannel(Hit.ImpactPoint + FVector(0,0,HalfHeight+2.f), FQuat::Identity,
					ECC_Pawn, FCollisionShape::MakeCapsule(Radius, HalfHeight), Clearance))
			{ Out = Hit.ImpactPoint; bFound = true; }
			Start.Z = Hit.ImpactPoint.Z - 1.f;
		}
		return bFound;
	}
	bool BakeSpawns(UWorld* World, const UMT2VnumRegistry* Registry, const AMT2MapPresentationActor* Map, FFloorPlan& Plan,
		const TArray<FMT2MobSpawnEntry>& Entries, FRandomStream& Random)
	{
		int32 Group = 0;
		for (const auto& Entry : Entries)
		{
			if (Entry.Variants.IsEmpty() || Entry.SpawnChancePercent != 100.f) return false;
			for (int32 Count = 0; Count < Entry.DesiredGroupCount; ++Count)
			{
				++Group;
				for (int32 VariantIndex = 0; VariantIndex < Entry.Variants.Num(); ++VariantIndex)
				for (const auto& Member : Entry.Variants[VariantIndex].Members)
				{
					const auto* Reference = Registry->GetMobClasses().Find(Member.MobVnum);
					UClass* Class = Reference ? Reference->LoadSynchronous() : nullptr;
					const auto* Defaults = Class ? Class->GetDefaultObject<AMT2Mob>() : nullptr;
					if (!Defaults) { UE_LOG(LogTemp, Error, TEXT("Missing tower mob VNUM %d."), Member.MobVnum); return false; }
					FVector Ground; bool bFound = false; FString Rejection;
					const auto* Capsule = Defaults->GetCapsuleComponent();
					for (int32 Attempt = 0; Attempt < 64 && !bFound; ++Attempt)
					{
						FVector2D XY(Entry.Center.X + Random.FRandRange(-Entry.HorizontalExtent.X, Entry.HorizontalExtent.X),
							Entry.Center.Y + Random.FRandRange(-Entry.HorizontalExtent.Y, Entry.HorizontalExtent.Y));
						if (Entry.HorizontalExtent.IsNearlyZero() && Attempt > 0)
						{
							// Legacy points may touch a pillar. Keep the exact point when clear, otherwise
							// bake a nearby collision-safe placement rather than embedding a UE capsule.
							const int32 Step = Attempt - 1;
							XY += FVector2D((Step % 7 - 3) * 100.f, (Step / 7 - 3) * 100.f);
						}
						if (!Surface(Plan.Mesh, XY, Ground, Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight(), Plan.FloorZ + 150.f)) { Rejection = TEXT("no clear upward arena surface"); continue; }
						uint8 Flags = 0;
						if (Map->Attributes.Query(Ground, Map->WorldMin, Map->WorldMax, Flags) && (Flags & MT2MapAttribute::NoWalk)) { Rejection = TEXT("NoWalk attribute"); continue; }
						// Do not accept horizontal wall caps/roof ledges as the arena floor.
						if (FMath::Abs(Ground.Z - (Plan.Entrance.Z - 100.f)) > 150.f)
						{ Rejection = FString::Printf(TEXT("surface Z=%f entrance ground Z=%f"), Ground.Z, Plan.Entrance.Z - 100.f); continue; }
						Ground.Z += Capsule->GetScaledCapsuleHalfHeight() + 2.f;
						FCollisionQueryParams Query(SCENE_QUERY_STAT(MT2TowerSpawnBake), false);
						bFound = !World->OverlapBlockingTestByChannel(Ground, FQuat::Identity, ECC_Pawn,
							FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight()), Query);
						if (!bFound) Rejection = FString::Printf(TEXT("blocked capsule radius=%f halfheight=%f at %s"), Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight(), *Ground.ToString());
						for (const auto& Previous : Plan.Spawns)
							if (!(Previous.GroupId == Group && Previous.VariantId != (Entry.Variants.Num() > 1 ? VariantIndex + 1 : 0)) &&
								FVector::DistSquared2D(Previous.LocalTransform.GetLocation(), Ground) < FMath::Square(150.f)) bFound = false;
					}
					if (!bFound) { UE_LOG(LogTemp, Error, TEXT("No clear room surface for %s VNUM %d: %s."), *Entry.SourceId.ToString(), Member.MobVnum, *Rejection); return false; }
					auto& Spawn = Plan.Spawns.AddDefaulted_GetRef(); Spawn.Vnum = Member.MobVnum;
					Spawn.bForceAggressive = Entry.bForceAggressive; Spawn.GroupId = Group;
					Spawn.RespawnSeconds = Entry.RespawnDelay;
					Spawn.VariantId = Entry.Variants.Num() > 1 ? VariantIndex + 1 : 0;
					Spawn.VariantWeight = Entry.Variants[VariantIndex].Weight;
					Spawn.LocalTransform = FTransform(FRotator(0, Entry.Yaw < 0 ? Random.FRandRange(0.f, 360.f) : Entry.Yaw + 90.f, 0), Ground);
				}
			}
		}
		return !Plan.Spawns.IsEmpty();
	}
	bool BakeFixed(UWorld* World, const UMT2VnumRegistry* Registry, const AMT2MapPresentationActor* Map,
		FFloorPlan& Plan, int32 Vnum, FVector2D Local, FMT2DungeonSpawn& Out)
	{
		FFloorPlan Fixed; Fixed.Mesh = Plan.Mesh; Fixed.Entrance = Plan.Entrance; Fixed.FloorZ = Plan.FloorZ;
		FMT2MobSpawnEntry Entry; Entry.Center = FVector(76800 - Local.X * 100, -Local.Y * 100, 0);
		Entry.DesiredGroupCount = 1; Entry.SpawnChancePercent = 100;
		auto& Variant = Entry.Variants.AddDefaulted_GetRef(); Variant.Weight = 1;
		Variant.Members.AddDefaulted_GetRef().MobVnum = Vnum;
		FRandomStream Random(Vnum);
		if (!BakeSpawns(World, Registry, Map, Fixed, {Entry}, Random)) return false;
		Out = Fixed.Spawns[0]; Out.RespawnSeconds = 0; return true;
	}
}

UMT2ConfigureDevilTowerCommandlet::UMT2ConfigureDevilTowerCommandlet()
{
	IsClient = false; IsEditor = true; IsServer = false; LogToConsole = true;
}

int32 UMT2ConfigureDevilTowerCommandlet::Main(const FString& Params)
{
	const FString MapPath = TEXT("/Game/Maps/Game/devil_tower");
	UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(
		FPackageName::LongPackageNameToFilename(MapPath, FPackageName::GetMapPackageExtension()));
	if (!World) return 1;
	FWorldPartitionHelpers::FForEachActorWithLoadingResult LoadedActors;
	if (UWorldPartition* Partition = World->GetWorldPartition())
	{
		FWorldPartitionHelpers::FForEachActorWithLoadingParams LoadParams;
		LoadParams.bKeepReferences = true;
		FWorldPartitionHelpers::ForEachActorWithLoading(Partition, [](const FWorldPartitionActorDescInstance*) { return true; }, LoadParams, LoadedActors);
	}
	for (TActorIterator<AMT2MapPresentationActor> It(World); It; ++It)
		UE_LOG(LogTemp, Display, TEXT("Tower metadata: id=%s index=%d min=%s max=%s cells=%s"),
			*It->MapId, It->MapIndex, *It->WorldMin.ToString(), *It->WorldMax.ToString(), *It->MapCells.ToString());
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		TInlineComponentArray<UStaticMeshComponent*> Meshes; It->GetComponents(Meshes);
		for (UStaticMeshComponent* Component : Meshes)
			if (Component->GetStaticMesh())
				UE_LOG(LogTemp, Display, TEXT("Tower mesh: %s asset=%s center=%s extent=%s collision=%d"),
					*It->GetActorLabel(), *Component->GetStaticMesh()->GetPathName(),
					*Component->Bounds.Origin.ToString(), *Component->Bounds.BoxExtent.ToString(),
					static_cast<int32>(Component->GetCollisionEnabled()));
	}
	for (TActorIterator<AMT2MobSpawnActor> It(World); It; ++It)
		UE_LOG(LogTemp, Display, TEXT("Tower ambient spawner: %s entries=%d"),
			*It->GetActorLabel(), It->GetMobSpawnComponent()->GetSpawnEntries().Num());
	FString LocaleRoot, DungeonRoot;
	FParse::Value(*Params, TEXT("LocaleRoot="), LocaleRoot);
	FParse::Value(*Params, TEXT("DungeonRoot="), DungeonRoot);
	if (LocaleRoot.IsEmpty() || DungeonRoot.IsEmpty())
	{
		UE_LOG(LogTemp, Display, TEXT("Inspection only. Supply -LocaleRoot=... -DungeonRoot=... to validate; add -Apply to save.")); return 0;
	}
	AMT2MapPresentationActor* Map = nullptr;
	for (TActorIterator<AMT2MapPresentationActor> It(World); It; ++It) { if (Map) return 1; Map = *It; }
	if (!Map || Map->MapIndex != 66 || Map->MapCells != FIntPoint(3,3) || Map->WorldMax.X != 76800.f)
	{
		UE_LOG(LogTemp, Error, TEXT("Tower map metadata does not match the verified legacy layout.")); return 1;
	}
	if (FParse::Param(*Params, TEXT("Apply")) && !ImportSealProps()) return 1;
	const auto* Registry = LoadObject<UMT2VnumRegistry>(nullptr, UMT2PathSettings::Path(TEXT("VnumRegistry")));
	if (!Registry) return 1;
	TArray<UPackage*> CollisionPackages;
	const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
	TMap<FName, FText> Messages;
	FString Translation;
	if (!FFileHelper::LoadFileToString(Translation, *(LocaleRoot / TEXT("translate.lua")))) return 1;
	FRegexMatcher Texts(FRegexPattern(TEXT("gameforge\\.deviltower_zone\\.([A-Za-z0-9_]+)\\s*=\\s*\"([^\"]*)\"")), Translation);
	while (Texts.FindNext())
	{
		const FString Key = Texts.GetCaptureGroup(1);
		Messages.Add(FName(Key), FText::ChangeKey(TEXT("MT2LegacyTower"), Key,
			FText::FromString(Texts.GetCaptureGroup(2).Replace(TEXT("\r"), TEXT("")).Replace(TEXT("\n"), TEXT(" ")))));
	}
	if (Messages.Num() < 28) { UE_LOG(LogTemp, Error, TEXT("Tower locale notices incomplete.")); return 1; }
	TArray<FFloorPlan> Plans; Plans.SetNum(9);
	const FVector2D LocalEntrances[] = { {117,614}, {126,384}, {134,147}, {369,629}, {369,401}, {374,167}, {590,638}, {590,403}, {590,155} };
	for (int32 Floor = 0; Floor < 9; ++Floor)
	{
		const FString LabelPart = Floor < 3 ? FString::Printf(TEXT("_00000%d_deviltower_1F"), 2 - Floor) :
			FString::Printf(TEXT("_00%d00%d_deviltower%d"), Floor / 3, 2 - Floor % 3, Floor / 3 + 1);
		for (TActorIterator<AActor> It(World); It; ++It)
			if (It->GetActorLabel().Contains(LabelPart, ESearchCase::IgnoreCase))
			{
				if (Plans[Floor].Mesh) return 1;
				Plans[Floor].Mesh = It->FindComponentByClass<UStaticMeshComponent>();
			}
		if (Plans[Floor].Mesh)
		{
			UStaticMesh* Asset = Plans[Floor].Mesh->GetStaticMesh();
			FStaticMeshCompilingManager::Get().FinishCompilation({Asset});
			auto* Body = Asset->GetBodySetup();
			if (!Body) return 1;
			// A single convex hull fills an entire hollow dungeon room. Its roof is not a floor.
			// Validate against triangle collision, then save that same collision policy with -Apply.
			if (Body->CollisionTraceFlag != CTF_UseComplexAsSimple)
			{
				if (bVerify) { UE_LOG(LogTemp, Error, TEXT("Saved room mesh requires triangle collision policy.")); return 1; }
				Asset->Modify(); Body->Modify(); Body->CollisionTraceFlag = CTF_UseComplexAsSimple;
				Body->InvalidatePhysicsData(); Body->CreatePhysicsMeshes(); Asset->MarkPackageDirty();
				CollisionPackages.AddUnique(Asset->GetPackage());
				for (TActorIterator<AActor> It(World); It; ++It)
				{
					TInlineComponentArray<UStaticMeshComponent*> Components; It->GetComponents(Components);
					for (auto* Component : Components) if (Component->GetStaticMesh() == Asset) Component->RecreatePhysicsState();
				}
			}
			if (!Asset->ContainsPhysicsTriMeshData(false))
			{
				UE_LOG(LogTemp, Error, TEXT("Room mesh has no triangle collision source: %s"), *Asset->GetPathName()); return 1;
			}
		}
		if (Plans[Floor].Mesh)
		{
			for (const FVector2D Offset : {FVector2D(0,0), {1000,0}, {-1000,0}, {0,1000}, {0,-1000}})
			{
				FVector Ground;
				if (Surface(Plans[Floor].Mesh, FVector2D(Plans[Floor].Mesh->Bounds.Origin) + Offset, Ground))
					Plans[Floor].FloorZ = FMath::Min(Plans[Floor].FloorZ, static_cast<float>(Ground.Z));
			}
		}
		if (!Plans[Floor].Mesh || Plans[Floor].FloorZ == MAX_flt)
		{
			if (Plans[Floor].Mesh)
			{
				auto* Mesh = Plans[Floor].Mesh;
				UE_LOG(LogTemp, Error, TEXT("Room trace diagnostics: physics=%d body=%d setup=%d complex=%d location=%s"),
					World->GetPhysicsScene() != nullptr, Mesh->GetBodyInstance()->IsValidBodyInstance(),
					Mesh->GetStaticMesh()->GetBodySetup() != nullptr,
					Mesh->GetStaticMesh()->GetBodySetup() ? static_cast<int32>(Mesh->GetStaticMesh()->GetBodySetup()->CollisionTraceFlag) : -1,
					*Mesh->GetComponentLocation().ToString());
				if (auto* Body = Mesh->GetStaticMesh()->GetBodySetup())
					UE_LOG(LogTemp, Display, TEXT("Tower collision: convex=%d boxes=%d trimeshes=%d"), Body->AggGeom.ConvexElems.Num(), Body->AggGeom.BoxElems.Num(), Body->ChaosTriMeshes.Num());
			}
			UE_LOG(LogTemp, Error, TEXT("Cannot verify floor %d entrance on its room mesh."), Floor + 1); return 1;
		}
		// Entrance is stored as ground + a conservative player capsule clearance; TeleportTo
		// checks actual occupancy on authority at runtime.
		const FVector2D RequestedXY(76800 - LocalEntrances[Floor].X * 100, -LocalEntrances[Floor].Y * 100);
		bool bEntranceClear = false;
		for (int32 Attempt = 0; Attempt < 50 && !bEntranceClear; ++Attempt)
		{
			FVector Ground;
			const FVector2D Offset = Attempt == 0 ? FVector2D::ZeroVector : FVector2D(((Attempt-1)%7-3)*100.f, ((Attempt-1)/7-3)*100.f);
			if (!Surface(Plans[Floor].Mesh, RequestedXY + Offset, Ground, 45.f, 96.f, Plans[Floor].FloorZ + 150.f)) continue;
			Ground.Z += 100.f;
			FCollisionQueryParams Query(SCENE_QUERY_STAT(MT2TowerEntrance), false);
			if (!World->OverlapBlockingTestByChannel(Ground, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(45.f,96.f), Query))
			{ Plans[Floor].Entrance = Ground; bEntranceClear = true; }
		}
		if (!bEntranceClear) { UE_LOG(LogTemp, Error, TEXT("Floor %d has no clear entrance near its legacy point."), Floor + 1); return 1; }
		TArray<FMT2MobSpawnEntry> Entries; TArray<FString> Warnings;
		if (Floor == 0)
		{
			for (TActorIterator<AMT2MobSpawnActor> It(World); It; ++It)
				for (const auto& Entry : It->GetMobSpawnComponent()->GetSpawnEntries())
					if (Entry.SourceVnum == 8015 && Entry.Type == EMT2MobSpawnType::Mob) Entries.Add(Entry);
			if (Entries.IsEmpty())
			{
				// Reruns use the source entry, which was removed from the ambient scheduler.
				FMT2MapTerrainInfo Info; Info.MapSizeX = 3; Info.MapSizeY = 3;
				TArray<FMT2MobSpawnEntry> Source;
				if (!FMT2MapTerrainImporter::ReadDungeonRegen(LocaleRoot, LocaleRoot / TEXT("map/metin2_map_deviltower1/regen.txt"), Info, Source, Warnings)) return 1;
				for (const auto& Entry : Source) if (Entry.SourceVnum == 8015 && Entry.Type == EMT2MobSpawnType::Mob) Entries.Add(Entry);
			}
			if (Entries.Num() != 1) return 1;
		}
		else if (Floor != 6)
		{
			FMT2MapTerrainInfo Info; Info.MapSizeX = 3; Info.MapSizeY = 3;
			if (!FMT2MapTerrainImporter::ReadDungeonRegen(LocaleRoot,
				DungeonRoot / FString::Printf(TEXT("deviltower%d_regen.txt"), Floor + 1), Info, Entries, Warnings))
			{
				for (const auto& Warning : Warnings) UE_LOG(LogTemp, Error, TEXT("%s"), *Warning);
				return 1;
			}
		}
		FRandomStream Random(66001 + Floor);
		if (Floor > 0) for (auto& Entry : Entries) Entry.bForceAggressive = true;
		if (Floor != 6 && !BakeSpawns(World, Registry, Map, Plans[Floor], Entries, Random)) return 1;
		if (Floor != 4 && Floor != 7) for (auto& Spawn : Plans[Floor].Spawns) Spawn.RespawnSeconds = 0;
		const auto AddFixed = [&](int32 Vnum, FVector2D XY, TArray<FMT2DungeonSpawn>& Destination)
		{
			FMT2DungeonSpawn Spawn;
			if (!BakeFixed(World, Registry, Map, Plans[Floor], Vnum, XY, Spawn)) return false;
			Destination.Add(Spawn); return true;
		};
		if (Floor == 3)
		{
			for (const FVector2D XY : {FVector2D(368,629), {419,630}, {428,653}, {422,679}, {395,689}, {369,679}, {361,658}})
				if (!AddFixed(8017, XY, Plans[Floor].Puzzle)) return 1;
		}
		else if (Floor == 4)
		{
			for (const FVector2D XY : {FVector2D(421,452), {380,460}, {428,414}, {398,392}, {359,426}})
				if (!AddFixed(20073, XY, Plans[Floor].Puzzle)) return 1;
		}
		else if (Floor == 5)
		{
			for (const int32 Vnum : {20074,20075,20076}) if (!AddFixed(Vnum, {425,216}, Plans[Floor].Puzzle)) return 1;
		}
		else if (Floor == 6)
		{
			for (const FVector2D XY : {FVector2D(639,658), {611,637}, {596,674}, {629,670}})
				if (!AddFixed(8018, XY, Plans[Floor].Spawns)) return 1;
			FMT2MapTerrainInfo Info; Info.MapSizeX = 3; Info.MapSizeY = 3;
			if (!FMT2MapTerrainImporter::ReadDungeonRegen(LocaleRoot, DungeonRoot / TEXT("deviltower7_regen.txt"), Info, Entries, Warnings)) return 1;
			for (auto& Entry : Entries) Entry.bForceAggressive = true;
			FFloorPlan Regen; Regen.Mesh = Plans[Floor].Mesh; Regen.Entrance = Plans[Floor].Entrance; Regen.FloorZ = Plans[Floor].FloorZ;
			if (!BakeSpawns(World, Registry, Map, Regen, Entries, Random)) return 1;
			Plans[Floor].Regen = MoveTemp(Regen.Spawns);
		}
		else if (Floor == 7 && !AddFixed(20366, {640,460}, Plans[Floor].Puzzle)) return 1;
		UE_LOG(LogTemp, Display, TEXT("Validated tower floor %d: entrance=%s, %d baked mobs."), Floor + 1, *Plans[Floor].Entrance.ToString(), Plans[Floor].Spawns.Num());
	}
	if (bVerify)
	{
		TArray<AMT2DevilTowerRoom*> Rooms; Rooms.SetNumZeroed(9);
		for (TActorIterator<AMT2DevilTowerRoom> It(World); It; ++It)
		{
			const int32 Index = It->Floor - 1;
			if (!Rooms.IsValidIndex(Index) || Rooms[Index]) return 1;
			Rooms[Index] = *It;
		}
		for (int32 Index = 0; Index < 9; ++Index)
		{
			auto* Room = Rooms[Index];
			if (!Room || Room->RoomId != FName(*FString::Printf(TEXT("DevilTower.Floor%d"), Index + 1)) ||
				Room->Spawns.Num() != Plans[Index].Spawns.Num() || Room->EncounterSeconds != 0.f ||
				!Room->Entrance->GetComponentLocation().Equals(Plans[Index].Entrance, .1f) ||
				Room->NextRoom != (Index < 8 ? Rooms[Index+1] : nullptr) || Room->GetIsSpatiallyLoaded() ||
				Room->PuzzleSpawns.Num() != Plans[Index].Puzzle.Num() || Room->RegenSpawns.Num() != Plans[Index].Regen.Num() ||
				Room->LegacyMessages.Num() != Messages.Num())
			{
				UE_LOG(LogTemp, Error, TEXT("Saved floor %d configuration differs from source."), Index + 1); return 1;
			}
			for (int32 SpawnIndex = 0; SpawnIndex < Room->Spawns.Num(); ++SpawnIndex)
			{
				const auto& Spawn = Room->Spawns[SpawnIndex]; const auto& Plan = Plans[Index].Spawns[SpawnIndex];
				if (Spawn.Vnum != Plan.Vnum || Spawn.GroupId != Plan.GroupId || Spawn.bForceAggressive != Plan.bForceAggressive ||
					!(Spawn.LocalTransform * Room->GetActorTransform()).Equals(Plan.LocalTransform, .1f) ||
					!Room->ContainsLocation((Spawn.LocalTransform * Room->GetActorTransform()).GetLocation())) return 1;
			}
			const auto Matches = [Room](const TArray<FMT2DungeonSpawn>& Saved, const TArray<FMT2DungeonSpawn>& Source)
			{
				if (Saved.Num() != Source.Num()) return false;
				for (int32 I = 0; I < Saved.Num(); ++I)
				{
					const auto& A = Saved[I]; const auto& B = Source[I];
					if (A.Vnum != B.Vnum || A.GroupId != B.GroupId || A.VariantId != B.VariantId || A.VariantWeight != B.VariantWeight ||
						A.RespawnSeconds != B.RespawnSeconds || A.bForceAggressive != B.bForceAggressive ||
						!(A.LocalTransform * Room->GetActorTransform()).Equals(B.LocalTransform, .1f) ||
						!Room->ContainsLocation((A.LocalTransform * Room->GetActorTransform()).GetLocation())) return false;
				}
				return true;
			};
			if (!Matches(Room->Spawns, Plans[Index].Spawns) || !Matches(Room->PuzzleSpawns, Plans[Index].Puzzle) || !Matches(Room->RegenSpawns, Plans[Index].Regen)) return 1;
			const float Delay = Index == 8 ? 60.f : Index == 3 ? 5.f : Index < 3 && Index != 0 ? 4.f : 6.f;
			if (Room->TransitionSeconds != Delay) return 1;
			for (const auto& Message : Messages)
				if (!Room->LegacyMessages.Contains(Message.Key) || Room->LegacyMessages[Message.Key].ToString() != Message.Value.ToString()) return 1;
		}
		for (TActorIterator<AMT2MobSpawnActor> It(World); It; ++It)
			for (const auto& Entry : It->GetMobSpawnComponent()->GetSpawnEntries()) if (Entry.SourceVnum == 8015) return 1;
		UE_LOG(LogTemp, Display, TEXT("Saved tower verified: nine unique linked rooms, baked encounters, puzzles and legacy notices.")); return 0;
	}
	if (!FParse::Param(*Params, TEXT("Apply"))) return 0;
	// No persistent modifications occur until all inputs, classes and room surfaces passed.
	TArray<AMT2DevilTowerRoom*> Rooms; TArray<UPackage*> Packages = CollisionPackages;
	for (int32 Floor = 0; Floor < 9; ++Floor)
	{
		const FName Id(*FString::Printf(TEXT("DevilTower.Floor%d"), Floor + 1));
		AMT2DevilTowerRoom* Room = nullptr;
		for (TActorIterator<AMT2DevilTowerRoom> It(World); It; ++It) if (It->RoomId == Id)
		{
			if (Room) return 1; Room = *It;
		}
		if (!Room)
		{
			Room = World->SpawnActor<AMT2DevilTowerRoom>();
			if (!Room) return 1;
			Room->SetPackageExternal(true);
		}
		Room->Modify(); Room->RoomId = Id; Room->Floor = Floor + 1;
		Room->SetActorLabel(Id.ToString()); Room->SetIsSpatiallyLoaded(false);
		Room->SetActorLocation(Plans[Floor].Mesh->Bounds.Origin);
		Room->Bounds->SetBoxExtent(Plans[Floor].Mesh->Bounds.BoxExtent + FVector(500,500,500));
		Room->Entrance->SetWorldLocation(Plans[Floor].Entrance);
		Room->EncounterSeconds = 0;
		Room->TransitionSeconds = Floor == 8 ? 60.f : Floor == 3 ? 5.f : Floor < 3 && Floor != 0 ? 4.f : 6.f;
		Room->Spawns = Plans[Floor].Spawns;
		for (auto& Spawn : Room->Spawns) Spawn.LocalTransform = Spawn.LocalTransform.GetRelativeTransform(Room->GetActorTransform());
		Room->PuzzleSpawns = Plans[Floor].Puzzle; Room->RegenSpawns = Plans[Floor].Regen; Room->LegacyMessages = Messages;
		for (auto* Wave : { &Room->PuzzleSpawns, &Room->RegenSpawns })
			for (auto& Spawn : *Wave) Spawn.LocalTransform = Spawn.LocalTransform.GetRelativeTransform(Room->GetActorTransform());
		Room->MarkPackageDirty(); Packages.AddUnique(Room->GetPackage()); Rooms.Add(Room);
	}
	for (int32 Index = 0; Index < Rooms.Num(); ++Index) Rooms[Index]->NextRoom = Index < 8 ? Rooms[Index+1] : nullptr;
	for (TActorIterator<AMT2MobSpawnActor> It(World); It; ++It)
	{
		auto Entries = It->GetMobSpawnComponent()->GetSpawnEntries();
		if (Entries.RemoveAll([](const FMT2MobSpawnEntry& Entry) { return Entry.SourceVnum == 8015 && Entry.Type == EMT2MobSpawnType::Mob; }) > 0)
		{
			It->Modify(); It->GetMobSpawnComponent()->SetSpawnEntries(Entries); It->MarkPackageDirty(); Packages.AddUnique(It->GetPackage());
		}
	}
	Packages.AddUnique(World->GetPackage());
	if (!UEditorLoadingAndSavingUtils::SavePackages(Packages, true)) return 1;
	UE_LOG(LogTemp, Display, TEXT("Saved all nine shared Devil Tower stages."));
	return 0;
}
