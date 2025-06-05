// ============================================================================
// Character2DMeshGenerator.cpp   (UE 5.5 ready)
// ============================================================================

#include "Character2DBuilderWindow/Character2DMeshGenerator.h"
#include "Character2DBuilderWindow/Character2DMeshGeneratorOptions.h"
#include "Character2DBuilderWindow/Character2DMeshFactories.h"

#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Modules/ModuleManager.h"

#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "StaticMeshAttributes.h"
#include "MeshDescriptionBuilder.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "Materials/Material.h"
#include "MaterialDomain.h"

#include "PaperSprite.h"
#include "Engine/Texture2D.h"

#include "Logging/LogMacros.h"

// ─────────────────────────────────────────────────────────────────────────────
// helper-структура
// ─────────────────────────────────────────────────────────────────────────────
struct FSpriteEntry
{
	UPaperSprite* Sprite         = nullptr;
	FVector        Offset         = FVector::ZeroVector;
	bool           bUseGridMesh   = true;
	int32          GridCellSize   = 32;
	uint8          AlphaThreshold = 64;
};

// ─────────────────────────────────────────────────────────────────────────────
// Character2DMeshGenerator  – util-методы
// ─────────────────────────────────────────────────────────────────────────────
bool Character2DMeshGenerator::IsTextureFormatSupported(UTexture2D* Texture)
{
	return Texture &&
	       Texture->GetPlatformData() &&
	       Texture->GetPlatformData()->PixelFormat == PF_B8G8R8A8;
}

// ─────────────────────────────────────────────────────────────────────────────
// Grid-меш из спрайта
// ─────────────────────────────────────────────────────────────────────────────
bool Character2DMeshGenerator::GenerateGridMeshFromSprite(
	UPaperSprite*            Sprite,
	const FVector&           Offset,
	const FPolygonGroupID&   Group,
	FMeshDescriptionBuilder& Builder,
	TArray<FVector2D>&       /*OutUVs*/,
	int32                    CellSize,
	uint8                    AlphaThreshold,
	float                    MeshScale)
{
	if (!IsValid(Sprite) || !IsValid(Sprite->GetSourceTexture()))
		return false;

	UTexture2D* Texture = Sprite->GetSourceTexture();
	if (!IsTextureFormatSupported(Texture))
		return false;

	const FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	const int32 Width  = Mip.SizeX;
	const int32 Height = Mip.SizeY;

	const void* Data = Mip.BulkData.LockReadOnly();
	if (!Data) return false;
	ON_SCOPE_EXIT { Mip.BulkData.Unlock(); };

	const FColor* Colors = static_cast<const FColor*>(Data);

	const FVector2D SourceUV  = Sprite->GetSourceUV();
	const FVector2D SourceDim = Sprite->GetSourceSize();
	const float CenterX = SourceDim.X * .5f;
	const float BottomZ = SourceDim.Y * .5f;

	TMap<FIntPoint,FVertexID> Vertices;

	for (int32 Y = 0; Y <= Height - CellSize; Y += CellSize)
	for (int32 X = 0; X <= Width  - CellSize; X += CellSize)
	{
		const uint8 A0 = Colors[Y*Width + X].A;
		const uint8 A1 = Colors[Y*Width + (X+CellSize)].A;
		const uint8 A2 = Colors[(Y+CellSize)*Width + X].A;
		const uint8 A3 = Colors[(Y+CellSize)*Width + (X+CellSize)].A;
		const uint8 A4 = Colors[(Y+CellSize/2)*Width + (X+CellSize/2)].A;

		const uint8 AlphaMax = FMath::Max( FMath::Max(A0,A1),
		                                   FMath::Max(FMath::Max(A2,A3),A4) );
		if (AlphaMax <= AlphaThreshold)
			continue;

		const FVector2D P[4] =
		{
			{ (float)X,           (float)Y },
			{ (float)(X+CellSize), (float)Y },
			{ (float)X,           (float)(Y+CellSize) },
			{ (float)(X+CellSize), (float)(Y+CellSize) }
		};

		FVertexInstanceID Inst[4];
		for (int32 i=0;i<4;++i)
		{
			const FIntPoint Key{ (int32)P[i].X,(int32)P[i].Y };
			if (!Vertices.Contains(Key))
			{
				const float LocalY   = P[i].Y - SourceUV.Y;
				const float FlippedY = SourceDim.Y - LocalY;

				const FVector Pos(
					(P[i].X - CenterX) * MeshScale + Offset.X,
					 Offset.Z,
					(FlippedY - BottomZ) * MeshScale + Offset.Y);

				Vertices.Add(Key, Builder.AppendVertex(Pos));
			}
			Inst[i] = Builder.AppendInstance(Vertices[Key]);

			Builder.SetInstanceNormal (Inst[i], FVector(0,1,0));
			Builder.SetInstanceColor  (Inst[i], FVector4f(1.f));
			Builder.SetInstanceUV     (Inst[i],
				( (P[i]-SourceUV) / SourceDim ).ClampAxes(0.f,1.f) );
		}
		Builder.AppendTriangle(Inst[0],Inst[2],Inst[1],Group);
		Builder.AppendTriangle(Inst[1],Inst[2],Inst[3],Group);
	}
	return Vertices.Num() > 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// BuildMeshDescriptionAndTextures
// ─────────────────────────────────────────────────────────────────────────────
void Character2DMeshGenerator::BuildMeshDescriptionAndTextures(
	const TArray<TSharedPtr<FCharacter2DLayerCategory>>& Categories,
	FMeshDescription&            OutDesc,
	TArray<UTexture*>&           OutTextures,
	TArray<UPaperSprite*>&       OutUniqueSprites,
	const FCharacter2DMeshGenerationOptions& Options)
{
	OutDesc = FMeshDescription();
	OutTextures.Reset();
	OutUniqueSprites.Reset();

	// --- собираем валидные спрайты
	TArray<FSpriteEntry> Entries;
	for (const auto& Cat : Categories)
	for (const auto& Slot: Cat->Slots)
	{
		if (!Slot->bVisible || !Slot->Sprite.IsValid())
			continue;

		UTexture2D* Tex = Slot->Sprite->GetSourceTexture();
		if (Cat->bUseGridMesh && !IsTextureFormatSupported(Tex))
			continue;

		Entries.Emplace(FSpriteEntry{
			Slot->Sprite.Get(),
			Slot->Location,
			Cat->bUseGridMesh,
			Cat->GridCellSize,
			Cat->AlphaThreshold});
	}
	if (Entries.IsEmpty()) return;

	// --- готовим MeshDescription
	FStaticMeshAttributes Attr(OutDesc); Attr.Register();
	FMeshDescriptionBuilder Bld; Bld.SetMeshDescription(&OutDesc);
	Bld.EnablePolyGroups(); Bld.SetNumUVLayers(1);

	TMap<UPaperSprite*,FPolygonGroupID> GroupBySprite;

	// группы + сбор текстур
	for (const FSpriteEntry& E : Entries)
	{
		if (!GroupBySprite.Contains(E.Sprite))
		{
			FPolygonGroupID NewGroup = Bld.AppendPolygonGroup();

			// ► имя слота задаём напрямую в OutDesc
			FStaticMeshAttributes MeshAttr(OutDesc);
			MeshAttr.GetPolygonGroupMaterialSlotNames()[NewGroup] = E.Sprite->GetFName();

			GroupBySprite.Add(E.Sprite, NewGroup);
			OutUniqueSprites.Add(E.Sprite);
		}

		if (UTexture* T = E.Sprite->GetSourceTexture())
			OutTextures.AddUnique(T);
	}


	// генерация треугольников
	for (const FSpriteEntry& E : Entries)
	{
		const FPolygonGroupID Group = GroupBySprite[E.Sprite];

		if (E.bUseGridMesh)
		{
			TArray<FVector2D> DummyUV;
			GenerateGridMeshFromSprite(E.Sprite,E.Offset,Group,Bld,
				DummyUV,E.GridCellSize,E.AlphaThreshold,Options.MeshScale);
			continue;
		}

		const TArray<FVector4>& V = E.Sprite->BakedRenderData;
		for (int32 i=0;i<V.Num();i+=3)
		{
			FVertexInstanceID I[3];
			for (int32 k=0;k<3;++k)
			{
				const FVector4& XYUV = V[i+k];
				const FVector Pos(
					XYUV.X*Options.MeshScale + E.Offset.X,
					 E.Offset.Z,
					XYUV.Y*Options.MeshScale + E.Offset.Y);

				I[k] = Bld.AppendInstance(Bld.AppendVertex(Pos));
				Bld.SetInstanceNormal(I[k],FVector(0,1,0));
				Bld.SetInstanceTangentSpace(I[k],FVector(1,0,0),FVector(0,0,1),1.f);
				Bld.SetInstanceUV(I[k], FVector2D(XYUV.Z,XYUV.W));
				Bld.SetInstanceColor(I[k],FVector4f(1.f));
			}
			Bld.AppendTriangle(I[0],I[1],I[2],Group);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// вспомогательные функции (материал, синхронизация)
// ─────────────────────────────────────────────────────────────────────────────
static UMaterialInterface* CreateSpriteMaterial(
	UTexture*        Texture,
	IAssetTools&     AssetTools,
	const FString&   BasePkgPath,
	int32            Index)
{
	UCharacter2D_MaterialFactory* Factory = NewObject<UCharacter2D_MaterialFactory>();
	Factory->InitialTexture = Texture;

	FString MatPkg,MatName;
	AssetTools.CreateUniqueAssetName(BasePkgPath,
		FString::Printf(TEXT("_Mat%d"),Index),MatPkg,MatName);

	return Cast<UMaterial>(AssetTools.CreateAsset(
		MatName,
		FPackageName::GetLongPackagePath(MatPkg),
		UMaterial::StaticClass(),Factory));
}

static void SyncToAssets(const TArray<UObject*>& Objects)
{
	FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser")
		.Get().SyncBrowserToAssets(Objects);
}

// ─────────────────────────────────────────────────────────────────────────────
// GenerateMeshFromOptions  (Static / Skeletal)
// ─────────────────────────────────────────────────────────────────────────────
void Character2DMeshGenerator::GenerateMeshFromOptions(
	const TArray<TSharedPtr<FCharacter2DLayerCategory>>& Categories,
	const FCharacter2DMeshGenerationOptions& Options)
{
	if (Categories.IsEmpty()) return;

	IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();

	// 1) MeshDescription
	FMeshDescription   MeshDesc;
	TArray<UTexture*>  Textures;
	TArray<UPaperSprite*> UniqueSprites; 
	TArray<UPaperSprite*> Dummy;
	BuildMeshDescriptionAndTextures(Categories,MeshDesc, Textures, UniqueSprites, Options);
	if (MeshDesc.Polygons().Num() == 0) return;

	// 2) смещение по Pivot
	{
		FStaticMeshAttributes A(MeshDesc);
		auto Pos = A.GetVertexPositions();

		FBox3f B(ForceInit);
		for (FVertexID V : MeshDesc.Vertices().GetElementIDs())
			B += Pos[V];

		FVector3f Pivot(ForceInit);
		switch (Options.PivotPlacement)
		{
		case ECharacter2DRootBonePlacement::Center:       Pivot = B.GetCenter(); break;
		case ECharacter2DRootBonePlacement::BottomCenter: Pivot = FVector3f(B.GetCenter().X,B.GetCenter().Y,B.Min.Z); break;
		case ECharacter2DRootBonePlacement::Origin:       Pivot = FVector3f::ZeroVector; break;
		}
		for (FVertexID V : MeshDesc.Vertices().GetElementIDs())
			Pos[V] -= Pivot;
	}

	// 3) пакет
	FString PkgPath,AssetName;
	AssetTools.CreateUniqueAssetName(Options.SavePath / Options.AssetName,TEXT(""),PkgPath,AssetName);
	UPackage* Pkg = CreatePackage(*PkgPath);

	// ===================================================================
	// ----------------------  STATIC  MESH  -----------------------------
	// ===================================================================
	if (Options.OutputType == ECharacter2DMeshOutputType::StaticMesh)
	{
		UStaticMesh* Mesh = NewObject<UStaticMesh>(Pkg,*AssetName,RF_Public|RF_Standalone);
		if (!Mesh->IsSourceModelValid(0)) Mesh->AddSourceModel();

		FStaticMeshSourceModel& SM = Mesh->GetSourceModel(0);
		SM.BuildSettings.bRecomputeNormals             = true;
		SM.BuildSettings.bRecomputeTangents            = true;
		SM.BuildSettings.bRemoveDegenerates            = false;
		SM.BuildSettings.bUseHighPrecisionTangentBasis = false;
		SM.BuildSettings.bUseFullPrecisionUVs          = true;

		Mesh->CreateMeshDescription(0,MeshDesc);
		UStaticMesh::FCommitMeshDescriptionParams P; P.bMarkPackageDirty=true; P.bUseHashAsGuid=true;
		Mesh->CommitMeshDescription(0,P);
		Mesh->ImportVersion = EImportStaticMeshVersion::LastVersion;

		TArray<FStaticMaterial> StaticMats;
		const int32 NumPG = MeshDesc.PolygonGroups().Num();

		for (int32 i = 0; i < NumPG; ++i)
		{
			UMaterialInterface* Mat = (i < Textures.Num())
				? CreateSpriteMaterial(Textures[i], AssetTools, PkgPath, i)
				: UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface);

			const FName SlotName = UniqueSprites.IsValidIndex(i)
				? UniqueSprites[i]->GetFName()
				: FName(*FString::Printf(TEXT("Slot_%d"), i));

			StaticMats.Add( FStaticMaterial(Mat, SlotName, FName(TEXT("Imported"))) );
		}

		Mesh->SetStaticMaterials(StaticMats);

		Mesh->Build(); Mesh->PostEditChange(); (void)Mesh->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Mesh);

		SyncToAssets({Mesh});
		return;
	}

	// ===================================================================
	// ----------------------  SKELETAL  MESH ----------------------------
	// ===================================================================
	// 3.1 temp StaticMesh
	UStaticMesh* Temp = NewObject<UStaticMesh>(GetTransientPackage(),NAME_None,RF_Transient);
	if (!Temp->IsSourceModelValid(0)) Temp->AddSourceModel();

	FStaticMeshSourceModel& TM = Temp->GetSourceModel(0);
	TM.BuildSettings.bRecomputeNormals             = true;
	TM.BuildSettings.bRecomputeTangents            = true;
	TM.BuildSettings.bRemoveDegenerates            = false;
	TM.BuildSettings.bUseHighPrecisionTangentBasis = false;
	TM.BuildSettings.bUseFullPrecisionUVs          = true;

	Temp->CreateMeshDescription(0,MeshDesc);
	UStaticMesh::FCommitMeshDescriptionParams P; P.bMarkPackageDirty=false; P.bUseHashAsGuid=false;
	Temp->CommitMeshDescription(0,P);
	Temp->Build();

	// 3.2 Skeleton
	FString SkelPkg,SkelName;
	AssetTools.CreateUniqueAssetName(Options.SavePath/Options.AssetName,TEXT("_Skeleton"),SkelPkg,SkelName);

	UCharacter2D_SkeletonFactory* SkelFactory = NewObject<UCharacter2D_SkeletonFactory>();
	SkelFactory->StaticMesh = Temp;
	switch (Options.PivotPlacement)
	{
	case ECharacter2DRootBonePlacement::Center:       SkelFactory->RootPosition=FVector(.5,.5,.5); SkelFactory->PositionReference=ECharacter2D_RootBoneReference::Relative; break;
	case ECharacter2DRootBonePlacement::BottomCenter: SkelFactory->RootPosition=FVector(.5,.5,0 ); SkelFactory->PositionReference=ECharacter2D_RootBoneReference::Relative; break;
	case ECharacter2DRootBonePlacement::Origin:       SkelFactory->RootPosition=FVector::ZeroVector; SkelFactory->PositionReference=ECharacter2D_RootBoneReference::Absolute; break;
	}

	USkeleton* Skeleton = Cast<USkeleton>(AssetTools.CreateAsset(
		SkelName,
		FPackageName::GetLongPackagePath(SkelPkg),
		USkeleton::StaticClass(),SkelFactory));
	if (!Skeleton){ UE_LOG(LogTemp,Error,TEXT("Skeleton failed")); return; }

	// 3.3 SkeletalMesh
	FString SkmPkg,SkmName;
	AssetTools.CreateUniqueAssetName(Options.SavePath/Options.AssetName,TEXT("_SKM"),SkmPkg,SkmName);

	UCharacter2D_SkeletalMeshFactory* SkmFactory = NewObject<UCharacter2D_SkeletalMeshFactory>();
	SkmFactory->StaticMesh        = Temp;
	SkmFactory->ReferenceSkeleton = Skeleton->GetReferenceSkeleton();
	SkmFactory->Skeleton          = Skeleton;
	SkmFactory->BindBoneName      = TEXT("Root");

	USkeletalMesh* SkelMesh = Cast<USkeletalMesh>(AssetTools.CreateAsset(
		SkmName,
		FPackageName::GetLongPackagePath(SkmPkg),
		USkeletalMesh::StaticClass(),SkmFactory));
	if (!SkelMesh){ UE_LOG(LogTemp,Error,TEXT("SKM failed")); return; }

	// 3.4 LOD build-settings
	const int32 NumLODs = SkelMesh->GetLODNum();
	for (int32 i=0;i<NumLODs;++i)
	{
		if (FSkeletalMeshLODInfo* L = SkelMesh->GetLODInfo(i))
		{
			L->BuildSettings.bUseFullPrecisionUVs = true;
			L->BuildSettings.bRemoveDegenerates   = false;
			L->BuildSettings.bRecomputeNormals    = true;
			L->BuildSettings.bRecomputeTangents   = true;
		}
	}
	SkelMesh->Build();

	// 3.5 материалы
	TArray<FSkeletalMaterial> SMat;

	for (int32 i = 0; i < Textures.Num(); ++i)
	{
		UMaterialInterface* Mat = CreateSpriteMaterial(Textures[i],
									   AssetTools, SkmPkg, i);

		const FName SlotName = UniqueSprites.IsValidIndex(i)
			? UniqueSprites[i]->GetFName()
			: FName(*FString::Printf(TEXT("Slot_%d"), i));

		SMat.Add( FSkeletalMaterial(Mat, SlotName, FName(TEXT("Imported"))) );
	}

	if (SMat.IsEmpty())
	{
		SMat.Add( FSkeletalMaterial(
			UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface)) );
	}

	SkelMesh->SetMaterials(SMat);

	// финал
	SkelMesh->PostEditChange(); (void)SkelMesh->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(SkelMesh);
	Skeleton->SetPreviewMesh(SkelMesh);

	SyncToAssets({SkelMesh,Skeleton});
}

// ─────────────────────────────────────────────────────────────────────────────
// ShowMeshGenerationDialog  (без диалога)
// ─────────────────────────────────────────────────────────────────────────────
void Character2DMeshGenerator::ShowMeshGenerationDialog(
	const TArray<TSharedPtr<FCharacter2DLayerCategory>>& Categories)
{
	if (auto* Cfg = GetMutableDefault<UCharacter2DMeshGeneratorOptions>())
	{
		FCharacter2DMeshGenerationOptions Opt;
		Opt.MeshScale      = Cfg->MeshScale;
		Opt.OutputType     = Cfg->OutputType;
		Opt.PivotPlacement = Cfg->PivotPlacement;
		Opt.AssetName      = Cfg->AssetName;
		Opt.SavePath       = Cfg->SavePath.Path;
		GenerateMeshFromOptions(Categories,Opt);
	}
}