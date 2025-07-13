#include "ManualSpriteEditorToolkit.h"
#include "ManualSpriteEditorViewport.h"
#include "ManualSpriteEditorCommands.h"
#include "ManualSpriteTransactions.h"
#include "PaperSprite.h"
#include "Widgets/Docking/SDockTab.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Commands/UICommandList.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "ManualSpriteMeshGenerator.h"
#include "ScopedTransaction.h"
#include "Editor/Transactor.h"

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
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_ManualSpriteEditor_Layout_v2")
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
	
	UE_LOG(LogTemp, Log, TEXT("Manual Sprite Editor initialized"));
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

	// Генерация мешей
	CommandList->MapAction(
		Commands.GenerateMesh,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnGenerateMesh),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanGenerateMesh)
	);

	// Импорт геометрии
	CommandList->MapAction(
		Commands.ImportRenderGeometry,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnImportRenderGeometry),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanImportRenderGeometry)
	);

	// Валидация триангуляции
	CommandList->MapAction(
		Commands.ValidateTriangulation,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnValidateTriangulation),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanValidateTriangulation)
	);
	
	CommandList->MapAction(
		Commands.AutoTriangulate,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnAutoTriangulate),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanAutoTriangulate)
	);

	// Удаление треугольников
	CommandList->MapAction(
		Commands.DeleteTriangles,
		FExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::OnDeleteTriangles),
		FCanExecuteAction::CreateSP(this, &FManualSpriteEditorToolkit::CanDeleteTriangles)
	);
	
}

// Упрощенный тулбар без автоматической триангуляции
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

        	// Секция утилит
        	ToolbarBuilder.BeginSection("UtilityCommands");
	        {
            	ToolbarBuilder.AddSeparator();

            	// Clear All
            	ToolbarBuilder.AddToolBarButton(Commands.ClearAll, NAME_None, LOCTEXT("ClearAllText", "Clear All"),
					LOCTEXT("ClearAllTooltip", "Clear all geometry"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "Cross"));

            	// Reset to Default
            	ToolbarBuilder.AddToolBarButton(Commands.ResetToDefault, NAME_None, LOCTEXT("ResetToDefaultText", "Reset"),
					LOCTEXT("ResetToDefaultTooltip", "Reset to default quad"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "PropertyWindow.DiffersFromDefault"));

            	// НОВОЕ: Кнопка валидации пересечений
            	ToolbarBuilder.AddToolBarButton(Commands.ValidateTriangulation, NAME_None, LOCTEXT("ValidateText", "Validate"),
					LOCTEXT("ValidateTooltip", "Check triangulation for intersecting edges (V)"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "Automation.Success"));

            	// НОВОЕ: Auto Triangulate
            	ToolbarBuilder.AddToolBarButton(Commands.AutoTriangulate, NAME_None, LOCTEXT("AutoTriangulateText", "Auto Triangulate"),
					LOCTEXT("AutoTriangulateTooltip", "Automatically triangulate selected vertices (3)"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Modes"));

            	// Delete Triangles
            	ToolbarBuilder.AddToolBarButton(Commands.DeleteTriangles, NAME_None, LOCTEXT("DeleteTrianglesText", "Delete Triangles"),
					LOCTEXT("DeleteTrianglesTooltip", "Delete all triangles connected to selected vertices (4)"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Delete"));
	        }
        	ToolbarBuilder.EndSection();

        	// Секция генерации меша
        	ToolbarBuilder.BeginSection("MeshGeneration");
	        {
            	ToolbarBuilder.AddSeparator();

            	// Generate Mesh
            	ToolbarBuilder.AddToolBarButton(Commands.GenerateMesh, NAME_None, LOCTEXT("GenerateMeshText", "Generate Mesh"),
					LOCTEXT("GenerateMeshTooltip", "Generate Static or Skeletal Mesh from geometry (Ctrl+M)"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Build"));
	        }
        	ToolbarBuilder.EndSection();

        	// Секция импорта
        	ToolbarBuilder.BeginSection("ImportCommands");
	        {
            	ToolbarBuilder.AddSeparator();

            	// Кнопка импорта геометрии
            	ToolbarBuilder.AddToolBarButton(
					Commands.ImportRenderGeometry, 
					NAME_None, 
					LOCTEXT("ImportRenderGeometryText", "Import From Sprite"),
					LOCTEXT("ImportRenderGeometryTooltip", "Import full geometry (vertices + triangles) from the sprite's Edit Source Region"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "MeshPaint.Import")
				);
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

// ========== ФУНКЦИИ ТРАНЗАКЦИЙ ==========

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
			// Инвалидируем кэш пересечений
			if (TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient())
			{
				ViewportClient->InvalidateIntersectionsCache();
			}
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
		// Инвалидируем кэш пересечений
		if (TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient())
		{
			ViewportClient->InvalidateIntersectionsCache();
		}
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
			// Инвалидируем кэш пересечений
			if (TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient())
			{
				ViewportClient->InvalidateIntersectionsCache();
			}
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
		// Инвалидируем кэш пересечений при перемещении вершин
		if (TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient())
		{
			ViewportClient->InvalidateIntersectionsCache();
		}
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
		// Инвалидируем кэш пересечений при очистке
		if (TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient())
		{
			ViewportClient->InvalidateIntersectionsCache();
		}
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
		// Инвалидируем кэш пересечений при сбросе
		if (TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient())
		{
			ViewportClient->InvalidateIntersectionsCache();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Reset geometry with transaction"));
}

// ========== КОМАНДЫ UNDO/REDO ==========

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

// ========== КОМАНДЫ ВЫДЕЛЕНИЯ И УДАЛЕНИЯ ==========

void FManualSpriteEditorToolkit::OnDeleteSelected()
{
	if (Viewport.IsValid())
	{
		if (TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient())
		{
			ViewportClient->DeleteSelectedVertices();
			Viewport->RefreshViewport();
		}
	}
}

bool FManualSpriteEditorToolkit::CanDeleteSelected() const
{
	if (Viewport.IsValid())
	{
		if (TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient())
		{
			return ViewportClient->HasSelection();
		}
	}
	return false;
}

void FManualSpriteEditorToolkit::OnSelectAll()
{
	if (Viewport.IsValid())
	{
		if (TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient())
		{
			ViewportClient->SelectAllVertices();
			Viewport->RefreshViewport();
		}
	}
}

void FManualSpriteEditorToolkit::OnDeselectAll()
{
	if (Viewport.IsValid())
	{
		if (TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient())
		{
			ViewportClient->ClearSelection();
			Viewport->RefreshViewport();
		}
	}
}

// ========== РЕЖИМЫ РЕДАКТИРОВАНИЯ ==========

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

// ========== ФУНКЦИИ КОПИРОВАНИЯ/ВСТАВКИ ==========

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
		TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient();
		if (ViewportClient.IsValid())
		{
			TArray<int32> NewSelection;
			const int32 StartIndex = ManualSprite->ManualGeometry.Vertices.Num() - CopiedVertices.Num();
			
			for (int32 i = 0; i < CopiedVertices.Num(); i++)
			{
				NewSelection.Add(StartIndex + i);
			}
			
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

// ========== КОМАНДЫ КОПИРОВАНИЯ/ВСТАВКИ ==========

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

	TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient();
	if (!ViewportClient.IsValid())
		return false;
    
	return ViewportClient->GetSelectedVertices().Num() > 0;
}

// ========== ФУНКЦИИ СЕТКИ ==========

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

void FManualSpriteEditorToolkit::CopySelectedVertices()
{
	if (!Viewport.IsValid() || !ManualSprite)
		return;

	TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient();
	if (!ViewportClient.IsValid())
		return;
    
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

void FManualSpriteEditorToolkit::OnGenerateMesh()
{
	if (CanGenerateMesh())
	{
		ManualSpriteMeshGenerator::ShowMeshGenerationDialog(ManualSprite);
	}
}

bool FManualSpriteEditorToolkit::CanGenerateMesh() const
{
	return ManualSprite != nullptr 
		&& ManualSprite->bUseManualGeometry 
		&& ManualSprite->IsManualGeometryValid()
		&& ManualSprite->ManualGeometry.Vertices.Num() >= 3
		&& ManualSprite->ManualGeometry.Triangles.Num() >= 1;
}

void FManualSpriteEditorToolkit::OnImportRenderGeometry()
{
	ImportRenderGeometryWithTransaction();
}

bool FManualSpriteEditorToolkit::CanImportRenderGeometry() const
{
	return ManualSprite != nullptr;
}


void FManualSpriteEditorToolkit::ImportRenderGeometryWithTransaction()
{
	if (!CanImportRenderGeometry())
	{
		return;
	}

	// ИСПРАВЛЕНИЕ: Используем правильный класс транзакции
	FImportGeometryRenderTransaction Transaction(ManualSprite);
	Transaction.Execute();

	if (Viewport.IsValid())
	{
		Viewport->RefreshViewport();
	}

	UE_LOG(LogTemp, Log, TEXT("Imported render geometry with transaction"));
}


void FManualSpriteEditorToolkit::OnAutoTriangulate()
{
	AutoTriangulateWithTransaction();
}

bool FManualSpriteEditorToolkit::CanAutoTriangulate() const
{
	if (!Viewport.IsValid())
		return false;

	TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient();
	if (!ViewportClient.IsValid())
		return false;
    
	// Нужно минимум 3 выделенные вершины для триангуляции
	return ViewportClient->GetSelectedVertices().Num() >= 3;
}

void FManualSpriteEditorToolkit::AutoTriangulateWithTransaction()
{
	if (!CanAutoTriangulate() || !ManualSprite)
		return;

	TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient();
	if (!ViewportClient.IsValid())
		return;

	const TArray<int32>& SelectedVertices = ViewportClient->GetSelectedVertices();
	
	// Создаем транзакцию для Undo/Redo
	FScopedTransaction Transaction(LOCTEXT("AutoTriangulate", "Auto Triangulate Selected Vertices"));
	ManualSprite->Modify();

	// Получаем позиции выделенных вершин
	TArray<FVector2D> SelectedPositions;
	for (int32 VertexIndex : SelectedVertices)
	{
		if (VertexIndex >= 0 && VertexIndex < ManualSprite->ManualGeometry.Vertices.Num())
		{
			SelectedPositions.Add(ManualSprite->ManualGeometry.Vertices[VertexIndex].Position);
		}
	}

	// Выполняем улучшенную триангуляцию Делоне с проверкой пересечений
	TArray<FIntVector> Triangles;
	if (ImprovedDelaunayTriangulation(SelectedPositions, Triangles))
	{
		int32 AddedTriangles = 0;
		int32 SkippedTriangles = 0;

		// Добавляем новые треугольники с проверкой пересечений
		for (const FIntVector& Triangle : Triangles)
		{
			// Преобразуем локальные индексы обратно в глобальные
			const int32 GlobalIndex0 = SelectedVertices[Triangle.X];
			const int32 GlobalIndex1 = SelectedVertices[Triangle.Y];
			const int32 GlobalIndex2 = SelectedVertices[Triangle.Z];
			
			// Проверяем, что такого треугольника еще нет
			if (!TriangleExists(GlobalIndex0, GlobalIndex1, GlobalIndex2))
			{
				// НОВАЯ ПРОВЕРКА: Проверяем, не создадим ли мы пересекающиеся рёбра
				if (WillTriangleCreateIntersections(GlobalIndex0, GlobalIndex1, GlobalIndex2))
				{
					SkippedTriangles++;
					UE_LOG(LogTemp, Warning, TEXT("⚠ Skipped triangle (%d,%d,%d) - would create intersections"), 
						   GlobalIndex0, GlobalIndex1, GlobalIndex2);
					continue;
				}

				ManualSprite->AddTriangle(GlobalIndex0, GlobalIndex1, GlobalIndex2);
				AddedTriangles++;
			}
		}

		(void)ManualSprite->MarkPackageDirty();

		if (Viewport.IsValid())
		{
			ViewportClient->InvalidateIntersectionsCache();
			Viewport->RefreshViewport();
		}

		UE_LOG(LogTemp, Log, TEXT("✅ Smart triangulation: %d triangles added, %d skipped to avoid intersections"), 
			   AddedTriangles, SkippedTriangles);

		if (SkippedTriangles > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("💡 Consider manual triangulation for complex areas"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to triangulate selected vertices"));
	}
}

bool FManualSpriteEditorToolkit::TriangleExists(int32 Index0, int32 Index1, int32 Index2) const
{
	if (!ManualSprite)
		return false;

	// Проверяем все существующие треугольники
	for (const FManualSpriteTriangle& ExistingTriangle : ManualSprite->ManualGeometry.Triangles)
	{
		// Проверяем все возможные комбинации индексов (порядок может отличаться)
		TArray<int32> ExistingIndices = {ExistingTriangle.VertexIndex0, ExistingTriangle.VertexIndex1, ExistingTriangle.VertexIndex2};
		TArray<int32> NewIndices = {Index0, Index1, Index2};
		
		ExistingIndices.Sort();
		NewIndices.Sort();
		
		if (ExistingIndices[0] == NewIndices[0] && 
			ExistingIndices[1] == NewIndices[1] && 
			ExistingIndices[2] == NewIndices[2])
		{
			return true;
		}
	}
	
	return false;
}

bool FManualSpriteEditorToolkit::DelaunayTriangulation(const TArray<FVector2D>& Points, TArray<FIntVector>& OutTriangles) const
{
	OutTriangles.Empty();
	
	if (Points.Num() < 3)
		return false;

	if (Points.Num() == 3)
	{
		OutTriangles.Add(FIntVector(0, 1, 2));
		return true;
	}

	// Используем простую триангуляцию Делоне
	return SimpleDelaunayTriangulation(Points, OutTriangles);
}

bool FManualSpriteEditorToolkit::SimpleDelaunayTriangulation(const TArray<FVector2D>& Points, TArray<FIntVector>& OutTriangles) const
{
	OutTriangles.Empty();
	
	// Создаем все возможные треугольники и выбираем лучшие
	for (int32 i = 0; i < Points.Num(); i++)
	{
		for (int32 j = i + 1; j < Points.Num(); j++)
		{
			for (int32 k = j + 1; k < Points.Num(); k++)
			{
				// Проверяем, что треугольник валиден (не вырожденный)
				if (IsValidTriangle(Points[i], Points[j], Points[k]))
				{
					// Проверяем Delaunay условие - нет других точек внутри окружности
					if (IsDelaunayTriangle(Points, i, j, k))
					{
						OutTriangles.Add(FIntVector(i, j, k));
					}
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Delaunay triangulation: %d points -> %d triangles"), Points.Num(), OutTriangles.Num());
	return OutTriangles.Num() > 0;
}

bool FManualSpriteEditorToolkit::IsValidTriangle(const FVector2D& A, const FVector2D& B, const FVector2D& C) const
{
	// Проверяем площадь треугольника (не должен быть вырожденным)
	const float Area = FMath::Abs((B.X - A.X) * (C.Y - A.Y) - (C.X - A.X) * (B.Y - A.Y));
	return Area > 1.0f; // Минимальная площадь
}

bool FManualSpriteEditorToolkit::IsDelaunayTriangle(const TArray<FVector2D>& Points, int32 A, int32 B, int32 C) const
{
	const FVector2D& PA = Points[A];
	const FVector2D& PB = Points[B]; 
	const FVector2D& PC = Points[C];

	// Вычисляем центр и радиус описанной окружности
	FVector2D CircumCenter;
	float CircumRadiusSquared;
	if (!GetCircumcircle(PA, PB, PC, CircumCenter, CircumRadiusSquared))
	{
		return false; // Вырожденный треугольник
	}

	// Проверяем, что никакая другая точка не лежит внутри окружности
	for (int32 i = 0; i < Points.Num(); i++)
	{
		if (i == A || i == B || i == C)
			continue;

		const float DistSquared = FVector2D::DistSquared(Points[i], CircumCenter);
		if (DistSquared < CircumRadiusSquared - 1.0f) // Небольшой tolerance
		{
			return false; // Точка внутри окружности
		}
	}

	return true;
}

bool FManualSpriteEditorToolkit::GetCircumcircle(const FVector2D& A, const FVector2D& B, const FVector2D& C, FVector2D& OutCenter, float& OutRadiusSquared) const
{
	const float D = 2.0f * (A.X * (B.Y - C.Y) + B.X * (C.Y - A.Y) + C.X * (A.Y - B.Y));
	
	if (FMath::Abs(D) < 1e-6f)
	{
		return false; // Коллинеарные точки
	}

	const float ASq = A.X * A.X + A.Y * A.Y;
	const float BSq = B.X * B.X + B.Y * B.Y;
	const float CSq = C.X * C.X + C.Y * C.Y;

	OutCenter.X = (ASq * (B.Y - C.Y) + BSq * (C.Y - A.Y) + CSq * (A.Y - B.Y)) / D;
	OutCenter.Y = (ASq * (C.X - B.X) + BSq * (A.X - C.X) + CSq * (B.X - A.X)) / D;

	OutRadiusSquared = FVector2D::DistSquared(A, OutCenter);
	return true;
}

void FManualSpriteEditorToolkit::OnValidateTriangulation()
{
	if (!CanValidateTriangulation())
		return;

	if (Viewport.IsValid())
	{
		if (TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient())
		{
			// Принудительно обновляем кэш пересечений
			ViewportClient->InvalidateIntersectionsCache();
			
			// Получаем результаты валидации
			const auto& IntersectingEdges = ViewportClient->GetCachedIntersectingEdges();
			
			// Показываем результаты в логе и UI
			if (IntersectingEdges.Num() > 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("🔴 VALIDATION FAILED: Found %d intersecting edges in triangulation!"), 
					   IntersectingEdges.Num());
			}
			else if (ManualSprite && ManualSprite->ManualGeometry.Triangles.Num() > 0)
			{
				UE_LOG(LogTemp, Log, TEXT("✅ VALIDATION PASSED: No intersecting edges found. Triangulation is clean!"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("⚠ No triangles to validate."));
			}
			
			Viewport->RefreshViewport();
		}
	}
}

bool FManualSpriteEditorToolkit::CanValidateTriangulation() const
{
	return ManualSprite != nullptr 
		&& ManualSprite->bUseManualGeometry 
		&& ManualSprite->ManualGeometry.Vertices.Num() >= 3;
}

bool FManualSpriteEditorToolkit::ImprovedDelaunayTriangulation(const TArray<FVector2D>& Points, TArray<FIntVector>& OutTriangles) const
{
	OutTriangles.Empty();
	
	if (Points.Num() < 3)
		return false;

	if (Points.Num() == 3)
	{
		OutTriangles.Add(FIntVector(0, 1, 2));
		return true;
	}

	// Создаем кандидатов на треугольники и проверяем их качество
	TArray<FIntVector> CandidateTriangles;
	TArray<float> TriangleQualities;

	// Генерируем все возможные треугольники
	for (int32 i = 0; i < Points.Num(); i++)
	{
		for (int32 j = i + 1; j < Points.Num(); j++)
		{
			for (int32 k = j + 1; k < Points.Num(); k++)
			{
				// Проверяем, что треугольник валиден
				if (IsValidTriangle(Points[i], Points[j], Points[k]))
				{
					// Вычисляем качество треугольника (больше = лучше)
					float Quality = CalculateTriangleQuality(Points[i], Points[j], Points[k]);
					
					// Проверяем Delaunay условие
					if (IsDelaunayTriangle(Points, i, j, k))
					{
						Quality += 10.0f; // Бонус за соответствие Delaunay
					}

					CandidateTriangles.Add(FIntVector(i, j, k));
					TriangleQualities.Add(Quality);
				}
			}
		}
	}

	// Сортируем треугольники по качеству (лучшие первыми)
	for (int32 i = 0; i < CandidateTriangles.Num() - 1; i++)
	{
		for (int32 j = i + 1; j < CandidateTriangles.Num(); j++)
		{
			if (TriangleQualities[i] < TriangleQualities[j])
			{
				CandidateTriangles.Swap(i, j);
				TriangleQualities.Swap(i, j);
			}
		}
	}

	// Добавляем треугольники, избегая пересечений
	TSet<TPair<int32, int32>> UsedEdges;
	
	for (int32 i = 0; i < CandidateTriangles.Num(); i++)
	{
		const FIntVector& Triangle = CandidateTriangles[i];
		
		// Проверяем рёбра треугольника
		TArray<TPair<int32, int32>> TriangleEdges = {
			TPair<int32, int32>(FMath::Min(Triangle.X, Triangle.Y), FMath::Max(Triangle.X, Triangle.Y)),
			TPair<int32, int32>(FMath::Min(Triangle.Y, Triangle.Z), FMath::Max(Triangle.Y, Triangle.Z)),
			TPair<int32, int32>(FMath::Min(Triangle.Z, Triangle.X), FMath::Max(Triangle.Z, Triangle.X))
		};
		
		// Проверяем, не пересекутся ли новые рёбра с существующими
		bool bWillIntersect = false;
		for (const auto& NewEdge : TriangleEdges)
		{
			if (WillEdgeIntersectWithUsedEdges(Points, NewEdge, UsedEdges))
			{
				bWillIntersect = true;
				break;
			}
		}
		
		if (!bWillIntersect)
		{
			OutTriangles.Add(Triangle);
			
			// Добавляем рёбра в список использованных
			for (const auto& Edge : TriangleEdges)
			{
				UsedEdges.Add(Edge);
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Improved Delaunay: %d points -> %d quality triangles (from %d candidates)"), 
		   Points.Num(), OutTriangles.Num(), CandidateTriangles.Num());
	
	return OutTriangles.Num() > 0;
}

bool FManualSpriteEditorToolkit::WillTriangleCreateIntersections(int32 Index0, int32 Index1, int32 Index2) const
{
	if (!ManualSprite)
		return true;

	// Проверяем каждое ребро нового треугольника
	return WillEdgeIntersectExisting(Index0, Index1) ||
		   WillEdgeIntersectExisting(Index1, Index2) ||
		   WillEdgeIntersectExisting(Index2, Index0);
}

bool FManualSpriteEditorToolkit::WillEdgeIntersectExisting(int32 VertexA, int32 VertexB) const
{
	if (!ManualSprite)
		return true;

	const FManualSpriteGeometry& Geometry = ManualSprite->ManualGeometry;
	
	// Получаем позиции вершин нового ребра
	if (VertexA >= Geometry.Vertices.Num() || VertexB >= Geometry.Vertices.Num())
		return true;
		
	const FVector2D PosA = Geometry.Vertices[VertexA].Position;
	const FVector2D PosB = Geometry.Vertices[VertexB].Position;

	// Проверяем пересечение с каждым существующим ребром
	for (const FManualSpriteTriangle& Triangle : Geometry.Triangles)
	{
		// Проверяем все три ребра треугольника
		TArray<TPair<int32, int32>> ExistingEdges = {
			TPair<int32, int32>(Triangle.VertexIndex0, Triangle.VertexIndex1),
			TPair<int32, int32>(Triangle.VertexIndex1, Triangle.VertexIndex2),
			TPair<int32, int32>(Triangle.VertexIndex2, Triangle.VertexIndex0)
		};

		for (const auto& ExistingEdge : ExistingEdges)
		{
			// Пропускаем рёбра, которые имеют общие вершины
			if (ExistingEdge.Key == VertexA || ExistingEdge.Key == VertexB ||
				ExistingEdge.Value == VertexA || ExistingEdge.Value == VertexB)
			{
				continue;
			}

			// Проверяем валидность индексов
			if (ExistingEdge.Key >= Geometry.Vertices.Num() || ExistingEdge.Value >= Geometry.Vertices.Num())
				continue;

			const FVector2D ExistingPosA = Geometry.Vertices[ExistingEdge.Key].Position;
			const FVector2D ExistingPosB = Geometry.Vertices[ExistingEdge.Value].Position;

			// Проверяем пересечение отрезков
			if (DoSegmentsIntersect(PosA, PosB, ExistingPosA, ExistingPosB))
			{
				return true;
			}
		}
	}

	return false;
}

float FManualSpriteEditorToolkit::CalculateTriangleQuality(const FVector2D& A, const FVector2D& B, const FVector2D& C) const
{
	// Вычисляем качество треугольника на основе соотношения сторон
	const float SideA = FVector2D::Distance(B, C);
	const float SideB = FVector2D::Distance(A, C);
	const float SideC = FVector2D::Distance(A, B);
	
	const float Perimeter = SideA + SideB + SideC;
	const float Area = FMath::Abs((B.X - A.X) * (C.Y - A.Y) - (C.X - A.X) * (B.Y - A.Y)) * 0.5f;
	
	if (Perimeter <= 0.0f || Area <= 0.0f)
		return 0.0f;
	
	// Коэффициент качества (больше = лучше для равносторонних треугольников)
	return (4.0f * Area) / (Perimeter * Perimeter);
}

bool FManualSpriteEditorToolkit::WillEdgeIntersectWithUsedEdges(const TArray<FVector2D>& Points, 
																const TPair<int32, int32>& NewEdge, 
																const TSet<TPair<int32, int32>>& UsedEdges) const
{
	const FVector2D NewEdgeStart = Points[NewEdge.Key];
	const FVector2D NewEdgeEnd = Points[NewEdge.Value];

	for (const auto& UsedEdge : UsedEdges)
	{
		// Пропускаем рёбра с общими вершинами
		if (UsedEdge.Key == NewEdge.Key || UsedEdge.Key == NewEdge.Value ||
			UsedEdge.Value == NewEdge.Key || UsedEdge.Value == NewEdge.Value)
		{
			continue;
		}

		const FVector2D UsedEdgeStart = Points[UsedEdge.Key];
		const FVector2D UsedEdgeEnd = Points[UsedEdge.Value];

		if (DoSegmentsIntersect(NewEdgeStart, NewEdgeEnd, UsedEdgeStart, UsedEdgeEnd))
		{
			return true;
		}
	}

	return false;
}

bool FManualSpriteEditorToolkit::DoSegmentsIntersect(const FVector2D& A1, const FVector2D& A2, 
													const FVector2D& B1, const FVector2D& B2) const
{
	// Используем тот же алгоритм, что и в viewport
	auto Orientation = [](const FVector2D& P, const FVector2D& Q, const FVector2D& R) -> int32
	{
		const float Val = (Q.Y - P.Y) * (R.X - Q.X) - (Q.X - P.X) * (R.Y - Q.Y);
		if (FMath::Abs(Val) < 1e-6f) return 0;
		return (Val > 0) ? 1 : 2;
	};
	
	auto OnSegment = [](const FVector2D& P, const FVector2D& Q, const FVector2D& R) -> bool
	{
		return Q.X <= FMath::Max(P.X, R.X) && Q.X >= FMath::Min(P.X, R.X) &&
		       Q.Y <= FMath::Max(P.Y, R.Y) && Q.Y >= FMath::Min(P.Y, R.Y);
	};
	
	const int32 O1 = Orientation(A1, A2, B1);
	const int32 O2 = Orientation(A1, A2, B2);
	const int32 O3 = Orientation(B1, B2, A1);
	const int32 O4 = Orientation(B1, B2, A2);
	
	if (O1 != O2 && O3 != O4)
		return true;
	
	if (O1 == 0 && OnSegment(A1, B1, A2)) return true;
	if (O2 == 0 && OnSegment(A1, B2, A2)) return true;
	if (O3 == 0 && OnSegment(B1, A1, B2)) return true;
	if (O4 == 0 && OnSegment(B1, A2, B2)) return true;
	
	return false;
}

void FManualSpriteEditorToolkit::OnDeleteTriangles()
{
	if (!CanDeleteTriangles())
		return;

	TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient();
	if (!ViewportClient.IsValid())
		return;

	const TArray<int32>& SelectedVertices = ViewportClient->GetSelectedVertices();
	if (SelectedVertices.Num() == 0)
		return;

	// Создаем транзакцию для Undo/Redo
	FScopedTransaction Transaction(LOCTEXT("DeleteTriangles", "Delete Connected Triangles"));
	ManualSprite->Modify();

	// Находим все треугольники, которые используют выделенные вершины
	TArray<int32> TrianglesToDelete;
	
	for (int32 TriangleIndex = 0; TriangleIndex < ManualSprite->ManualGeometry.Triangles.Num(); TriangleIndex++)
	{
		const FManualSpriteTriangle& Triangle = ManualSprite->ManualGeometry.Triangles[TriangleIndex];
		
		// Проверяем, использует ли треугольник хотя бы одну из выделенных вершин
		bool bUsesSelectedVertex = false;
		for (int32 SelectedVertex : SelectedVertices)
		{
			if (Triangle.VertexIndex0 == SelectedVertex ||
				Triangle.VertexIndex1 == SelectedVertex ||
				Triangle.VertexIndex2 == SelectedVertex)
			{
				bUsesSelectedVertex = true;
				break;
			}
		}
		
		if (bUsesSelectedVertex)
		{
			TrianglesToDelete.Add(TriangleIndex);
		}
	}

	if (TrianglesToDelete.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠ No triangles connected to selected vertices"));
		return;
	}

	// Удаляем треугольники в обратном порядке (чтобы индексы оставались валидными)
	TrianglesToDelete.Sort([](const int32& A, const int32& B) {
		return A > B; // Сортируем по убыванию
	});

	int32 DeletedCount = 0;
	for (int32 TriangleIndex : TrianglesToDelete)
	{
		if (TriangleIndex >= 0 && TriangleIndex < ManualSprite->ManualGeometry.Triangles.Num())
		{
			ManualSprite->ManualGeometry.Triangles.RemoveAt(TriangleIndex);
			DeletedCount++;
		}
	}

	(void)ManualSprite->MarkPackageDirty();

	if (Viewport.IsValid())
	{
		// Инвалидируем кэш пересечений
		ViewportClient->InvalidateIntersectionsCache();
		Viewport->RefreshViewport();
	}

	UE_LOG(LogTemp, Log, TEXT("✅ Deleted %d triangles connected to %d selected vertices"), 
		   DeletedCount, SelectedVertices.Num());
}

bool FManualSpriteEditorToolkit::CanDeleteTriangles() const
{
	if (!ManualSprite || !ManualSprite->bUseManualGeometry)
		return false;

	if (!Viewport.IsValid())
		return false;

	TSharedPtr<ManualSpriteEditorViewport> ViewportClient = Viewport->GetManualSpriteViewportClient();
	if (!ViewportClient.IsValid())
		return false;

	// Можно удалять треугольники если есть выделенные вершины и есть треугольники
	return ViewportClient->GetSelectedVertices().Num() > 0 && 
		   ManualSprite->ManualGeometry.Triangles.Num() > 0;
}

#undef LOCTEXT_NAMESPACE