// SCharacter2DPreviewViewport.cpp

#include "Character2DBuilderWindow/Slate/SCharacter2DPreviewViewport.h"
#include "Character2DBuilderWindow/AssetData/Character2DLayerData.h"

#include "EditorViewportClient.h"
#include "MaterialDomain.h"
#include "PaperSpriteComponent.h"
#include "PreviewScene.h"
#include "Character2DBuilderWindow/Character2DMeshGenerator.h"
#include "Character2DBuilderWindow/Character2DMeshGeneratorOptions.h"
#include "Engine/World.h"

/////////////////////////////////////////////////////
// Клиент вьюпорта: настраивает камеру и режим рендеринга
/////////////////////////////////////////////////////
class FCharacter2DPreviewViewportClient : public FEditorViewportClient
{
public:
    explicit FCharacter2DPreviewViewportClient(FPreviewScene* InPreviewScene)
        : FEditorViewportClient(nullptr, InPreviewScene)
    {
        // Ортографический режим, смотрим сверху по XY
        FEditorViewportClient::SetViewportType(LVT_Perspective);
        FEditorViewportClient::SetViewMode(VMI_Unlit);
        
        // Позиция камеры: X=0, Y=1200, Z=200
        SetViewLocation(FVector(0.0f, 1200.0f, 200.0f));
        // Поворачиваем на −90° по Yaw, чтобы «смотреть» вниз по Z
        SetViewRotation(FRotator(0.0f, -90.0f, 0.0f));

        // Включаем real-time, чтобы Invalidate() сразу перерисовывал
        SetRealtime(true);
        
    }
};

/////////////////////////////////////////////////////
// SCharacter2DPreviewViewport
/////////////////////////////////////////////////////

void SCharacter2DPreviewViewport::Construct(const FArguments& InArgs)
{
    PreviewScene = MakeUnique<FPreviewScene>(FPreviewScene::ConstructionValues());

    UWorld* World = PreviewScene->GetWorld();
    PivotArrow = NewObject<UArrowComponent>(World, NAME_None, RF_Transient);
    PivotArrow->ArrowSize = 0.5f;      // визуальный размер стрелки
    PivotArrow->bIsScreenSizeScaled = true;
    PivotArrow->RegisterComponentWithWorld(World);

    PreviewMeshComp = NewObject<UStaticMeshComponent>(GetPreviewWorld());
    PreviewMeshComp->SetMobility(EComponentMobility::Movable);
    PreviewMeshComp->bCastDynamicShadow = false;
    PreviewMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PreviewScene->AddComponent(PreviewMeshComp, FTransform::Identity);
    
    PreviewScene->AddComponent(PivotArrow, FTransform::Identity);
    
    // Стандартная инициализация SEditorViewport
    SEditorViewport::Construct(SEditorViewport::FArguments());
}

SCharacter2DPreviewViewport::~SCharacter2DPreviewViewport() = default;

TSharedRef<FEditorViewportClient> SCharacter2DPreviewViewport::MakeEditorViewportClient()
{
    ViewportClient = MakeShared<FCharacter2DPreviewViewportClient>(PreviewScene.Get());
    // Возвращаем как базовый FEditorViewportClient
    return StaticCastSharedRef<FEditorViewportClient>(ViewportClient.ToSharedRef());
}

UWorld* SCharacter2DPreviewViewport::GetPreviewWorld() const
{
    return PreviewScene.IsValid() ? PreviewScene->GetWorld() : nullptr;
}

void SCharacter2DPreviewViewport::EnsureComponentPoolSize(int32 Count)
{
    UWorld* World = GetPreviewWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("Preview world is invalid!"));
        return;
    }

    while (ComponentPool.Num() < Count)
    {
        UPaperSpriteComponent* NewComp =
            NewObject<UPaperSpriteComponent>(World, NAME_None, RF_Transient);
        NewComp->SetComponentTickEnabled(false);
        NewComp->SetCastShadow(false);
        NewComp->bReceivesDecals = false;
        NewComp->RegisterComponentWithWorld(World);
        PreviewScene->AddComponent(NewComp, FTransform::Identity);
        ComponentPool.Add(NewComp);
    }
}

void SCharacter2DPreviewViewport::UpdatePreviewSprites(
    const TArray<TSharedPtr<FCharacter2DLayerCategory>>& Categories,
    float InPreviewScale)
{
    PreviewScale = InPreviewScale;

    // 1) Считаем, сколько спрайтов нужно отобразить
    int32 RequiredCount = 0;
    for (const auto& Cat : Categories)
    {
        for (const auto& Slot : Cat->Slots)
        {
            if (Slot->bVisible && Slot->Sprite.IsValid())
            {
                ++RequiredCount;
            }
        }
    }

    // 2) Увеличиваем пул компонентов, если нужно
    EnsureComponentPoolSize(RequiredCount);

    // 3) Скрываем все
    for (UPaperSpriteComponent* Comp : ComponentPool)
    {
        Comp->SetVisibility(false);
    }

    // 4) Заполняем первые RequiredCount компонентов
    ActiveComponents.Reset();
    int32 Index = 0;
    for (const auto& Cat : Categories)
    {
        for (const auto& Slot : Cat->Slots)
        {
            if (!Slot->bVisible || !Slot->Sprite.IsValid())
            {
                continue;
            }

            UPaperSpriteComponent* Comp = ComponentPool[Index++];
            Comp->SetSprite(Slot->Sprite.Get());

            Comp->SetRelativeLocation(FVector(
                Slot->Location.X,
                Slot->Location.Z,
                Slot->Location.Y
            ));
            Comp->SetRelativeScale3D(FVector(PreviewScale));

            Comp->BoundsScale = 2.0f * PreviewScale;
            Comp->RecreateRenderState_Concurrent();
            Comp->SetVisibility(true);

            ActiveComponents.Add(Comp);
            if (Index >= RequiredCount)
            {
                break;
            }
        }
    }

    /* ── вычисляем pivot так же, как при экспорте ── */
    FBox Bounds(ForceInit);
    for (const auto* Comp : ActiveComponents)
        Bounds += Comp->Bounds.Origin;

    FVector Pivot = FVector::ZeroVector;
    switch (GetDefault<UCharacter2DMeshGeneratorOptions>()->PivotPlacement)
    {
    case ECharacter2DRootBonePlacement::Center:        Pivot = Bounds.GetCenter();                    break;
    case ECharacter2DRootBonePlacement::BottomCenter:  Pivot = FVector(Bounds.GetCenter().X,
                                                                        Bounds.GetCenter().Y,
                                                                        Bounds.Min.Z);                 break;
    case ECharacter2DRootBonePlacement::Origin:        Pivot = FVector::ZeroVector;                   break;
    }
    PivotArrow->SetWorldLocation(Pivot);
    UpdatePreviewMeshDescription(Categories, PreviewScale);
    PivotArrow->SetVisibility(true);
    
    // 5) Обновляем вьюпорт
    if (ViewportClient.IsValid())
    {
        ViewportClient->Invalidate();
    }

    for (UPaperSpriteComponent* Comp : ActiveComponents)
        Comp->SetVisibility(!bShowWireframe);
}

void SCharacter2DPreviewViewport::SetWireframe(bool bEnable)
{
    bShowWireframe = bEnable;

    /* показать / скрыть StaticMesh-каркас */
    if (PreviewMeshComp) PreviewMeshComp->SetVisibility(bShowWireframe);

    /* показать / скрыть PaperSpriteComponent-ы */
    for (UPaperSpriteComponent* C : ComponentPool)
        if (C) C->SetVisibility(!bShowWireframe);

    /* переключить режим отображения клиента */
    if (ViewportClient.IsValid())
    {
        ViewportClient->SetViewMode(bShowWireframe ? VMI_Wireframe : VMI_Unlit);
        ViewportClient->Invalidate();
    }
}

void SCharacter2DPreviewViewport::UpdatePreviewMeshDescription(
    const TArray<TSharedPtr<FCharacter2DLayerCategory>>& Categories,
    float InPreviewScale)
{
    // ------------------------------------------------------------------
    // 1) собираем MeshDescription точно так же, как при финальной генерации
    // ------------------------------------------------------------------
    FMeshDescription MeshDesc;

    TArray<UTexture*>     DummyTextures;   // lvalue, нужен Build-функции
    TArray<UPaperSprite*> DummySprites;

    FCharacter2DMeshGenerationOptions Opt;
    Opt.MeshScale = InPreviewScale;

    Character2DMeshGenerator::BuildMeshDescriptionAndTextures(
        Categories,
        MeshDesc,
        DummyTextures,
        DummySprites,
        Opt);

    if (MeshDesc.Polygons().Num() == 0)
    {
        PreviewMeshComp->SetStaticMesh(nullptr);
        return;
    }

    // ------------------------------------------------------------------
    // 2) создаём временный StaticMesh из MeshDescription
    // ------------------------------------------------------------------
    UStaticMesh* TempMesh = NewObject<UStaticMesh>(
        GetTransientPackage(), NAME_None, RF_Transient);

    if (!TempMesh->IsSourceModelValid(0))
        TempMesh->AddSourceModel();

    TempMesh->CreateMeshDescription(0, MeshDesc);

    UStaticMesh::FCommitMeshDescriptionParams Params;
    Params.bMarkPackageDirty = false;
    Params.bUseHashAsGuid    = false;
    TempMesh->CommitMeshDescription(0, Params);
    TempMesh->Build();

    PreviewMeshComp->SetStaticMesh(TempMesh);

    /* ---------- создаём материалы для превью ---------- */
    for (int32 i = 0; i < DummyTextures.Num(); ++i)
    {
        // динамический материал поверх Engine/UnlitSpriteMaterial
        static const FName ParamName(TEXT("SpriteTexture"));
        UMaterialInterface* BaseMat =
            UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface);

        UMaterialInstanceDynamic* DynMat =
            UMaterialInstanceDynamic::Create(BaseMat, PreviewMeshComp);
        DynMat->SetTextureParameterValue(ParamName, DummyTextures[i]);

        PreviewMeshComp->SetMaterial(i, DynMat);
    }

    /* если текстур меньше, чем секций ─ заполняем дефолтным */
    for (int32 i = DummyTextures.Num(); i < TempMesh->GetStaticMaterials().Num(); ++i)
    {
        PreviewMeshComp->SetMaterial(i, UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface));
    }

    /* ---------- назначаем компоненту (материалы уже заданы выше) ---------- */
    PreviewMeshComp->SetStaticMesh(TempMesh);

    /* ► обновляем видимость объектов и ViewMode */
    SetWireframe(bShowWireframe);
}


void SCharacter2DPreviewViewport::CenterCameraOnSprites()
{
    if (!ViewportClient.IsValid() || ActiveComponents.Num() == 0)
    {
        return;
    }

    // Сбросить камеру на исходную позицию
    ViewportClient->SetViewLocation(FVector(0.0f, 1200.0f, 200.0f));
    ViewportClient->SetViewRotation(FRotator(0.0f, -90.0f, 0.0f));
    ViewportClient->Invalidate();
}
