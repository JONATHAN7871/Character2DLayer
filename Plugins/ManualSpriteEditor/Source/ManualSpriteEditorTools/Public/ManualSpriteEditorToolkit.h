#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/IToolkitHost.h"
#include "ManualSprite.h"
#include "Framework/Commands/UICommandList.h"

class SManualSpriteEditorViewport;
class UManualSprite;
class FManualSpriteEditorCommands;

/**
 * Редактор для Manual Sprite ассетов с поддержкой Undo/Redo и копирования/вставки
 */
class MANUALSPRITEEDITORTOOLS_API FManualSpriteEditorToolkit : public FAssetEditorToolkit, public FNotifyHook
{
public:
	// Конструктор с инициализацией всех полей
	FManualSpriteEditorToolkit();

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

	/**
	 * Инициализация редактора Manual Sprite
	 */
	void InitManualSpriteEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UManualSprite* InManualSprite);

	/** FAssetEditorToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	/** FNotifyHook interface для отслеживания изменений в Details панели */
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;

	/** Получение редактируемого спрайта */
	UManualSprite* GetManualSprite() const { return ManualSprite; }

	/** Получение viewport */
	TSharedPtr<SManualSpriteEditorViewport> GetViewport() const { return Viewport; }

	/** Режимы редактирования */
	enum class EEditMode
	{
		Select,     // Выбор и перемещение вершин
		AddVertex,  // Добавление новых вершин
		Triangle,   // Создание треугольников
		Delete,     // Удаление вершин/треугольников
		Paste       // Режим вставки (показывает превью)
	};

	/** Настройки сетки */
	struct FGridSettings
	{
		bool bShowGrid;
		bool bSnapToGrid;
		float GridSize;
		FLinearColor GridColor;
		
		FGridSettings()
			: bShowGrid(true)
			, bSnapToGrid(true)
			, GridSize(25.0f)
			, GridColor(FLinearColor(0.5f, 0.5f, 0.5f, 0.3f))
		{
		}
	};

	/** Структура для хранения скопированной геометрии */
	struct FCopiedVertexData
	{
		FVector2D RelativePosition;  // Позиция относительно центра копирования
		FVector2D UV;                // UV координаты
		
		FCopiedVertexData()
			: RelativePosition(FVector2D::ZeroVector)
			, UV(FVector2D::ZeroVector)
		{
		}
		
		FCopiedVertexData(const FVector2D& InRelativePosition, const FVector2D& InUV)
			: RelativePosition(InRelativePosition)
			, UV(InUV)
		{
		}
	};

	/** Получение настроек сетки */
	const FGridSettings& GetGridSettings() const { return GridSettings; }

	/** Переключение отображения сетки */
	void ToggleGridDisplay();

	/** Переключение привязки к сетке */
	void ToggleGridSnap();

	/** Установка размера сетки */
	void SetGridSize(float NewSize);

	/** Установка режима редактирования */
	void SetEditMode(EEditMode NewMode);
	EEditMode GetEditMode() const { return CurrentEditMode; }

	/** Функции для транзакций - теперь с поддержкой Undo/Redo */
	void AddVertexWithTransaction(const FVector2D& Position, const FVector2D& UV);
	void RemoveVertexWithTransaction(int32 VertexIndex);
	void AddTriangleWithTransaction(int32 Index0, int32 Index1, int32 Index2);
	void RemoveTriangleWithTransaction(int32 TriangleIndex);
	void MoveVertexWithTransaction(int32 VertexIndex, const FVector2D& NewPosition, const FVector2D& NewUV);
	void MoveVerticesWithTransaction(const TArray<int32>& VertexIndices, const TArray<FVector2D>& NewPositions, const TArray<FVector2D>& NewUVs);
	void ClearGeometryWithTransaction();
	void ResetGeometryWithTransaction();

	/** Функции копирования/вставки */
	void CopySelectedVertices();
	void PasteVertices(const FVector2D& PastePosition);
	void CutSelectedVertices();
	void DuplicateSelectedVertices();
	bool CanPaste() const;
	bool HasCopiedVertices() const;

	/** Получение/установка позиции превью вставки */
	void SetPastePreviewPosition(const FVector2D& Position);
	FVector2D GetPastePreviewPosition() const { return PastePreviewPosition; }
	
	/** Получение превью вершин для отображения */
	TArray<FVector2D> GetPastePreviewVertices() const;

	/** Получение команд редактора */
	TSharedPtr<FUICommandList> GetCommandList() const { return CommandList; }

private:
	/** Создание вкладки Viewport */
	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	
	/** Создание вкладки Details */
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);

	/** Создание тулбара */
	void ExtendToolbar();
	
	/** Инициализация команд */
	void InitializeCommands();
	
	/** Привязка команд к функциям */
	void BindCommands();

	/** Действия команд */
	void OnSelectMode();
	void OnAddVertexMode();
	void OnTriangleMode();
	void OnDeleteMode();
	
	void OnUndo();
	void OnRedo();
	bool CanUndo() const;
	bool CanRedo() const;
	
	void OnDeleteSelected();
	bool CanDeleteSelected() const;
	
	// Команды копирования/вставки
	void OnCopy();
	void OnPaste();
	void OnCut();
	void OnDuplicate();
	bool CanCopy() const;
	
	void OnToggleGrid();
	void OnToggleSnap();
	
	void OnSelectAll();
	void OnDeselectAll();
	
	void OnClearGeometry();
	void OnResetToDefault();

	/** Проверка состояний кнопок тулбара */
	bool IsSelectModeActive() const;
	bool IsAddVertexModeActive() const;
	bool IsTriangleModeActive() const;
	bool IsDeleteModeActive() const;
	bool IsPasteModeActive() const;

	/** Настройки сетки */
	FGridSettings GridSettings;

	/** Действия для сетки */
	void OnGridSize10();
	void OnGridSize25();
	void OnGridSize50();
	void OnGridSize100();

	/** Проверка состояний кнопок сетки */
	bool IsGridEnabled() const;
	bool IsSnapEnabled() const;
	bool IsGridSize10Active() const;
	bool IsGridSize25Active() const;
	bool IsGridSize50Active() const;
	bool IsGridSize100Active() const;

	/** Вспомогательные функции для копирования */
	FVector2D CalculateCopyOrigin(const TArray<int32>& VertexIndices) const;
	void AddVerticesWithTransaction(const TArray<FVector2D>& Positions, const TArray<FVector2D>& UVs);

private:
	/** Редактируемый спрайт */
	UManualSprite* ManualSprite;

	/** Viewport для отображения спрайта */
	TSharedPtr<SManualSpriteEditorViewport> Viewport;

	/** Panel для свойств */
	TSharedPtr<class IDetailsView> DetailsView;

	/** Система команд */
	TSharedPtr<FUICommandList> CommandList;

	/** ID вкладок */
	static const FName ViewportTabId;
	static const FName DetailsTabId;

	/** Текущий режим редактирования */
	EEditMode CurrentEditMode;

	/** Буфер копирования */
	TArray<FCopiedVertexData> CopiedVertices;
	FVector2D CopyOrigin;  // Центр скопированной области
	
	/** Позиция превью вставки (в мировых координатах) */
	FVector2D PastePreviewPosition;

	/** Команды генерации мешей */
	void OnGenerateMesh();
	bool CanGenerateMesh() const;

	/** Действия команд */
	void OnImportRenderGeometry();
	bool CanImportRenderGeometry() const;

	/** Функция для транзакций */
	void ImportRenderGeometryWithTransaction();

public:
	/** Структуры для триангуляции */
	struct FEdge
	{
		int32 A, B;
		FEdge(int32 InA, int32 InB) : A(InA), B(InB) {}
	};

	/** Автоматическая триангуляция */
	void OnAutoTriangulate();
	bool CanAutoTriangulate() const;
	void AutoTriangulateWithTransaction();

private:
	/** Основные методы триангуляции */
	bool TriangleExists(int32 Index0, int32 Index1, int32 Index2) const;
	bool DelaunayTriangulation(const TArray<FVector2D>& Points, TArray<FIntVector>& OutTriangles) const;
	bool SimpleDelaunayTriangulation(const TArray<FVector2D>& Points, TArray<FIntVector>& OutTriangles) const;
	
	/** Вспомогательные геометрические методы */
	bool IsValidTriangle(const FVector2D& A, const FVector2D& B, const FVector2D& C) const;
	bool IsDelaunayTriangle(const TArray<FVector2D>& Points, int32 A, int32 B, int32 C) const;
	bool GetCircumcircle(const FVector2D& A, const FVector2D& B, const FVector2D& C, FVector2D& OutCenter, float& OutRadiusSquared) const;
};