#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Framework/Application/IInputProcessor.h"
#include "ManualSprite.h"
#include "EditorViewportClient.h"
#include "SEditorViewport.h"
#include "PreviewScene.h"
#include "SCommonEditorViewportToolbarBase.h"

class FManualSpriteEditorToolkit;
class SViewport;
class FSceneViewport;
class ManualSpriteEditorViewport;

// Константы для viewport
namespace FManualSpriteViewportUtils
{
	const FVector PaperAxisX(1.0f, 0.0f, 0.0f);
	const FVector PaperAxisY(0.0f, 0.0f, 1.0f);
	const FVector PaperAxisZ(0.0f, 1.0f, 0.0f);
}

/**
 * Viewport widget для редактирования Manual Sprite - ИСПРАВЛЕННАЯ ВЕРСИЯ
 */
class MANUALSPRITEEDITORTOOLS_API SManualSpriteEditorViewport final
	: public SEditorViewport
	, public ICommonEditorViewportToolbarInfoProvider
{
public:
	SLATE_BEGIN_ARGS(SManualSpriteEditorViewport) {}
	SLATE_ARGUMENT(TSharedPtr<FManualSpriteEditorToolkit>, ManualSpriteEditor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Специализированный клиент */
	TSharedPtr<ManualSpriteEditorViewport> GetManualSpriteViewportClient() const { return ViewportClient; }

	/** Принудительное обновление рендера */
	void RefreshViewport();

	/* ---------- SEditorViewport ---------- */
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget>               MakeViewportToolbar()    override;
	/* ---------- End SEditorViewport ------ */

	/* --- ICommonEditorViewportToolbarInfoProvider -------- */
	virtual TSharedRef<SEditorViewport>       GetViewportWidget() override;
	virtual TSharedPtr<FEditorViewportClient> GetViewportClient();
	virtual TSharedPtr<FExtender>             GetExtenders()      const override;
	virtual void                              OnFloatingButtonClicked()          override {}
	virtual bool                              IsVisible()                        const override { return true; }
	/* ------------------------------------------------------ */

private:
	/** Клиент, обрабатывающий ввод и рендеринг */
	TSharedPtr<ManualSpriteEditorViewport> ViewportClient;

	/** Редактор-владелец */
	TWeakPtr<FManualSpriteEditorToolkit>   ManualSpriteEditorPtr;
};

/**
 * Viewport Client для обработки ввода и рендеринга - ИСПРАВЛЕННАЯ ВЕРСИЯ
 */
class MANUALSPRITEEDITORTOOLS_API ManualSpriteEditorViewport final : public FEditorViewportClient
{
public:
	explicit ManualSpriteEditorViewport(TWeakPtr<FManualSpriteEditorToolkit> InManualSpriteEditor);
	virtual ~ManualSpriteEditorViewport();

	// FEditorViewportClient interface
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupViewForRendering(FSceneViewFamily& ViewFamily, FSceneView& View) override;
	
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
	virtual bool InputAxis(FViewport* InViewport, FInputDeviceId DeviceId, FKey Key, float Delta, float DeltaTime, int32 NumSamples = 1, bool bGamepad = false) override;
	virtual void MouseMove(FViewport* InViewport, int32 X, int32 Y) override;
	virtual void CapturedMouseMove(FViewport* InViewport, int32 InMouseX, int32 InMouseY) override;
	// End of FEditorViewportClient interface

	/** Установка Manual Sprite для отображения */
	void SetManualSprite(UManualSprite* InManualSprite);

	/** Получение текущего Manual Sprite */
	UManualSprite* GetManualSprite() const { return ManualSpritePtr.Get(); }

	/** Обновление render component при изменении спрайта */
	void RefreshManualSprite();

	/** Установка масштаба и позиции */
	void SetZoom(float NewZoom);
	void SetViewOffset(FVector2D NewOffset);
	float GetZoom() const { return ZoomFactor; }
	FVector2D GetViewOffset() const { return ViewOffset; }

	/** Конвертация координат - ИСПРАВЛЕННЫЕ ВЕРСИИ */
	FVector2D ScreenToWorld(FVector2D ScreenPos, const FViewport* InViewport) const;
	FVector2D WorldToScreen(FVector2D WorldPos, const FViewport* InViewport) const;

	/** Функции для работы с выделением */
	const TArray<int32>& GetSelectedVertices() const { return SelectedVertices; }
	void SetSelectedVertices(const TArray<int32>& NewSelection);
	void SelectAllVertices();
	void ClearSelection();
	bool HasSelection() const { return SelectedVertices.Num() > 0; }

	/** Удаление выделенных элементов */
	void DeleteSelectedVertices();

	/** Статистика для отображения в HUD */
	struct FManualSpriteStats
	{
		int32 VertexCount;
		int32 TriangleCount;
		FVector2D SpriteSize;
		FString TextureName;
		bool bIsValid;

		FManualSpriteStats()
			: VertexCount(0)
			, TriangleCount(0)
			, SpriteSize(FVector2D::ZeroVector)
			, TextureName(TEXT("None"))
			, bIsValid(false)
		{
		}
	};

	/** Получение статистики для HUD */
	FManualSpriteStats GetSpriteStats() const;

protected:
	/** ИСПРАВЛЕНИЕ: Синхронизированная отрисовка спрайта */
	void DrawSpriteOnCanvas(FCanvas* Canvas, const FViewport* InViewport) const;
	
	/** Рендеринг отладочной геометрии */
	void DrawDebugGeometry(FCanvas* Canvas, const FViewport* InViewport) const;
	
	/** Рендеринг вершин */
	void DrawVertices(FCanvas* Canvas, const FViewport* InViewport) const;
	
	/** Рендеринг треугольников */
	void DrawTriangles(FCanvas* Canvas, const FViewport* InViewport) const;

	/** Рендеринг HUD с информацией */
	void DrawHUD(FCanvas* Canvas, const FViewport* InViewport) const;
	
	/** Рендеринг превью вставки */
	void DrawPastePreview(FCanvas* Canvas, const FViewport* InViewport) const;
	
	/** Обработка клика мыши */
	void HandleMouseClick(FViewport* InViewport, FKey Key, EInputEvent Event, FVector2D MousePos);
	
	/** Поиск ближайшей вершины */
	int32 FindVertexAtPosition(FVector2D WorldPos, float Tolerance = 10.0f) const;
	
	/** Поиск треугольника под курсором */
	int32 FindTriangleAtPosition(FVector2D WorldPos) const;

	/** Вычисление UV координат на основе мировой позиции */
	FVector2D CalculateUVFromWorldPosition(const FVector2D& WorldPos, const UManualSprite* ManualSprite) const;

	/** Получение размеров спрайта для вычислений */
	FVector2D GetSpriteSize(const UManualSprite* ManualSprite) const;

	/** Отрисовка сетки - ИСПРАВЛЕННАЯ ВЕРСИЯ */
	void DrawGrid(FCanvas* Canvas, const FViewport* InViewport) const;

	/** Привязка позиции к сетке */
	FVector2D SnapToGrid(const FVector2D& Position) const;

	/** Обработка транзакций перемещения вершин */
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
		bool bIsAdditive;
		
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
			return Size.X > 5.0f && Size.Y > 5.0f;
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
	void DrawBoxSelection(FCanvas* Canvas, const FViewport* InViewport) const;

	/** Проверка пересечения вершины с рамкой выделения */
	TArray<int32> GetVerticesInSelectionBox() const;

private:
	// Preview scene для корректного рендеринга
	FPreviewScene OwnedPreviewScene;

	/** Ссылка на редактор */
	TWeakPtr<FManualSpriteEditorToolkit> ManualSpriteEditorPtr;

	/** Manual Sprite для отображения */
	TWeakObjectPtr<UManualSprite> ManualSpritePtr;

	/** Render component для спрайта (НЕ ИСПОЛЬЗУЕТСЯ в исправленной версии) */
	TWeakObjectPtr<UPrimitiveComponent> SpriteRenderComponent;
	
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
	TArray<FVector2D> OriginalVertexPositions;
	TArray<FVector2D> OriginalVertexUVs;
	bool bDragTransactionStarted;

	/** Состояние выделения рамкой */
	FBoxSelectionState BoxSelection;

	/** Флаг для автоматического масштабирования при первом показе */
	bool bSpriteZoomed;

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