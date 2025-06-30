#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Framework/Application/IInputProcessor.h"
#include "ManualSprite.h"

class FManualSpriteEditorToolkit;
class SViewport;
class FSceneViewport;
class FManualSpriteEditorViewportClient;

/**
 * Viewport widget для редактирования Manual Sprite
 */
class MANUALSPRITEEDITOR_API SManualSpriteEditorViewport final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SManualSpriteEditorViewport) {}
		SLATE_ARGUMENT(TSharedPtr<FManualSpriteEditorToolkit>, ManualSpriteEditor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Получение viewport client */
	TSharedPtr<FManualSpriteEditorViewportClient> GetViewportClient() const { return ViewportClient; }

	/** Обновление viewport */
	void RefreshViewport();

protected:
	/** Viewport widget */
	TSharedPtr<SViewport> ViewportWidget;

	/** Scene viewport */
	TSharedPtr<FSceneViewport> SceneViewport;

	/** Viewport client */
	TSharedPtr<FManualSpriteEditorViewportClient> ViewportClient;

	/** Ссылка на редактор */
	TWeakPtr<FManualSpriteEditorToolkit> ManualSpriteEditorPtr;
};

/**
 * Viewport Client для обработки ввода и рендеринга с поддержкой множественного перетаскивания
 */
class MANUALSPRITEEDITOR_API FManualSpriteEditorViewportClient final : public FViewportClient
{
public:
	explicit FManualSpriteEditorViewportClient(TWeakPtr<FManualSpriteEditorToolkit> InManualSpriteEditor);

	// FViewportClient interface
	virtual void Draw(FViewport* Viewport, FCanvas* Canvas) override;
	
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
	virtual bool InputAxis(FViewport* Viewport, FInputDeviceId DeviceId, FKey Key, float Delta, float DeltaTime, int32 NumSamples = 1, bool bGamepad = false) override;
	
	virtual void MouseMove(FViewport* Viewport, int32 X, int32 Y) override;
	virtual void CapturedMouseMove(FViewport* Viewport, int32 InMouseX, int32 InMouseY) override;

	/** Установка масштаба и позиции */
	void SetZoom(float NewZoom);
	void SetViewOffset(FVector2D NewOffset);
	float GetZoom() const { return ZoomFactor; }
	FVector2D GetViewOffset() const { return ViewOffset; }

	/** Конвертация координат */
	FVector2D ScreenToWorld(FVector2D ScreenPos, const FViewport* Viewport) const;
	FVector2D WorldToScreen(FVector2D WorldPos, const FViewport* Viewport) const;

	/** Функции для работы с выделением - добавлены для поддержки команд */
	const TArray<int32>& GetSelectedVertices() const { return SelectedVertices; }
	void SetSelectedVertices(const TArray<int32>& NewSelection);
	void SelectAllVertices();
	void ClearSelection();
	bool HasSelection() const { return SelectedVertices.Num() > 0; }

	/** Удаление выделенных элементов */
	void DeleteSelectedVertices();

protected:
	/** Рендеринг спрайта */
	void DrawSprite(FCanvas* Canvas, const FViewport* Viewport) const;
	
	/** Рендеринг отладочной геометрии */
	void DrawDebugGeometry(FCanvas* Canvas, const FViewport* Viewport) const;
	
	/** Рендеринг вершин */
	void DrawVertices(FCanvas* Canvas, const FViewport* Viewport) const;
	
	/** Рендеринг треугольников */
	void DrawTriangles(FCanvas* Canvas, const FViewport* Viewport) const;
	
	/** Рендеринг превью вставки */
	void DrawPastePreview(FCanvas* Canvas, const FViewport* Viewport) const;
	
	/** Обработка клика мыши */
	void HandleMouseClick(FViewport* Viewport, FKey Key, EInputEvent Event, FVector2D MousePos);
	
	/** Поиск ближайшей вершины */
	int32 FindVertexAtPosition(FVector2D WorldPos, float Tolerance = 10.0f) const;
	
	/** Поиск треугольника под курсором */
	int32 FindTriangleAtPosition(FVector2D WorldPos) const;

	/** Вычисление UV координат на основе мировой позиции */
	FVector2D CalculateUVFromWorldPosition(const FVector2D& WorldPos, const UManualSprite* ManualSprite) const;

	/** Получение размеров спрайта для вычислений */
	FVector2D GetSpriteSize(const UManualSprite* ManualSprite) const;

	/** Отрисовка сетки */
	void DrawGrid(FCanvas* Canvas, const FViewport* Viewport) const;

	/** Привязка позиции к сетке */
	FVector2D SnapToGrid(const FVector2D& Position) const;

	/** Обработка транзакций перемещения вершин (множественное перетаскивание) */
	void BeginVerticesDrag();
	void UpdateVerticesDrag(const FVector2D& MouseDelta);
	void EndVerticesDrag();
	void CancelVerticesDrag();

	/** Состояние выделения рамкой */
	struct FBoxSelectionState
	{
		bool bIsActive;
		FVector2D StartPosition;
		FVector2D CurrentPosition;
		bool bIsAdditive; // Если true, добавляем к существующему выделению (Ctrl)
		
		FBoxSelectionState()
			: bIsActive(false)
			, StartPosition(FVector2D::ZeroVector)
			, CurrentPosition(FVector2D::ZeroVector)
			, bIsAdditive(false)
		{
		}
		
		FVector2D GetTopLeft() const
		{
			return FVector2D(
				FMath::Min(StartPosition.X, CurrentPosition.X),
				FMath::Min(StartPosition.Y, CurrentPosition.Y)
			);
		}
		
		FVector2D GetBottomRight() const
		{
			return FVector2D(
				FMath::Max(StartPosition.X, CurrentPosition.X),
				FMath::Max(StartPosition.Y, CurrentPosition.Y)
			);
		}
		
		FVector2D GetSize() const
		{
			return GetBottomRight() - GetTopLeft();
		}
		
		bool IsValidSelection() const
		{
			const FVector2D Size = GetSize();
			return Size.X > 5.0f && Size.Y > 5.0f; // Минимальный размер для валидного выделения
		}
		
		bool ContainsPoint(const FVector2D& Point) const
		{
			const FVector2D TopLeft = GetTopLeft();
			const FVector2D BottomRight = GetBottomRight();
			
			return Point.X >= TopLeft.X && Point.X <= BottomRight.X &&
				   Point.Y >= TopLeft.Y && Point.Y <= BottomRight.Y;
		}
	};

	/** Функции для выделения рамкой */
	void StartBoxSelection(const FVector2D& StartPos, bool bAdditive);
	void UpdateBoxSelection(const FVector2D& CurrentPos);
	void EndBoxSelection();
	void CancelBoxSelection();

	/** Отрисовка рамки выделения */
	void DrawBoxSelection(FCanvas* Canvas, const FViewport* Viewport) const;

	/** Проверка пересечения вершины с рамкой выделения */
	TArray<int32> GetVerticesInSelectionBox() const;

private:
	/** Ссылка на редактор */
	TWeakPtr<FManualSpriteEditorToolkit> ManualSpriteEditorPtr;
	
	/** Параметры отображения */
	float ZoomFactor;
	FVector2D ViewOffset;
	bool bIsPanning;
	FVector2D LastMousePosition;
	
	/** Состояние выбора */
	TArray<int32> SelectedVertices;
	int32 HoveredVertex;
	int32 HoveredTriangle;

	/** Переменные для множественного Drag & Drop */
	bool bIsDraggingVertices;
	FVector2D DragStartPosition;
	TArray<FVector2D> OriginalVertexPositions;  // Исходные позиции всех перетаскиваемых вершин
	TArray<FVector2D> OriginalVertexUVs;        // Исходные UV всех перетаскиваемых вершин
	bool bDragTransactionStarted;

	/** Состояние выделения рамкой */
	FBoxSelectionState BoxSelection;

	/** Сохранённый размер viewport для функций преобразования координат */
	mutable FIntPoint LastViewportSize;
	
	/** Константы для рендеринга */
	static const float VertexSize;
	static const float VertexSelectSize;
	static const FLinearColor VertexColor;
	static const FLinearColor SelectedVertexColor;
	static const FLinearColor TriangleColor;
	static const FLinearColor SelectedTriangleColor;
};