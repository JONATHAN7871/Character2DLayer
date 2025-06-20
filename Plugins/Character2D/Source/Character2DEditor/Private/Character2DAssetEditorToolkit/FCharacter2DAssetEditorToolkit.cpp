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
#include "HAL/PlatformFilemanager.h"
#include "Misc/PackageName.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Text.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacter2DEditor, Log, All);

#define LOCTEXT_NAMESPACE "Character2DAssetEditor"

const FName FCharacter2DAssetEditorToolkit::ViewportTabID(TEXT("Character2DAssetEditor_Viewport"));
const FName FCharacter2DAssetEditorToolkit::SkeletalDetailsTabID(TEXT("Character2DAssetEditor_SkeletalDetails"));
const FName FCharacter2DAssetEditorToolkit::SpriteDetailsTabID(TEXT("Character2DAssetEditor_SpriteDetails"));
const FName FCharacter2DAssetEditorToolkit::ActionsTabID(TEXT("Character2DAssetEditor_Actions"));
const FName FCharacter2DAssetEditorToolkit::PresetsTabID(TEXT("Character2DAssetEditor_Presets"));

void FCharacter2DAssetEditorToolkit::InitEditor(EToolkitMode::Type Mode,
                                                const TSharedPtr<IToolkitHost>& Host,
                                                UCharacter2DAsset* InAsset)
{
    AssetBeingEdited = InAsset;
    ViewportWidget = SNew(SCharacter2DAssetViewport).CharacterAsset(InAsset);

    /* ------------------- Editor Layout ------------------- */
    const TSharedRef<FTabManager::FLayout> Layout =
        FTabManager::NewLayout("Character2DAssetEditorLayout_v5")
        ->AddArea
        (
            FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
            
            // Left Panel - Presets
            ->Split(
                FTabManager::NewStack()
                ->AddTab(PresetsTabID, ETabState::OpenedTab)
                ->SetSizeCoefficient(0.25f)
            )
            
            // Center Panel - Viewport
            ->Split(
                FTabManager::NewStack()
                ->AddTab(ViewportTabID, ETabState::OpenedTab)
                ->SetSizeCoefficient(0.5f)
            )
            
            // Right Panel - Details with Tabs
            ->Split(
                FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
                ->SetSizeCoefficient(0.25f)
                
                // Details Tabs Area
                ->Split(
                    FTabManager::NewStack()
                    ->AddTab(SkeletalDetailsTabID, ETabState::OpenedTab)
                    ->AddTab(SpriteDetailsTabID, ETabState::OpenedTab)
                    ->AddTab(ActionsTabID, ETabState::OpenedTab)
                    ->SetForegroundTab(SkeletalDetailsTabID)
                    ->SetSizeCoefficient(1.0f)
                )
            )
        );

    /* -------- Base Asset Editor Initialization ------- */
    InitAssetEditor(Mode, Host, GetToolkitFName(),
                    Layout,
                    /*DefaultMenu*/ true,
                    /*DefaultToolbar*/ true,
                    InAsset);

    /* ------------------- Toolbar Extensions ------------------- */
    TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

    ToolbarExtender->AddToolBarExtension(
        "Asset",
        EExtensionHook::After,
        TSharedPtr<FUICommandList>(),
        FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& ToolbarBuilder)
        {
            /* ▼ Mode Selection Combo ▼ */
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
                                if (PresetPanel.IsValid()) 
                                {
                                    PresetPanel->Refresh();
                                }
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

            /* ▼ Save Preset Button ▼ */
            ToolbarBuilder.AddToolBarButton(
                FUIAction(FExecuteAction::CreateLambda([this]()
                {
                    FString BasePath = TEXT("/Game/Character2D/Presets/") + AssetBeingEdited->GetName();

                    if (CurrentMode == ECharacter2DEditMode::Pose)
                    {
                        UCharacter2DPosePreset* NewPose =
                            NewObject<UCharacter2DPosePreset>(CreatePackage(*(BasePath + "/Poses")));
                        NewPose->PresetName = FName("NewPose");
                        FAssetRegistryModule::AssetCreated(NewPose);
                    }
                    else
                    {
                        UCharacter2DPartPreset* NewPart =
                            NewObject<UCharacter2DPartPreset>(CreatePackage(*(BasePath + "/Parts")));
                        NewPart->PresetName = FName("NewPart");
                        NewPart->Part = CurrentMode;
                        
                        // Set mesh based on current mode
                        switch (CurrentMode)
                        {
                        case ECharacter2DEditMode::Body:
                            NewPart->Mesh = AssetBeingEdited->Body.Mesh;
                            break;
                        case ECharacter2DEditMode::Arms:
                            NewPart->Mesh = AssetBeingEdited->Arms.Mesh;
                            break;
                        case ECharacter2DEditMode::Head:
                            NewPart->Mesh = AssetBeingEdited->Head.Mesh;
                            break;
                        default:
                            break;
                        }
                        
                        FAssetRegistryModule::AssetCreated(NewPart);
                    }
                    
                    if (PresetPanel.IsValid()) 
                    {
                        PresetPanel->Refresh();
                    }
                })),
                NAME_None,
                LOCTEXT("SavePreset", "Save Preset"),
                LOCTEXT("SavePresetTooltip", "Save current configuration as preset"),
                FSlateIcon());

            /* ▼ Refresh Preview Button ▼ */
            ToolbarBuilder.AddToolBarButton(
                FUIAction(FExecuteAction::CreateLambda([this]()
                {
                    if (ViewportWidget.IsValid())
                    {
                        ViewportWidget->RefreshPreview();
                    }
                })),
                NAME_None,
                LOCTEXT("RefreshPreview", "Refresh"),
                LOCTEXT("RefreshPreviewTooltip", "Refresh preview"),
                FSlateIcon());

            /* ▼ NEW: Head Hierarchy Debug Button ▼ */
            ToolbarBuilder.AddToolBarButton(
                FUIAction(FExecuteAction::CreateLambda([this]()
                {
                    if (AssetBeingEdited && CurrentMode == ECharacter2DEditMode::Head)
                    {
                        LogHeadHierarchyInfo();
                    }
                })),
                NAME_None,
                LOCTEXT("DebugHead", "Debug Head"),
                LOCTEXT("DebugHeadTooltip", "Log head hierarchy information to console"),
                FSlateIcon());
        }));

    AddToolbarExtender(ToolbarExtender);
}

void FCharacter2DAssetEditorToolkit::LogHeadHierarchyInfo()
{
    if (!AssetBeingEdited) return;

    const auto& HeadStruct = AssetBeingEdited->SpriteStructure.Head;
    
    UE_LOG(LogCharacter2DEditor, Log, TEXT("=== HEAD HIERARCHY DEBUG INFO ==="));
    
    // Информация о корне головы
    UE_LOG(LogCharacter2DEditor, Log, TEXT("Head Root:"));
    UE_LOG(LogCharacter2DEditor, Log, TEXT("  Sprite: %s"), 
           HeadStruct.Head.Sprite ? *HeadStruct.Head.Sprite->GetName() : TEXT("None"));
    UE_LOG(LogCharacter2DEditor, Log, TEXT("  Offset: %s"), *HeadStruct.Head.Offset.ToString());
    UE_LOG(LogCharacter2DEditor, Log, TEXT("  Scale: %f"), HeadStruct.Head.Scale);
    UE_LOG(LogCharacter2DEditor, Log, TEXT("  Visible: %s"), HeadStruct.Head.bVisible ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogCharacter2DEditor, Log, TEXT("  Attachment: %s -> %s"), 
           *UEnum::GetValueAsString(HeadStruct.Head.AttachmentTarget),
           *HeadStruct.Head.SocketName.ToString());
    UE_LOG(LogCharacter2DEditor, Log, TEXT("  UseSocketTransform: %s"), 
           HeadStruct.Head.bUseSocketTransform ? TEXT("Yes") : TEXT("No"));

    // Информация о дочерних элементах
    UE_LOG(LogCharacter2DEditor, Log, TEXT("Facial Elements:"));
    
    auto LogChildElement = [this](const FCharacter2DHeadChildSprite& Child, const FString& Name)
    {
        UE_LOG(LogCharacter2DEditor, Log, TEXT("  %s:"), *Name);
        UE_LOG(LogCharacter2DEditor, Log, TEXT("    Sprite: %s"), 
               Child.Sprite ? *Child.Sprite->GetName() : TEXT("None"));
        UE_LOG(LogCharacter2DEditor, Log, TEXT("    LocalOffset: %s"), *Child.LocalOffset.ToString());
        UE_LOG(LogCharacter2DEditor, Log, TEXT("    LocalScale: %f"), Child.LocalScale);
        UE_LOG(LogCharacter2DEditor, Log, TEXT("    Visible: %s"), Child.bVisible ? TEXT("Yes") : TEXT("No"));
        UE_LOG(LogCharacter2DEditor, Log, TEXT("    OverrideHeadVisibility: %s"), 
               Child.bOverrideHeadVisibility ? TEXT("Yes") : TEXT("No"));
        
        // Вычисляем финальную видимость
        bool bFinalVisibility = AssetBeingEdited->SpriteStructure.Head.GetFinalChildVisibility(Child);
        UE_LOG(LogCharacter2DEditor, Log, TEXT("    FinalVisibility: %s"), bFinalVisibility ? TEXT("Yes") : TEXT("No"));
    };
    
    LogChildElement(HeadStruct.Eyebrows, TEXT("Eyebrows"));
    LogChildElement(HeadStruct.Eyes, TEXT("Eyes"));
    LogChildElement(HeadStruct.Eyelids, TEXT("Eyelids"));
    LogChildElement(HeadStruct.Mouth, TEXT("Mouth"));

    // Информация о глобальных настройках
    UE_LOG(LogCharacter2DEditor, Log, TEXT("Global Settings:"));
    UE_LOG(LogCharacter2DEditor, Log, TEXT("  GlobalSpriteOffset: %s"), 
           *AssetBeingEdited->GetGlobalSpriteOffset().ToString());
    UE_LOG(LogCharacter2DEditor, Log, TEXT("  GlobalSpriteScale: %f"), 
           AssetBeingEdited->GetGlobalSpriteScale());

    // Вычисленные финальные позиции
    UE_LOG(LogCharacter2DEditor, Log, TEXT("Calculated Final Offsets:"));
    UE_LOG(LogCharacter2DEditor, Log, TEXT("  Eyebrows: %s"), *AssetBeingEdited->GetFinalEyebrowOffset().ToString());
    UE_LOG(LogCharacter2DEditor, Log, TEXT("  Eyes: %s"), *AssetBeingEdited->GetFinalEyesOffset().ToString());
    UE_LOG(LogCharacter2DEditor, Log, TEXT("  Eyelids: %s"), *AssetBeingEdited->GetFinalEyelidsOffset().ToString());
    UE_LOG(LogCharacter2DEditor, Log, TEXT("  Mouth: %s"), *AssetBeingEdited->GetFinalMouthOffset().ToString());
    
    // Информация о анимациях
    UE_LOG(LogCharacter2DEditor, Log, TEXT("Animations:"));
    UE_LOG(LogCharacter2DEditor, Log, TEXT("  BlinkFlipbook: %s"), 
           HeadStruct.BlinkSettings.BlinkFlipbook ? *HeadStruct.BlinkSettings.BlinkFlipbook->GetName() : TEXT("None"));
    UE_LOG(LogCharacter2DEditor, Log, TEXT("  TalkFlipbook: %s"), 
           HeadStruct.TalkSettings.TalkFlipbook ? *HeadStruct.TalkSettings.TalkFlipbook->GetName() : TEXT("None"));
    
    UE_LOG(LogCharacter2DEditor, Log, TEXT("=== END HEAD HIERARCHY DEBUG INFO ==="));
    
    // Выполняем валидацию
    ValidateHeadConfiguration();
}

void FCharacter2DAssetEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    // Register base UE tabs
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

    // Workspace menu category
    if (!WorkspaceMenuCategory.IsValid())
    {
        WorkspaceMenuCategory =
            InTabManager->AddLocalWorkspaceMenuCategory(
                LOCTEXT("Character2DAssetEditorMenu", "Character-2D Asset"));
    }

    // Viewport Tab
    InTabManager->RegisterTabSpawner(
        ViewportTabID,
        FOnSpawnTab::CreateRaw(this, &FCharacter2DAssetEditorToolkit::SpawnViewportTab))
        .SetDisplayName(LOCTEXT("ViewportTab", "Preview"))
        .SetGroup(WorkspaceMenuCategory.ToSharedRef());

    // Skeletal Details Tab
    InTabManager->RegisterTabSpawner(
        SkeletalDetailsTabID,
        FOnSpawnTab::CreateRaw(this, &FCharacter2DAssetEditorToolkit::SpawnSkeletalDetailsTab))
        .SetDisplayName(LOCTEXT("SkeletalDetailsTab", "SkeletalMesh"))
        .SetGroup(WorkspaceMenuCategory.ToSharedRef());

    // Sprite Details Tab (now includes head hierarchy)
    InTabManager->RegisterTabSpawner(
        SpriteDetailsTabID,
        FOnSpawnTab::CreateRaw(this, &FCharacter2DAssetEditorToolkit::SpawnSpriteDetailsTab))
        .SetDisplayName(LOCTEXT("SpriteDetailsTab", "Sprites & Head"))
        .SetGroup(WorkspaceMenuCategory.ToSharedRef());

    // Actions Tab
    InTabManager->RegisterTabSpawner(
        ActionsTabID,
        FOnSpawnTab::CreateRaw(this, &FCharacter2DAssetEditorToolkit::SpawnActionsTab))
        .SetDisplayName(LOCTEXT("ActionsTab", "Actions"))
        .SetGroup(WorkspaceMenuCategory.ToSharedRef());

    // Presets Tab
    InTabManager->RegisterTabSpawner(
        PresetsTabID,
        FOnSpawnTab::CreateRaw(this, &FCharacter2DAssetEditorToolkit::SpawnPresetsTab))
        .SetDisplayName(LOCTEXT("PresetsTab", "Presets"))
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
}

/* ====================================================================== */
/*                            Tab Spawners                               */
/* ====================================================================== */

TSharedRef<SDockTab> FCharacter2DAssetEditorToolkit::SpawnViewportTab(const FSpawnTabArgs& Args)
{
    check(ViewportWidget.IsValid());
    return SNew(SDockTab)
        .Label(LOCTEXT("ViewportLabel", "Preview"))
        [
            ViewportWidget.ToSharedRef()
        ];
}

TSharedRef<SDockTab> FCharacter2DAssetEditorToolkit::SpawnSkeletalDetailsTab(const FSpawnTabArgs& Args)
{
    SkeletalDetailsView = CreateSkeletalDetailsView();
    
    return SNew(SDockTab)
        .Label(LOCTEXT("SkeletalDetailsLabel", "SkeletalMesh"))
        [
            SkeletalDetailsView.ToSharedRef()
        ];
}

TSharedRef<SDockTab> FCharacter2DAssetEditorToolkit::SpawnSpriteDetailsTab(const FSpawnTabArgs& Args)
{
    SpriteDetailsView = CreateSpriteDetailsView();
    
    return SNew(SDockTab)
        .Label(LOCTEXT("SpriteDetailsLabel", "Sprites & Head"))
        [
            SpriteDetailsView.ToSharedRef()
        ];
}

TSharedRef<SDockTab> FCharacter2DAssetEditorToolkit::SpawnActionsTab(const FSpawnTabArgs& Args)
{
    // Получаем валидный PreviewActor из SCharacter2DAssetViewport
    TWeakObjectPtr<ACharacter2DActor> PreviewActor;
    if (ViewportWidget.IsValid())
    {
        PreviewActor = ViewportWidget->GetPreviewActor();
    }

    // Создаём панель Actions и передаём ей CharacterAsset + PreviewActor
    ActionPanel = SNew(SCharacter2DActionPanel)
        .CharacterAsset(AssetBeingEdited)
        .PreviewActor(PreviewActor);

    return SNew(SDockTab)
        .Label(LOCTEXT("ActionsLabel", "Actions"))
        [
            ActionPanel.ToSharedRef()
        ];
}

TSharedRef<SDockTab> FCharacter2DAssetEditorToolkit::SpawnPresetsTab(const FSpawnTabArgs& Args)
{
    PresetPanel = SNew(SCharacter2DPresetPanel)
        .CharacterAsset(AssetBeingEdited)
        .Mode(CurrentMode)
        .OnChosen(FOnPresetChosen::CreateLambda([this](UObject* PresetObj)
        {
            if (!PresetObj || !AssetBeingEdited)
            {
                return;
            }

            if (UCharacter2DPartPreset* Part = Cast<UCharacter2DPartPreset>(PresetObj))
            {
                switch (Part->Part)
                {
                case ECharacter2DEditMode::Body:
                    AssetBeingEdited->Body.Mesh = Part->Mesh;
                    break;
                case ECharacter2DEditMode::Arms:
                    AssetBeingEdited->Arms.Mesh = Part->Mesh;
                    break;
                case ECharacter2DEditMode::Head:
                    AssetBeingEdited->Head.Mesh = Part->Mesh;
                    break;
                default:
                    break;
                }
            }
            else if (UCharacter2DPosePreset* Pose = Cast<UCharacter2DPosePreset>(PresetObj))
            {
                if (Pose->BodyPreset && Pose->BodyPreset->Mesh)
                {
                    AssetBeingEdited->Body.Mesh = Pose->BodyPreset->Mesh;
                }
                if (Pose->ArmsPreset && Pose->ArmsPreset->Mesh)
                {
                    AssetBeingEdited->Arms.Mesh = Pose->ArmsPreset->Mesh;
                }
                if (Pose->HeadPreset && Pose->HeadPreset->Mesh)
                {
                    AssetBeingEdited->Head.Mesh = Pose->HeadPreset->Mesh;
                }
            }

            // Refresh views
            if (SkeletalDetailsView.IsValid())
            {
                SkeletalDetailsView->SetObject(AssetBeingEdited);
            }
            if (SpriteDetailsView.IsValid())
            {
                SpriteDetailsView->SetObject(AssetBeingEdited);
            }
            if (ViewportWidget.IsValid())
            {
                ViewportWidget->RefreshPreview();
            }
        }));

    return SNew(SDockTab)
        .Label(LOCTEXT("PresetsLabel", "Presets"))
        [
            PresetPanel.ToSharedRef()
        ];
}

/* ====================================================================== */
/*                        Details View Creation                          */
/* ====================================================================== */

TSharedRef<IDetailsView> FCharacter2DAssetEditorToolkit::CreateSkeletalDetailsView()
{
    // Загружаем модуль редактора свойств
    FPropertyEditorModule& PropertyEditorModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    // Настраиваем аргументы для создания DetailsView
    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.bHideSelectionTip = true;
    DetailsViewArgs.bAllowSearch = true;
    DetailsViewArgs.bShowOptions = false;
    DetailsViewArgs.bAllowMultipleTopLevelObjects = false;
    DetailsViewArgs.NotifyHook = nullptr;

    // Создаём само представление
    TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

    // Устанавливаем делегат видимости свойств: показываем только "скелетные" параметры
    DetailsView->SetIsPropertyVisibleDelegate(
        FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& PropertyAndParent) -> bool
        {
            const FProperty* Property = &PropertyAndParent.Property;
            const FString PropertyName = Property->GetName();
            const FString CategoryName = Property->GetMetaData(TEXT("Category"));

            // Показываем свойства, относящиеся к Skeletal или связанные с Body/Arms/Head/GlobalScale/SkeletalGlobalOffset
            if (CategoryName.Contains(TEXT("Skeletal")) ||
                PropertyName.Contains(TEXT("Body")) ||
                PropertyName.Contains(TEXT("Arms")) ||
                PropertyName.Contains(TEXT("Head")) ||
                PropertyName.Contains(TEXT("GlobalScale")) ||
                PropertyName.Contains(TEXT("SkeletalGlobalOffset")))
            {
                return true;
            }

            // Всё остальное скрываем
            return false;
        })
    );

    // Привязываем к текущему Asset'у и подписываемся на событие изменения
    DetailsView->SetObject(AssetBeingEdited);
    DetailsView->OnFinishedChangingProperties().AddRaw(this, &FCharacter2DAssetEditorToolkit::OnAssetPropertyChanged);

    return DetailsView;
}

TSharedRef<IDetailsView> FCharacter2DAssetEditorToolkit::CreateSpriteDetailsView()
{
    // Загружаем модуль редактора свойств
    FPropertyEditorModule& PropertyEditorModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    // Настраиваем аргументы для создания DetailsView
    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.bHideSelectionTip = true;
    DetailsViewArgs.bAllowSearch = true;
    DetailsViewArgs.bShowOptions = false;
    DetailsViewArgs.bAllowMultipleTopLevelObjects = false;
    DetailsViewArgs.NotifyHook = nullptr;

    // Создаём само представление
    TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

    // ═══ NEW: Updated property filter for hierarchical head structure ═══
    DetailsView->SetIsPropertyVisibleDelegate(
        FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& PropertyAndParent) -> bool
        {
            const FProperty* Property = &PropertyAndParent.Property;
            const FString PropertyName = Property->GetName();
            const FString CategoryName = Property->GetMetaData(TEXT("Category"));

            // Скрываем нежелательные категории
            if (CategoryName.Contains(TEXT("Skeletal")) ||
                CategoryName.Contains(TEXT("Visual Novel")) ||
                CategoryName.Contains(TEXT("General")))
            {
                return false;
            }

            // ═══ NEW: Show hierarchical head categories ═══
            if (CategoryName.StartsWith(TEXT("Sprite")) ||
                CategoryName.Contains(TEXT("Head")) ||        // Head Root, Head Elements, Head Animations
                CategoryName.Contains(TEXT("Blink")) ||       // Blink Animation, Blink Timing
                CategoryName.Contains(TEXT("Talk")) ||        // Talk Animation, Talk Animation|Transform, etc.
                CategoryName == TEXT("Vector"))               // For FVector components (X, Y, Z)
            {
                return true;
            }

            // ═══ NEW: Show specific head-related properties ═══
            if (PropertyName.Contains(TEXT("Head")) ||
                PropertyName.Contains(TEXT("Eyebrow")) ||
                PropertyName.Contains(TEXT("Eyes")) ||
                PropertyName.Contains(TEXT("Eyelids")) ||
                PropertyName.Contains(TEXT("Mouth")) ||
                PropertyName.Contains(TEXT("LocalOffset")) ||
                PropertyName.Contains(TEXT("LocalScale")) ||
                PropertyName.Contains(TEXT("AttachmentTarget")) ||
                PropertyName.Contains(TEXT("SocketName")))
            {
                return true;
            }

            // Скрываем все остальное
            return false;
        })
    );

    // Привязываем представление к текущему Asset'у и подписываемся на событие изменения
    DetailsView->SetObject(AssetBeingEdited);
    DetailsView->OnFinishedChangingProperties().AddRaw(this, &FCharacter2DAssetEditorToolkit::OnAssetPropertyChanged);

    return DetailsView;
}

/* ====================================================================== */
/*                           Event Handlers                              */
/* ====================================================================== */

void FCharacter2DAssetEditorToolkit::OnAssetPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent)
{
    if (!ViewportWidget.IsValid() || !AssetBeingEdited)
    {
        return;
    }

    // Сохраняем состояние видимости ДО обновления
    bool bWasSpritesVisible = true;
    bool bWasSkeletalVisible = true;
    bool bWasActorHidden = false;
    FVector SavedLocation = FVector::ZeroVector;
    FRotator SavedRotation = FRotator::ZeroRotator;
    FVector SavedScale = FVector::OneVector;
    
    if (ACharacter2DActor* ExistingActor = ViewportWidget->GetPreviewActor().Get())
    {
        bWasSpritesVisible = ExistingActor->bSpritesVisible;
        bWasSkeletalVisible = ExistingActor->bSkeletalVisible;
        bWasActorHidden = ExistingActor->IsHidden();
        SavedLocation = ExistingActor->GetActorLocation();
        SavedRotation = ExistingActor->GetActorRotation();
        SavedScale = ExistingActor->GetActorScale3D();
    }

    // ═══ НОВАЯ ЛОГИКА: Обработка изменений свойств головы ═══
    bool bHeadPropertyChanged = false;
    bool bRequiresFullRefresh = false;
    
    if (PropertyChangedEvent.Property)
    {
        const FString PropertyName = PropertyChangedEvent.Property->GetName();
        const FString CategoryName = PropertyChangedEvent.Property->GetMetaData(TEXT("Category"));
        
        // Проверяем, изменились ли свойства головы
        if (PropertyName.Contains(TEXT("Head")) || 
            PropertyName.Contains(TEXT("Eyebrow")) || 
            PropertyName.Contains(TEXT("Eyes")) || 
            PropertyName.Contains(TEXT("Eyelids")) || 
            PropertyName.Contains(TEXT("Mouth")) ||
            CategoryName.Contains(TEXT("Head")))
        {
            bHeadPropertyChanged = true;
            
            // Логируем изменение для отладки
            UE_LOG(LogCharacter2DEditor, Log, TEXT("Head hierarchy property changed: %s (Category: %s)"), 
                   *PropertyName, *CategoryName);
            
            // Специальная обработка трансформационных изменений
            if (PropertyName.Contains(TEXT("Offset")) || 
                PropertyName.Contains(TEXT("Scale")) || 
                PropertyName.Contains(TEXT("AttachmentTarget")) ||
                PropertyName.Contains(TEXT("SocketName")) ||
                PropertyName.Contains(TEXT("bUseSocketTransform")))
            {
                bRequiresFullRefresh = true;
                UE_LOG(LogCharacter2DEditor, Log, TEXT("Head transform/attachment change detected - full refresh required"));
            }
        }
        
        // Проверяем изменения скелетных мешей (тоже требуют полного обновления)
        if (PropertyName.Contains(TEXT("Mesh")) || PropertyName.Contains(TEXT("Body")) || 
            PropertyName.Contains(TEXT("Arms")) || CategoryName.Contains(TEXT("Skeletal")))
        {
            bRequiresFullRefresh = true;
            UE_LOG(LogCharacter2DEditor, Log, TEXT("Skeletal mesh property changed - full refresh required"));
        }
    }

    // ИСПРАВЛЕНИЕ: Используем правильный метод обновления
    if (bRequiresFullRefresh)
    {
        // Для критических изменений используем RefreshFromAsset
        ViewportWidget->RefreshPreview();
    }
    else if (bHeadPropertyChanged)
    {
        // Для менее критических изменений головы тоже обновляем
        ViewportWidget->RefreshPreview();
    }
    else
    {
        // Для остальных изменений - легкое обновление
        ViewportWidget->RefreshPreview();
    }

    // ИСПРАВЛЕНИЕ: Восстанавливаем состояние ПОСЛЕ обновления с задержкой
    // Это гарантирует, что новый актер уже создан
    if (ACharacter2DActor* NewActor = ViewportWidget->GetPreviewActor().Get())
    {
        // Используем тикер для отложенного восстановления состояния
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this, bWasSpritesVisible, bWasSkeletalVisible, bWasActorHidden, SavedLocation, SavedRotation, SavedScale](float DeltaTime)
        {
            if (ViewportWidget.IsValid())
            {
                if (ACharacter2DActor* Actor = ViewportWidget->GetPreviewActor().Get())
                {
                    // Восстанавливаем трансформацию актера
                    Actor->SetActorLocation(SavedLocation);
                    Actor->SetActorRotation(SavedRotation);
                    Actor->SetActorScale3D(SavedScale);
                    
                    // Восстанавливаем видимость
                    Actor->SetBothVisible(bWasSpritesVisible, bWasSkeletalVisible);
                    Actor->SetActorHiddenInGame(bWasActorHidden);
                    
                    UE_LOG(LogCharacter2DEditor, Verbose, TEXT("Preview actor state restored"));
                }
            }
            return false; // Выполнить только один раз
        }), 0.1f);
    }

    // Update action panel if preview actor changed
    if (ActionPanel.IsValid() && ViewportWidget.IsValid())
    {
        // ActionPanel автоматически получит новый PreviewActor через GetPreviewActor()
        // Дополнительная синхронизация не требуется
    }
}

// НОВЫЙ МЕТОД: Валидация конфигурации головы
void FCharacter2DAssetEditorToolkit::ValidateHeadConfiguration()
{
    if (!AssetBeingEdited)
    {
        return;
    }

    const auto& HeadStruct = AssetBeingEdited->SpriteStructure.Head;
    bool bHasWarnings = false;

    // Проверяем, есть ли дочерние элементы без родительского спрайта головы
    if (!HeadStruct.Head.Sprite)
    {
        bool bHasFacialSprites = (HeadStruct.Eyebrows.Sprite || 
                                 HeadStruct.Eyes.Sprite || 
                                 HeadStruct.Eyelids.Sprite || 
                                 HeadStruct.Mouth.Sprite);
        if (bHasFacialSprites)
        {
            UE_LOG(LogCharacter2DEditor, Warning, TEXT("Head hierarchy warning: Facial sprites present but Head root sprite is missing"));
            bHasWarnings = true;
        }
    }

    // Проверяем настройки attachment
    if (HeadStruct.Head.AttachmentTarget != ECharacter2DAttachmentTarget::None)
    {
        if (HeadStruct.Head.SocketName == NAME_None)
        {
            UE_LOG(LogCharacter2DEditor, Warning, TEXT("Head attachment warning: Attachment target set but socket name is empty"));
            bHasWarnings = true;
        }
    }

    // Проверяем анимации
    if (HeadStruct.BlinkSettings.BlinkFlipbook && !HeadStruct.Eyelids.Sprite)
    {
        UE_LOG(LogCharacter2DEditor, Warning, TEXT("Blink animation warning: Blink flipbook set but static eyelids sprite is missing"));
        bHasWarnings = true;
    }

    if (HeadStruct.TalkSettings.TalkFlipbook && !HeadStruct.Mouth.Sprite)
    {
        UE_LOG(LogCharacter2DEditor, Warning, TEXT("Talk animation warning: Talk flipbook set but static mouth sprite is missing"));
        bHasWarnings = true;
    }

    if (bHasWarnings)
    {
        UE_LOG(LogCharacter2DEditor, Log, TEXT("Head configuration validation completed with warnings"));
    }
}

/* ====================================================================== */
/*                        Toolkit Information                            */
/* ====================================================================== */

FName FCharacter2DAssetEditorToolkit::GetToolkitFName() const 
{ 
    return "Character2DAssetEditor"; 
}

FText FCharacter2DAssetEditorToolkit::GetBaseToolkitName() const 
{ 
    return LOCTEXT("AppLabel", "Character2D Asset Editor"); 
}

FString FCharacter2DAssetEditorToolkit::GetWorldCentricTabPrefix() const 
{ 
    return TEXT("Character2DAsset"); 
}

FLinearColor FCharacter2DAssetEditorToolkit::GetWorldCentricTabColorScale() const 
{ 
    return FLinearColor::White; 
}

void FCharacter2DAssetEditorToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
    // TObjectPtr automatically handles reference collection
}

#undef LOCTEXT_NAMESPACE