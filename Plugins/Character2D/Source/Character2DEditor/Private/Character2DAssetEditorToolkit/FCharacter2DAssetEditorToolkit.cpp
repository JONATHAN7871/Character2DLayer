#include "Character2DAssetEditorToolkit/FCharacter2DAssetEditorToolkit.h"
#include "Character2DAssetEditorToolkit/Slate/SCharacter2DAssetViewport.h"
#include "Character2DAssetEditorToolkit/Slate/SCharacter2DActionPanel.h"
#include "Character2DAssetEditorToolkit/Slate/SCharacter2DPresetPanel.h"
#include "Widgets/Docking/SDockTab.h"
#include "PropertyEditorModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Commands/UICommandList.h"
#include "Data/Character2DPosePreset.h"
#include "Data/Character2DPartPreset.h"
#include "IDetailsView.h"
#include "PropertyCustomizationHelpers.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/PackageName.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Text.h"
#include "Editor.h" // Для FTicker
#include "Character2DActor.h" // <<< ВАЖНО: ДОБАВЛЕН НЕДОСТАЮЩИЙ INCLUDE
#include "Character2DLayerOptimizer/Character2DLayerOptimizer.h"
#include "Character2DLayerOptimizer/SCharacter2DOptimizationPanel.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacter2DEditor, Log, All);

#define LOCTEXT_NAMESPACE "Character2DAssetEditor"

const FName FCharacter2DAssetEditorToolkit::ViewportTabID(TEXT("Character2DAssetEditor_Viewport"));
const FName FCharacter2DAssetEditorToolkit::SkeletalDetailsTabID(TEXT("Character2DAssetEditor_SkeletalDetails"));
const FName FCharacter2DAssetEditorToolkit::SpriteDetailsTabID(TEXT("Character2DAssetEditor_SpriteDetails"));
const FName FCharacter2DAssetEditorToolkit::ActionsTabID(TEXT("Character2DAssetEditor_Actions"));
const FName FCharacter2DAssetEditorToolkit::PresetsTabID(TEXT("Character2DAssetEditor_Presets"));
const FName FCharacter2DAssetEditorToolkit::OptimizationTabID(TEXT("Character2DAssetEditor_Optimization"));

void FCharacter2DAssetEditorToolkit::InitEditor(const EToolkitMode::Type Mode,
                                                const TSharedPtr<IToolkitHost>& Host,
                                                UCharacter2DAsset* InAsset)
{
    AssetBeingEdited = InAsset;
    ViewportWidget = SNew(SCharacter2DAssetViewport).CharacterAsset(InAsset);

    const TSharedRef<FTabManager::FLayout> Layout =
        FTabManager::NewLayout("Character2DAssetEditorLayout_v6")
        ->AddArea
        (
            FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
            ->Split(FTabManager::NewStack()->AddTab(PresetsTabID, ETabState::OpenedTab)->SetSizeCoefficient(0.25f))
            ->Split(FTabManager::NewStack()->AddTab(ViewportTabID, ETabState::OpenedTab)->SetSizeCoefficient(0.5f))
            ->Split
            (
                FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
                ->SetSizeCoefficient(0.25f)
                ->Split
                (
                    FTabManager::NewStack()
                    ->AddTab(SkeletalDetailsTabID, ETabState::OpenedTab)
                    ->AddTab(SpriteDetailsTabID, ETabState::OpenedTab)
                    ->AddTab(ActionsTabID, ETabState::OpenedTab)
                    ->AddTab(OptimizationTabID, ETabState::OpenedTab)
                    ->SetForegroundTab(SkeletalDetailsTabID)
                    ->SetSizeCoefficient(1.0f)
                )
            )
        );

    InitAssetEditor(Mode, Host, GetToolkitFName(), Layout, true, true, InAsset);

    TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

    ToolbarExtender->AddToolBarExtension(
        "Asset",
        EExtensionHook::After,
        TSharedPtr<FUICommandList>(),
        FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& ToolbarBuilder)
        {
            ToolbarBuilder.AddComboButton(
                FUIAction(),
                FOnGetContent::CreateLambda([this]()
                {
                    FMenuBuilder MenuBuilder(true, nullptr);
                    auto AddModeOption = [&MenuBuilder, this](ECharacter2DEditMode Mode, FText Text)
                    {
                        MenuBuilder.AddMenuEntry(
                            Text, FText(), FSlateIcon(),
                            FUIAction(FExecuteAction::CreateLambda([this, Mode]()
                            {
                                CurrentMode = Mode;
                                if (PresetPanel.IsValid()) { PresetPanel->Refresh(); }
                            })));
                    };
                    
                    AddModeOption(ECharacter2DEditMode::Body, LOCTEXT("ModeBody", "Body"));
                    AddModeOption(ECharacter2DEditMode::Arms, LOCTEXT("ModeArms", "Arms"));
                    AddModeOption(ECharacter2DEditMode::Head, LOCTEXT("ModeHead", "Head (Hierarchical)"));
                    AddModeOption(ECharacter2DEditMode::Pose, LOCTEXT("ModePose", "Pose"));
                    
                    return MenuBuilder.MakeWidget();
                }),
                LOCTEXT("ModeLabel", "Mode"),
                LOCTEXT("ModeTooltip", "Select editing mode"),
                FSlateIcon());

            ToolbarBuilder.AddToolBarButton(
                FUIAction(FExecuteAction::CreateLambda([this]()
                {
                    if (!AssetBeingEdited) return;

                    FString BasePath = TEXT("/Game/Character2D/Presets/") + AssetBeingEdited->GetName();
                    
                    // ИСПРАВЛЕНО: Получаем ссылку на модуль AssetRegistry один раз
                    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

                    if (CurrentMode == ECharacter2DEditMode::Pose)
                    {
                        // Рекомендуется делать имя пакета уникальным, например, добавляя временную метку
                        FString PackageName = BasePath + TEXT("/Poses/NewPosePreset_") + FGuid::NewGuid().ToString();
                        UCharacter2DPosePreset* NewPose = NewObject<UCharacter2DPosePreset>(CreatePackage(*PackageName));
                        NewPose->PresetName = FName("NewPose");
                        
                        // ИСПРАВЛЕНО: Используем правильный вызов
                        AssetRegistryModule.AssetCreated(NewPose);
                        NewPose->MarkPackageDirty();
                    }
                    else
                    {
                        FString PackageName = BasePath + TEXT("/Parts/NewPartPreset_") + FGuid::NewGuid().ToString();
                        UCharacter2DPartPreset* NewPart = NewObject<UCharacter2DPartPreset>(CreatePackage(*PackageName));
                        NewPart->PresetName = FName("NewPart");
                        NewPart->Part = CurrentMode;
                        
                        switch (CurrentMode)
                        {
                            case ECharacter2DEditMode::Body: NewPart->Mesh = AssetBeingEdited->Body.Mesh; break;
                            case ECharacter2DEditMode::Arms: NewPart->Mesh = AssetBeingEdited->Arms.Mesh; break;
                            case ECharacter2DEditMode::Head: NewPart->Mesh = AssetBeingEdited->Head.Mesh; break;
                            default: break;
                        }
                        
                        // ИСПРАВЛЕНО: Используем правильный вызов
                        AssetRegistryModule.AssetCreated(NewPart);
                        NewPart->MarkPackageDirty();
                    }
                    
                    if (PresetPanel.IsValid()) { PresetPanel->Refresh(); }
                })),
                NAME_None, LOCTEXT("SavePreset", "Save Preset"), LOCTEXT("SavePresetTooltip", "Save current configuration as preset"), FSlateIcon());

            ToolbarBuilder.AddToolBarButton(
                FUIAction(FExecuteAction::CreateLambda([this]()
                {
                    if (ViewportWidget.IsValid()) { ViewportWidget->RefreshPreview(); }
                })),
                NAME_None, LOCTEXT("RefreshPreview", "Refresh"), LOCTEXT("RefreshPreviewTooltip", "Refresh preview"), FSlateIcon());
        }));

    AddToolbarExtender(ToolbarExtender);
}

void FCharacter2DAssetEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
    if (!WorkspaceMenuCategory.IsValid())
    {
        WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("Character2DAssetEditorMenu", "Character-2D Asset"));
    }
    InTabManager->RegisterTabSpawner(ViewportTabID, FOnSpawnTab::CreateSP(this, &FCharacter2DAssetEditorToolkit::SpawnViewportTab)).SetDisplayName(LOCTEXT("ViewportTab", "Preview")).SetGroup(WorkspaceMenuCategory.ToSharedRef());
    InTabManager->RegisterTabSpawner(SkeletalDetailsTabID, FOnSpawnTab::CreateSP(this, &FCharacter2DAssetEditorToolkit::SpawnSkeletalDetailsTab)).SetDisplayName(LOCTEXT("SkeletalDetailsTab", "SkeletalMesh")).SetGroup(WorkspaceMenuCategory.ToSharedRef());
    InTabManager->RegisterTabSpawner(SpriteDetailsTabID, FOnSpawnTab::CreateSP(this, &FCharacter2DAssetEditorToolkit::SpawnSpriteDetailsTab)).SetDisplayName(LOCTEXT("SpriteDetailsTab", "Sprites & Effects")).SetGroup(WorkspaceMenuCategory.ToSharedRef());
    InTabManager->RegisterTabSpawner(ActionsTabID, FOnSpawnTab::CreateSP(this, &FCharacter2DAssetEditorToolkit::SpawnActionsTab)).SetDisplayName(LOCTEXT("ActionsTab", "Actions")).SetGroup(WorkspaceMenuCategory.ToSharedRef());
    InTabManager->RegisterTabSpawner(PresetsTabID, FOnSpawnTab::CreateSP(this, &FCharacter2DAssetEditorToolkit::SpawnPresetsTab)).SetDisplayName(LOCTEXT("PresetsTab", "Presets")).SetGroup(WorkspaceMenuCategory.ToSharedRef());
    InTabManager->RegisterTabSpawner(OptimizationTabID, FOnSpawnTab::CreateSP(this, &FCharacter2DAssetEditorToolkit::SpawnOptimizationTab))
    .SetDisplayName(LOCTEXT("OptimizationTab", "Optimization"))
    .SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FCharacter2DAssetEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
    InTabManager->UnregisterTabSpawner(ViewportTabID);
    InTabManager->UnregisterTabSpawner(SkeletalDetailsTabID);
    InTabManager->UnregisterTabSpawner(SpriteDetailsTabID);
    InTabManager->UnregisterTabSpawner(ActionsTabID);
    InTabManager->UnregisterTabSpawner(PresetsTabID);
    InTabManager->UnregisterTabSpawner(OptimizationTabID);
}

TSharedRef<SDockTab> FCharacter2DAssetEditorToolkit::SpawnViewportTab(const FSpawnTabArgs& Args)
{
    check(ViewportWidget.IsValid());
    return SNew(SDockTab).Label(LOCTEXT("ViewportLabel", "Preview"))[ViewportWidget.ToSharedRef()];
}

TSharedRef<SDockTab> FCharacter2DAssetEditorToolkit::SpawnSkeletalDetailsTab(const FSpawnTabArgs& Args)
{
    SkeletalDetailsView = CreateSkeletalDetailsView();
    return SNew(SDockTab).Label(LOCTEXT("SkeletalDetailsLabel", "SkeletalMesh"))[SkeletalDetailsView.ToSharedRef()];
}

TSharedRef<SDockTab> FCharacter2DAssetEditorToolkit::SpawnSpriteDetailsTab(const FSpawnTabArgs& Args)
{
    SpriteDetailsView = CreateSpriteDetailsView();
    return SNew(SDockTab).Label(LOCTEXT("SpriteDetailsLabel", "Sprites & Effects"))[SpriteDetailsView.ToSharedRef()];
}

TSharedRef<SDockTab> FCharacter2DAssetEditorToolkit::SpawnActionsTab(const FSpawnTabArgs& Args)
{
    TWeakObjectPtr<ACharacter2DActor> PreviewActor;
    if (ViewportWidget.IsValid())
    {
        PreviewActor = ViewportWidget->GetPreviewActor();
    }
    ActionPanel = SNew(SCharacter2DActionPanel).CharacterAsset(AssetBeingEdited).PreviewActor(PreviewActor);
    return SNew(SDockTab).Label(LOCTEXT("ActionsLabel", "Actions"))[ActionPanel.ToSharedRef()];
}

TSharedRef<SDockTab> FCharacter2DAssetEditorToolkit::SpawnPresetsTab(const FSpawnTabArgs& Args)
{
    PresetPanel = SNew(SCharacter2DPresetPanel)
        .CharacterAsset(AssetBeingEdited)
        .Mode(CurrentMode)
        .OnChosen(FOnPresetChosen::CreateLambda([this](UObject* PresetObj)
        {
            if (!PresetObj || !AssetBeingEdited) return;
            if (UCharacter2DPartPreset* Part = Cast<UCharacter2DPartPreset>(PresetObj))
            {
                switch (Part->Part)
                {
                    case ECharacter2DEditMode::Body: AssetBeingEdited->Body.Mesh = Part->Mesh; break;
                    case ECharacter2DEditMode::Arms: AssetBeingEdited->Arms.Mesh = Part->Mesh; break;
                    case ECharacter2DEditMode::Head: AssetBeingEdited->Head.Mesh = Part->Mesh; break;
                    default: break;
                }
            }
            else if (UCharacter2DPosePreset* Pose = Cast<UCharacter2DPosePreset>(PresetObj))
            {
                if (Pose->BodyPreset && Pose->BodyPreset->Mesh) AssetBeingEdited->Body.Mesh = Pose->BodyPreset->Mesh;
                if (Pose->ArmsPreset && Pose->ArmsPreset->Mesh) AssetBeingEdited->Arms.Mesh = Pose->ArmsPreset->Mesh;
                if (Pose->HeadPreset && Pose->HeadPreset->Mesh) AssetBeingEdited->Head.Mesh = Pose->HeadPreset->Mesh;
            }
            if (SkeletalDetailsView.IsValid()) SkeletalDetailsView->SetObject(AssetBeingEdited);
            if (SpriteDetailsView.IsValid()) SpriteDetailsView->SetObject(AssetBeingEdited);
            if (ViewportWidget.IsValid()) ViewportWidget->RefreshPreview();
        }));
    return SNew(SDockTab).Label(LOCTEXT("PresetsLabel", "Presets"))[PresetPanel.ToSharedRef()];
}

TSharedRef<IDetailsView> FCharacter2DAssetEditorToolkit::CreateSkeletalDetailsView()
{
    FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.bHideSelectionTip = true;
    DetailsViewArgs.bAllowSearch = true;
    TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
    DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& P) -> bool
    {
        const FString CategoryName = P.Property.GetMetaData(TEXT("Category"));
        return CategoryName.Contains(TEXT("Skeletal"));
    }));
    DetailsView->SetObject(AssetBeingEdited);
    DetailsView->OnFinishedChangingProperties().AddSP(this, &FCharacter2DAssetEditorToolkit::OnAssetPropertyChanged);
    return DetailsView;
}

TSharedRef<IDetailsView> FCharacter2DAssetEditorToolkit::CreateSpriteDetailsView()
{
    FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.bHideSelectionTip = true;
    DetailsViewArgs.bAllowSearch = true;
    TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
    DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& P) -> bool
    {
        const FString CategoryName = P.Property.GetMetaData(TEXT("Category"));
        return !CategoryName.Contains(TEXT("Skeletal"));
    }));
    DetailsView->SetObject(AssetBeingEdited);
    DetailsView->OnFinishedChangingProperties().AddSP(this, &FCharacter2DAssetEditorToolkit::OnAssetPropertyChanged);
    return DetailsView;
}

void FCharacter2DAssetEditorToolkit::OnAssetPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent)
{
    if (!ViewportWidget.IsValid() || !AssetBeingEdited) return;

    // Сохраняем состояние актора перед обновлением
    FVector SavedLocation = FVector::ZeroVector;
    FRotator SavedRotation = FRotator::ZeroRotator;
    FVector SavedScale = FVector::OneVector;
    bool bWasSpritesVisible = true;
    bool bWasSkeletalVisible = true;
    bool bWasActorHidden = false;

    if (ACharacter2DActor* ExistingActor = ViewportWidget->GetPreviewActor().Get())
    {
        SavedLocation = ExistingActor->GetActorLocation();
        SavedRotation = ExistingActor->GetActorRotation();
        SavedScale = ExistingActor->GetActorScale3D();
        bWasSpritesVisible = ExistingActor->bSpritesVisible;
        bWasSkeletalVisible = ExistingActor->bSkeletalVisible;
        bWasActorHidden = ExistingActor->IsHidden();
    }

    // Обновляем превью
    ViewportWidget->RefreshPreview();

    // Восстанавливаем состояние актора после обновления с небольшой задержкой через Ticker
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
        [this, SavedLocation, SavedRotation, SavedScale, bWasSpritesVisible, bWasSkeletalVisible, bWasActorHidden](float)
        {
            if (ACharacter2DActor* Actor = ViewportWidget->GetPreviewActor().Get())
            {
                Actor->SetActorLocationAndRotation(SavedLocation, SavedRotation);
                Actor->SetActorScale3D(SavedScale);
                Actor->SetBothVisible(bWasSpritesVisible, bWasSkeletalVisible);
                Actor->SetActorHiddenInGame(bWasActorHidden);
            }
            return false; // Запустить тикер только один раз
        }), 0.0f);

    if (OptimizationPanel.IsValid())
    {
        OptimizationPanel->SetCharacterAsset(AssetBeingEdited);
    }
}

// <<< ИСПРАВЛЕНО: УДАЛЕНА РЕАЛИЗАЦИЯ ValidateHeadHierarchy, ТАК КАК ЕЕ ЗДЕСЬ БЫТЬ НЕ ДОЛЖНО >>>

FName FCharacter2DAssetEditorToolkit::GetToolkitFName() const { return "Character2DAssetEditor"; }
FText FCharacter2DAssetEditorToolkit::GetBaseToolkitName() const { return LOCTEXT("AppLabel", "Character2D Asset Editor"); }
FString FCharacter2DAssetEditorToolkit::GetWorldCentricTabPrefix() const { return TEXT("Character2DAsset"); }
FLinearColor FCharacter2DAssetEditorToolkit::GetWorldCentricTabColorScale() const { return FLinearColor::White; }

void FCharacter2DAssetEditorToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
    Collector.AddReferencedObject(AssetBeingEdited);
}

TSharedRef<SDockTab> FCharacter2DAssetEditorToolkit::SpawnOptimizationTab(const FSpawnTabArgs& Args)
{
    OptimizationPanel = SNew(SCharacter2DOptimizationPanel)
        .CharacterAsset(AssetBeingEdited)
        .OnOptimizationComplete_Lambda([this](const TArray<FLayerOptimizationResult>& Results)
        {
            // Обновляем другие панели после оптимизации
            if (SkeletalDetailsView.IsValid()) SkeletalDetailsView->SetObject(AssetBeingEdited);
            if (SpriteDetailsView.IsValid()) SpriteDetailsView->SetObject(AssetBeingEdited);
            if (ViewportWidget.IsValid()) ViewportWidget->RefreshPreview();
            
            // Показываем уведомление о успешной оптимизации
            float TotalSavings = 0;
            for (const auto& Result : Results)
            {
                TotalSavings += (Result.OriginalSizeMB - Result.OptimizedSizeMB);
            }
            
            FText NotificationText = FText::Format(
                LOCTEXT("OptimizationCompleteNotification", "Optimization complete! Saved {0:.1f} MB ({1} layers optimized)"),
                TotalSavings,
                Results.Num()
            );
            
            // Здесь можно добавить показ notification toast
            UE_LOG(LogTemp, Log, TEXT("%s"), *NotificationText.ToString());
        });
    
    return SNew(SDockTab)
        .Label(LOCTEXT("OptimizationLabel", "Optimization"))
        [
            OptimizationPanel.ToSharedRef()
        ];
}

#undef LOCTEXT_NAMESPACE