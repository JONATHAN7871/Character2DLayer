#include "ManualSpriteMeshGenerator.h"
#include "ManualSpriteMeshGeneratorDialog.h"
#include "ManualSpriteMeshFactories.h"

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
#include "Materials/MaterialExpressionTextureSample.h"
#include "MaterialDomain.h"

#include "StaticToSkeletalMeshConverter.h"
#include "ReferenceSkeleton.h"

#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"

#include "Logging/LogMacros.h"

void ManualSpriteMeshGenerator::ShowMeshGenerationDialog(UManualSprite* ManualSprite)
{
    if (!ManualSprite)
    {
        UE_LOG(LogTemp, Error, TEXT("ManualSpriteMeshGenerator: ManualSprite is null"));
        return;
    }

    TSharedRef<SWindow> MeshGeneratorWindow = SNew(SWindow)
        .Title(FText::FromString(FString::Printf(TEXT("Generate Mesh - %s"), *ManualSprite->GetName())))
        .ClientSize(FVector2D(500, 700))
        .SupportsMinimize(false)
        .SupportsMaximize(false)
        .IsTopmostWindow(true);

    TSharedRef<SManualSpriteMeshGeneratorDialog> Dialog = SNew(SManualSpriteMeshGeneratorDialog)
        .ManualSprite(ManualSprite);

    MeshGeneratorWindow->SetContent(Dialog);

    FSlateApplication::Get().AddModalWindow(MeshGeneratorWindow, FGlobalTabmanager::Get()->GetRootWindow());
}

bool ManualSpriteMeshGenerator::GenerateMeshFromSprite(UManualSprite* ManualSprite, const FManualSpriteMeshGenerationParams& Params)
{
    if (!ManualSprite || !ManualSprite->bUseManualGeometry || !ManualSprite->IsManualGeometryValid())
    {
        UE_LOG(LogTemp, Error, TEXT("ManualSpriteMeshGenerator: Invalid Manual Sprite"));
        return false;
    }

    // Создаем MeshDescription
    FMeshDescription MeshDesc;
    if (!CreateMeshDescriptionFromSprite(ManualSprite, Params, MeshDesc))
    {
        UE_LOG(LogTemp, Error, TEXT("ManualSpriteMeshGenerator: Failed to create MeshDescription"));
        return false;
    }

    IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();

    // Генерируем уникальное имя ассета
    FString FinalAssetPath, FinalAssetName;
    AssetTools.CreateUniqueAssetName(Params.SavePath / Params.AssetName, TEXT(""), FinalAssetPath, FinalAssetName);

    TArray<UObject*> CreatedAssets;

    if (Params.MeshType == EManualSpriteMeshType::StaticMesh)
    {
        // Создаем StaticMesh
        UStaticMesh* StaticMesh = CreateStaticMesh(MeshDesc, FinalAssetPath, FinalAssetName);
        if (!StaticMesh)
        {
            UE_LOG(LogTemp, Error, TEXT("ManualSpriteMeshGenerator: Failed to create StaticMesh"));
            return false;
        }

        // Создаем материал если нужно
        if (Params.bCreateMaterial && ManualSprite->GetSourceTexture())
        {
            FString MaterialPath, MaterialName;
            AssetTools.CreateUniqueAssetName(FinalAssetPath, TEXT("_Mat"), MaterialPath, MaterialName);
            
            UMaterialInterface* Material = CreateSpriteMaterial(
                ManualSprite->GetSourceTexture(), 
                MaterialPath, 
                MaterialName, 
                Params.bCreateUnlitMaterial, 
                Params.bTwoSidedMaterial
            );

            if (Material)
            {
                // Применяем материал к мешу
                TArray<FStaticMaterial> Materials;
                Materials.Add(FStaticMaterial(Material, FName("Material_0"), FName("Default")));
                StaticMesh->SetStaticMaterials(Materials);
                StaticMesh->Build();

                CreatedAssets.Add(Material);
            }
        }

        CreatedAssets.Add(StaticMesh);
    }
    else // SkeletalMesh
    {
        // Сначала создаем временный StaticMesh
        UStaticMesh* TempStaticMesh = CreateStaticMesh(MeshDesc, FinalAssetPath, FinalAssetName + TEXT("_Temp"));
        if (!TempStaticMesh)
        {
            UE_LOG(LogTemp, Error, TEXT("ManualSpriteMeshGenerator: Failed to create temporary StaticMesh"));
            return false;
        }

        // Вычисляем позицию корневой кости
        FBox MeshBounds = TempStaticMesh->GetBoundingBox();
        FVector RootBonePosition = CalculatePivotPosition(MeshBounds, Params.PivotPlacement, Params.CustomPivotOffset);

        // Создаем Skeleton
        FString SkeletonPath, SkeletonName;
        AssetTools.CreateUniqueAssetName(FinalAssetPath, TEXT("_Skeleton"), SkeletonPath, SkeletonName);
        
        USkeleton* Skeleton = CreateSkeleton(SkeletonPath, SkeletonName, RootBonePosition);
        if (!Skeleton)
        {
            UE_LOG(LogTemp, Error, TEXT("ManualSpriteMeshGenerator: Failed to create Skeleton"));
            return false;
        }

        // Создаем SkeletalMesh
        FString SkeletalMeshPath, SkeletalMeshName;
        AssetTools.CreateUniqueAssetName(FinalAssetPath, TEXT("_SKM"), SkeletalMeshPath, SkeletalMeshName);
        
        USkeletalMesh* SkeletalMesh = CreateSkeletalMesh(TempStaticMesh, SkeletalMeshPath, SkeletalMeshName, RootBonePosition);
        if (!SkeletalMesh)
        {
            UE_LOG(LogTemp, Error, TEXT("ManualSpriteMeshGenerator: Failed to create SkeletalMesh"));
            return false;
        }

        // Создаем материал если нужно
        if (Params.bCreateMaterial && ManualSprite->GetSourceTexture())
        {
            FString MaterialPath, MaterialName;
            AssetTools.CreateUniqueAssetName(SkeletalMeshPath, TEXT("_Mat"), MaterialPath, MaterialName);
            
            UMaterialInterface* Material = CreateSpriteMaterial(
                ManualSprite->GetSourceTexture(), 
                MaterialPath, 
                MaterialName, 
                Params.bCreateUnlitMaterial, 
                Params.bTwoSidedMaterial
            );

            if (Material)
            {
                // Применяем материал к мешу
                TArray<FSkeletalMaterial> Materials;
                Materials.Add(FSkeletalMaterial(Material, FName("Material_0"), FName("Default")));
                SkeletalMesh->SetMaterials(Materials);
                SkeletalMesh->Build();

                CreatedAssets.Add(Material);
            }
        }

        // Связываем Skeleton и SkeletalMesh
        SkeletalMesh->SetSkeleton(Skeleton);
        Skeleton->SetPreviewMesh(SkeletalMesh);
        Skeleton->PostEditChange();
        SkeletalMesh->PostEditChange();

        CreatedAssets.Add(Skeleton);
        CreatedAssets.Add(SkeletalMesh);

        // Удаляем временный StaticMesh
        TempStaticMesh->ConditionalBeginDestroy();
    }

    // Синхронизируем с Content Browser
    if (CreatedAssets.Num() > 0)
    {
        FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser")
            .Get().SyncBrowserToAssets(CreatedAssets);
    }

    UE_LOG(LogTemp, Log, TEXT("ManualSpriteMeshGenerator: Successfully generated %s"), 
           Params.MeshType == EManualSpriteMeshType::StaticMesh ? TEXT("StaticMesh") : TEXT("SkeletalMesh"));

    return true;
}

bool ManualSpriteMeshGenerator::CreateMeshDescriptionFromSprite(UManualSprite* ManualSprite, const FManualSpriteMeshGenerationParams& Params, FMeshDescription& OutMeshDesc)
{
    if (!ManualSprite || !ManualSprite->bUseManualGeometry)
    {
        return false;
    }

    const FManualSpriteGeometry& Geometry = ManualSprite->ManualGeometry;
    if (Geometry.Vertices.Num() < 3 || Geometry.Triangles.Num() < 1)
    {
        return false;
    }

    // Инициализируем MeshDescription
    OutMeshDesc = FMeshDescription();
    FStaticMeshAttributes Attributes(OutMeshDesc);
    Attributes.Register();

    FMeshDescriptionBuilder Builder;
    Builder.SetMeshDescription(&OutMeshDesc);
    Builder.EnablePolyGroups();
    Builder.SetNumUVLayers(1);

    // Создаем группу полигонов
    FPolygonGroupID PolyGroup = Builder.AppendPolygonGroup();
    Attributes.GetPolygonGroupMaterialSlotNames()[PolyGroup] = FName("Material_0");

    // Вычисляем границы для расчета пивота
    FBox SpriteBounds(ForceInit);
    for (const FManualSpriteVertex& Vertex : Geometry.Vertices)
    {
        // Используем инвертированный Y для корректного расчета границ в 3D
        SpriteBounds += FVector(Vertex.Position.X, 0.0f, -Vertex.Position.Y);
    }

    // Вычисляем смещение пивота
    FVector PivotOffset = CalculatePivotPosition(SpriteBounds, Params.PivotPlacement, Params.CustomPivotOffset);

    // Создаем вершины
    TArray<FVertexID> VertexIDs;
    VertexIDs.Reserve(Geometry.Vertices.Num());

    for (const FManualSpriteVertex& SpriteVertex : Geometry.Vertices)
    {
        // Преобразуем 2D координаты в 3D (Y=0, Z вверх) с инверсией Y спрайта
        FVector WorldPosition(
            SpriteVertex.Position.X * Params.MeshScale,
            0.0f,
            -SpriteVertex.Position.Y * Params.MeshScale // <<< ИЗМЕНЕНИЕ 1
        );

        // Применяем смещение пивота и дополнительное смещение
        WorldPosition -= PivotOffset;
        WorldPosition += Params.MeshOffset;

        FVertexID VertexID = Builder.AppendVertex(WorldPosition);
        VertexIDs.Add(VertexID);
    }

    // Определяем корректный базис для нормалей и касательных
    const FVector Normal(0.0f, 1.0f, 0.0f);
    const FVector Tangent(1.0f, 0.0f, 0.0f);
    const FVector Bitangent(0.0f, 0.0f, -1.0f);

    // Создаем треугольники
for (const FManualSpriteTriangle& Triangle : Geometry.Triangles)
{
    // Проверяем валидность индексов
    if (Triangle.VertexIndex0 >= VertexIDs.Num() || 
        Triangle.VertexIndex1 >= VertexIDs.Num() || 
        Triangle.VertexIndex2 >= VertexIDs.Num())
    {
        continue;
    }

    // Получаем 2D-позиции вершин для проверки порядка обхода
    const FManualSpriteVertex& V0 = Geometry.Vertices[Triangle.VertexIndex0];
    const FManualSpriteVertex& V1 = Geometry.Vertices[Triangle.VertexIndex1];
    const FManualSpriteVertex& V2 = Geometry.Vertices[Triangle.VertexIndex2];

    // Вычисляем 2D "векторное произведение", чтобы определить порядок обхода.
    // Положительное значение обычно означает обход против часовой стрелки (CCW).
    const float Winding = (V1.Position.X - V0.Position.X) * (V2.Position.Y - V0.Position.Y) - 
                          (V1.Position.Y - V0.Position.Y) * (V2.Position.X - V0.Position.X);

    // Создаем экземпляры вершин для треугольника
    FVertexInstanceID Instance0 = Builder.AppendInstance(VertexIDs[Triangle.VertexIndex0]);
    FVertexInstanceID Instance1 = Builder.AppendInstance(VertexIDs[Triangle.VertexIndex1]);
    FVertexInstanceID Instance2 = Builder.AppendInstance(VertexIDs[Triangle.VertexIndex2]);

    // Устанавливаем UV с инвертированной V-координатой
    Builder.SetInstanceUV(Instance0, FVector2D(V0.UV.X, 1.0 - V0.UV.Y));
    Builder.SetInstanceUV(Instance1, FVector2D(V1.UV.X, 1.0 - V1.UV.Y));
    Builder.SetInstanceUV(Instance2, FVector2D(V2.UV.X, 1.0 - V2.UV.Y));

    // Устанавливаем нормали и касательные
    Builder.SetInstanceNormal(Instance0, Normal);
    Builder.SetInstanceNormal(Instance1, Normal);
    Builder.SetInstanceNormal(Instance2, Normal);
    Builder.SetInstanceTangentSpace(Instance0, Tangent, Bitangent, 1.0f);
    Builder.SetInstanceTangentSpace(Instance1, Tangent, Bitangent, 1.0f);
    Builder.SetInstanceTangentSpace(Instance2, Tangent, Bitangent, 1.0f);

    // Устанавливаем цвета
    Builder.SetInstanceColor(Instance0, FVector4f(1.0f, 1.0f, 1.0f, 1.0f));
    Builder.SetInstanceColor(Instance1, FVector4f(1.0f, 1.0f, 1.0f, 1.0f));
    Builder.SetInstanceColor(Instance2, FVector4f(1.0f, 1.0f, 1.0f, 1.0f));

    // Создаем треугольник, принудительно задавая правильный порядок обхода.
    // Чтобы в 3D получить обход ПО часовой стрелке (стандарт для UE),
    // нам нужен исходный 2D-обход ПРОТИВ часовой стрелки (так как мы инвертируем ось Z).
    // Winding < 0 теперь будет считаться правильным порядком
    if (Winding < 0) 
    {
        // Этот порядок правильный, он станет нужным нам в 3D.
        Builder.AppendTriangle(Instance0, Instance1, Instance2, PolyGroup);
    }
    else
    {
        // Этот порядок нужно инвертировать, чтобы он стал правильным.
        Builder.AppendTriangle(Instance0, Instance2, Instance1, PolyGroup);
    }
}

    return OutMeshDesc.Vertices().Num() > 0 && OutMeshDesc.Triangles().Num() > 0;
}

UMaterialInterface* ManualSpriteMeshGenerator::CreateSpriteMaterial(UTexture* Texture, const FString& AssetPath, const FString& MaterialName, bool bUnlit, bool bTwoSided)
{
    IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();
    
    UManualSpriteMaterialFactory* Factory = NewObject<UManualSpriteMaterialFactory>();
    Factory->SourceTexture = Texture;
    Factory->bUnlitMaterial = bUnlit;
    Factory->bTwoSided = bTwoSided;

    UMaterial* Material = Cast<UMaterial>(AssetTools.CreateAsset(
        MaterialName,
        FPackageName::GetLongPackagePath(AssetPath),
        UMaterial::StaticClass(),
        Factory
    ));

    if (Material)
    {
        Material->PostEditChange();
        FAssetRegistryModule::AssetCreated(Material);
    }

    return Material;
}

UStaticMesh* ManualSpriteMeshGenerator::CreateStaticMesh(const FMeshDescription& MeshDesc, const FString& AssetPath, const FString& MeshName)
{
    UPackage* Package = CreatePackage(*AssetPath);
    if (!Package)
    {
        return nullptr;
    }

    UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, *MeshName, RF_Public | RF_Standalone);
    
    if (!StaticMesh->IsSourceModelValid(0))
    {
        StaticMesh->AddSourceModel();
    }

    FStaticMeshSourceModel& SourceModel = StaticMesh->GetSourceModel(0);
    SourceModel.BuildSettings.bRecomputeNormals = true;
    SourceModel.BuildSettings.bRecomputeTangents = true;
    SourceModel.BuildSettings.bRemoveDegenerates = false;
    SourceModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
    SourceModel.BuildSettings.bUseFullPrecisionUVs = true;

    StaticMesh->CreateMeshDescription(0, MeshDesc);
    
    UStaticMesh::FCommitMeshDescriptionParams CommitParams;
    CommitParams.bMarkPackageDirty = true;
    CommitParams.bUseHashAsGuid = true;
    StaticMesh->CommitMeshDescription(0, CommitParams);

    StaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;
    StaticMesh->Build();
    StaticMesh->PostEditChange();

    FAssetRegistryModule::AssetCreated(StaticMesh);

    return StaticMesh;
}

USkeletalMesh* ManualSpriteMeshGenerator::CreateSkeletalMesh(UStaticMesh* StaticMesh, const FString& AssetPath, const FString& MeshName, const FVector& RootBonePosition)
{
    if (!StaticMesh)
    {
        return nullptr;
    }

    IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();
    
    // Создаем Skeleton если его еще нет
    FString SkeletonPath, SkeletonName;
    AssetTools.CreateUniqueAssetName(AssetPath, TEXT("_Skeleton"), SkeletonPath, SkeletonName);
    
    USkeleton* Skeleton = CreateSkeleton(SkeletonPath, SkeletonName, RootBonePosition);
    if (!Skeleton)
    {
        return nullptr;
    }

    UManualSpriteSkeletalMeshFactory* Factory = NewObject<UManualSpriteSkeletalMeshFactory>();
    Factory->SourceStaticMesh = StaticMesh;
    Factory->TargetSkeleton = Skeleton;
    Factory->RootBoneName = TEXT("Root");

    USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(AssetTools.CreateAsset(
        MeshName,
        FPackageName::GetLongPackagePath(AssetPath),
        USkeletalMesh::StaticClass(),
        Factory
    ));

    if (SkeletalMesh)
    {
        // Настройка LOD
        const int32 NumLODs = SkeletalMesh->GetLODNum();
        for (int32 i = 0; i < NumLODs; ++i)
        {
            if (FSkeletalMeshLODInfo* LODInfo = SkeletalMesh->GetLODInfo(i))
            {
                LODInfo->BuildSettings.bUseFullPrecisionUVs = true;
                LODInfo->BuildSettings.bRemoveDegenerates = false;
                LODInfo->BuildSettings.bRecomputeNormals = true;
                LODInfo->BuildSettings.bRecomputeTangents = true;
            }
        }

        SkeletalMesh->Build();
        SkeletalMesh->PostEditChange();
        FAssetRegistryModule::AssetCreated(SkeletalMesh);
    }

    return SkeletalMesh;
}

USkeleton* ManualSpriteMeshGenerator::CreateSkeleton(const FString& AssetPath, const FString& SkeletonName, const FVector& RootBonePosition)
{
    IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();
    
    UManualSpriteSkeletonFactory* Factory = NewObject<UManualSpriteSkeletonFactory>();
    Factory->RootBonePosition = RootBonePosition;

    USkeleton* Skeleton = Cast<USkeleton>(AssetTools.CreateAsset(
        SkeletonName,
        FPackageName::GetLongPackagePath(AssetPath),
        USkeleton::StaticClass(),
        Factory
    ));

    if (Skeleton)
    {
        Skeleton->PostEditChange();
        FAssetRegistryModule::AssetCreated(Skeleton);
    }

    return Skeleton;
}

FVector ManualSpriteMeshGenerator::CalculatePivotPosition(const FBox& MeshBounds, EManualSpritePivotPlacement PivotPlacement, const FVector& CustomOffset)
{
    switch (PivotPlacement)
    {
    case EManualSpritePivotPlacement::Origin:
        return FVector::ZeroVector;
        
    case EManualSpritePivotPlacement::Center:
        return MeshBounds.GetCenter();
        
    case EManualSpritePivotPlacement::BottomCenter:
        return FVector(MeshBounds.GetCenter().X, MeshBounds.GetCenter().Y, MeshBounds.Min.Z);
        
    case EManualSpritePivotPlacement::Custom:
        return CustomOffset;
        
    default:
        return FVector::ZeroVector;
    }
}