// SCharacter2DAssetViewport.cpp

#include "Character2DAssetEditorToolkit/Slate/SCharacter2DAssetViewport.h"
#include "Character2DActor.h"
#include "AdvancedPreviewScene.h"
#include "EditorViewportClient.h"
#include "UnrealWidget.h"           // для EWidgetMode
#include "EditorModeManager.h"      // для GLevelEditorModeTools()
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Framework/Commands/UICommandList.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "SCharacter2DAssetViewport"

// Клиент с поддержкой инструментов Level Editor
class FCharacter2DViewportClient : public FEditorViewportClient
{
public:
    FCharacter2DViewportClient(FPreviewScene* InPreviewScene)
        : FEditorViewportClient(&GLevelEditorModeTools(), InPreviewScene)
    {}
};

SCharacter2DAssetViewport::~SCharacter2DAssetViewport()
{
    if (PreviewScene.IsValid() && PreviewScene->GetWorld())
    {
        PreviewScene->GetWorld()->GetTimerManager().ClearTimer(RefreshTimerHandle);
    }
    if (PreviewActor.IsValid())
    {
        PreviewActor->Destroy();
        PreviewActor = nullptr;
    }
}

void SCharacter2DAssetViewport::Construct(const FArguments& InArgs)
{
    Asset = InArgs._CharacterAsset;
    PreviewScene = MakeShareable(
        new FAdvancedPreviewScene(FPreviewScene::ConstructionValues())
    );

    // основная инициализация — внутри вызовёт MakeViewportToolbar()
    SEditorViewport::Construct(SEditorViewport::FArguments());

    if (Asset && PreviewScene->GetWorld())
    {
        PreviewActor = PreviewScene->GetWorld()->SpawnActor<ACharacter2DActor>();
        if (PreviewActor.IsValid())
        {
            PreviewActor->CharacterAsset = Asset;
            PreviewActor->RefreshFromAsset();
            
            // НОВОЕ: Отключаем Auto Blink/Talk для актора в редакторе
            PreviewActor->bBlinkingActive = false;
            PreviewActor->bTalkingActive = false;
        }
    }
}

TSharedRef<FEditorViewportClient> SCharacter2DAssetViewport::MakeEditorViewportClient()
{
    EditorViewportClient = MakeShareable(new FCharacter2DViewportClient(PreviewScene.Get()));

    // ИСПРАВЛЕНО: Устанавливаем Unlit вместо Lit
    EditorViewportClient->SetViewMode(VMI_Unlit);
    EditorViewportClient->SetRealtime(true);
    EditorViewportClient->EngineShowFlags.SetPaper2DSprites(true);

    // Отключаем gizmo
    EditorViewportClient->SetWidgetMode(UE::Widget::EWidgetMode::WM_None);

    // ИСПРАВЛЕНО: Настройки камеры для вида справа (Right)
    // Right view: камера смотрит вдоль оси +Y (справа налево)
    EditorViewportClient->SetViewLocation(FVector(0.f, 150.f, 0.f));  // Камера справа от объекта
    EditorViewportClient->SetViewRotation(FRotator(0.f, -90.f, 0.f)); // Поворот для вида справа

    return EditorViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> SCharacter2DAssetViewport::MakeViewportToolbar()
{
    return SNew(SCommonEditorViewportToolbarBase, SharedThis(this));
}

void SCharacter2DAssetViewport::BindCommands()
{
    SEditorViewport::BindCommands();
}

void SCharacter2DAssetViewport::Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime)
{
    SEditorViewport::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
    if (PreviewScene.IsValid() && PreviewScene->GetWorld())
    {
        PreviewScene->GetWorld()->Tick(LEVELTICK_All, InDeltaTime);
    }
}

void SCharacter2DAssetViewport::RefreshPreview()
{
    if (!Asset || !PreviewScene.IsValid() || !PreviewScene->GetWorld())
    {
        return;
    }
    UWorld* World = PreviewScene->GetWorld();

    const FVector  OldLoc   = PreviewActor.IsValid() ? PreviewActor->GetActorLocation() : FVector::ZeroVector;
    const FRotator OldRot   = PreviewActor.IsValid() ? PreviewActor->GetActorRotation() : FRotator::ZeroRotator;
    const FVector  OldScale = PreviewActor.IsValid() ? PreviewActor->GetActorScale3D() : FVector::OneVector;

    if (!PreviewActor.IsValid() || PreviewActor->CharacterAsset != Asset)
    {
        if (PreviewActor.IsValid()) PreviewActor->Destroy();
        PreviewActor = World->SpawnActor<ACharacter2DActor>();
        if (!PreviewActor.IsValid()) return;
        PreviewActor->CharacterAsset = Asset;
        
        // НОВОЕ: Принудительно отключаем Auto анимации для preview актора
        PreviewActor->bBlinkingActive = false;
        PreviewActor->bTalkingActive = false;
    }

    PreviewActor->RefreshFromAsset();
    
    // НОВОЕ: После обновления снова отключаем Auto анимации
    PreviewActor->bBlinkingActive = false;
    PreviewActor->bTalkingActive = false;

    World->GetTimerManager().ClearTimer(RefreshTimerHandle);
    World->GetTimerManager().SetTimer(
        RefreshTimerHandle,
        [this, OldLoc, OldRot, OldScale]()
        {
            if (PreviewActor.IsValid())
            {
                PreviewActor->SetActorLocation(OldLoc);
                PreviewActor->SetActorRotation(OldRot);
                PreviewActor->SetActorScale3D(OldScale);
                
                // НОВОЕ: Гарантируем отключение Auto анимаций после восстановления трансформа
                PreviewActor->bBlinkingActive = false;
                PreviewActor->bTalkingActive = false;
            }
        },
        0.1f, false
    );
}

void SCharacter2DAssetViewport::ForceRefreshPreview()
{
    if (PreviewActor.IsValid())
    {
        PreviewActor->Destroy();
        PreviewActor = nullptr;
    }
    if (Asset && PreviewScene.IsValid() && PreviewScene->GetWorld())
    {
        PreviewActor = PreviewScene->GetWorld()->SpawnActor<ACharacter2DActor>();
        if (PreviewActor.IsValid())
        {
            PreviewActor->CharacterAsset = Asset;
            PreviewActor->RefreshFromAsset();
            
            // НОВОЕ: Отключаем Auto анимации для нового актора
            PreviewActor->bBlinkingActive = false;
            PreviewActor->bTalkingActive = false;
        }
    }
}

FString SCharacter2DAssetViewport::GetDebugInfo() const
{
    if (!PreviewActor.IsValid())
    {
        return TEXT("No Preview Actor");
    }
    ACharacter2DActor* Actor = PreviewActor.Get();
    return FString::Printf(
        TEXT("Actor: %s\nAsset: %s\nLocation: %s\nScale: %s\n"),
        *Actor->GetName(),
        *Actor->CharacterAsset->GetName(),
        *Actor->GetActorLocation().ToString(),
        *Actor->GetActorScale3D().ToString()
    );
}

#undef LOCTEXT_NAMESPACE