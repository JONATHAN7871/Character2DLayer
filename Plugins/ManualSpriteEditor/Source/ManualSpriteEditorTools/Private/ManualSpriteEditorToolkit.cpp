#include "ManualSpriteEditorToolkit.h"
#include "ManualSpriteEditorViewport.h"
#include "ManualSpriteEditorCommands.h"
#include "ManualSpriteTransactions.h"
#include "Widgets/Docking/SDockTab.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Commands/UICommandList.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "Editor/Transactor.h"
// v1.1: Новые includes для диалогов
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "ManualSpriteEditorToolkit"

const FName FManualSpriteEditorToolkit::ViewportTabId(TEXT("ManualSpriteEditor_Viewport"));
const FName FManualSpriteEditorToolkit::DetailsTabId(TEXT("ManualSpriteEditor_Details"));

// Конструктор с инициализацией всех полей
FManualSpriteEditorToolkit::FManualSpriteEditorToolkit()
	: ManualSprite(nullptr)
	, CurrentEditMode(EEditMode::Select)
	, CopyOrigin(FVector2D::ZeroVector)
	, PastePreviewPosition(FVector2D::ZeroVector)
{
}

void FManualSpriteEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_ManualSpriteEditor", "Manual Sprite Editor"));
	const auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(ViewportTabId, FOnSpawnTab::CreateSP(this, &FManualSpriteEditorToolkit::SpawnTab_Viewport))
		.SetDisplayName(LOCTEXT("ViewportTab", "Viewport"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));

	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FManualSpriteEditorToolkit::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FManualSpriteEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(ViewportTabId);
	InTabManager->UnregisterTabSpawner(DetailsTabId);
}

void FManualSpriteEditorToolkit::InitManualSpriteEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UManualSprite* InManualSprite)
{
	ManualSprite = InManualSprite;
	CurrentEditMode = EEditMode::Select;
	PastePreviewPosition = FVector2D::ZeroVector;

	// Инициализируем команды ПЕРЕД созданием UI
	InitializeCommands();

	// Создаём viewport
	Viewport = SNew(SManualSpriteEditorViewport)
		.ManualSpriteEditor(SharedThis(this));

	// Создаём details view с поддержкой Undo/Redo
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.NotifyHook = this; // Устанавливаем NotifyHook для отслеживания изменений
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(ManualSprite);

	// Настройка layout редактора
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_ManualSpriteEditor_Layout_v5")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.7f)
					->AddTab(ViewportTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.3f)
					->AddTab(DetailsTabId, ETabState::OpenedTab)
				)
			)
		);

	constexpr bool bCreateDefaultStandaloneMenu = true;
	constexpr bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, FName(TEXT("ManualSpriteEditorApp")), StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, ManualSprite);

	ExtendToolbar();
	
	UE_LOG(LogTemp, Log, TEXT("Manual Sprite Editor v1.1 initialized with Auto-Triangulation support"));
}

void FManualSpriteEditorToolkit::InitializeCommands()
{
	// Регистрируем команды если ещё не зарегистрированы
	if (!FManualSpriteEditorCommands::IsRegistered())
	{
		FManualSpriteEditorCommands::Register();
	}

	// Создаём список команд
	CommandList = MakeShareable(new FUICommandList);
	
	// Привязываем команды к функциям
	BindCommands();
}

void FManualSpriteEditorToolkit::BindCommands()
{
	const FManualSpriteEditorCommands& Commands = FManualSpriteEditorCommands::Get();

	// Режимы редактирования
	CommandList->MapAction(
		Commands.SelectMode,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnSelectMode),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FManualSpriteEditorToolkit::IsSelectModeActive)
	);

	CommandList->MapAction(
		Commands.AddVertexMode,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnAddVertexMode),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FManualSpriteEditorToolkit::IsAddVertexModeActive)
	);

	CommandList->MapAction(
		Commands.TriangleMode,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnTriangleMode),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FManualSpriteEditorToolkit::IsTriangleModeActive)
	);

	CommandList->MapAction(
		Commands.DeleteMode,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnDeleteMode),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FManualSpriteEditorToolkit::IsDeleteModeActive)
	);

	// Undo/Redo
	CommandList->MapAction(
		Commands.Undo,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnUndo),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanUndo)
	);

	CommandList->MapAction(
		Commands.Redo,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnRedo),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanRedo)
	);

	// Удаление выделенного
	CommandList->MapAction(
		Commands.DeleteSelected,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnDeleteSelected),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanDeleteSelected)
	);

	// Копирование/вставка
	CommandList->MapAction(
		Commands.Copy,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnCopy),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanCopy)
	);

	CommandList->MapAction(
		Commands.Paste,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnPaste),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanPaste)
	);

	CommandList->MapAction(
		Commands.Cut,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnCut),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanCopy)
	);

	CommandList->MapAction(
		Commands.Duplicate,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnDuplicate),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanCopy)
	);

	// Сетка
	CommandList->MapAction(
		Commands.ToggleGrid,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnToggleGrid),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FManualSpriteEditorToolkit::IsGridEnabled)
	);

	CommandList->MapAction(
		Commands.ToggleSnap,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnToggleSnap),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FManualSpriteEditorToolkit::IsSnapEnabled)
	);

	// Выделение
	CommandList->MapAction(
		Commands.SelectAll,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnSelectAll)
	);

	CommandList->MapAction(
		Commands.DeselectAll,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnDeselectAll)
	);

	// Утилиты
	CommandList->MapAction(
		Commands.ClearAll,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnClearGeometry)
	);

	CommandList->MapAction(
		Commands.ResetToDefault,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnResetToDefault)
	);

	// v1.1: Команды автоматической триангуляции
	CommandList->MapAction(
		Commands.AutoTriangulate,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnAutoTriangulate),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanAutoTriangulate)
	);

	CommandList->MapAction(
		Commands.ClearTriangles,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnClearTriangles),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanClearTriangles)
	);

	CommandList->MapAction(
		Commands.TriangulateFan,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnTriangulateFan),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanAutoTriangulate)
	);

	CommandList->MapAction(
		Commands.TriangulateDelaunay,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnTriangulateDelaunay),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanAutoTriangulate)
	);

	CommandList->MapAction(
		Commands.TriangulateConvexHull,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnTriangulateConvexHull),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanAutoTriangulate)
	);

	CommandList->MapAction(
		Commands.TriangulateEarClipping,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnTriangulateEarClipping),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanAutoTriangulate)
	);

	CommandList->MapAction(
		Commands.SortVerticesByAngle,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnSortVerticesByAngle),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanSortVertices)
	);

	CommandList->MapAction(
		Commands.ReverseVertexOrder,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnReverseVertexOrder),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::HasVertices)
	);

	CommandList->MapAction(
		Commands.ShowPolygonInfo,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnShowPolygonInfo),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::HasVertices)
	);
}

// v1.1: Расширенный тулбар с автоматической триангуляцией
void FManualSpriteEditorToolkit::ExtendToolbar()
{
    struct FLocal
    {
        static void FillToolbar(FToolBarBuilder& ToolbarBuilder, FManualSpriteEditorToolkit* Toolkit)
        {
            const FManualSpriteEditorCommands& Commands = FManualSpriteEditorCommands::Get();
            
            // Секция Undo/Redo - только иконки
            ToolbarBuilder.BeginSection("UndoRedo");
            {
                ToolbarBuilder.AddToolBarButton(Commands.Undo, NAME_None, FText::GetEmpty(), 
                    LOCTEXT("UndoTooltip", "Undo the last operation"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Undo"));
                    
                ToolbarBuilder.AddToolBarButton(Commands.Redo, NAME_None, FText::GetEmpty(),
                    LOCTEXT("RedoTooltip", "Redo the last undone operation"), 
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Redo"));
            }
            ToolbarBuilder.EndSection();

            // Секция копирования/вставки - только иконки
            ToolbarBuilder.BeginSection("CopyPaste");
            {
                ToolbarBuilder.AddSeparator();
                
                ToolbarBuilder.AddToolBarButton(Commands.Copy, NAME_None, FText::GetEmpty(),
                    LOCTEXT("CopyTooltip", "Copy selected vertices"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"));
                    
                ToolbarBuilder.AddToolBarButton(Commands.Paste, NAME_None, FText::GetEmpty(),
                    LOCTEXT("PasteTooltip", "Paste vertices from clipboard"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Paste"));
                    
                ToolbarBuilder.AddToolBarButton(Commands.Cut, NAME_None, FText::GetEmpty(),
                    LOCTEXT("CutTooltip", "Cut selected vertices"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Cut"));
                    
                ToolbarBuilder.AddToolBarButton(Commands.Duplicate, NAME_None, FText::GetEmpty(),
                    LOCTEXT("DuplicateTooltip", "Duplicate selected vertices"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Duplicate"));
            }
            ToolbarBuilder.EndSection();

            ToolbarBuilder.BeginSection("EditModes");
            {
                ToolbarBuilder.AddSeparator();

                // Режимы редактирования - с текстом и иконками
                ToolbarBuilder.AddToolBarButton(Commands.SelectMode, NAME_None, LOCTEXT("SelectModeText", "Select"),
                    LOCTEXT("SelectModeTooltip", "Select and move vertices (Q)"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.SelectMode"));
                    
                ToolbarBuilder.AddToolBarButton(Commands.AddVertexMode, NAME_None, LOCTEXT("AddVertexModeText", "Add"),
                    LOCTEXT("AddVertexModeTooltip", "Add new vertices (W)"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "Plus"));
                    
                ToolbarBuilder.AddToolBarButton(Commands.TriangleMode, NAME_None, LOCTEXT("TriangleModeText", "Triangle"),
                    LOCTEXT("TriangleModeTooltip", "Create triangles (E)"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Modes"));
                    
                ToolbarBuilder.AddToolBarButton(Commands.DeleteMode, NAME_None, LOCTEXT("DeleteModeText", "Delete"),
                    LOCTEXT("DeleteModeTooltip", "Delete vertices or triangles (R)"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Delete"));
            }
            ToolbarBuilder.EndSection();

            // v1.1: Новая секция автоматической триангуляции
            ToolbarBuilder.BeginSection("AutoTriangulation");
            {
                ToolbarBuilder.AddSeparator();

                // Основная кнопка автотриангуляции
                ToolbarBuilder.AddToolBarButton(Commands.AutoTriangulate, NAME_None, LOCTEXT("AutoTriangulateText", "Auto"),
                    LOCTEXT("AutoTriangulateTooltip", "Auto triangulate using preferred method (T)"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Modes"));

                // Выпадающее меню с различными методами
                ToolbarBuilder.AddComboButton(
                    FUIAction(),
                    FOnGetContent::CreateLambda([Toolkit]() -> TSharedRef<SWidget>
                    {
                        FMenuBuilder MenuBuilder(true, Toolkit->GetCommandList());
                        const FManualSpriteEditorCommands& Commands = FManualSpriteEditorCommands::Get();
                        
                        MenuBuilder.BeginSection("TriangulationMethods", LOCTEXT("TriangulationMethodsSection", "Triangulation Methods"));
                        {
                            MenuBuilder.AddMenuEntry(Commands.TriangulateFan);
                            MenuBuilder.AddMenuEntry(Commands.TriangulateDelaunay);
                            MenuBuilder.AddMenuEntry(Commands.TriangulateConvexHull);
                            MenuBuilder.AddMenuEntry(Commands.TriangulateEarClipping);
                        }
                        MenuBuilder.EndSection();

                        MenuBuilder.BeginSection("TriangulationUtils", LOCTEXT("TriangulationUtilsSection", "Utilities"));
                        {
                            MenuBuilder.AddMenuEntry(Commands.ClearTriangles);
                            MenuBuilder.AddSeparator();
                            MenuBuilder.AddMenuEntry(Commands.SortVerticesByAngle);
                            MenuBuilder.AddMenuEntry(Commands.ReverseVertexOrder);
                            MenuBuilder.AddSeparator();
                            MenuBuilder.AddMenuEntry(Commands.ShowPolygonInfo);
                        }
                        MenuBuilder.EndSection();

                        return MenuBuilder.MakeWidget();
                    }),
                    FText::GetEmpty(),
                    LOCTEXT("TriangulationMethodsTooltip", "Choose triangulation method"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.SelectAll")
                );
            }
            ToolbarBuilder.EndSection();

            // Секция сетки - текст + иконки
            ToolbarBuilder.BeginSection("GridCommands");
            {
                ToolbarBuilder.AddSeparator();

                // Grid и Snap с текстом и одинаковой иконкой
                ToolbarBuilder.AddToolBarButton(Commands.ToggleGrid, NAME_None, LOCTEXT("ToggleGridText", "Grid"),
                    LOCTEXT("ToggleGridTooltip", "Show/Hide grid (G)"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "EditorViewport.LocationGridSnap"));
                    
                ToolbarBuilder.AddToolBarButton(Commands.ToggleSnap, NAME_None, LOCTEXT("ToggleSnapText", "Snap"),
                    LOCTEXT("ToggleSnapTooltip", "Snap to grid (Ctrl+G)"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "EditorViewport.LocationGridSnap"));

                // Компактное меню размеров сетки
                ToolbarBuilder.AddComboButton(
                    FUIAction(),
                    FOnGetContent::CreateLambda([Toolkit]() -> TSharedRef<SWidget>
                    {
                        FMenuBuilder MenuBuilder(true, nullptr);
                        
                        MenuBuilder.AddMenuEntry(
                            LOCTEXT("GridSize10", "10px"),
                            LOCTEXT("GridSize10Tooltip", "Set grid size to 10 pixels"),
                            FSlateIcon(),
                            FUIAction(
                                FExecuteAction::CreateSP(Toolkit, &FManualSpriteEditorToolkit::OnGridSize10),
                                FCanExecuteAction(),
                                FIsActionChecked::CreateSP(Toolkit, &FManualSpriteEditorToolkit::IsGridSize10Active)
                            ),
                            NAME_None,
                            EUserInterfaceActionType::RadioButton
                        );

                        MenuBuilder.AddMenuEntry(
                            LOCTEXT("GridSize25", "25px"),
                            LOCTEXT("GridSize25Tooltip", "Set grid size to 25 pixels"),
                            FSlateIcon(),
                            FUIAction(
                                FExecuteAction::CreateSP(Toolkit, &FManualSpriteEditorToolkit::OnGridSize25),
                                FCanExecuteAction(),
                                FIsActionChecked::CreateSP(Toolkit, &FManualSpriteEditorToolkit::IsGridSize25Active)
                            ),
                            NAME_None,
                            EUserInterfaceActionType::RadioButton
                        );

                        MenuBuilder.AddMenuEntry(
                            LOCTEXT("GridSize50", "50px"),
                            LOCTEXT("GridSize50Tooltip", "Set grid size to 50 pixels"),
                            FSlateIcon(),
                            FUIAction(
                                FExecuteAction::CreateSP(Toolkit, &FManualSpriteEditorToolkit::OnGridSize50),
                                FCanExecuteAction(),
                                FIsActionChecked::CreateSP(Toolkit, &FManualSpriteEditorToolkit::IsGridSize50Active)
                            ),
                            NAME_None,
                            EUserInterfaceActionType::RadioButton
                        );

                        MenuBuilder.AddMenuEntry(
                            LOCTEXT("GridSize100", "100px"),
                            LOCTEXT("GridSize100Tooltip", "Set grid size to 100 pixels"),
                            FSlateIcon(),
                            FUIAction(
                                FExecuteAction::CreateSP(Toolkit, &FManualSpriteEditorToolkit::OnGridSize100),
                                FCanExecuteAction(),
                                FIsActionChecked::CreateSP(Toolkit, &FManualSpriteEditorToolkit::IsGridSize100Active)
                            ),
                            NAME_None,
                            EUserInterfaceActionType::RadioButton
                        );

                        return MenuBuilder.MakeWidget();
                    }),
                    FText::GetEmpty(),
                    LOCTEXT("GridSizeTooltip", "Select grid size"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "EditorViewport.TranslateMode")
                );
            }
            ToolbarBuilder.EndSection();

            ToolbarBuilder.BeginSection("UtilityCommands");
            {
                ToolbarBuilder.AddSeparator();
                
                // Clear All - теперь с текстом и иконкой
                ToolbarBuilder.AddToolBarButton(Commands.ClearAll, NAME_None, LOCTEXT("ClearAllText", "Clear All"),
                    LOCTEXT("ClearAllTooltip", "Clear all geometry"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "Cross"));
                    
                // Reset to Default - теперь с текстом и иконкой
                ToolbarBuilder.AddToolBarButton(Commands.ResetToDefault, NAME_None, LOCTEXT("ResetToDefaultText", "Reset"),
                    LOCTEXT("ResetToDefaultTooltip", "Reset to default quad"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "PropertyWindow.DiffersFromDefault"));
            }
            ToolbarBuilder.EndSection();
        }
    };

    const TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

    ToolbarExtender->AddToolBarExtension(
        "Asset",
        EExtensionHook::After,
        CommandList,
        FToolBarExtensionDelegate::CreateStatic(&FLocal::FillToolbar, this)
    );

    AddToolbarExtender(ToolbarExtender);
    RegenerateMenusAndToolbars();
}

// ========== TABS И ОСНОВНЫЕ ФУНКЦИИ ==========

TSharedRef<SDockTab> FManualSpriteEditorToolkit::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == ViewportTabId);

	return SNew(SDockTab)
		.Label(LOCTEXT("ViewportTab_Title", "Viewport"))
		[
			Viewport.ToSharedRef()
		];
}

TSharedRef<SDockTab> FManualSpriteEditorToolkit::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == DetailsTabId);

	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTab_Title", "Details"))
		[
			DetailsView.ToSharedRef()
		];
}

void FManualSpriteEditorToolkit::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	if (ManualSprite && PropertyThatChanged)
	{
		const FText TransactionText = FText::Format(
			LOCTEXT("PropertyChange", "Change {0}"),
			FText::FromString(PropertyThatChanged->GetName())
		);

		FScopedTransaction Transaction(TransactionText);
		ManualSprite->Modify();

		if (Viewport.IsValid())
		{
			Viewport->RefreshViewport();
		}

		UE_LOG(LogTemp, Log, TEXT("Property changed: %s"), *PropertyThatChanged->GetName());
	}
}

FName FManualSpriteEditorToolkit::GetToolkitFName() const
{
	return FName("ManualSpriteEditor");
}

FText FManualSpriteEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("ManualSpriteEditorAppLabel", "Manual Sprite Editor");
}

FString FManualSpriteEditorToolkit::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Manual Sprite ").ToString();
}

FLinearColor FManualSpriteEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.5f, 0.8f, 0.5f, 0.5f);
}

// v1.1: Новые функции автоматической триангуляции

void FManualSpriteEditorToolkit::AutoTriangulateWithTransaction()
{
	if (!ManualSprite)
		return;

	FAutoTriangulateTransaction Transaction(ManualSprite, ManualSprite->ManualGeometry.PreferredTriangulationMethod);
	Transaction.Execute();

	if (Viewport.IsValid())
	{
		Viewport->RefreshViewport();
	}

	// ИСПРАВЛЕНИЕ: Используем простое описание вместо UEnum::GetValueAsString
	FString MethodName;
	switch (ManualSprite->ManualGeometry.PreferredTriangulationMethod)
	{
	case ETriangulationMethod::Fan:
		MethodName = TEXT("Fan");
		break;
	case ETriangulationMethod::Delaunay:
		MethodName = TEXT("Delaunay");
		break;
	case ETriangulationMethod::ConvexHull:
		MethodName = TEXT("Convex Hull");
		break;
	case ETriangulationMethod::EarClipping:
		MethodName = TEXT("Ear Clipping");
		break;
	default:
		MethodName = TEXT("Unknown");
		break;
	}

	// ИСПРАВЛЕНИЕ: Используем напрямую синхронизированные массивы
	const TArray<FManualSpriteTriangle>& Triangles = ManualSprite->ManualGeometry.Triangles;

	// Показываем уведомление о результате
	const FText NotificationText = FText::Format(
		LOCTEXT("AutoTriangulateComplete", "Auto-triangulation completed. Created {0} triangles using {1} method."),
		FText::AsNumber(Triangles.Num()),
		FText::FromString(MethodName)
	);
	
	FNotificationInfo NotificationInfo(NotificationText);
	NotificationInfo.ExpireDuration = 3.0f;
	NotificationInfo.bFireAndForget = true;
	FSlateNotificationManager::Get().AddNotification(NotificationInfo);

	UE_LOG(LogTemp, Log, TEXT("Auto-triangulation completed with %d triangles"), Triangles.Num());
}

void FManualSpriteEditorToolkit::AutoTriangulateWithMethodAndTransaction(ETriangulationMethod Method)
{
	if (!ManualSprite)
		return;

	FAutoTriangulateTransaction Transaction(ManualSprite, Method);
	Transaction.Execute();

	if (Viewport.IsValid())
	{
		Viewport->RefreshViewport();
	}

	// ИСПРАВЛЕНИЕ: Используем простое описание вместо UEnum::GetValueAsString
	FString MethodName;
	switch (Method)
	{
	case ETriangulationMethod::Fan:
		MethodName = TEXT("Fan");
		break;
	case ETriangulationMethod::Delaunay:
		MethodName = TEXT("Delaunay");
		break;
	case ETriangulationMethod::ConvexHull:
		MethodName = TEXT("Convex Hull");
		break;
	case ETriangulationMethod::EarClipping:
		MethodName = TEXT("Ear Clipping");
		break;
	default:
		MethodName = TEXT("Unknown");
		break;
	}

	// ИСПРАВЛЕНИЕ: Используем напрямую синхронизированные массивы
	const TArray<FManualSpriteTriangle>& Triangles = ManualSprite->ManualGeometry.Triangles;

	// Показываем уведомление о результате
	const FText NotificationText = FText::Format(
		LOCTEXT("AutoTriangulateMethodComplete", "Triangulation with {0} method completed. Created {1} triangles."),
		FText::FromString(MethodName),
		FText::AsNumber(Triangles.Num())
	);
	
	FNotificationInfo NotificationInfo(NotificationText);
	NotificationInfo.ExpireDuration = 3.0f;
	NotificationInfo.bFireAndForget = true;
	FSlateNotificationManager::Get().AddNotification(NotificationInfo);

	UE_LOG(LogTemp, Log, TEXT("Triangulation with method %d completed with %d triangles"), 
		   static_cast<int32>(Method), Triangles.Num());
}

void FManualSpriteEditorToolkit::ClearTrianglesWithTransaction()
{
	if (!ManualSprite)
		return;

	FClearTrianglesTransaction Transaction(ManualSprite);
	Transaction.Execute();

	if (Viewport.IsValid())
	{
		Viewport->RefreshViewport();
	}

	UE_LOG(LogTemp, Log, TEXT("Cleared all triangles"));
}

void FManualSpriteEditorToolkit::SortVerticesByAngleWithTransaction()
{
	if (!ManualSprite)
		return;

	FSortVerticesByAngleTransaction Transaction(ManualSprite);
	Transaction.Execute();

	if (Viewport.IsValid())
	{
		Viewport->RefreshViewport();
	}

	UE_LOG(LogTemp, Log, TEXT("Sorted vertices by angle"));
}

void FManualSpriteEditorToolkit::ReverseVertexOrderWithTransaction()
{
	if (!ManualSprite)
		return;

	FReverseVertexOrderTransaction Transaction(ManualSprite);
	Transaction.Execute();

	if (Viewport.IsValid())
	{
		Viewport->RefreshViewport();
	}

	UE_LOG(LogTemp, Log, TEXT("Reversed vertex order"));
}

// v1.1: Функция получения информации о полигоне
FManualSpriteEditorToolkit::FPolygonInfo FManualSpriteEditorToolkit::GetPolygonInfo() const
{
	FPolygonInfo Info;
	
	if (!ManualSprite)
		return Info;

	// ИСПРАВЛЕНИЕ: Используем напрямую синхронизированные массивы
	const TArray<FManualSpriteVertex>& Vertices = ManualSprite->ManualGeometry.Vertices;
	const TArray<FManualSpriteTriangle>& Triangles = ManualSprite->ManualGeometry.Triangles;
	
	Info.VertexCount = Vertices.Num();
	Info.TriangleCount = Triangles.Num();
	Info.bIsConvex = ManualSprite->IsConvexPolygon();
	
	// ИСПРАВЛЕНИЕ: Вычисляем центроид локально
	if (Info.VertexCount > 0)
	{
		FVector2D CentroidSum = FVector2D::ZeroVector;
		for (const FManualSpriteVertex& Vertex : Vertices)
		{
			CentroidSum += Vertex.Position;
		}
		Info.Centroid = CentroidSum / static_cast<float>(Info.VertexCount);
	}
	else
	{
		Info.Centroid = FVector2D::ZeroVector;
	}

	// Вычисляем площадь и bounding box
	if (Info.VertexCount > 0)
	{
		Info.BoundingBoxMin = Vertices[0].Position;
		Info.BoundingBoxMax = Vertices[0].Position;
		
		float TotalArea = 0.0f;
		
		// Bounding box
		for (const FManualSpriteVertex& Vertex : Vertices)
		{
			Info.BoundingBoxMin.X = FMath::Min(Info.BoundingBoxMin.X, Vertex.Position.X);
			Info.BoundingBoxMin.Y = FMath::Min(Info.BoundingBoxMin.Y, Vertex.Position.Y);
			Info.BoundingBoxMax.X = FMath::Max(Info.BoundingBoxMax.X, Vertex.Position.X);
			Info.BoundingBoxMax.Y = FMath::Max(Info.BoundingBoxMax.Y, Vertex.Position.Y);
		}
		
		// Площадь полигона (используя формулу Гаусса)
		for (int32 i = 0; i < Info.VertexCount; i++)
		{
			const int32 NextIndex = (i + 1) % Info.VertexCount;
			const FVector2D Current = Vertices[i].Position;
			const FVector2D Next = Vertices[NextIndex].Position;
			
			TotalArea += (Current.X * Next.Y - Next.X * Current.Y);
		}
		
		Info.Area = FMath::Abs(TotalArea) * 0.5f;
	}

	return Info;
}

void FManualSpriteEditorToolkit::ShowPolygonInfoDialog()
{
	const FPolygonInfo Info = GetPolygonInfo();
	
	// Создаём содержимое диалога
	TSharedRef<SVerticalBox> DialogContent = SNew(SVerticalBox);
	
	// Заголовок
	DialogContent->AddSlot()
	.AutoHeight()
	.Padding(5)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("PolygonInfoTitle", "Polygon Information"))
		.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
		.Justification(ETextJustify::Center)
	];

	// Основная информация
	DialogContent->AddSlot()
	.AutoHeight()
	.Padding(5)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(FText::Format(LOCTEXT("VertexCountInfo", "Vertices: {0}"), FText::AsNumber(Info.VertexCount)))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(FText::Format(LOCTEXT("TriangleCountInfo", "Triangles: {0}"), FText::AsNumber(Info.TriangleCount)))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(FText::Format(LOCTEXT("ConvexInfo", "Convex: {0}"), Info.bIsConvex ? LOCTEXT("Yes", "Yes") : LOCTEXT("No", "No")))
				.ColorAndOpacity(Info.bIsConvex ? FLinearColor::Green : FLinearColor::Yellow)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(FText::Format(LOCTEXT("AreaInfo", "Area: {0:0.2f} units²"), FText::AsNumber(Info.Area)))
			]
		]
	];

	// Геометрическая информация
	DialogContent->AddSlot()
	.AutoHeight()
	.Padding(5)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("GeometricInfoTitle", "Geometric Properties"))
			.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("CentroidInfo", "Centroid: ({0:0.1f}, {1:0.1f})"), 
				FText::AsNumber(Info.Centroid.X), FText::AsNumber(Info.Centroid.Y)))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("BoundingBoxInfo", "Bounding Box: ({0:0.1f}, {1:0.1f}) to ({2:0.1f}, {3:0.1f})"), 
				FText::AsNumber(Info.BoundingBoxMin.X), FText::AsNumber(Info.BoundingBoxMin.Y),
				FText::AsNumber(Info.BoundingBoxMax.X), FText::AsNumber(Info.BoundingBoxMax.Y)))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("SizeInfo", "Size: {0:0.1f} × {1:0.1f}"), 
				FText::AsNumber(Info.BoundingBoxMax.X - Info.BoundingBoxMin.X),
				FText::AsNumber(Info.BoundingBoxMax.Y - Info.BoundingBoxMin.Y)))
		]
	];

	// Рекомендации по триангуляции
	DialogContent->AddSlot()
	.AutoHeight()
	.Padding(5)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TriangulationRecommendationsTitle", "Triangulation Recommendations"))
			.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(STextBlock)
			.Text(Info.bIsConvex ? 
				LOCTEXT("ConvexRecommendation", "• Use Fan or Delaunay for best results\n• Convex Hull will give same result as Fan") :
				LOCTEXT("ConcaveRecommendation", "• Use Ear Clipping for complex shapes\n• Delaunay for automatic optimization\n• Consider sorting vertices first"))
			.AutoWrapText(true)
		]
	];

	// Показываем диалог
	FText DialogTitle = LOCTEXT("PolygonInfoDialogTitle", "Polygon Analysis");
	
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(DialogTitle)
		.SizingRule(ESizingRule::Autosized)
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		[
			SNew(SBorder)
			.Padding(10)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					DialogContent
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.Padding(0, 10, 0, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("CloseButton", "Close"))
					.OnClicked_Lambda([Window]() -> FReply
					{
						Window->RequestDestroyWindow();
						return FReply::Handled();
					})
				]
			]
		];

	FSlateApplication::Get().AddWindow(Window);
}

// v1.1: Команды автоматической триангуляции
void FManualSpriteEditorToolkit::OnAutoTriangulate()
{
	AutoTriangulateWithTransaction();
}

void FManualSpriteEditorToolkit::OnClearTriangles()
{
	ClearTrianglesWithTransaction();
}

void FManualSpriteEditorToolkit::OnTriangulateFan()
{
	AutoTriangulateWithMethodAndTransaction(ETriangulationMethod::Fan);
}

void FManualSpriteEditorToolkit::OnTriangulateDelaunay()
{
	AutoTriangulateWithMethodAndTransaction(ETriangulationMethod::Delaunay);
}

void FManualSpriteEditorToolkit::OnTriangulateConvexHull()
{
	AutoTriangulateWithMethodAndTransaction(ETriangulationMethod::ConvexHull);
}

void FManualSpriteEditorToolkit::OnTriangulateEarClipping()
{
	AutoTriangulateWithMethodAndTransaction(ETriangulationMethod::EarClipping);
}

void FManualSpriteEditorToolkit::OnSortVerticesByAngle()
{
	SortVerticesByAngleWithTransaction();
}

void FManualSpriteEditorToolkit::OnReverseVertexOrder()
{
	ReverseVertexOrderWithTransaction();
}

void FManualSpriteEditorToolkit::OnShowPolygonInfo()
{
	ShowPolygonInfoDialog();
}

// v1.1: Проверки состояний для новых команд
bool FManualSpriteEditorToolkit::CanAutoTriangulate() const
{
	return ManualSprite && ManualSprite->CanAutoTriangulate();
}

bool FManualSpriteEditorToolkit::CanClearTriangles() const
{
	if (!ManualSprite)
		return false;
	
	// ИСПРАВЛЕНИЕ: Используем напрямую синхронизированные массивы
	const TArray<FManualSpriteTriangle>& Triangles = ManualSprite->ManualGeometry.Triangles;
	return Triangles.Num() > 0;
}

bool FManualSpriteEditorToolkit::CanSortVertices() const
{
	if (!ManualSprite)
		return false;
	
	// ИСПРАВЛЕНИЕ: Используем напрямую синхронизированные массивы
	const TArray<FManualSpriteVertex>& Vertices = ManualSprite->ManualGeometry.Vertices;
	return Vertices.Num() >= 3;
}

bool FManualSpriteEditorToolkit::HasVertices() const
{
	if (!ManualSprite)
		return false;
	
	// ИСПРАВЛЕНИЕ: Используем напрямую синхронизированные массивы
	const TArray<FManualSpriteVertex>& Vertices = ManualSprite->ManualGeometry.Vertices;
	return Vertices.Num() > 0;
}

// ========== ОСТАЛЬНЫЕ ФУНКЦИИ (СУЩЕСТВУЮЩИЕ) ==========

// Функции копирования/вставки (существующий код)
void FManualSpriteEditorToolkit::CopySelectedVertices()
{
	if (!Viewport.IsValid() || !ManualSprite)
		return;

	// ИСПРАВЛЕНИЕ: Мы должны привести тип к нашему классу, чтобы получить доступ к его методам
	TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient();
	if (!ViewportClient.IsValid())
		return;
    
	// Теперь мы можем безопасно вызывать кастомные методы
	const TArray<int32>& SelectedVertices = ViewportClient->GetSelectedVertices();
	if (SelectedVertices.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No vertices selected for copying"));
		return;
	}

	CopiedVertices.Empty();
	CopyOrigin = CalculateCopyOrigin(SelectedVertices);

	const FManualSpriteGeometry& Geometry = ManualSprite->ManualGeometry;
	for (int32 VertexIndex : SelectedVertices)
	{
		if (VertexIndex >= 0 && VertexIndex < Geometry.Vertices.Num())
		{
			const FManualSpriteVertex& Vertex = Geometry.Vertices[VertexIndex];
			const FVector2D RelativePosition = Vertex.Position - CopyOrigin;
			CopiedVertices.Add(FCopiedVertexData(RelativePosition, Vertex.UV));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Copied %d vertices to clipboard"), CopiedVertices.Num());
}

void FManualSpriteEditorToolkit::PasteVertices(const FVector2D& PastePosition)
{
	if (!ManualSprite || CopiedVertices.Num() == 0)
		return;

	TArray<FVector2D> NewPositions;
	TArray<FVector2D> NewUVs;

	for (const FCopiedVertexData& CopiedVertex : CopiedVertices)
	{
		const FVector2D NewPosition = PastePosition + CopiedVertex.RelativePosition;
		NewPositions.Add(NewPosition);
		NewUVs.Add(CopiedVertex.UV);
	}

	AddVerticesWithTransaction(NewPositions, NewUVs);

	if (Viewport.IsValid())
	{
		// ИСПРАВЛЕНИЕ: То же самое приведение типа
		TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient();
		if (ViewportClient.IsValid())
		{
			TArray<int32> NewSelection;
			const int32 StartIndex = ManualSprite->ManualGeometry.Vertices.Num() - CopiedVertices.Num();
			
			for (int32 i = 0; i < CopiedVertices.Num(); i++)
			{
				NewSelection.Add(StartIndex + i);
			}
			
			// Теперь вызов корректен
			ViewportClient->SetSelectedVertices(NewSelection);
			Viewport->RefreshViewport();
		}
	}

	if (CurrentEditMode == EEditMode::Paste)
	{
		SetEditMode(EEditMode::Select);
	}

	UE_LOG(LogTemp, Log, TEXT("Pasted %d vertices at position (%.2f, %.2f)"), 
		   CopiedVertices.Num(), PastePosition.X, PastePosition.Y);
}

void FManualSpriteEditorToolkit::CutSelectedVertices()
{
	CopySelectedVertices();
	OnDeleteSelected();
	UE_LOG(LogTemp, Log, TEXT("Cut selected vertices"));
}

void FManualSpriteEditorToolkit::DuplicateSelectedVertices()
{
	if (!Viewport.IsValid())
		return;

	auto ViewportClient = Viewport->GetViewportClient();
	if (!ViewportClient.IsValid())
		return;

	CopySelectedVertices();
	
	if (CopiedVertices.Num() == 0)
		return;

	const float Offset = GridSettings.GridSize * 2.0f;
	const FVector2D DuplicatePosition = CopyOrigin + FVector2D(Offset, Offset);
	
	PasteVertices(DuplicatePosition);
	
	UE_LOG(LogTemp, Log, TEXT("Duplicated selected vertices"));
}

bool FManualSpriteEditorToolkit::CanPaste() const
{
	return CopiedVertices.Num() > 0;
}

bool FManualSpriteEditorToolkit::HasCopiedVertices() const
{
	return CopiedVertices.Num() > 0;
}

void FManualSpriteEditorToolkit::SetPastePreviewPosition(const FVector2D& Position)
{
	PastePreviewPosition = Position;
}

TArray<FVector2D> FManualSpriteEditorToolkit::GetPastePreviewVertices() const
{
	TArray<FVector2D> PreviewVertices;
	
	if (CurrentEditMode == EEditMode::Paste && CopiedVertices.Num() > 0)
	{
		for (const FCopiedVertexData& CopiedVertex : CopiedVertices)
		{
			const FVector2D PreviewPosition = PastePreviewPosition + CopiedVertex.RelativePosition;
			PreviewVertices.Add(PreviewPosition);
		}
	}
	
	return PreviewVertices;
}

FVector2D FManualSpriteEditorToolkit::CalculateCopyOrigin(const TArray<int32>& VertexIndices) const
{
	if (!ManualSprite || VertexIndices.Num() == 0)
		return FVector2D::ZeroVector;

	const FManualSpriteGeometry& Geometry = ManualSprite->ManualGeometry;
	FVector2D Sum = FVector2D::ZeroVector;
	int32 ValidVertices = 0;

	for (int32 VertexIndex : VertexIndices)
	{
		if (VertexIndex >= 0 && VertexIndex < Geometry.Vertices.Num())
		{
			Sum += Geometry.Vertices[VertexIndex].Position;
			ValidVertices++;
		}
	}

	if (ValidVertices > 0)
	{
		return Sum / static_cast<float>(ValidVertices);
	}

	return FVector2D::ZeroVector;
}

void FManualSpriteEditorToolkit::AddVerticesWithTransaction(const TArray<FVector2D>& Positions, const TArray<FVector2D>& UVs)
{
	if (!ManualSprite || Positions.Num() != UVs.Num())
		return;

	FPasteVerticesTransaction Transaction(ManualSprite, Positions, UVs);
	Transaction.Execute();

	UE_LOG(LogTemp, Log, TEXT("Added %d vertices with transaction"), Positions.Num());
}

// Команды копирования/вставки
void FManualSpriteEditorToolkit::OnCopy()
{
	CopySelectedVertices();
}

void FManualSpriteEditorToolkit::OnPaste()
{
	if (CanPaste())
	{
		if (CurrentEditMode == EEditMode::Paste)
		{
			SetEditMode(EEditMode::Select);
		}
		else
		{
			SetEditMode(EEditMode::Paste);
			
			if (Viewport.IsValid())
			{
				SetPastePreviewPosition(FVector2D::ZeroVector);
				Viewport->RefreshViewport();
			}
		}
	}
}

void FManualSpriteEditorToolkit::OnCut()
{
	CutSelectedVertices();
}

void FManualSpriteEditorToolkit::OnDuplicate()
{
	DuplicateSelectedVertices();
}

bool FManualSpriteEditorToolkit::CanCopy() const
{
	if (!Viewport.IsValid())
		return false;

	// ИСПРАВЛЕНИЕ: Приведение типа
	TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient();
	if (!ViewportClient.IsValid())
		return false;
    
	// Вызов корректен
	return ViewportClient->GetSelectedVertices().Num() > 0;
}

// Функции транзакций
void FManualSpriteEditorToolkit::AddVertexWithTransaction(const FVector2D& Position, const FVector2D& UV)
{
	FAddVertexTransaction Transaction(ManualSprite, Position, UV);
	Transaction.Execute();
	
	if (Viewport.IsValid())
	{
		Viewport->RefreshViewport();
	}

	UE_LOG(LogTemp, Log, TEXT("Added vertex with transaction at (%.2f, %.2f)"), Position.X, Position.Y);
}

void FManualSpriteEditorToolkit::RemoveVertexWithTransaction(int32 VertexIndex)
{
	if (VertexIndex >= 0 && ManualSprite && VertexIndex < ManualSprite->ManualGeometry.Vertices.Num())
	{
		FRemoveVertexTransaction Transaction(ManualSprite, VertexIndex);
		Transaction.Execute();
		
		if (Viewport.IsValid())
		{
			Viewport->RefreshViewport();
		}

		UE_LOG(LogTemp, Log, TEXT("Removed vertex %d with transaction"), VertexIndex);
	}
}

void FManualSpriteEditorToolkit::AddTriangleWithTransaction(int32 Index0, int32 Index1, int32 Index2)
{
	FAddTriangleTransaction Transaction(ManualSprite, Index0, Index1, Index2);
	Transaction.Execute();
	
	if (Viewport.IsValid())
	{
		Viewport->RefreshViewport();
	}

	UE_LOG(LogTemp, Log, TEXT("Added triangle with transaction (%d, %d, %d)"), Index0, Index1, Index2);
}

void FManualSpriteEditorToolkit::RemoveTriangleWithTransaction(int32 TriangleIndex)
{
	if (TriangleIndex >= 0 && ManualSprite && TriangleIndex < ManualSprite->ManualGeometry.Triangles.Num())
	{
		FRemoveTriangleTransaction Transaction(ManualSprite, TriangleIndex);
		Transaction.Execute();
		
		if (Viewport.IsValid())
		{
			Viewport->RefreshViewport();
		}

		UE_LOG(LogTemp, Log, TEXT("Removed triangle %d with transaction"), TriangleIndex);
	}
}

void FManualSpriteEditorToolkit::MoveVertexWithTransaction(int32 VertexIndex, const FVector2D& NewPosition, const FVector2D& NewUV)
{
	if (VertexIndex >= 0 && ManualSprite && VertexIndex < ManualSprite->ManualGeometry.Vertices.Num())
	{
		FMoveVertexTransaction Transaction(ManualSprite, VertexIndex, NewPosition, NewUV);
		Transaction.Execute();
		
		if (Viewport.IsValid())
		{
			Viewport->RefreshViewport();
		}

		UE_LOG(LogTemp, Log, TEXT("Moved vertex %d with transaction to (%.2f, %.2f)"), VertexIndex, NewPosition.X, NewPosition.Y);
	}
}

void FManualSpriteEditorToolkit::MoveVerticesWithTransaction(const TArray<int32>& VertexIndices, const TArray<FVector2D>& NewPositions, const TArray<FVector2D>& NewUVs)
{
	if (!ManualSprite || VertexIndices.Num() != NewPositions.Num() || VertexIndices.Num() != NewUVs.Num())
		return;

	FMoveVerticesTransaction Transaction(ManualSprite, VertexIndices, NewPositions, NewUVs);
	Transaction.Execute();
	
	if (Viewport.IsValid())
	{
		Viewport->RefreshViewport();
	}

	UE_LOG(LogTemp, Log, TEXT("Moved %d vertices with transaction"), VertexIndices.Num());
}

void FManualSpriteEditorToolkit::ClearGeometryWithTransaction()
{
	FClearGeometryTransaction Transaction(ManualSprite);
	Transaction.Execute();
	
	if (Viewport.IsValid())
	{
		Viewport->RefreshViewport();
	}

	UE_LOG(LogTemp, Log, TEXT("Cleared geometry with transaction"));
}

void FManualSpriteEditorToolkit::ResetGeometryWithTransaction()
{
	FResetGeometryTransaction Transaction(ManualSprite);
	Transaction.Execute();
	
	if (Viewport.IsValid())
	{
		Viewport->RefreshViewport();
	}

	UE_LOG(LogTemp, Log, TEXT("Reset geometry with transaction"));
}

// Команды Undo/Redo
void FManualSpriteEditorToolkit::OnUndo()
{
	if (CanUndo())
	{
		GEditor->UndoTransaction();
		
		if (Viewport.IsValid())
		{
			Viewport->RefreshViewport();
		}
		
		if (DetailsView.IsValid())
		{
			DetailsView->ForceRefresh();
		}

		UE_LOG(LogTemp, Log, TEXT("Undo executed"));
	}
}

void FManualSpriteEditorToolkit::OnRedo()
{
	if (CanRedo())
	{
		GEditor->RedoTransaction();
		
		if (Viewport.IsValid())
		{
			Viewport->RefreshViewport();
		}
		
		if (DetailsView.IsValid())
		{
			DetailsView->ForceRefresh();
		}

		UE_LOG(LogTemp, Log, TEXT("Redo executed"));
	}
}

bool FManualSpriteEditorToolkit::CanUndo() const
{
	return GEditor && GEditor->Trans && GEditor->Trans->CanUndo();
}

bool FManualSpriteEditorToolkit::CanRedo() const
{
	return GEditor && GEditor->Trans && GEditor->Trans->CanRedo();
}

// Команды выделения и удаления
void FManualSpriteEditorToolkit::OnDeleteSelected()
{
	if (Viewport.IsValid())
	{
		// ИСПРАВЛЕНИЕ: Приведение типа
		if (TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient())
		{
			// Вызов корректен
			ViewportClient->DeleteSelectedVertices();
			Viewport->RefreshViewport();
		}
	}
}

bool FManualSpriteEditorToolkit::CanDeleteSelected() const
{
	if (Viewport.IsValid())
	{
		// ИСПРАВЛЕНИЕ: Приведение типа
		if (TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient())
		{
			// Вызов корректен
			return ViewportClient->HasSelection();
		}
	}
	return false;
}

void FManualSpriteEditorToolkit::OnSelectAll()
{
	if (Viewport.IsValid())
	{
		// ИСПРАВЛЕНИЕ: Приведение типа
		if (TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient())
		{
			// Вызов корректен
			ViewportClient->SelectAllVertices();
			Viewport->RefreshViewport();
		}
	}
}

void FManualSpriteEditorToolkit::OnDeselectAll()
{
	if (Viewport.IsValid())
	{
		// ИСПРАВЛЕНИЕ: Приведение типа
		if (TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient())
		{
			// Вызов корректен
			ViewportClient->ClearSelection();
			Viewport->RefreshViewport();
		}
	}
}

// Режимы редактирования
void FManualSpriteEditorToolkit::OnSelectMode()
{
	SetEditMode(EEditMode::Select);
}

void FManualSpriteEditorToolkit::OnAddVertexMode()
{
	SetEditMode(EEditMode::AddVertex);
}

void FManualSpriteEditorToolkit::OnTriangleMode()
{
	SetEditMode(EEditMode::Triangle);
}

void FManualSpriteEditorToolkit::OnDeleteMode()
{
	SetEditMode(EEditMode::Delete);
}

void FManualSpriteEditorToolkit::OnClearGeometry()
{
	ClearGeometryWithTransaction();
}

void FManualSpriteEditorToolkit::OnResetToDefault()
{
	ResetGeometryWithTransaction();
}

bool FManualSpriteEditorToolkit::IsSelectModeActive() const
{
	return CurrentEditMode == EEditMode::Select;
}

bool FManualSpriteEditorToolkit::IsAddVertexModeActive() const
{
	return CurrentEditMode == EEditMode::AddVertex;
}

bool FManualSpriteEditorToolkit::IsTriangleModeActive() const
{
	return CurrentEditMode == EEditMode::Triangle;
}

bool FManualSpriteEditorToolkit::IsDeleteModeActive() const
{
	return CurrentEditMode == EEditMode::Delete;
}

bool FManualSpriteEditorToolkit::IsPasteModeActive() const
{
	return CurrentEditMode == EEditMode::Paste;
}

void FManualSpriteEditorToolkit::SetEditMode(EEditMode NewMode) 
{ 
	CurrentEditMode = NewMode; 
    
	FString ModeName;
	switch(NewMode)
	{
	case EEditMode::Select: ModeName = TEXT("Select"); break;
	case EEditMode::AddVertex: ModeName = TEXT("AddVertex"); break;
	case EEditMode::Triangle: ModeName = TEXT("Triangle"); break;
	case EEditMode::Delete: ModeName = TEXT("Delete"); break;
	case EEditMode::Paste: ModeName = TEXT("Paste"); break;
	}
    
	UE_LOG(LogTemp, Log, TEXT("Manual Sprite Editor: Switched to mode %s"), *ModeName);
	
	if (Viewport.IsValid())
	{
		Viewport->RefreshViewport();
	}
}

// Функции сетки
void FManualSpriteEditorToolkit::ToggleGridDisplay()
{
	GridSettings.bShowGrid = !GridSettings.bShowGrid;
	if (Viewport.IsValid())
	{
		Viewport->RefreshViewport();
	}
	UE_LOG(LogTemp, Log, TEXT("Grid display: %s"), GridSettings.bShowGrid ? TEXT("ON") : TEXT("OFF"));
}

void FManualSpriteEditorToolkit::ToggleGridSnap()
{
	GridSettings.bSnapToGrid = !GridSettings.bSnapToGrid;
	UE_LOG(LogTemp, Log, TEXT("Grid snap: %s"), GridSettings.bSnapToGrid ? TEXT("ON") : TEXT("OFF"));
}

void FManualSpriteEditorToolkit::SetGridSize(float NewSize)
{
	GridSettings.GridSize = NewSize;
	if (Viewport.IsValid())
	{
		Viewport->RefreshViewport();
	}
	UE_LOG(LogTemp, Log, TEXT("Grid size set to: %.1f"), NewSize);
}

void FManualSpriteEditorToolkit::OnToggleGrid()
{
	ToggleGridDisplay();
}

void FManualSpriteEditorToolkit::OnToggleSnap()
{
	ToggleGridSnap();
}

void FManualSpriteEditorToolkit::OnGridSize10()
{
	SetGridSize(10.0f);
}

void FManualSpriteEditorToolkit::OnGridSize25()
{
	SetGridSize(25.0f);
}

void FManualSpriteEditorToolkit::OnGridSize50()
{
	SetGridSize(50.0f);
}

void FManualSpriteEditorToolkit::OnGridSize100()
{
	SetGridSize(100.0f);
}

bool FManualSpriteEditorToolkit::IsGridEnabled() const
{
	return GridSettings.bShowGrid;
}

bool FManualSpriteEditorToolkit::IsSnapEnabled() const
{
	return GridSettings.bSnapToGrid;
}

bool FManualSpriteEditorToolkit::IsGridSize10Active() const
{
	return FMath::IsNearlyEqual(GridSettings.GridSize, 10.0f);
}

bool FManualSpriteEditorToolkit::IsGridSize25Active() const
{
	return FMath::IsNearlyEqual(GridSettings.GridSize, 25.0f);
}

bool FManualSpriteEditorToolkit::IsGridSize50Active() const
{
	return FMath::IsNearlyEqual(GridSettings.GridSize, 50.0f);
}

bool FManualSpriteEditorToolkit::IsGridSize100Active() const
{
	return FMath::IsNearlyEqual(GridSettings.GridSize, 100.0f);
}

#undef LOCTEXT_NAMESPACE