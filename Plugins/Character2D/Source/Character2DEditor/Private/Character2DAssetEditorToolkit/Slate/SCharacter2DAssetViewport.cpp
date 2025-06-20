#include "Character2DAssetEditorToolkit/Slate/SCharacter2DAssetViewport.h"
#include "Character2DActor.h"
#include "AdvancedPreviewScene.h"
#include "EditorViewportClient.h"
#include "UnrealWidget.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "SEditorViewport.h"
#include "Styling/AppStyle.h"
#include "EngineUtils.h"

#define LOCTEXT_NAMESPACE "SCharacter2DAssetViewport"

// Custom viewport client with transform widget support
class FCharacter2DViewportClient : public FEditorViewportClient
{
public:
    FCharacter2DViewportClient(FPreviewScene* InPreviewScene, const TWeakPtr<SCharacter2DAssetViewport>& InViewport)
        : FEditorViewportClient(nullptr, InPreviewScene)
        , ViewportPtr(InViewport)
    {
        FEditorViewportClient::SetViewMode(VMI_Lit);
        SetRealtime(true);
        
        // Enable widget for transform
        Widget->SetDefaultVisibility(true);
        EngineShowFlags.SetPaper2DSprites(true);
        
        // Set default view
        SetViewLocation(FVector(0.f, 500.f, 75.f));
        SetViewRotation(FRotator(0.f, -90.f, 0.f));
        
        // Enable widget
        FEditorViewportClient::SetWidgetMode(UE::Widget::WM_Translate);
    }

    virtual void ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) override
    {
        if (HitProxy && HitProxy->IsA(HActor::StaticGetType()))
        {
            HActor* ActorProxy = static_cast<HActor*>(HitProxy);
            AActor* Actor = ActorProxy->Actor;
            
            if (TSharedPtr<SCharacter2DAssetViewport> ViewportWidget = ViewportPtr.Pin())
            {
                ViewportWidget->OnActorSelected(Actor);
            }
        }
        
        FEditorViewportClient::ProcessClick(View, HitProxy, Key, Event, HitX, HitY);
    }

    virtual bool InputWidgetDelta(FViewport* InViewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale) override
    {
        bool bHandled = FEditorViewportClient::InputWidgetDelta(InViewport, CurrentAxis, Drag, Rot, Scale);
        
        if (bHandled && SelectedActor.IsValid())
        {
            // Apply transform based on current widget mode
            switch (GetWidgetMode())
            {
                case UE::Widget::WM_Translate:
                    SelectedActor->SetActorLocation(SelectedActor->GetActorLocation() + Drag);
                    break;
                case UE::Widget::WM_Rotate:
                    SelectedActor->SetActorRotation(SelectedActor->GetActorRotation() + Rot);
                    break;
                case UE::Widget::WM_Scale:
                    SelectedActor->SetActorScale3D(SelectedActor->GetActorScale3D() + Scale);
                    break;
                default:
                    break;
            }
        }
        
        return bHandled;
    }

    void SetSelectedActor(AActor* Actor)
    {
        SelectedActor = Actor;
        if (Actor && Widget)
        {
            // Just set the widget target
            Widget->SetDefaultVisibility(true);
        }
    }

private:
    TWeakPtr<SCharacter2DAssetViewport> ViewportPtr;
    TWeakObjectPtr<AActor> SelectedActor;
};

// Main viewport implementation
void SCharacter2DAssetViewport::OnFloatingButtonClicked()
{
}

SCharacter2DAssetViewport::~SCharacter2DAssetViewport()
{
    if (PreviewActor && PreviewActor->IsValidLowLevel())
    {
        PreviewActor->Destroy();
    }
}

void SCharacter2DAssetViewport::Construct(const FArguments& InArgs)
{
    Asset = InArgs._CharacterAsset;
    PreviewScene = MakeShareable(
        new FAdvancedPreviewScene(FPreviewScene::ConstructionValues())
    );

    SEditorViewport::Construct(SEditorViewport::FArguments());

    if (Asset && PreviewScene->GetWorld())
    {
        PreviewActor = PreviewScene->GetWorld()->SpawnActor<ACharacter2DActor>();
        PreviewActor->CharacterAsset = Asset;
        PreviewActor->RefreshFromAsset();
    }
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
        UE_LOG(LogTemp, Warning, TEXT("RefreshPreview: Missing Asset or PreviewScene"));
        return;
    }

    // Сохраняем текущее состояние перед обновлением
    bool bOldSpritesVisible = true;
    bool bOldSkeletalVisible = true;
    bool bOldActorHidden = false;
    bool bOldBlinkingActive = false;
    bool bOldTalkingActive = false;
    FVector OldLocation = FVector::ZeroVector;
    FRotator OldRotation = FRotator::ZeroRotator;
    FVector OldScale = FVector::OneVector;
    
    if (PreviewActor && IsValid(PreviewActor))
    {
        bOldSpritesVisible = PreviewActor->bSpritesVisible;
        bOldSkeletalVisible = PreviewActor->bSkeletalVisible;
        bOldActorHidden = PreviewActor->IsHidden();
        bOldBlinkingActive = PreviewActor->bBlinkingActive;
        bOldTalkingActive = PreviewActor->bTalkingActive;
        OldLocation = PreviewActor->GetActorLocation();
        OldRotation = PreviewActor->GetActorRotation();
        OldScale = PreviewActor->GetActorScale3D();
        
        UE_LOG(LogTemp, Log, TEXT("RefreshPreview: Saving state - Sprites:%s, Skeletal:%s, Hidden:%s, Pos:%s"),
               bOldSpritesVisible ? TEXT("T") : TEXT("F"),
               bOldSkeletalVisible ? TEXT("T") : TEXT("F"),
               bOldActorHidden ? TEXT("T") : TEXT("F"),
               *OldLocation.ToString());
    }

    // ИСПРАВЛЕНИЕ: Проверяем, нужно ли пересоздать актер
    bool bNeedNewActor = false;
    
    if (!PreviewActor || !IsValid(PreviewActor))
    {
        bNeedNewActor = true;
        UE_LOG(LogTemp, Log, TEXT("RefreshPreview: Preview actor is invalid, creating new one"));
    }
    else if (PreviewActor->CharacterAsset != Asset)
    {
        bNeedNewActor = true;
        UE_LOG(LogTemp, Log, TEXT("RefreshPreview: Asset mismatch, creating new actor"));
    }

    if (bNeedNewActor)
    {
        // Уничтожаем старый актер
        if (PreviewActor && IsValid(PreviewActor))
        {
            PreviewActor->Destroy();
            PreviewActor = nullptr;
        }

        // Создаем новый актер
        PreviewActor = PreviewScene->GetWorld()->SpawnActor<ACharacter2DActor>();
        if (!PreviewActor)
        {
            UE_LOG(LogTemp, Error, TEXT("RefreshPreview: Failed to spawn preview actor"));
            return;
        }

        PreviewActor->CharacterAsset = Asset;
        UE_LOG(LogTemp, Log, TEXT("RefreshPreview: Created new preview actor"));
    }

    // ИСПРАВЛЕНИЕ: Используем RefreshFromAsset вместо прямого вызова OnConstruction
    if (PreviewActor && IsValid(PreviewActor))
    {
        // Устанавливаем CharacterAsset если он отличается
        if (PreviewActor->CharacterAsset != Asset)
        {
            PreviewActor->CharacterAsset = Asset;
        }

        // Вызываем обновление
        PreviewActor->RefreshFromAsset();
        
        UE_LOG(LogTemp, Log, TEXT("RefreshPreview: Called RefreshFromAsset"));

        // ИСПРАВЛЕНИЕ: Восстанавливаем состояние с небольшой задержкой
        // Это гарантирует, что все компоненты обновились
        FTimerHandle RestoreStateTimer;
        PreviewScene->GetWorld()->GetTimerManager().SetTimer(
            RestoreStateTimer,
            [this, bOldSpritesVisible, bOldSkeletalVisible, bOldActorHidden, bOldBlinkingActive, bOldTalkingActive, OldLocation, OldRotation, OldScale]()
            {
                if (PreviewActor && IsValid(PreviewActor))
                {
                    // Восстанавливаем трансформацию
                    PreviewActor->SetActorLocation(OldLocation);
                    PreviewActor->SetActorRotation(OldRotation);
                    PreviewActor->SetActorScale3D(OldScale);
                    
                    // Восстанавливаем видимость
                    PreviewActor->SetBothVisible(bOldSpritesVisible, bOldSkeletalVisible);
                    PreviewActor->SetActorHiddenInGame(bOldActorHidden);
                    
                    // Восстанавливаем анимации
                    if (bOldBlinkingActive && !PreviewActor->bBlinkingActive)
                    {
                        PreviewActor->EnableBlinking(true);
                    }
                    if (bOldTalkingActive && !PreviewActor->bTalkingActive)
                    {
                        PreviewActor->EnableTalking(true);
                    }
                    
                    UE_LOG(LogTemp, Log, TEXT("RefreshPreview: State restored - Pos:%s, Sprites:%s, Skeletal:%s"),
                           *PreviewActor->GetActorLocation().ToString(),
                           PreviewActor->bSpritesVisible ? TEXT("T") : TEXT("F"),
                           PreviewActor->bSkeletalVisible ? TEXT("T") : TEXT("F"));
                }
            },
            0.1f,  // Задержка 100ms
            false
        );
    }
}

FString SCharacter2DAssetViewport::GetDebugInfo() const
{
    if (!PreviewActor || !IsValid(PreviewActor))
    {
        return TEXT("No Preview Actor");
    }

    FString Info;
    Info += FString::Printf(TEXT("Actor: %s\n"), *PreviewActor->GetName());
    Info += FString::Printf(TEXT("Asset: %s\n"), PreviewActor->CharacterAsset ? *PreviewActor->CharacterAsset->GetName() : TEXT("None"));
    Info += FString::Printf(TEXT("Location: %s\n"), *PreviewActor->GetActorLocation().ToString());
    Info += FString::Printf(TEXT("Sprites Visible: %s\n"), PreviewActor->bSpritesVisible ? TEXT("Yes") : TEXT("No"));
    Info += FString::Printf(TEXT("Skeletal Visible: %s\n"), PreviewActor->bSkeletalVisible ? TEXT("Yes") : TEXT("No"));
    Info += FString::Printf(TEXT("Hidden: %s\n"), PreviewActor->IsHidden() ? TEXT("Yes") : TEXT("No"));
    Info += FString::Printf(TEXT("Blinking: %s\n"), PreviewActor->bBlinkingActive ? TEXT("Yes") : TEXT("No"));
    Info += FString::Printf(TEXT("Talking: %s\n"), PreviewActor->bTalkingActive ? TEXT("Yes") : TEXT("No"));

    // Информация о компонентах головы
    if (PreviewActor->SpriteHead)
    {
        Info += FString::Printf(TEXT("Head Attached To: %s\n"), 
                               PreviewActor->SpriteHead->GetAttachParent() ? 
                               *PreviewActor->SpriteHead->GetAttachParent()->GetName() : TEXT("None"));
        Info += FString::Printf(TEXT("Head Location: %s\n"), 
                               *PreviewActor->SpriteHead->GetRelativeLocation().ToString());
        Info += FString::Printf(TEXT("Head Scale: %s\n"), 
                               *PreviewActor->SpriteHead->GetRelativeScale3D().ToString());
    }

    return Info;
}

TSharedRef<FEditorViewportClient> SCharacter2DAssetViewport::MakeEditorViewportClient()
{
    EditorViewportClient = MakeShareable(new FCharacter2DViewportClient(PreviewScene.Get(), SharedThis(this)));
    return EditorViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> SCharacter2DAssetViewport::MakeViewportToolbar()
{
    // Build the toolbar
    TSharedRef<SHorizontalBox> ToolbarBox = SNew(SHorizontalBox);
    
    // Add transform toolbar
    ToolbarBox->AddSlot()
        .AutoWidth()
        .Padding(2.0f, 2.0f)
        [
            BuildTransformToolBar()
        ];
    
    // Add camera toolbar
    ToolbarBox->AddSlot()
        .AutoWidth()
        .Padding(2.0f, 2.0f)
        [
            BuildCameraToolbar()
        ];
    
    return ToolbarBox;
}

void SCharacter2DAssetViewport::PopulateViewportOverlays(TSharedRef<SOverlay> Overlay)
{
    SEditorViewport::PopulateViewportOverlays(Overlay);

    // Add toolbar at the top
    Overlay->AddSlot()
        .VAlign(VAlign_Top)
        .HAlign(HAlign_Left)
        .Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
    [
        MakeViewportToolbar().ToSharedRef()
    ];
}

TSharedRef<SWidget> SCharacter2DAssetViewport::BuildTransformToolBar()
{
    FSlimHorizontalToolBarBuilder ToolbarBuilder(GetCommandList(), FMultiBoxCustomization::None);
    
    // Transform mode buttons
    ToolbarBuilder.BeginSection("Transform");
    {
        // Translate
        ToolbarBuilder.AddToolBarButton(
            FUIAction(
                FExecuteAction::CreateLambda([this]() {
                    if (EditorViewportClient.IsValid())
                    {
                        EditorViewportClient->SetWidgetMode(UE::Widget::WM_Translate);
                    }
                }),
                FCanExecuteAction(),
                FIsActionChecked::CreateLambda([this]() {
                    return EditorViewportClient.IsValid() && 
                           EditorViewportClient->GetWidgetMode() == UE::Widget::WM_Translate;
                })
            ),
            NAME_None,
            LOCTEXT("TranslateMode", "Translate"),
            LOCTEXT("TranslateModeTooltip", "Translate Mode"),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "EditorViewport.TranslateMode"),
            EUserInterfaceActionType::ToggleButton
        );
        
        // Rotate
        ToolbarBuilder.AddToolBarButton(
            FUIAction(
                FExecuteAction::CreateLambda([this]() {
                    if (EditorViewportClient.IsValid())
                    {
                        EditorViewportClient->SetWidgetMode(UE::Widget::WM_Rotate);
                    }
                }),
                FCanExecuteAction(),
                FIsActionChecked::CreateLambda([this]() {
                    return EditorViewportClient.IsValid() && 
                           EditorViewportClient->GetWidgetMode() == UE::Widget::WM_Rotate;
                })
            ),
            NAME_None,
            LOCTEXT("RotateMode", "Rotate"),
            LOCTEXT("RotateModeTooltip", "Rotate Mode"),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "EditorViewport.RotateMode"),
            EUserInterfaceActionType::ToggleButton
        );
        
        // Scale
        ToolbarBuilder.AddToolBarButton(
            FUIAction(
                FExecuteAction::CreateLambda([this]() {
                    if (EditorViewportClient.IsValid())
                    {
                        EditorViewportClient->SetWidgetMode(UE::Widget::WM_Scale);
                    }
                }),
                FCanExecuteAction(),
                FIsActionChecked::CreateLambda([this]() {
                    return EditorViewportClient.IsValid() && 
                           EditorViewportClient->GetWidgetMode() == UE::Widget::WM_Scale;
                })
            ),
            NAME_None,
            LOCTEXT("ScaleMode", "Scale"),
            LOCTEXT("ScaleModeTooltip", "Scale Mode"),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "EditorViewport.ScaleMode"),
            EUserInterfaceActionType::ToggleButton
        );
    }
    ToolbarBuilder.EndSection();
    
    return ToolbarBuilder.MakeWidget();
}

TSharedRef<SWidget> SCharacter2DAssetViewport::BuildCameraToolbar()
{
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().Padding(2)
        [
            SNew(SButton)
            .Text(FText::FromString("Perspective"))
            .OnClicked(this, &SCharacter2DAssetViewport::OnViewPerspective)
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(2)
        [
            SNew(SButton)
            .Text(FText::FromString("Top"))
            .OnClicked(this, &SCharacter2DAssetViewport::OnViewTop)
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(2)
        [
            SNew(SButton)
            .Text(FText::FromString("Side"))
            .OnClicked(this, &SCharacter2DAssetViewport::OnViewSide)
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(2)
        [
            SNew(SButton)
            .Text(FText::FromString("Front"))
            .OnClicked(this, &SCharacter2DAssetViewport::OnViewFront)
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(2)
        [
            SNew(SButton)
            .Text(FText::FromString("Reset Camera"))
            .OnClicked(this, &SCharacter2DAssetViewport::OnResetCamera)
        ];
}

void SCharacter2DAssetViewport::OnActorSelected(AActor* Actor)
{
    if (Actor == PreviewActor)
    {
        UE_LOG(LogTemp, Log, TEXT("Preview actor selected: %s"), Actor ? *Actor->GetName() : TEXT("None"));
        UE_LOG(LogTemp, Log, TEXT("Debug Info:\n%s"), *GetDebugInfo());
    }

    if (FCharacter2DViewportClient* ViewportClient = static_cast<FCharacter2DViewportClient*>(EditorViewportClient.Get()))
    {
        ViewportClient->SetSelectedActor(Actor);
    }
}

void SCharacter2DAssetViewport::ForceRefreshPreview()
{
    UE_LOG(LogTemp, Warning, TEXT("ForceRefreshPreview: Performing complete refresh"));
    
    // Уничтожаем текущий актер
    if (PreviewActor && IsValid(PreviewActor))
    {
        PreviewActor->Destroy();
        PreviewActor = nullptr;
    }

    // Принудительно пересоздаем актер
    if (Asset && PreviewScene->GetWorld())
    {
        PreviewActor = PreviewScene->GetWorld()->SpawnActor<ACharacter2DActor>();
        if (PreviewActor)
        {
            PreviewActor->CharacterAsset = Asset;
            PreviewActor->RefreshFromAsset();
            UE_LOG(LogTemp, Log, TEXT("ForceRefreshPreview: New actor created and configured"));
        }
    }
}

FReply SCharacter2DAssetViewport::OnViewPerspective()
{
    if (EditorViewportClient.IsValid())
    {
        EditorViewportClient->SetViewLocation(FVector(300.f, 300.f, 300.f));
        EditorViewportClient->SetViewRotation(FRotator(-45.f, -45.f, 0.f));
    }
    return FReply::Handled();
}

FReply SCharacter2DAssetViewport::OnViewTop()
{
    if (EditorViewportClient.IsValid())
    {
        EditorViewportClient->SetViewLocation(FVector(0.f, 0.f, 600.f));
        EditorViewportClient->SetViewRotation(FRotator(-90.f, 0.f, 0.f));
    }
    return FReply::Handled();
}

FReply SCharacter2DAssetViewport::OnViewSide()
{
    if (EditorViewportClient.IsValid())
    {
        EditorViewportClient->SetViewLocation(FVector(0.f, 600.f, 75.f));
        EditorViewportClient->SetViewRotation(FRotator(0.f, -90.f, 0.f));
    }
    return FReply::Handled();
}

FReply SCharacter2DAssetViewport::OnViewFront()
{
    if (EditorViewportClient.IsValid())
    {
        EditorViewportClient->SetViewLocation(FVector(-600.f, 0.f, 75.f));
        EditorViewportClient->SetViewRotation(FRotator(0.f, 0.f, 0.f));
    }
    return FReply::Handled();
}

FReply SCharacter2DAssetViewport::OnResetCamera()
{
    if (EditorViewportClient.IsValid())
    {
        EditorViewportClient->SetViewLocation(FVector(0.f, 500.f, 75.f));
        EditorViewportClient->SetViewRotation(FRotator(0.f, -90.f, 0.f));
    }
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE