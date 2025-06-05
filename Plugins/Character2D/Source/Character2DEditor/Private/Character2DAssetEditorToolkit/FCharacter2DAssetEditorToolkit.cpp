#include "Character2DAssetEditorToolkit/FCharacter2DAssetEditorToolkit.h"
#include "Character2DAssetEditorToolkit/SCharacter2DAssetViewport.h"
#include "Character2DAssetEditorToolkit/Slate/SCharacter2DActionPanel.h"
#include "Widgets/Docking/SDockTab.h"
#include "PropertyEditorModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Commands/UICommandList.h"
#include "Data/Character2DPosePreset.h"
#include "IDetailsView.h"
#include "PropertyCustomizationHelpers.h"

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
		FTabManager::NewLayout("Character2DAssetEditorLayout_v4")
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
					AddModeOption(ECharacter2DEditMode::Head, LOCTEXT("ModeHead", "Head"));
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
						// TODO: Set Body/Arms/Head references
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
		}));

	AddToolbarExtender(ToolbarExtender);
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

	// Sprite Details Tab
	InTabManager->RegisterTabSpawner(
		SpriteDetailsTabID,
		FOnSpawnTab::CreateRaw(this, &FCharacter2DAssetEditorToolkit::SpawnSpriteDetailsTab))
		.SetDisplayName(LOCTEXT("SpriteDetailsTab", "Sprites"))
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
		.Label(LOCTEXT("SpriteDetailsLabel", "Sprites"))
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

    // Привязываем к текущему Asset’у и подписываемся на событие изменения
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

	// Устанавливаем делегат видимости свойств:
	// показываем теперь строго только свойства из категорий Sprite, Blink и Talk
	DetailsView->SetIsPropertyVisibleDelegate(
		FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& PropertyAndParent) -> bool
		{
			const FProperty* Property = &PropertyAndParent.Property;
			const FString PropertyName = Property->GetName();
			const FString CategoryName = Property->GetMetaData(TEXT("Category"));

			// Показываем свойства, относящиеся к Sprite (слои, Blink, Talk)
			if (CategoryName.Contains(TEXT("Sprite")) ||
				CategoryName.Contains(TEXT("Blink")) ||
				CategoryName.Contains(TEXT("Talk")))
			{
				return true;
			}

			// Всё остальное скрываем
			return false;
		})
	);

	// Привязываем представление к текущему Asset’у и подписываемся на событие изменения
	DetailsView->SetObject(AssetBeingEdited);
	DetailsView->OnFinishedChangingProperties().AddRaw(this, &FCharacter2DAssetEditorToolkit::OnAssetPropertyChanged);

	return DetailsView;
}

/* ====================================================================== */
/*                           Event Handlers                              */
/* ====================================================================== */

void FCharacter2DAssetEditorToolkit::OnAssetPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (ViewportWidget.IsValid())
	{
		ViewportWidget->RefreshPreview();
	}

	// Update action panel if preview actor changed
	if (ActionPanel.IsValid() && ViewportWidget.IsValid())
	{
		// ActionPanel->SetPreviewActor(ViewportWidget->GetPreviewActor());
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