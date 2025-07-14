#include "ManualSpriteEditorViewport.h"
#include "ManualSpriteEditorToolkit.h"
#include "ManualSprite.h"
#include "Widgets/SViewport.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Canvas.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SceneViewport.h"
#include "Editor.h"
#include "SViewportToolBar.h"
#include "EditorViewportCommands.h"
#include "SCommonEditorViewportToolbarBase.h"

// Console variable для отладки
static TAutoConsoleVariable<int32> CVarDebugManualSprite(
	TEXT("ManualSprite.Debug"),
	0,
	TEXT("Enable debug rendering for Manual Sprite editor\n")
	TEXT("0: Disabled (default)\n")
	TEXT("1: Show sprite bounds and center\n"),
	ECVF_Default
);

// Константы для рендеринга - улучшенные контрастные цвета
const float ManualSpriteEditorViewport::VertexSize = 5.0f;
const float ManualSpriteEditorViewport::VertexSelectSize = 8.0f;
const FLinearColor ManualSpriteEditorViewport::VertexColor = FLinearColor(0.2f, 0.5f, 1.0f, 1.0f); // Синий
const FLinearColor ManualSpriteEditorViewport::SelectedVertexColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f); // Ярко-зеленый
const FLinearColor ManualSpriteEditorViewport::HoveredVertexColor = FLinearColor::White;
const FLinearColor ManualSpriteEditorViewport::TriangleColor = FLinearColor(0.0f, 1.0f, 0.0f, 0.15f); // Полупрозрачный зеленый
const FLinearColor ManualSpriteEditorViewport::SelectedTriangleColor = FLinearColor(0.0f, 1.0f, 0.0f, 0.4f); // Ярко-зеленый полупрозрачный
const FLinearColor ManualSpriteEditorViewport::HoveredTriangleColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.3f); // Белый полупрозрачный
const FLinearColor ManualSpriteEditorViewport::EdgeLineColor = FLinearColor(0.0f, 0.8f, 0.0f, 0.8f); // Зеленый для рёбер
const FLinearColor ManualSpriteEditorViewport::SelectedEdgeColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f); // Ярко-зеленый для выделенных рёбер

//////////////////////////////////////////////////////////////////////////
// SManualSpriteEditorViewport

void SManualSpriteEditorViewport::Construct(const FArguments& InArgs)
{
	ManualSpriteEditorPtr = InArgs._ManualSpriteEditor;
	SEditorViewport::Construct(SEditorViewport::FArguments());
}

TSharedRef<FEditorViewportClient> SManualSpriteEditorViewport::MakeEditorViewportClient()
{
	ViewportClient = MakeShareable(new ManualSpriteEditorViewport(ManualSpriteEditorPtr));
	if (const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin())
	{
		ViewportClient->SetManualSprite(Editor->GetManualSprite());
	}
	return ViewportClient.ToSharedRef();
}

TSharedPtr<FExtender> SManualSpriteEditorViewport::GetExtenders() const
{
	return MakeShared<FExtender>();
}

TSharedRef<SEditorViewport> SManualSpriteEditorViewport::GetViewportWidget()
{
	return StaticCastSharedRef<SEditorViewport>(AsShared());
}

TSharedPtr<FEditorViewportClient> SManualSpriteEditorViewport::GetViewportClient()
{
	return StaticCastSharedPtr<FEditorViewportClient>(ViewportClient);
}

TSharedPtr<SWidget> SManualSpriteEditorViewport::MakeViewportToolbar()
{
	return SNullWidget::NullWidget;
}

void SManualSpriteEditorViewport::RefreshViewport()
{
	if (ViewportClient.IsValid())
	{
		ViewportClient->Invalidate();
	}
}

//////////////////////////////////////////////////////////////////////////
// ManualSpriteEditorViewport - ИСПРАВЛЕННАЯ ВЕРСИЯ

ManualSpriteEditorViewport::ManualSpriteEditorViewport(TWeakPtr<FManualSpriteEditorToolkit> InManualSpriteEditor)
	: FEditorViewportClient(nullptr)
	, ManualSpriteEditorPtr(InManualSpriteEditor)
	, ZoomFactor(1.0f)
	, ViewOffset(FVector2D::ZeroVector)
	, bIsPanning(false)
	, LastMousePosition(FVector2D::ZeroVector)
	, HoveredVertex(-1)
	, HoveredTriangle(-1)
	, bIsDraggingVertices(false)
	, DragStartPosition(FVector2D::ZeroVector)
	, bDragTransactionStarted(false)
	, bSpriteZoomed(false)
	, LastViewportSize(FIntPoint::ZeroValue)
	, bIntersectionsCacheValid(false)  // ПЕРЕМЕСТИТЬ ПЕРЕД SelectedTriangles
{
	// ИСПРАВЛЕНИЕ: Минимальная инициализация для 2D редактора
	PreviewScene = &OwnedPreviewScene;
	
	// Настройка для 2D рендеринга
	SetRealtime(false); // Отключаем real-time для лучшей производительности
	DrawHelper.bDrawGrid = false;
	EngineShowFlags.DisableAdvancedFeatures();
	EngineShowFlags.SetCompositeEditorPrimitives(false);
	
	UE_LOG(LogTemp, Warning, TEXT("✅ Manual Sprite Viewport: Initialized with sync fix"));
}

ManualSpriteEditorViewport::~ManualSpriteEditorViewport()
{
	// Очистка без 3D компонентов
	UE_LOG(LogTemp, Log, TEXT("Manual Sprite Viewport: Destroyed"));
}

void ManualSpriteEditorViewport::SetManualSprite(UManualSprite* InManualSprite)
{
	ManualSpritePtr = InManualSprite;
	
	// ИСПРАВЛЕНИЕ: НЕ создаем 3D render component, работаем только с 2D
	SpriteRenderComponent.Reset();
	
	// Сброс флага масштабирования для пересчёта
	bSpriteZoomed = false;
	// Инвалидируем кэш при смене спрайта
	InvalidateIntersectionsCache();
	
	if (ManualSpritePtr.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Manual Sprite set: %s"), *ManualSpritePtr->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Manual Sprite cleared"));
	}
}

void ManualSpriteEditorViewport::RefreshManualSprite()
{
	// Простое обновление без 3D компонентов
	if (Viewport)
	{
		Viewport->Invalidate();
	}
}

// ИСПРАВЛЕНИЕ: Правильная система координат
FVector2D ManualSpriteEditorViewport::ScreenToWorld(FVector2D ScreenPos, const FViewport* InViewport) const
{
	const FVector2D ViewportSize = FVector2D(InViewport->GetSizeXY().X, InViewport->GetSizeXY().Y);
	const FVector2D ViewportCenter = ViewportSize * 0.5f;
	
	// Формула: World = (Screen - Center - Offset*Zoom) / Zoom
	return (ScreenPos - ViewportCenter - ViewOffset * ZoomFactor) / ZoomFactor;
}

FVector2D ManualSpriteEditorViewport::WorldToScreen(FVector2D WorldPos, const FViewport* InViewport) const
{
	const FVector2D ViewportSize = FVector2D(InViewport->GetSizeXY().X, InViewport->GetSizeXY().Y);
	const FVector2D ViewportCenter = ViewportSize * 0.5f;
	
	// Формула: Screen = Center + Offset*Zoom + World*Zoom
	return ViewportCenter + ViewOffset * ZoomFactor + WorldPos * ZoomFactor;
}

void ManualSpriteEditorViewport::SetZoom(float NewZoom)
{
	const float OldZoom = ZoomFactor;
	ZoomFactor = FMath::Clamp(NewZoom, 0.1f, 10.0f);
	
	if (!FMath::IsNearlyEqual(OldZoom, ZoomFactor) && Viewport)
	{
		Viewport->Invalidate();
		UE_LOG(LogTemp, VeryVerbose, TEXT("Zoom changed: %.2f -> %.2f"), OldZoom, ZoomFactor);
	}
}

void ManualSpriteEditorViewport::SetViewOffset(FVector2D NewOffset)
{
	const FVector2D OldOffset = ViewOffset;
	ViewOffset = NewOffset;
	
	if (!OldOffset.Equals(ViewOffset, 1.0f) && Viewport)
	{
		Viewport->Invalidate();
		UE_LOG(LogTemp, VeryVerbose, TEXT("View offset: (%.1f,%.1f) -> (%.1f,%.1f)"), 
			OldOffset.X, OldOffset.Y, ViewOffset.X, ViewOffset.Y);
	}
}

ManualSpriteEditorViewport::FManualSpriteStats ManualSpriteEditorViewport::GetSpriteStats() const
{
	FManualSpriteStats Stats;
	
	if (ManualSpritePtr.IsValid())
	{
		const UManualSprite* Sprite = ManualSpritePtr.Get();
		Stats.VertexCount = Sprite->ManualGeometry.Vertices.Num();
		Stats.TriangleCount = Sprite->ManualGeometry.Triangles.Num();
		Stats.SpriteSize = Sprite->GetSourceSize();
		Stats.bIsValid = Sprite->IsManualGeometryValid();
		
		if (UTexture2D* Texture = Sprite->GetSourceTexture())
		{
			Stats.TextureName = Texture->GetName();
		}
	}
	
	return Stats;
}

void ManualSpriteEditorViewport::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	// НЕ вызываем родительский метод чтобы избежать нежелательного 3D рендеринга
	// FEditorViewportClient::Draw(View, PDI);
	
	// Минимальная 3D отрисовка если необходимо
}

// ОСНОВНАЯ ФУНКЦИЯ РЕНДЕРИНГА - ПОЛНОСТЬЮ ИСПРАВЛЕННАЯ
void ManualSpriteEditorViewport::DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas)
{
	// НЕ вызываем родительский метод чтобы избежать 3D рендеринга
	// FEditorViewportClient::DrawCanvas(InViewport, View, Canvas);

	// Сохраняем размер viewport
	LastViewportSize = InViewport.GetSizeXY();

	// Очищаем canvas
	Canvas.Clear(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f)); // Темно-серый фон

	// ПОРЯДОК РЕНДЕРИНГА (от заднего плана к переднему):
	
	// 1. Рисуем сетку (задний план)
	DrawGrid(&Canvas, &InViewport);
	
	// 2. Рисуем спрайт (средний план) - КЛЮЧЕВОЕ ИСПРАВЛЕНИЕ
	DrawSpriteOnCanvas(&Canvas, &InViewport);

	// 3. Рисуем геометрию (передний план)
	DrawDebugGeometry(&Canvas, &InViewport);
	
	// 4. Рисуем UI элементы (самый передний план)
	DrawBoxSelection(&Canvas, &InViewport);
	DrawHUD(&Canvas, &InViewport);
}

// НОВАЯ ФУНКЦИЯ: Синхронизированная отрисовка спрайта
void ManualSpriteEditorViewport::DrawSpriteOnCanvas(FCanvas* Canvas, const FViewport* InViewport) const
{
	if (!ManualSpritePtr.IsValid())
		return;

	UTexture2D* SourceTexture = ManualSpritePtr->GetSourceTexture();
	if (!SourceTexture)
		return;

	// Получаем размер спрайта
	const FVector2D SpriteSize = ManualSpritePtr->GetSourceSize();
	if (SpriteSize.IsNearlyZero())
		return;

	// КРИТИЧНО: Используем ту же систему координат что и для точек
	const FVector2D SpriteWorldCenter = FVector2D::ZeroVector; // Спрайт в центре мира
	const FVector2D SpriteScreenCenter = WorldToScreen(SpriteWorldCenter, InViewport);
	const FVector2D SpriteSizeOnScreen = SpriteSize * ZoomFactor;
	
	// Позиция для отрисовки (верхний левый угол)
	const FVector2D DrawPosition = SpriteScreenCenter - SpriteSizeOnScreen * 0.5f;

	// Проверяем видимость
	const FVector2D ViewportSize = FVector2D(InViewport->GetSizeXY().X, InViewport->GetSizeXY().Y);
	if (DrawPosition.X > ViewportSize.X || DrawPosition.Y > ViewportSize.Y ||
		DrawPosition.X + SpriteSizeOnScreen.X < 0 || DrawPosition.Y + SpriteSizeOnScreen.Y < 0)
	{
		return; // Спрайт за пределами видимости
	}

	// Рисуем спрайт
	FCanvasTileItem TileItem(
		DrawPosition,
		SourceTexture->GetResource(),
		SpriteSizeOnScreen,
		FLinearColor::White
	);
	
	TileItem.BlendMode = SE_BLEND_AlphaBlend;
	Canvas->DrawItem(TileItem);

	// Отладочная информация (только в debug билдах)
	#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (CVarDebugManualSprite.GetValueOnGameThread() > 0)
	{
		// Рамка вокруг спрайта
		FCanvasBoxItem DebugBox(DrawPosition, SpriteSizeOnScreen);
		DebugBox.SetColor(FLinearColor(FColor::Cyan));
		DebugBox.LineThickness = 1.0f;
		Canvas->DrawItem(DebugBox);
		
		// Крестик в центре
		FCanvasLineItem CrossH(
			SpriteScreenCenter - FVector2D(10, 0), 
			SpriteScreenCenter + FVector2D(10, 0)
		);
		CrossH.SetColor(FLinearColor(FColor::Cyan));
		Canvas->DrawItem(CrossH);
		
		FCanvasLineItem CrossV(
			SpriteScreenCenter - FVector2D(0, 10), 
			SpriteScreenCenter + FVector2D(0, 10)
		);
		CrossV.SetColor(FLinearColor(FColor::Cyan));
		Canvas->DrawItem(CrossV);
	}
	#endif

	UE_LOG(LogTemp, VeryVerbose, TEXT("Sprite drawn: Center(%.1f,%.1f), Size(%.1f,%.1f), Zoom %.2f"),
		SpriteScreenCenter.X, SpriteScreenCenter.Y, SpriteSizeOnScreen.X, SpriteSizeOnScreen.Y, ZoomFactor);
}

void ManualSpriteEditorViewport::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);

	// ИСПРАВЛЕНИЕ: Автоматическое масштабирование при первой загрузке
	if (!bSpriteZoomed && ManualSpritePtr.IsValid())
	{
		const FIntPoint ViewportSize = Viewport->GetSizeXY();
		if (ViewportSize.X > 0 && ViewportSize.Y > 0)
		{
			const FVector2D SpriteSize = ManualSpritePtr->GetSourceSize();
			
			if (!SpriteSize.IsNearlyZero())
			{
				// Вычисляем оптимальный зум для отображения спрайта (80% от размера viewport)
				const float MaxViewportSize = FMath::Max(ViewportSize.X, ViewportSize.Y) * 0.8f;
				const float MaxSpriteSize = FMath::Max(SpriteSize.X, SpriteSize.Y);
				const float OptimalZoom = MaxViewportSize / MaxSpriteSize;
				
				SetZoom(FMath::Clamp(OptimalZoom, 0.1f, 2.0f));
				SetViewOffset(FVector2D::ZeroVector); // Центрируем
				
				bSpriteZoomed = true;
				
				UE_LOG(LogTemp, Log, TEXT("Auto-zoom set to %.2f for sprite size (%.1f,%.1f)"), 
					OptimalZoom, SpriteSize.X, SpriteSize.Y);
			}
		}
	}

	// Обновляем preview scene минимально
	if (!GIntraFrameDebuggingGameThread)
	{
		OwnedPreviewScene.GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
	}
}

void ManualSpriteEditorViewport::SetupViewForRendering(FSceneViewFamily& ViewFamily, FSceneView& View)
{
	FEditorViewportClient::SetupViewForRendering(ViewFamily, View);
}

// ИСПРАВЛЕНИЕ: Обработка ввода
bool ManualSpriteEditorViewport::InputKey(const FInputKeyEventArgs& EventArgs)
{
	const FKey Key = EventArgs.Key;
	const EInputEvent Event = EventArgs.Event;
	FViewport* InViewport = EventArgs.Viewport;

	// Проверяем команды редактора
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (Editor.IsValid())
	{
		if (const TSharedPtr<FUICommandList> CommandList = Editor->GetCommandList())
		{
			if (CommandList->ProcessCommandBindings(Key, FSlateApplication::Get().GetModifierKeys(), Event == IE_Repeat))
			{
				return true;
			}
		}
	}

	// Обработка клавиши Escape
	if (Key == EKeys::Escape && Event == IE_Pressed)
	{
		bool bHandled = false;
		
		if (BoxSelection.bIsActive)
		{
			CancelBoxSelection();
			InViewport->Invalidate();
			bHandled = true;
		}
		
		if (bIsDraggingVertices)
		{
			CancelVerticesDrag();
			InViewport->Invalidate();
			bHandled = true;
		}
		
		if (!bHandled && SelectedVertices.Num() > 0)
		{
			SelectedVertices.Empty();
			InViewport->Invalidate();
			bHandled = true;
		}
		
		if (bHandled)
		{
			return true;
		}
	}

	// Обработка кликов мыши
	if (Key == EKeys::LeftMouseButton || Key == EKeys::RightMouseButton)
	{
		const FVector2D ViewportMousePos = FVector2D(InViewport->GetMouseX(), InViewport->GetMouseY());
		HandleMouseClick(InViewport, Key, Event, ViewportMousePos);
		return true;
	}

	// Обработка масштабирования
	if (Key == EKeys::MouseScrollUp)
	{
		SetZoom(FMath::Clamp(ZoomFactor * 1.1f, 0.1f, 10.0f));
		InViewport->Invalidate();
		return true;
	}
	if (Key == EKeys::MouseScrollDown)
	{
		SetZoom(FMath::Clamp(ZoomFactor * 0.9f, 0.1f, 10.0f));
		InViewport->Invalidate();
		return true;
	}

	// Панорамирование
	if (Key == EKeys::MiddleMouseButton)
	{
		if (Event == IE_Pressed)
		{
			bIsPanning = true;
			LastMousePosition = FVector2D(InViewport->GetMouseX(), InViewport->GetMouseY());
		}
		else if (Event == IE_Released)
		{
			bIsPanning = false;
		}
		return true;
	}

	return false;
}

bool ManualSpriteEditorViewport::InputAxis(FViewport* InViewport, FInputDeviceId DeviceId, FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	return false;
}

void ManualSpriteEditorViewport::MouseMove(FViewport* InViewport, int32 X, int32 Y)
{
	const FVector2D LocalMousePos(X, Y);
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();

	if (bIsPanning)
	{
		const FVector2D Delta = (LocalMousePos - LastMousePosition) / ZoomFactor;
		SetViewOffset(ViewOffset + Delta);
		InViewport->Invalidate();
	}
	else if (BoxSelection.bIsActive)
	{
		UpdateBoxSelection(LocalMousePos);
		InViewport->Invalidate();
	}
	else if (bIsDraggingVertices)
	{
		const FVector2D MouseDelta = LocalMousePos - DragStartPosition;
		UpdateVerticesDrag(MouseDelta);
		InViewport->Invalidate();
	}
	else if (Editor.IsValid() && Editor->GetEditMode() == FManualSpriteEditorToolkit::EEditMode::Paste)
	{
		const FVector2D WorldPos = ScreenToWorld(LocalMousePos, InViewport);
		const FVector2D SnappedWorldPos = SnapToGrid(WorldPos);
		Editor->SetPastePreviewPosition(SnappedWorldPos);
		InViewport->Invalidate();
	}

	LastMousePosition = LocalMousePos;

	// ИСПРАВЛЕНИЕ: Точное определение hover с использованием экранных координат
	if (!bIsDraggingVertices && !BoxSelection.bIsActive && 
		(!Editor.IsValid() || Editor->GetEditMode() != FManualSpriteEditorToolkit::EEditMode::Paste))
	{
		// Используем адаптивный tolerance в экранных координатах
		const float BaseScreenTolerance = 8.0f; // Базовый размер в пикселях
		
		const int32 NewHoveredVertex = FindVertexAtScreenPosition(LocalMousePos, InViewport, BaseScreenTolerance);
		
		// Для треугольников используем мировые координаты (они менее критичны к точности)
		const int32 NewHoveredTriangle = FindTriangleAtPosition(ScreenToWorld(LocalMousePos, InViewport));
		
		// Обновляем только если hover изменился
		if (NewHoveredVertex != HoveredVertex || NewHoveredTriangle != HoveredTriangle)
		{
			HoveredVertex = NewHoveredVertex;
			HoveredTriangle = NewHoveredTriangle;
			InViewport->Invalidate();
		}
	}
}

void ManualSpriteEditorViewport::CapturedMouseMove(FViewport* InViewport, int32 InMouseX, int32 InMouseY)
{
	MouseMove(InViewport, InMouseX, InMouseY);
}

// ИСПРАВЛЕНИЕ: Отрисовка сетки синхронизированно со спрайтом
void ManualSpriteEditorViewport::DrawGrid(FCanvas* Canvas, const FViewport* InViewport) const
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid())
		return;

	const auto& GridSettings = Editor->GetGridSettings();
	if (!GridSettings.bShowGrid)
		return;

	const FVector2D ViewportSize = FVector2D(InViewport->GetSizeXY().X, InViewport->GetSizeXY().Y);
	const float GridSizeOnScreen = GridSettings.GridSize * ZoomFactor;
	
	// Не рисуем сетку если она слишком мелкая
	if (GridSizeOnScreen < 4.0f)
		return;

	// ИСПРАВЛЕНИЕ: Вычисляем границы сетки в мировых координатах
	const FVector2D TopLeftWorld = ScreenToWorld(FVector2D::ZeroVector, InViewport);
	const FVector2D BottomRightWorld = ScreenToWorld(ViewportSize, InViewport);
	
	const float GridWorldSize = GridSettings.GridSize;
	
	// Вычисляем диапазон линий сетки
	const int32 StartX = FMath::FloorToInt(TopLeftWorld.X / GridWorldSize) - 1;
	const int32 EndX = FMath::CeilToInt(BottomRightWorld.X / GridWorldSize) + 1;
	const int32 StartY = FMath::FloorToInt(TopLeftWorld.Y / GridWorldSize) - 1;
	const int32 EndY = FMath::CeilToInt(BottomRightWorld.Y / GridWorldSize) + 1;

	// Рисуем вертикальные линии
	for (int32 X = StartX; X <= EndX; ++X)
	{
		const float WorldX = X * GridWorldSize;
		const FVector2D StartScreen = WorldToScreen(FVector2D(WorldX, TopLeftWorld.Y), InViewport);
		const FVector2D EndScreen = WorldToScreen(FVector2D(WorldX, BottomRightWorld.Y), InViewport);
		
		// Выделяем главные оси
		FLinearColor LineColor = GridSettings.GridColor;
		if (X == 0) LineColor = FLinearColor(1.0f, 0.0f, 0.0f, 0.8f); // Красная линия X=0
		
		FCanvasLineItem LineItem(StartScreen, EndScreen);
		LineItem.SetColor(LineColor);
		Canvas->DrawItem(LineItem);
	}

	// Рисуем горизонтальные линии
	for (int32 Y = StartY; Y <= EndY; ++Y)
	{
		const float WorldY = Y * GridWorldSize;
		const FVector2D StartScreen = WorldToScreen(FVector2D(TopLeftWorld.X, WorldY), InViewport);
		const FVector2D EndScreen = WorldToScreen(FVector2D(BottomRightWorld.X, WorldY), InViewport);
		
		// Выделяем главные оси
		FLinearColor LineColor = GridSettings.GridColor;
		if (Y == 0) LineColor = FLinearColor(0.0f, 1.0f, 0.0f, 0.8f); // Зеленая линия Y=0
		
		FCanvasLineItem LineItem(StartScreen, EndScreen);
		LineItem.SetColor(LineColor);
		Canvas->DrawItem(LineItem);
	}
}

void ManualSpriteEditorViewport::DrawDebugGeometry(FCanvas* Canvas, const FViewport* InViewport) const
{
	if (!ManualSpritePtr.IsValid())
		return;

	if (ManualSpritePtr->bUseManualGeometry)
	{
		DrawTriangles(Canvas, InViewport);
		DrawVertices(Canvas, InViewport);
	}

	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (Editor.IsValid() && Editor->GetEditMode() == FManualSpriteEditorToolkit::EEditMode::Paste)
	{
		DrawPastePreview(Canvas, InViewport);
	}
}

void ManualSpriteEditorViewport::DrawVertices(FCanvas* Canvas, const FViewport* InViewport) const
{
	if (!ManualSpritePtr.IsValid())
		return;

	const FManualSpriteGeometry& Geometry = ManualSpritePtr->ManualGeometry;

	for (int32 i = 0; i < Geometry.Vertices.Num(); i++)
	{
		const FVector2D WorldPos = Geometry.Vertices[i].Position;
		const FVector2D ScreenPos = WorldToScreen(WorldPos, InViewport);

		// Определяем цвет и размер точки в зависимости от состояния
		FLinearColor Color = VertexColor;
		float Size = VertexSize;
		bool bDrawOutline = false;
		bool bDrawHalo = false;

		// Проверяем режим редактирования
		const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
		const bool bInTriangleMode = Editor.IsValid() && Editor->GetEditMode() == FManualSpriteEditorToolkit::EEditMode::Triangle;

		// Выделенные вершины
		if (SelectedVertices.Contains(i))
		{
			if (bInTriangleMode)
			{
				// В режиме треугольника используем специальный цвет
				Color = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f); // Оранжевый для режима треугольника
			}
			else
			{
				Color = SelectedVertexColor;
			}
			Size = VertexSelectSize;
			bDrawOutline = true;
			bDrawHalo = true;
		}
		
		// Вершина под курсором (приоритет над выделением)
		else if (i == HoveredVertex)
		{
			Color = FLinearColor::White;
			Size = VertexSelectSize * 1.2f; // Увеличиваем для лучшей видимости
			bDrawOutline = true;
			bDrawHalo = true;
		}
		// Вершины во время перетаскивания
		else if (bIsDraggingVertices && SelectedVertices.Contains(i))
		{
			Color = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f);
			Size = VertexSelectSize;
			bDrawOutline = true;
		}

		// Рисуем ореол для важных точек
		if (bDrawHalo)
		{
			const float HaloSize = Size * 1.5f;
			FCanvasBoxItem HaloItem(
				ScreenPos - FVector2D(HaloSize * 0.5f),
				FVector2D(HaloSize)
			);
			HaloItem.SetColor(FLinearColor(Color.R, Color.G, Color.B, 0.2f));
			Canvas->DrawItem(HaloItem);
		}

		// Основная точка
		FCanvasBoxItem BoxItem(
			ScreenPos - FVector2D(Size * 0.5f),
			FVector2D(Size)
		);
		BoxItem.SetColor(Color);
		Canvas->DrawItem(BoxItem);

		// Контур для лучшей видимости
		if (bDrawOutline)
		{
			const float OutlineThickness = 1.5f;
			FCanvasBoxItem OutlineItem(
				ScreenPos - FVector2D((Size + OutlineThickness) * 0.5f),
				FVector2D(Size + OutlineThickness)
			);
			OutlineItem.SetColor(FLinearColor::Black);
			OutlineItem.LineThickness = OutlineThickness;
			Canvas->DrawItem(OutlineItem);
		}

		// Номер вершины для выделенных или под курсором
		if (SelectedVertices.Contains(i) || i == HoveredVertex)
		{
			const FString VertexNumber = FString::Printf(TEXT("%d"), i);
			FCanvasTextItem TextItem(
				ScreenPos + FVector2D(Size * 0.5f + 2.0f, -Size * 0.5f - 2.0f),
				FText::FromString(VertexNumber),
				GEngine->GetSmallFont(),
				FLinearColor::White
			);
			TextItem.EnableShadow(FLinearColor::Black);
			Canvas->DrawItem(TextItem);
		}
		
		// Детальная информация только для точки под курсором
		if (i == HoveredVertex && !bIsDraggingVertices)
		{
			const FVector2D UV = Geometry.Vertices[i].UV;
			const FString InfoText = FString::Printf(TEXT("V%d: (%.1f, %.1f) UV:(%.3f, %.3f)"), 
				i, WorldPos.X, WorldPos.Y, UV.X, UV.Y);
			
			// Фон для текста
			const float TextWidth = 200.0f;
			const float TextHeight = 16.0f;
			const FVector2D TextPos = ScreenPos + FVector2D(Size * 0.5f + 5.0f, Size * 0.5f);
			
			FCanvasBoxItem BackgroundItem(TextPos - FVector2D(2.0f, 2.0f), FVector2D(TextWidth, TextHeight));
			BackgroundItem.SetColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
			Canvas->DrawItem(BackgroundItem);
			
			FCanvasTextItem InfoTextItem(TextPos, FText::FromString(InfoText), GEngine->GetSmallFont(), FLinearColor::Yellow);
			Canvas->DrawItem(InfoTextItem);
		}
	}
}

void ManualSpriteEditorViewport::DrawTriangles(FCanvas* Canvas, const FViewport* InViewport) const
{
	if (!ManualSpritePtr.IsValid())
		return;

	const FManualSpriteGeometry& Geometry = ManualSpritePtr->ManualGeometry;

	// Обновляем кэш пересечений и частичное выделение
	UpdateIntersectingEdges();
	const_cast<ManualSpriteEditorViewport*>(this)->UpdatePartialTrianglesSelection();

	for (int32 i = 0; i < Geometry.Triangles.Num(); i++)
	{
		const FManualSpriteTriangle& Triangle = Geometry.Triangles[i];

		if (Triangle.VertexIndex0 >= Geometry.Vertices.Num() ||
			Triangle.VertexIndex1 >= Geometry.Vertices.Num() ||
			Triangle.VertexIndex2 >= Geometry.Vertices.Num())
			continue;

		const FVector2D Pos0 = WorldToScreen(Geometry.Vertices[Triangle.VertexIndex0].Position, InViewport);
		const FVector2D Pos1 = WorldToScreen(Geometry.Vertices[Triangle.VertexIndex1].Position, InViewport);
		const FVector2D Pos2 = WorldToScreen(Geometry.Vertices[Triangle.VertexIndex2].Position, InViewport);

		// Определяем состояние треугольника
		const bool bIsFullySelected = SelectedTriangles.Contains(i);
		const bool bIsPartiallySelected = PartiallySelectedTriangles.Contains(i);
		const bool bIsHovered = (i == HoveredTriangle);
		
		// Выбираем цвета и стили - HOVER имеет приоритет
		FLinearColor FillColor = TriangleColor;
		FLinearColor EdgeColorToUse = EdgeLineColor;
		float EdgeThickness = 1.0f;
		
		if (bIsHovered)
		{
			// HOVER имеет наивысший приоритет
			FillColor = HoveredTriangleColor;
			EdgeColorToUse = FLinearColor::White;
			EdgeThickness = 2.0f;
		}
		else if (bIsFullySelected)
		{
			// Полностью выделенный треугольник - ярко-зеленый
			FillColor = SelectedTriangleColor;
			EdgeColorToUse = SelectedEdgeColor;
			EdgeThickness = 2.5f;
		}
		else if (bIsPartiallySelected)
		{
			// Частично выделенный треугольник - синий
			FillColor = FLinearColor(0.0f, 0.5f, 1.0f, 0.3f); // Синий полупрозрачный
			EdgeColorToUse = FLinearColor(0.0f, 0.7f, 1.0f, 1.0f); // Синие рёбра
			EdgeThickness = 1.8f;
		}

		// Рисуем заливку треугольника
		if (bIsFullySelected || bIsPartiallySelected || bIsHovered)
		{
			// Создаем массив вершин для треугольника
			TArray<FCanvasUVTri> Triangles;
			FCanvasUVTri TriangleItem;
			TriangleItem.V0_Pos = FVector2D(Pos0.X, Pos0.Y);
			TriangleItem.V1_Pos = FVector2D(Pos1.X, Pos1.Y);
			TriangleItem.V2_Pos = FVector2D(Pos2.X, Pos2.Y);
			TriangleItem.V0_UV = FVector2D(0.0f, 0.0f);
			TriangleItem.V1_UV = FVector2D(1.0f, 0.0f);
			TriangleItem.V2_UV = FVector2D(0.5f, 1.0f);
			TriangleItem.V0_Color = FillColor;
			TriangleItem.V1_Color = FillColor;
			TriangleItem.V2_Color = FillColor;
			Triangles.Add(TriangleItem);
			
			FCanvasTriangleItem CanvasTriangle(Triangles, GWhiteTexture);
			CanvasTriangle.BlendMode = SE_BLEND_AlphaBlend;
			Canvas->DrawItem(CanvasTriangle);
		}

		// Рисуем рёбра треугольника с проверкой пересечений
		DrawTriangleEdge(Canvas, Pos0, Pos1, Triangle.VertexIndex0, Triangle.VertexIndex1, EdgeColorToUse, EdgeThickness);
		DrawTriangleEdge(Canvas, Pos1, Pos2, Triangle.VertexIndex1, Triangle.VertexIndex2, EdgeColorToUse, EdgeThickness);
		DrawTriangleEdge(Canvas, Pos2, Pos0, Triangle.VertexIndex2, Triangle.VertexIndex0, EdgeColorToUse, EdgeThickness);

		// Показываем номер треугольника если он выделен или под курсором
		if (bIsFullySelected || bIsPartiallySelected || bIsHovered)
		{
			const FVector2D Center = (Pos0 + Pos1 + Pos2) / 3.0f;
			const FString TriangleNumber = FString::Printf(TEXT("T%d"), i);
			
			// Фон для номера
			const float TextWidth = 30.0f;
			const float TextHeight = 12.0f;
			FCanvasBoxItem BackgroundItem(Center - FVector2D(TextWidth * 0.5f, TextHeight * 0.5f), FVector2D(TextWidth, TextHeight));
			BackgroundItem.SetColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
			Canvas->DrawItem(BackgroundItem);
			
			FLinearColor TextColor = FLinearColor::White;
			if (bIsHovered) TextColor = FLinearColor::White;
			else if (bIsFullySelected) TextColor = FLinearColor::Green;
			else if (bIsPartiallySelected) TextColor = FLinearColor(0.0f, 0.7f, 1.0f, 1.0f);
			
			FCanvasTextItem TextItem(Center - FVector2D(12.0f, 6.0f), FText::FromString(TriangleNumber), 
				GEngine->GetTinyFont(), TextColor);
			Canvas->DrawItem(TextItem);
		}
	}
}

void ManualSpriteEditorViewport::DrawHUD(FCanvas* Canvas, const FViewport* InViewport) const
{
	const FManualSpriteStats Stats = GetSpriteStats();
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	
	// Информация в левом верхнем углу
	const float TextYPos = 20.0f;
	const float LineHeight = 16.0f;
	float CurrentY = TextYPos;
	
	// Название спрайта
	if (ManualSpritePtr.IsValid())
	{
		FCanvasTextItem TextItem(FVector2D(10, CurrentY), 
			FText::FromString(FString::Printf(TEXT("Manual Sprite: %s"), *ManualSpritePtr->GetName())),
			GEngine->GetMediumFont(), FLinearColor::White);
		TextItem.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(TextItem);
		CurrentY += LineHeight;
	}
	
	// Статистика геометрии с проверкой пересечений
	{
		// Обновляем кэш пересечений для подсчёта
		UpdateIntersectingEdges();
		const int32 IntersectionCount = CachedIntersectingEdges.Num();
		
		FLinearColor StatsColor = Stats.bIsValid ? FLinearColor::Green : FLinearColor::Red;
		FString StatsText = FString::Printf(TEXT("Vertices: %d, Triangles: %d"), Stats.VertexCount, Stats.TriangleCount);
		
		// Добавляем информацию о пересечениях
		if (IntersectionCount > 0)
		{
			StatsColor = FLinearColor::Red; // Красный если есть пересечения
			StatsText += FString::Printf(TEXT(" | ⚠ %d intersecting edges"), IntersectionCount);
		}
		else if (Stats.TriangleCount > 0)
		{
			StatsText += TEXT(" | ✓ Clean triangulation");
		}
		
		FCanvasTextItem TextItem(FVector2D(10, CurrentY), 
			FText::FromString(StatsText), GEngine->GetSmallFont(), StatsColor);
		TextItem.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(TextItem);
		CurrentY += LineHeight;
	}
	
	// Размер спрайта
	if (!Stats.SpriteSize.IsNearlyZero())
	{
		FCanvasTextItem TextItem(FVector2D(10, CurrentY), 
			FText::FromString(FString::Printf(TEXT("Size: %.0f x %.0f"), Stats.SpriteSize.X, Stats.SpriteSize.Y)),
			GEngine->GetSmallFont(), FLinearColor::Gray);
		TextItem.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(TextItem);
		CurrentY += LineHeight;
	}
	
	// Текстура
	if (!Stats.TextureName.IsEmpty())
	{
		FCanvasTextItem TextItem(FVector2D(10, CurrentY), 
			FText::FromString(FString::Printf(TEXT("Texture: %s"), *Stats.TextureName)),
			GEngine->GetSmallFont(), FLinearColor::Gray);
		TextItem.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(TextItem);
		CurrentY += LineHeight;
	}

	// НОВОЕ: Предупреждение о пересечениях (если есть)
	UpdateIntersectingEdges();
	if (CachedIntersectingEdges.Num() > 0)
	{
		CurrentY += 5; // Небольшой отступ
		
		// Фон для предупреждения
		const FString WarningText = FString::Printf(TEXT("⚠ TRIANGULATION ISSUES: %d intersecting edges detected!"), 
			CachedIntersectingEdges.Num());
		const float WarningWidth = 450.0f;
		const float WarningHeight = 18.0f;
		
		FCanvasBoxItem WarningBackground(FVector2D(8, CurrentY - 2), FVector2D(WarningWidth, WarningHeight));
		WarningBackground.SetColor(FLinearColor(1.0f, 0.0f, 0.0f, 0.2f)); // Полупрозрачный красный фон
		Canvas->DrawItem(WarningBackground);
		
		// Мигающий текст предупреждения
		const float Time = FApp::GetCurrentTime();
		const float BlinkAlpha = 0.7f + 0.3f * FMath::Sin(Time * 6.0f); // Медленное мигание
		
		FCanvasTextItem WarningTextItem(FVector2D(10, CurrentY), 
			FText::FromString(WarningText),
			GEngine->GetSmallFont(), 
			FLinearColor(1.0f, 1.0f, 1.0f, BlinkAlpha));
		WarningTextItem.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(WarningTextItem);
		CurrentY += LineHeight + 5;
		
		// Советы по исправлению
		const FString AdviceText = TEXT("Tip: Manually fix overlapping triangles");
		FCanvasTextItem AdviceTextItem(FVector2D(10, CurrentY), 
			FText::FromString(AdviceText),
			GEngine->GetTinyFont(), 
			FLinearColor(1.0f, 1.0f, 0.0f, 0.8f)); // Жёлтый цвет для совета
		AdviceTextItem.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(AdviceTextItem);
		CurrentY += 12;
	}
	
	// Режим редактирования
	if (Editor.IsValid())
	{
		FString ModeText;
		FLinearColor ModeColor = FLinearColor::Yellow;
		
		switch (Editor->GetEditMode())
		{
		case FManualSpriteEditorToolkit::EEditMode::Select:
			ModeText = TEXT("Mode: Select (Q) - Click and drag vertices");
			break;
		case FManualSpriteEditorToolkit::EEditMode::AddVertex:
			ModeText = TEXT("Mode: Add Vertex (W) - Click to place");
			ModeColor = FLinearColor::Green;
			break;
		case FManualSpriteEditorToolkit::EEditMode::Triangle:
			ModeText = TEXT("Mode: Triangle (E) - Select 3 vertices");
			ModeColor = FLinearColor::Blue;
			break;
		case FManualSpriteEditorToolkit::EEditMode::Delete:
			ModeText = TEXT("Mode: Delete (R) - Click to remove");
			ModeColor = FLinearColor::Red;
			break;
		case FManualSpriteEditorToolkit::EEditMode::Paste:
			ModeText = TEXT("Mode: Paste - Click to place copied vertices");
			ModeColor = FColor::Cyan;
			break;
		}
		
		FCanvasTextItem TextItem(FVector2D(10, CurrentY), 
			FText::FromString(ModeText),
			GEngine->GetSmallFont(), ModeColor);
		TextItem.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(TextItem);
		CurrentY += LineHeight;
	}
	
	// Информация о выделении
// Информация о выделении
if (SelectedVertices.Num() > 0 || SelectedTriangles.Num() > 0)
{
    FString SelectionText;
    
    // Проверяем режим редактирования
    const bool bInTriangleMode = Editor.IsValid() && Editor->GetEditMode() == FManualSpriteEditorToolkit::EEditMode::Triangle;
    
    if (SelectedVertices.Num() > 0)
    {
        if (bInTriangleMode)
        {
            // Специальное отображение для режима треугольника
            SelectionText = FString::Printf(TEXT("Triangle Mode: %d/3 vertices selected"), SelectedVertices.Num());
            if (SelectedVertices.Num() == 3)
            {
                SelectionText += TEXT(" | Ready to create triangle!");
            }
        }
        else
        {
            SelectionText = FString::Printf(TEXT("Selected: %d vertices"), SelectedVertices.Num());
            
            // Подсчитываем количество связанных треугольников (только вне режима Triangle)
            if (ManualSpritePtr.IsValid())
            {
                int32 ConnectedTriangles = 0;
                for (const FManualSpriteTriangle& Triangle : ManualSpritePtr->ManualGeometry.Triangles)
                {
                    bool bIsConnected = false;
                    for (int32 SelectedVertex : SelectedVertices)
                    {
                        if (Triangle.VertexIndex0 == SelectedVertex ||
                            Triangle.VertexIndex1 == SelectedVertex ||
                            Triangle.VertexIndex2 == SelectedVertex)
                        {
                            bIsConnected = true;
                            break;
                        }
                    }
                    if (bIsConnected)
                    {
                        ConnectedTriangles++;
                    }
                }
                
                if (ConnectedTriangles > 0)
                {
                    SelectionText += FString::Printf(TEXT(" (%d connected triangles)"), ConnectedTriangles);
                }
            }
        }
    }
    else if (SelectedTriangles.Num() > 0)
    {
        SelectionText = FString::Printf(TEXT("Selected: %d triangles"), SelectedTriangles.Num());
        
        // Подсчитываем количество уникальных вершин
        TSet<int32> UniqueVertices;
        if (ManualSpritePtr.IsValid())
        {
            for (int32 TriangleIndex : SelectedTriangles)
            {
                if (TriangleIndex >= 0 && TriangleIndex < ManualSpritePtr->ManualGeometry.Triangles.Num())
                {
                    const FManualSpriteTriangle& Triangle = ManualSpritePtr->ManualGeometry.Triangles[TriangleIndex];
                    UniqueVertices.Add(Triangle.VertexIndex0);
                    UniqueVertices.Add(Triangle.VertexIndex1);
                    UniqueVertices.Add(Triangle.VertexIndex2);
                }
            }
            
            if (UniqueVertices.Num() > 0)
            {
                SelectionText += FString::Printf(TEXT(" (%d unique vertices)"), UniqueVertices.Num());
            }
        }
    }
		
		// Добавляем информацию о возможных действиях
		if (SelectedVertices.Num() >= 3)
		{
			SelectionText += TEXT(" | 3 - Auto Triangulate, 4 - Delete Triangles");
		}
		else if (SelectedVertices.Num() > 0)
		{
			SelectionText += TEXT(" | 4 - Delete Connected Triangles");
		}
		else if (SelectedTriangles.Num() > 0)
		{
			SelectionText += TEXT(" | Del - Delete Triangles");
		}
		
		FCanvasTextItem TextItem(FVector2D(10, CurrentY), 
			FText::FromString(SelectionText),
			GEngine->GetSmallFont(), FLinearColor::FromSRGBColor(FColor::Cyan));
		TextItem.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(TextItem);
		CurrentY += LineHeight;
	}

	// НОВОЕ: Информация о наведении и точности в правом верхнем углу
	const FVector2D ViewportSize = FVector2D(InViewport->GetSizeXY().X, InViewport->GetSizeXY().Y);
	
	if (HoveredVertex != -1)
	{
		const FString HoverText = FString::Printf(TEXT("Hovered: Vertex #%d"), HoveredVertex);
		FCanvasTextItem HoverTextItem(FVector2D(ViewportSize.X - 180, 10), 
			FText::FromString(HoverText), GEngine->GetMediumFont(), FColor::Cyan);
		HoverTextItem.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(HoverTextItem);
		
		// Показываем zoom и точность
		const FString PrecisionText = FString::Printf(TEXT("Zoom: %.0f%% | Precision: Enhanced"), ZoomFactor * 100.0f);
		FCanvasTextItem PrecisionTextItem(FVector2D(ViewportSize.X - 180, 30), 
			FText::FromString(PrecisionText), GEngine->GetSmallFont(), FLinearColor::Gray);
		PrecisionTextItem.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(PrecisionTextItem);
	}
	else
	{
		// Zoom информация когда нет hover
		const FString ZoomText = FString::Printf(TEXT("Zoom: %.0f%%"), ZoomFactor * 100.0f);
		FCanvasTextItem ZoomTextItem(FVector2D(ViewportSize.X - 120, ViewportSize.Y - 30), 
			FText::FromString(ZoomText), GEngine->GetSmallFont(), FLinearColor::Gray);
		ZoomTextItem.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(ZoomTextItem);
	}

	// Дополнительные подсказки в зависимости от режима
	if (Editor.IsValid())
	{
		FString HintText;
		bool bShowHint = false;
		
		switch (Editor->GetEditMode())
		{
		case FManualSpriteEditorToolkit::EEditMode::Select:
			if (SelectedVertices.Num() > 0)
			{
				HintText = TEXT("Ctrl+C to copy, Ctrl+V to paste, Delete to remove");
				bShowHint = true;
			}
			else if (HoveredVertex == -1)
			{
				HintText = TEXT("Click vertex to select, Drag to select multiple");
				bShowHint = true;
			}
			break;
		case FManualSpriteEditorToolkit::EEditMode::AddVertex:
			HintText = TEXT("Click anywhere to add a new vertex");
			bShowHint = true;
			break;
		case FManualSpriteEditorToolkit::EEditMode::Triangle:
			if (SelectedVertices.Num() > 0)
			{
				HintText = FString::Printf(TEXT("Triangle: %d/3 vertices selected | Click vertex again to deselect | Click empty space to clear"), SelectedVertices.Num());
				bShowHint = true;
			}
			else
			{
				HintText = TEXT("Select 3 vertices to create a triangle | Click empty space to clear selection");
				bShowHint = true;
			}
			break;
		case FManualSpriteEditorToolkit::EEditMode::Delete:
			HintText = TEXT("LMB on vertex to delete vertex | LMB on triangle to delete triangle");
			bShowHint = true;
			break;
		case FManualSpriteEditorToolkit::EEditMode::Paste:
			if (Editor->HasCopiedVertices())
			{
				const int32 CopiedCount = Editor->GetPastePreviewVertices().Num();
				HintText = FString::Printf(TEXT("Pasting %d vertices - Click to place, Right-click to cancel"), CopiedCount);
				bShowHint = true;
			}
			break;
		}
		
		if (bShowHint)
		{
			const float HintY = ViewportSize.Y - 50.0f;
			const float HintX = (ViewportSize.X - 400.0f) * 0.5f; // Центрируем
			
			// Фон для подсказки
			FCanvasBoxItem HintBackground(FVector2D(HintX - 5, HintY - 2), FVector2D(410, 20));
			HintBackground.SetColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
			Canvas->DrawItem(HintBackground);
			
			FCanvasTextItem HintTextItem(FVector2D(HintX, HintY), 
				FText::FromString(HintText), GEngine->GetSmallFont(), FLinearColor::White);
			Canvas->DrawItem(HintTextItem);
		}
	}

	// Информация о сетке (если включена)
	if (Editor.IsValid() && Editor->GetGridSettings().bShowGrid)
	{
		const auto& GridSettings = Editor->GetGridSettings();
		const FString GridText = FString::Printf(TEXT("Grid: %.0fpx %s"), 
			GridSettings.GridSize, 
			GridSettings.bSnapToGrid ? TEXT("(Snap ON)") : TEXT("(Snap OFF)"));
		
		FCanvasTextItem GridTextItem(FVector2D(10, ViewportSize.Y - 30), 
			FText::FromString(GridText), GEngine->GetSmallFont(), 
			GridSettings.bSnapToGrid ? FLinearColor::Green : FLinearColor::Gray);
		GridTextItem.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(GridTextItem);
	}

	// Горячие клавиши (показываем в левом нижнем углу, поднимаем выше осей координат)
	{
		const TArray<FString> Shortcuts = {
			TEXT("Q - Select | W - Add | E - Triangle | R - Delete"),
			TEXT("G - Grid | Ctrl+G - Snap | Mouse Wheel - Zoom"),
			TEXT("Ctrl+C/V - Copy/Paste | Ctrl+Z/Y - Undo/Redo"),
			TEXT("3 - Auto Triangulate | 4 - Delete Triangles | V - Validate")
			TEXT("Ctrl+A - Select All")
		 };
    
		float ShortcutY = ViewportSize.Y - 100.0f; // Поднимаем выше чтобы не перекрывать оси
		for (const FString& Shortcut : Shortcuts)
		{
			FCanvasTextItem ShortcutItem(FVector2D(10, ShortcutY), 
			   FText::FromString(Shortcut), GEngine->GetTinyFont(), 
			   FLinearColor(0.7f, 0.7f, 0.7f, 0.8f));
			Canvas->DrawItem(ShortcutItem);
			ShortcutY += 12.0f;
		}
	}
}

// Остальные функции работы с выделением и мышью...
void ManualSpriteEditorViewport::SetSelectedVertices(const TArray<int32>& NewSelection)
{
	SelectedVertices = NewSelection;
	// При выделении вершин снимаем выделение с треугольников
	SelectedTriangles.Empty();
}

void ManualSpriteEditorViewport::SetSelectedTriangles(const TArray<int32>& NewSelection)
{
	SelectedTriangles = NewSelection;
}

void ManualSpriteEditorViewport::SelectAllVertices()
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid() || !ManualSpritePtr.IsValid())
		return;

	SelectedVertices.Empty();
	SelectedTriangles.Empty();
	for (int32 i = 0; i < ManualSpritePtr->ManualGeometry.Vertices.Num(); i++)
	{
		SelectedVertices.Add(i);
	}
}

void ManualSpriteEditorViewport::SelectAllTriangles()
{
	if (!ManualSpritePtr.IsValid())
		return;

	SelectedTriangles.Empty();
	SelectedVertices.Empty();
	for (int32 i = 0; i < ManualSpritePtr->ManualGeometry.Triangles.Num(); i++)
	{
		SelectedTriangles.Add(i);
	}
}

void ManualSpriteEditorViewport::ClearSelection()
{
	SelectedVertices.Empty();
	SelectedTriangles.Empty();
}

void ManualSpriteEditorViewport::ToggleVertexSelection(int32 VertexIndex, bool bAdditive, bool bSubtractive)
{
	if (VertexIndex < 0 || !ManualSpritePtr.IsValid() || VertexIndex >= ManualSpritePtr->ManualGeometry.Vertices.Num())
		return;

	if (bSubtractive)
	{
		// Alt - убираем из выделения
		SelectedVertices.Remove(VertexIndex);
	}
	else if (bAdditive)
	{
		// Ctrl - добавляем к выделению
		if (!SelectedVertices.Contains(VertexIndex))
		{
			SelectedVertices.Add(VertexIndex);
		}
	}
	else
	{
		// Обычный клик - заменяем выделение
		if (SelectedVertices.Contains(VertexIndex) && SelectedVertices.Num() == 1)
		{
			// Если единственная выделенная вершина - снимаем выделение
			SelectedVertices.Empty();
		}
		else
		{
			SelectedVertices.Empty();
			SelectedVertices.Add(VertexIndex);
		}
	}
	
	// При выделении вершин снимаем выделение с треугольников (только если не субтрактивное действие)
	if (!bSubtractive && SelectedVertices.Num() > 0)
	{
		SelectedTriangles.Empty();
	}
	
	// Обновляем связанные треугольники в зависимости от действия
	if (bSubtractive)
	{
		// При удалении вершин убираем неполные треугольники
		RemoveIncompleteTrianglesFromSelection();
	}
	else
	{
		// При добавлении/замене вершин обновляем связанные треугольники
		UpdateConnectedTrianglesSelection();
	}
}

void ManualSpriteEditorViewport::ToggleTriangleSelection(int32 TriangleIndex, bool bAdditive, bool bSubtractive)
{
	if (TriangleIndex < 0 || !ManualSpritePtr.IsValid() || TriangleIndex >= ManualSpritePtr->ManualGeometry.Triangles.Num())
		return;

	if (bSubtractive)
	{
		// Alt - убираем из выделения
		if (SelectedTriangles.Contains(TriangleIndex))
		{
			SelectedTriangles.Remove(TriangleIndex);
			
			// Убираем вершины этого треугольника, если они не принадлежат другим выделенным треугольникам
			RemoveTriangleVerticesFromSelection(TriangleIndex);
		}
	}
	else if (bAdditive)
	{
		// Ctrl - добавляем к выделению
		if (!SelectedTriangles.Contains(TriangleIndex))
		{
			SelectedTriangles.Add(TriangleIndex);
			// Добавляем вершины треугольника к выделению
			AddTriangleVerticestoSelection(TriangleIndex);
		}
	}
	else
	{
		// Обычный клик - заменяем выделение
		if (SelectedTriangles.Contains(TriangleIndex) && SelectedTriangles.Num() == 1)
		{
			SelectedTriangles.Empty();
			SelectedVertices.Empty();
		}
		else
		{
			SelectedTriangles.Empty();
			SelectedVertices.Empty();
			SelectedTriangles.Add(TriangleIndex);
			AddTriangleVerticestoSelection(TriangleIndex);
		}
	}
	
	// При выделении треугольников НЕ вызываем UpdateConnectedVerticesSelection
	// так как мы управляем вершинами вручную через специальные методы
}

void ManualSpriteEditorViewport::SelectTriangleVertices(int32 TriangleIndex)
{
	if (!ManualSpritePtr.IsValid() || TriangleIndex < 0 || TriangleIndex >= ManualSpritePtr->ManualGeometry.Triangles.Num())
		return;

	const FManualSpriteTriangle& Triangle = ManualSpritePtr->ManualGeometry.Triangles[TriangleIndex];
	
	SelectedVertices.Empty();
	SelectedTriangles.Empty();
	
	SelectedVertices.Add(Triangle.VertexIndex0);
	SelectedVertices.Add(Triangle.VertexIndex1);
	SelectedVertices.Add(Triangle.VertexIndex2);
}

void ManualSpriteEditorViewport::SelectVerticesOfSelectedTriangles()
{
	if (!ManualSpritePtr.IsValid() || SelectedTriangles.Num() == 0)
		return;

	TSet<int32> VertexSet;
	
	for (int32 TriangleIndex : SelectedTriangles)
	{
		if (TriangleIndex >= 0 && TriangleIndex < ManualSpritePtr->ManualGeometry.Triangles.Num())
		{
			const FManualSpriteTriangle& Triangle = ManualSpritePtr->ManualGeometry.Triangles[TriangleIndex];
			VertexSet.Add(Triangle.VertexIndex0);
			VertexSet.Add(Triangle.VertexIndex1);
			VertexSet.Add(Triangle.VertexIndex2);
		}
	}
	
	SelectedVertices = VertexSet.Array();
	SelectedTriangles.Empty();
}

void ManualSpriteEditorViewport::UpdateConnectedTrianglesSelection()
{
	if (!ManualSpritePtr.IsValid())
	{
		SelectedTriangles.Empty();
		return;
	}

	// Сохраняем существующие треугольники, выделенные вручную
	TSet<int32> ManuallySelectedTriangles(SelectedTriangles);
	SelectedTriangles.Empty();
	
	if (SelectedVertices.Num() == 0)
	{
		// Если нет выделенных вершин, оставляем только вручную выделенные треугольники
		SelectedTriangles = ManuallySelectedTriangles.Array();
		return;
	}

	const FManualSpriteGeometry& Geometry = ManualSpritePtr->ManualGeometry;
	
	// Находим все треугольники, которые полностью состоят из выделенных вершин
	for (int32 i = 0; i < Geometry.Triangles.Num(); i++)
	{
		const FManualSpriteTriangle& Triangle = Geometry.Triangles[i];
		
		// Проверяем, выделены ли ВСЕ вершины треугольника
		if (SelectedVertices.Contains(Triangle.VertexIndex0) &&
			SelectedVertices.Contains(Triangle.VertexIndex1) &&
			SelectedVertices.Contains(Triangle.VertexIndex2))
		{
			SelectedTriangles.Add(i);
		}
	}
	
	// Добавляем вручную выделенные треугольники
	for (int32 TriangleIndex : ManuallySelectedTriangles)
	{
		if (!SelectedTriangles.Contains(TriangleIndex))
		{
			SelectedTriangles.Add(TriangleIndex);
		}
	}
}

void ManualSpriteEditorViewport::UpdateConnectedVerticesSelection()
{
	if (!ManualSpritePtr.IsValid() || SelectedTriangles.Num() == 0)
	{
		return;
	}

	const FManualSpriteGeometry& Geometry = ManualSpritePtr->ManualGeometry;
	TSet<int32> VertexSet;
	
	// Собираем все вершины из выделенных треугольников
	for (int32 TriangleIndex : SelectedTriangles)
	{
		if (TriangleIndex >= 0 && TriangleIndex < Geometry.Triangles.Num())
		{
			const FManualSpriteTriangle& Triangle = Geometry.Triangles[TriangleIndex];
			VertexSet.Add(Triangle.VertexIndex0);
			VertexSet.Add(Triangle.VertexIndex1);
			VertexSet.Add(Triangle.VertexIndex2);
		}
	}
	
	// Добавляем к уже выделенным вершинам (не заменяем)
	for (int32 VertexIndex : VertexSet)
	{
		if (!SelectedVertices.Contains(VertexIndex))
		{
			SelectedVertices.Add(VertexIndex);
		}
	}
}

void ManualSpriteEditorViewport::UpdatePartialTrianglesSelection()
{
	if (!ManualSpritePtr.IsValid())
		return;

	PartiallySelectedTriangles.Empty();
	const FManualSpriteGeometry& Geometry = ManualSpritePtr->ManualGeometry;
	
	// Находим треугольники, которые частично выделены (не все вершины выделены)
	for (int32 i = 0; i < Geometry.Triangles.Num(); i++)
	{
		if (SelectedTriangles.Contains(i))
			continue; // Пропускаем полностью выделенные
			
		const FManualSpriteTriangle& Triangle = Geometry.Triangles[i];
		
		int32 SelectedVertexCount = 0;
		if (SelectedVertices.Contains(Triangle.VertexIndex0)) SelectedVertexCount++;
		if (SelectedVertices.Contains(Triangle.VertexIndex1)) SelectedVertexCount++;
		if (SelectedVertices.Contains(Triangle.VertexIndex2)) SelectedVertexCount++;
		
		// Если выделена хотя бы одна, но не все вершины
		if (SelectedVertexCount > 0 && SelectedVertexCount < 3)
		{
			PartiallySelectedTriangles.Add(i);
		}
	}
}

void ManualSpriteEditorViewport::AddTriangleVerticestoSelection(int32 TriangleIndex)
{
	if (!ManualSpritePtr.IsValid() || TriangleIndex < 0 || TriangleIndex >= ManualSpritePtr->ManualGeometry.Triangles.Num())
		return;

	const FManualSpriteGeometry& Geometry = ManualSpritePtr->ManualGeometry;
	const FManualSpriteTriangle& Triangle = Geometry.Triangles[TriangleIndex];
	
	// Добавляем вершины треугольника к выделению
	if (!SelectedVertices.Contains(Triangle.VertexIndex0))
		SelectedVertices.Add(Triangle.VertexIndex0);
	if (!SelectedVertices.Contains(Triangle.VertexIndex1))
		SelectedVertices.Add(Triangle.VertexIndex1);
	if (!SelectedVertices.Contains(Triangle.VertexIndex2))
		SelectedVertices.Add(Triangle.VertexIndex2);
}

void ManualSpriteEditorViewport::RemoveTriangleVerticesFromSelection(int32 TriangleIndex)
{
	if (!ManualSpritePtr.IsValid() || TriangleIndex < 0 || TriangleIndex >= ManualSpritePtr->ManualGeometry.Triangles.Num())
		return;

	const FManualSpriteGeometry& Geometry = ManualSpritePtr->ManualGeometry;
	const FManualSpriteTriangle& Triangle = Geometry.Triangles[TriangleIndex];
	
	// Проверяем каждую вершину треугольника
	TArray<int32> VerticestoCheck = {Triangle.VertexIndex0, Triangle.VertexIndex1, Triangle.VertexIndex2};
	
	for (int32 VertexIndex : VerticestoCheck)
	{
		if (!SelectedVertices.Contains(VertexIndex))
			continue;
			
		// Проверяем, принадлежит ли эта вершина другим выделенным треугольникам
		bool bBelongsToOtherSelectedTriangle = false;
		
		for (int32 OtherTriangleIndex : SelectedTriangles)
		{
			if (OtherTriangleIndex == TriangleIndex)
				continue; // Пропускаем сам удаляемый треугольник
				
			if (OtherTriangleIndex >= 0 && OtherTriangleIndex < Geometry.Triangles.Num())
			{
				const FManualSpriteTriangle& OtherTriangle = Geometry.Triangles[OtherTriangleIndex];
				if (OtherTriangle.VertexIndex0 == VertexIndex || 
					OtherTriangle.VertexIndex1 == VertexIndex || 
					OtherTriangle.VertexIndex2 == VertexIndex)
				{
					bBelongsToOtherSelectedTriangle = true;
					break;
				}
			}
		}
		
		// Если вершина не принадлежит другим выделенным треугольникам - убираем её
		if (!bBelongsToOtherSelectedTriangle)
		{
			SelectedVertices.Remove(VertexIndex);
		}
	}
}

bool ManualSpriteEditorViewport::CanDragFromPosition(const FVector2D& ScreenPos, const FViewport* InViewport) const
{
	if (!ManualSpritePtr.IsValid())
		return false;

	const float ClickTolerance = 10.0f;
	
	// Проверяем, кликнули ли мы по выделенной вершине
	const int32 VertexIndex = FindVertexAtScreenPosition(ScreenPos, InViewport, ClickTolerance);
	if (VertexIndex != -1 && SelectedVertices.Contains(VertexIndex))
	{
		return true;
	}
	
	// Проверяем, кликнули ли мы по ПОЛНОСТЬЮ выделенному треугольнику
	const FVector2D WorldPos = ScreenToWorld(ScreenPos, InViewport);
	const int32 TriangleIndex = FindTriangleAtPosition(WorldPos);
	if (TriangleIndex != -1 && SelectedTriangles.Contains(TriangleIndex))
	{
		return true;
	}
	
	// НЕ проверяем частично выделенные треугольники для перетаскивания
	// Это позволит кликать по ним для смены выделения
	
	return false;
}

void ManualSpriteEditorViewport::DeleteSelectedTriangles()
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid() || SelectedTriangles.Num() == 0)
		return;

	// Сортируем индексы в убывающем порядке для корректного удаления
	TArray<int32> SortedIndices = SelectedTriangles;
	SortedIndices.Sort([](const int32& A, const int32& B) {
		return A > B;
	});

	for (int32 TriangleIndex : SortedIndices)
	{
		Editor->RemoveTriangleWithTransaction(TriangleIndex);
	}

	SelectedTriangles.Empty();
}

void ManualSpriteEditorViewport::DeleteSelectedVertices()
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid() || SelectedVertices.Num() == 0)
		return;

	SelectedVertices.Sort([](const int32& A, const int32& B) {
		return A > B;
	});

	for (const int32 VertexIndex : SelectedVertices)
	{
		Editor->RemoveVertexWithTransaction(VertexIndex);
	}

	SelectedVertices.Empty();
}

void ManualSpriteEditorViewport::HandleMouseClick(FViewport* InViewport, FKey Key, EInputEvent Event, FVector2D MousePos)
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid() || !ManualSpritePtr.IsValid())
		return;

	const FVector2D WorldPos = ScreenToWorld(MousePos, InViewport);

	switch (Editor->GetEditMode())
	{
	case FManualSpriteEditorToolkit::EEditMode::AddVertex:
		if (Key == EKeys::LeftMouseButton && Event == IE_Pressed)
		{
			const FVector2D SnappedWorldPos = SnapToGrid(WorldPos);
			const FVector2D UV = CalculateUVFromWorldPosition(SnappedWorldPos, ManualSpritePtr.Get());
			
			Editor->AddVertexWithTransaction(SnappedWorldPos, UV);
			InViewport->Invalidate();
		}
		break;

	case FManualSpriteEditorToolkit::EEditMode::Paste:
		if (Key == EKeys::LeftMouseButton && Event == IE_Pressed)
		{
			const FVector2D SnappedWorldPos = SnapToGrid(WorldPos);
			Editor->PasteVertices(SnappedWorldPos);
			InViewport->Invalidate();
		}
		else if (Key == EKeys::RightMouseButton && Event == IE_Pressed)
		{
			Editor->SetEditMode(FManualSpriteEditorToolkit::EEditMode::Select);
			InViewport->Invalidate();
		}
		break;

case FManualSpriteEditorToolkit::EEditMode::Select:
	if (Key == EKeys::LeftMouseButton)
	{
		if (Event == IE_Pressed)
		{
			const float ClickTolerance = 10.0f;
			const int32 VertexIndex = FindVertexAtScreenPosition(MousePos, InViewport, ClickTolerance);
			const int32 TriangleIndex = FindTriangleAtPosition(WorldPos);
			
			const bool bCtrlPressed = FSlateApplication::Get().GetModifierKeys().IsControlDown();
			const bool bAltPressed = FSlateApplication::Get().GetModifierKeys().IsAltDown();
			
			if (VertexIndex != -1)
			{
				// Клик по вершине
				const bool bWasSelected = SelectedVertices.Contains(VertexIndex);
				
				if (!bCtrlPressed && !bAltPressed)
				{
					if (bWasSelected && SelectedVertices.Num() > 1)
					{
						// Если вершина выделена и есть другие выделенные - начинаем перетаскивание
						BeginVerticesDrag();
					}
					else if (bWasSelected && SelectedVertices.Num() == 1)
					{
						// Если выделена только эта вершина - начинаем перетаскивание
						BeginVerticesDrag();
					}
					else
					{
						// Выделяем только эту вершину
						SelectedVertices.Empty();
						SelectedTriangles.Empty();
						SelectedVertices.Add(VertexIndex);
						UpdateConnectedTrianglesSelection();
						BeginVerticesDrag();
					}
				}
				else
				{
					// Изменяем выделение с модификаторами
					ToggleVertexSelection(VertexIndex, bCtrlPressed, bAltPressed);
					UpdateConnectedTrianglesSelection();
					
					// Начинаем перетаскивание если есть выделенные вершины
					if (SelectedVertices.Num() > 0 && !bAltPressed)
					{
						BeginVerticesDrag();
					}
				}
			}
			else if (TriangleIndex != -1)
			{
				if (bCtrlPressed)
				{
					// Ctrl + клик по треугольнику - добавляем треугольник к выделению
					ToggleTriangleSelection(TriangleIndex, true, false);
					UpdateConnectedVerticesSelection();
				}
				else if (bAltPressed)
				{
					// Alt + клик по треугольнику - убираем треугольник из выделения
					ToggleTriangleSelection(TriangleIndex, false, true);
					UpdateConnectedVerticesSelection();
				}
				else
				{
					// Обычный клик по треугольнику
					const bool bCanDrag = CanDragFromPosition(MousePos, InViewport);
					
					if (!bCanDrag)
					{
						// Выделяем только этот треугольник и его вершины (сбрасываем предыдущее выделение)
						SelectedTriangles.Empty();
						SelectedVertices.Empty();
						SelectedTriangles.Add(TriangleIndex);
						UpdateConnectedVerticesSelection();
					}
					
					// Всегда пытаемся начать перетаскивание
					BeginVerticesDrag();
				}
			}
			else
			{
				// Клик в пустоту - начинаем выделение рамкой
				StartBoxSelection(MousePos, bCtrlPressed);
			}
			
			InViewport->Invalidate();
		}
		else if (Event == IE_Released)
		{
			if (bIsDraggingVertices)
			{
				EndVerticesDrag();
			}
			else if (BoxSelection.bIsActive)
			{
				EndBoxSelection();
				InViewport->Invalidate();
			}
		}
	}
	break;
		
	case FManualSpriteEditorToolkit::EEditMode::Triangle:
		if (Key == EKeys::LeftMouseButton && Event == IE_Pressed)
		{
			const float ClickTolerance = 10.0f;
			const int32 VertexIndex = FindVertexAtScreenPosition(MousePos, InViewport, ClickTolerance);
			
			if (VertexIndex != -1)
			{
				// Клик по вершине
				if (SelectedVertices.Contains(VertexIndex))
				{
					// Повторный клик по уже выделенной вершине - убираем её из выделения
					SelectedVertices.Remove(VertexIndex);
				}
				else
				{
					// Добавляем вершину к выделению
					SelectedVertices.Add(VertexIndex);
				}

				// Проверяем, набрали ли мы 3 вершины для создания треугольника
				if (SelectedVertices.Num() == 3)
				{
					Editor->AddTriangleWithTransaction(SelectedVertices[0], SelectedVertices[1], SelectedVertices[2]);
					SelectedVertices.Empty(); // Очищаем выделение после создания треугольника
					InViewport->Invalidate();
				}
			}
			else
			{
				// Клик в пустоту - сбрасываем выделение вершин
				if (SelectedVertices.Num() > 0)
				{
					SelectedVertices.Empty();
					InViewport->Invalidate();
				}
			}
		}
		break;

	case FManualSpriteEditorToolkit::EEditMode::Delete:
		if (Key == EKeys::LeftMouseButton && Event == IE_Pressed)
		{
			const float ClickTolerance = 10.0f;
			const int32 VertexIndex = FindVertexAtScreenPosition(MousePos, InViewport, ClickTolerance);
			const int32 TriangleIndex = FindTriangleAtPosition(WorldPos);
			
			if (VertexIndex != -1)
			{
				// ЛКМ по вершине - удаляем вершину
				Editor->RemoveVertexWithTransaction(VertexIndex);
				SelectedVertices.Empty();
				SelectedTriangles.Empty();
				InViewport->Invalidate();
			}
			else if (TriangleIndex != -1)
			{
				// ЛКМ по треугольнику - удаляем треугольник
				Editor->RemoveTriangleWithTransaction(TriangleIndex);
				
				// Очищаем выделение и сбрасываем hover для предотвращения скачков
				SelectedTriangles.Empty();
				SelectedVertices.Empty();
				HoveredTriangle = -1;
				HoveredVertex = -1;
				
				InViewport->Invalidate();
			}
		}
		break;
	}
}

int32 ManualSpriteEditorViewport::FindVertexAtPosition(FVector2D WorldPos, float Tolerance) const
{
	if (!ManualSpritePtr.IsValid())
		return -1;

	// ИСПРАВЛЕНИЕ: Преобразуем в экранные координаты для точного поиска
	if (Viewport)
	{
		const FVector2D ScreenPos = WorldToScreen(WorldPos, Viewport);
		const float ScreenTolerance = VertexSelectSize * 0.8f; // Используем размер точки как tolerance
		return FindVertexAtScreenPosition(ScreenPos, Viewport, ScreenTolerance);
	}

	return -1;
}

int32 ManualSpriteEditorViewport::FindVertexAtScreenPosition(FVector2D ScreenPos, const FViewport* InViewport, float ScreenTolerance) const
{
	if (!ManualSpritePtr.IsValid())
		return -1;

	const FManualSpriteGeometry& Geometry = ManualSpritePtr->ManualGeometry;
	
	// Найдем самую близкую точку по экранным координатам
	int32 ClosestVertexIndex = -1;
	float ClosestDistanceSquared = FLT_MAX;
	
	for (int32 i = 0; i < Geometry.Vertices.Num(); i++)
	{
		const FVector2D VertexWorldPos = Geometry.Vertices[i].Position;
		const FVector2D VertexScreenPos = WorldToScreen(VertexWorldPos, InViewport);
		
		const float DistanceSquared = FVector2D::DistSquared(ScreenPos, VertexScreenPos);
		
		// ИСПРАВЛЕНИЕ: Переименовываем переменную чтобы избежать конфликта с членом класса
		const float CurrentVertexSize = GetVertexScreenSize(i);
		const float EffectiveTolerance = FMath::Max(ScreenTolerance, CurrentVertexSize * 0.6f);
		
		// Проверяем, находится ли курсор в пределах области точки
		if (DistanceSquared <= EffectiveTolerance * EffectiveTolerance)
		{
			// Если эта точка ближе чем предыдущая найденная, запоминаем её
			if (DistanceSquared < ClosestDistanceSquared)
			{
				ClosestDistanceSquared = DistanceSquared;
				ClosestVertexIndex = i;
			}
		}
	}
	
	// ИСПРАВЛЕНИЕ: Добавляем return statement
	return ClosestVertexIndex;
}

float ManualSpriteEditorViewport::GetVertexScreenSize(int32 VertexIndex) const
{
	if (SelectedVertices.Contains(VertexIndex) || VertexIndex == HoveredVertex)
	{
		return VertexSelectSize;
	}
	return VertexSize;
}

int32 ManualSpriteEditorViewport::FindTriangleAtPosition(FVector2D WorldPos) const
{
	if (!ManualSpritePtr.IsValid())
		return -1;

	const FManualSpriteGeometry& Geometry = ManualSpritePtr->ManualGeometry;

	for (int32 i = 0; i < Geometry.Triangles.Num(); i++)
	{
		const FManualSpriteTriangle& Triangle = Geometry.Triangles[i];

		if (Triangle.VertexIndex0 >= Geometry.Vertices.Num() ||
			Triangle.VertexIndex1 >= Geometry.Vertices.Num() ||
			Triangle.VertexIndex2 >= Geometry.Vertices.Num())
			continue;

		const FVector2D V0 = Geometry.Vertices[Triangle.VertexIndex0].Position;
		const FVector2D V1 = Geometry.Vertices[Triangle.VertexIndex1].Position;
		const FVector2D V2 = Geometry.Vertices[Triangle.VertexIndex2].Position;

		const FVector2D V0ToV2 = V2 - V0;
		const FVector2D V0ToV1 = V1 - V0;
		const FVector2D V0ToPoint = WorldPos - V0;

		const float dot00 = FVector2D::DotProduct(V0ToV2, V0ToV2);
		const float dot01 = FVector2D::DotProduct(V0ToV2, V0ToV1);
		const float dot02 = FVector2D::DotProduct(V0ToV2, V0ToPoint);
		const float dot11 = FVector2D::DotProduct(V0ToV1, V0ToV1);
		const float dot12 = FVector2D::DotProduct(V0ToV1, V0ToPoint);

		const float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
		const float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
		const float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
		
		if (u >= 0 && v >= 0 && u + v <= 1)
		{
			return i;
		}
	}

	return -1;
}

FVector2D ManualSpriteEditorViewport::CalculateUVFromWorldPosition(const FVector2D& WorldPos, const UManualSprite* ManualSprite) const
{
	if (!ManualSprite)
	{
		return FVector2D(0.5f, 0.5f);
	}

	const FVector2D SpriteSize = GetSpriteSize(ManualSprite);
	
	if (SpriteSize.IsNearlyZero())
	{
		return FVector2D(0.5f, 0.5f);
	}

	const FVector2D HalfSize = SpriteSize * 0.5f;
	
	// Нормализуем позицию относительно размера спрайта
	const FVector2D NormalizedPos = FVector2D(
		WorldPos.X / HalfSize.X,
		WorldPos.Y / HalfSize.Y
	);
	
	// Конвертируем в UV координаты [0,1]
	const FVector2D UV = FVector2D(
		FMath::Clamp((NormalizedPos.X + 1.0f) * 0.5f, 0.0f, 1.0f),
		FMath::Clamp((1.0f - NormalizedPos.Y) * 0.5f, 0.0f, 1.0f)
	);
	
	return UV;
}

FVector2D ManualSpriteEditorViewport::GetSpriteSize(const UManualSprite* ManualSprite) const
{
	if (!ManualSprite)
	{
		return FVector2D(100.0f, 100.0f);
	}

	const FVector2D SourceSize = ManualSprite->GetSourceSize();
	
	if (!SourceSize.IsNearlyZero())
	{
		return SourceSize;
	}

	if (UTexture2D* Texture = ManualSprite->GetSourceTexture())
	{
		return FVector2D(static_cast<float>(Texture->GetSizeX()), static_cast<float>(Texture->GetSizeY()));
	}

	return FVector2D(100.0f, 100.0f);
}

FVector2D ManualSpriteEditorViewport::SnapToGrid(const FVector2D& Position) const
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid())
		return Position;

	const auto& GridSettings = Editor->GetGridSettings();
	if (!GridSettings.bSnapToGrid)
		return Position;

	const float GridSize = GridSettings.GridSize;
	
	return FVector2D(
		FMath::RoundToFloat(Position.X / GridSize) * GridSize,
		FMath::RoundToFloat(Position.Y / GridSize) * GridSize
	);
}

// Реализация методов drag & drop, box selection и paste preview
void ManualSpriteEditorViewport::BeginVerticesDrag()
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid() || !ManualSpritePtr.IsValid())
		return;

	// Если выделены треугольники, переводим их вершины в выделение
	if (SelectedTriangles.Num() > 0 && SelectedVertices.Num() == 0)
	{
		SelectVerticesOfSelectedTriangles();
	}

	// Проверяем, есть ли выделенные вершины для перетаскивания
	if (SelectedVertices.Num() == 0)
		return;

	bIsDraggingVertices = true;
	DragStartPosition = LastMousePosition;
	bDragTransactionStarted = false;

	OriginalVertexPositions.Empty();
	OriginalVertexUVs.Empty();

	const FManualSpriteGeometry& Geometry = ManualSpritePtr->ManualGeometry;
	for (int32 VertexIndex : SelectedVertices)
	{
		if (VertexIndex >= 0 && VertexIndex < Geometry.Vertices.Num())
		{
			OriginalVertexPositions.Add(Geometry.Vertices[VertexIndex].Position);
			OriginalVertexUVs.Add(Geometry.Vertices[VertexIndex].UV);
		}
	}
}

void ManualSpriteEditorViewport::UpdateVerticesDrag(const FVector2D& MouseDelta)
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid() || !bIsDraggingVertices || SelectedVertices.Num() == 0 || !ManualSpritePtr.IsValid())
		return;

	if (OriginalVertexPositions.Num() != SelectedVertices.Num())
		return;

	if (!bDragTransactionStarted)
	{
		GEditor->BeginTransaction(FText::FromString(TEXT("Move Vertices")));
		ManualSpritePtr->Modify();
		bDragTransactionStarted = true;
	}

	const FVector2D WorldDelta = MouseDelta / ZoomFactor;

	for (int32 i = 0; i < SelectedVertices.Num(); i++)
	{
		const int32 VertexIndex = SelectedVertices[i];
		if (VertexIndex >= 0 && VertexIndex < ManualSpritePtr->ManualGeometry.Vertices.Num())
		{
			const FVector2D NewPosition = SnapToGrid(OriginalVertexPositions[i] + WorldDelta);
			const FVector2D NewUV = CalculateUVFromWorldPosition(NewPosition, ManualSpritePtr.Get());
			
			ManualSpritePtr->ManualGeometry.Vertices[VertexIndex].Position = NewPosition;
			ManualSpritePtr->ManualGeometry.Vertices[VertexIndex].UV = NewUV;
		}
	}
	// Инвалидируем кэш пересечений при перетаскивании (для live обновления)
	InvalidateIntersectionsCache();
	(void)ManualSpritePtr->MarkPackageDirty();
}

void ManualSpriteEditorViewport::EndVerticesDrag()
{
	if (bIsDraggingVertices && bDragTransactionStarted)
	{
		GEditor->EndTransaction();
		bDragTransactionStarted = false;
	}
	
	bIsDraggingVertices = false;
	OriginalVertexPositions.Empty();
	OriginalVertexUVs.Empty();
}

void ManualSpriteEditorViewport::CancelVerticesDrag()
{
	if (bIsDraggingVertices && bDragTransactionStarted)
	{
		GEditor->CancelTransaction(0);
		bDragTransactionStarted = false;
	}
	
	bIsDraggingVertices = false;
	OriginalVertexPositions.Empty();
	OriginalVertexUVs.Empty();
}

void ManualSpriteEditorViewport::StartBoxSelection(const FVector2D& StartPos, bool bAdditive)
{
	BoxSelection.bIsActive = true;
	BoxSelection.StartPosition = StartPos;
	BoxSelection.CurrentPosition = StartPos;
	BoxSelection.bIsAdditive = bAdditive;
	
	// Определяем режим: добавление (Ctrl) или удаление (Alt)
	const bool bAltPressed = FSlateApplication::Get().GetModifierKeys().IsAltDown();
	BoxSelection.bIsSubtractive = bAltPressed;
	
	// Если не добавляем и не убираем - очищаем выделение
	if (!bAdditive && !bAltPressed)
	{
		SelectedVertices.Empty();
		SelectedTriangles.Empty();
	}
}

void ManualSpriteEditorViewport::UpdateBoxSelection(const FVector2D& CurrentPos)
{
	if (!BoxSelection.bIsActive)
		return;
		
	BoxSelection.CurrentPosition = CurrentPos;
}

void ManualSpriteEditorViewport::EndBoxSelection()
{
	if (!BoxSelection.bIsActive)
		return;
		
	if (BoxSelection.IsValidSelection())
	{
		TArray<int32> VerticesInBox = GetVerticesInSelectionBox();
		
		if (BoxSelection.bIsSubtractive)
		{
			// Alt - убираем вершины из выделения
			for (int32 VertexIndex : VerticesInBox)
			{
				SelectedVertices.Remove(VertexIndex);
			}
			
			// После удаления вершин, проверяем какие треугольники больше не полностью выделены
			RemoveIncompleteTrianglesFromSelection();
		}
		else if (BoxSelection.bIsAdditive)
		{
			// Ctrl - добавляем к выделению
			for (int32 VertexIndex : VerticesInBox)
			{
				if (!SelectedVertices.Contains(VertexIndex))
				{
					SelectedVertices.Add(VertexIndex);
				}
			}
			
			// Обновляем связанные треугольники
			UpdateConnectedTrianglesSelection();
		}
		else
		{
			// Обычное выделение - заменяем
			SelectedVertices = VerticesInBox;
			// Обновляем связанные треугольники
			UpdateConnectedTrianglesSelection();
		}
	}
	else
	{
		if (!BoxSelection.bIsAdditive && !BoxSelection.bIsSubtractive)
		{
			SelectedVertices.Empty();
			SelectedTriangles.Empty();
		}
	}
	
	BoxSelection.bIsActive = false;
}

void ManualSpriteEditorViewport::RemoveIncompleteTrianglesFromSelection()
{
	if (!ManualSpritePtr.IsValid())
		return;

	const FManualSpriteGeometry& Geometry = ManualSpritePtr->ManualGeometry;
	
	// Проверяем каждый выделенный треугольник
	for (int32 i = SelectedTriangles.Num() - 1; i >= 0; i--)
	{
		const int32 TriangleIndex = SelectedTriangles[i];
		
		if (TriangleIndex >= 0 && TriangleIndex < Geometry.Triangles.Num())
		{
			const FManualSpriteTriangle& Triangle = Geometry.Triangles[TriangleIndex];
			
			// Проверяем, выделены ли все вершины треугольника
			const bool bAllVerticesSelected = 
				SelectedVertices.Contains(Triangle.VertexIndex0) &&
				SelectedVertices.Contains(Triangle.VertexIndex1) &&
				SelectedVertices.Contains(Triangle.VertexIndex2);
			
			// Если не все вершины выделены - убираем треугольник из выделения
			if (!bAllVerticesSelected)
			{
				SelectedTriangles.RemoveAt(i);
			}
		}
	}
}

void ManualSpriteEditorViewport::CancelBoxSelection()
{
	if (BoxSelection.bIsActive)
	{
		BoxSelection.bIsActive = false;
	}
}

void ManualSpriteEditorViewport::DrawBoxSelection(FCanvas* Canvas, const FViewport* InViewport) const
{
	if (!BoxSelection.bIsActive)
		return;

	const FVector2D TopLeft = BoxSelection.GetTopLeft();
	const FVector2D Size = BoxSelection.GetSize();

	FCanvasBoxItem FillItem(TopLeft, Size);
	FillItem.SetColor(FLinearColor(0.0f, 0.5f, 1.0f, 0.1f));
	Canvas->DrawItem(FillItem);

	FCanvasLineItem LineItem;
	LineItem.SetColor(FLinearColor(0.0f, 0.7f, 1.0f, 0.8f));

	const FVector2D BottomRight = BoxSelection.GetBottomRight();

	LineItem.Origin = FVector(TopLeft, 0.0);
	LineItem.EndPos = FVector(BottomRight.X, TopLeft.Y, 0.0);
	Canvas->DrawItem(LineItem);

	LineItem.Origin = FVector(BottomRight.X, TopLeft.Y, 0.0);
	LineItem.EndPos = FVector(BottomRight, 0.0);
	Canvas->DrawItem(LineItem);

	LineItem.Origin = FVector(BottomRight, 0.0);
	LineItem.EndPos = FVector(TopLeft.X, BottomRight.Y, 0.0);
	Canvas->DrawItem(LineItem);

	LineItem.Origin = FVector(TopLeft.X, BottomRight.Y, 0.0);
	LineItem.EndPos = FVector(TopLeft, 0.0);
	Canvas->DrawItem(LineItem);
}

TArray<int32> ManualSpriteEditorViewport::GetVerticesInSelectionBox() const
{
	TArray<int32> VerticesInBox;
	
	if (!BoxSelection.bIsActive || !ManualSpritePtr.IsValid())
		return VerticesInBox;

	const FManualSpriteGeometry& Geometry = ManualSpritePtr->ManualGeometry;
	const FVector2D ViewportSize = FVector2D(LastViewportSize.X, LastViewportSize.Y);
	const FVector2D ViewportCenter = ViewportSize * 0.5f;
	
	for (int32 i = 0; i < Geometry.Vertices.Num(); i++)
	{
		const FVector2D VertexWorldPos = Geometry.Vertices[i].Position;
		const FVector2D VertexScreenPos = ViewportCenter + ViewOffset * ZoomFactor + VertexWorldPos * ZoomFactor;
		
		if (BoxSelection.ContainsPoint(VertexScreenPos))
		{
			VerticesInBox.Add(i);
		}
	}
	
	return VerticesInBox;
}

void ManualSpriteEditorViewport::DrawPastePreview(FCanvas* Canvas, const FViewport* InViewport) const
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid())
		return;

	const TArray<FVector2D> PreviewVertices = Editor->GetPastePreviewVertices();
	
	if (PreviewVertices.Num() == 0)
		return;

	const FLinearColor PreviewColor = FLinearColor(0.0f, 0.5f, 1.0f, 0.6f);
	const float PreviewVertexSize = VertexSize * 1.2f;

	for (int32 i = 0; i < PreviewVertices.Num(); i++)
	{
		const FVector2D WorldPos = PreviewVertices[i];
		const FVector2D ScreenPos = WorldToScreen(WorldPos, InViewport);

		FCanvasBoxItem BoxItem(
			ScreenPos - FVector2D(PreviewVertexSize * 0.5f),
			FVector2D(PreviewVertexSize)
		);
		BoxItem.SetColor(PreviewColor);
		Canvas->DrawItem(BoxItem);

		const FString VertexNumber = FString::Printf(TEXT("%d"), i);
		FCanvasTextItem TextItem(
			ScreenPos + FVector2D(PreviewVertexSize * 0.5f, -PreviewVertexSize * 0.5f),
			FText::FromString(VertexNumber),
			GEngine->GetSmallFont(),
			PreviewColor
		);
		Canvas->DrawItem(TextItem);
	}

	if (PreviewVertices.Num() > 1)
	{
		for (int32 i = 0; i < PreviewVertices.Num() - 1; i++)
		{
			const FVector2D StartPos = WorldToScreen(PreviewVertices[i], InViewport);
			const FVector2D EndPos = WorldToScreen(PreviewVertices[i + 1], InViewport);
			
			FCanvasLineItem LineItem(StartPos, EndPos);
			LineItem.SetColor(FLinearColor(0.0f, 0.3f, 0.8f, 0.4f));
			Canvas->DrawItem(LineItem);
		}
	}

	const FString HintText = FString::Printf(TEXT("Paste Preview (%d vertices) - Click to paste, Right-click to cancel"), PreviewVertices.Num());
	FCanvasTextItem HintTextItem(
		FVector2D(10, 10),
		FText::FromString(HintText),
		GEngine->GetMediumFont(),
		FLinearColor::Yellow
	);
	Canvas->DrawItem(HintTextItem);
}

bool ManualSpriteEditorViewport::DoSegmentsIntersect(const FVector2D& A1, const FVector2D& A2, 
                                                   const FVector2D& B1, const FVector2D& B2) const
{
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

TArray<ManualSpriteEditorViewport::FTriangleEdge> ManualSpriteEditorViewport::GetAllTriangleEdges() const
{
	TArray<FTriangleEdge> Edges;
	
	if (!ManualSpritePtr.IsValid())
		return Edges;
		
	const FManualSpriteGeometry& Geometry = ManualSpritePtr->ManualGeometry;
	
	for (int32 TriIndex = 0; TriIndex < Geometry.Triangles.Num(); TriIndex++)
	{
		const FManualSpriteTriangle& Triangle = Geometry.Triangles[TriIndex];
		
		if (Triangle.VertexIndex0 >= Geometry.Vertices.Num() ||
			Triangle.VertexIndex1 >= Geometry.Vertices.Num() ||
			Triangle.VertexIndex2 >= Geometry.Vertices.Num())
		{
			continue;
		}
		
		Edges.Add(FTriangleEdge(Triangle.VertexIndex0, Triangle.VertexIndex1, TriIndex));
		Edges.Add(FTriangleEdge(Triangle.VertexIndex1, Triangle.VertexIndex2, TriIndex));
		Edges.Add(FTriangleEdge(Triangle.VertexIndex2, Triangle.VertexIndex0, TriIndex));
	}
	
	return Edges;
}

void ManualSpriteEditorViewport::UpdateIntersectingEdges() const
{
	if (bIntersectionsCacheValid)
		return;
		
	CachedIntersectingEdges.Empty();
	
	if (!ManualSpritePtr.IsValid())
	{
		bIntersectionsCacheValid = true;
		return;
	}
	
	const FManualSpriteGeometry& Geometry = ManualSpritePtr->ManualGeometry;
	const TArray<FTriangleEdge> AllEdges = GetAllTriangleEdges();
	
	for (int32 i = 0; i < AllEdges.Num(); i++)
	{
		for (int32 j = i + 1; j < AllEdges.Num(); j++)
		{
			const FTriangleEdge& EdgeA = AllEdges[i];
			const FTriangleEdge& EdgeB = AllEdges[j];
			
			if (EdgeA.TriangleIndex == EdgeB.TriangleIndex)
				continue;
				
			if (EdgeA.VertexA == EdgeB.VertexA || EdgeA.VertexA == EdgeB.VertexB ||
				EdgeA.VertexB == EdgeB.VertexA || EdgeA.VertexB == EdgeB.VertexB)
			{
				continue;
			}
			
			const FVector2D PosA1 = Geometry.Vertices[EdgeA.VertexA].Position;
			const FVector2D PosA2 = Geometry.Vertices[EdgeA.VertexB].Position;
			const FVector2D PosB1 = Geometry.Vertices[EdgeB.VertexA].Position;
			const FVector2D PosB2 = Geometry.Vertices[EdgeB.VertexB].Position;
			
			if (DoSegmentsIntersect(PosA1, PosA2, PosB1, PosB2))
			{
				CachedIntersectingEdges.Add(TPair<int32, int32>(EdgeA.VertexA, EdgeA.VertexB));
				CachedIntersectingEdges.Add(TPair<int32, int32>(EdgeB.VertexA, EdgeB.VertexB));
			}
		}
	}
	
	bIntersectionsCacheValid = true;
	
	if (CachedIntersectingEdges.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("🔴 Found %d intersecting edges!"), CachedIntersectingEdges.Num());
	}
}

bool ManualSpriteEditorViewport::IsEdgeIntersecting(int32 VertexA, int32 VertexB) const
{
	UpdateIntersectingEdges();
	
	const int32 MinIndex = FMath::Min(VertexA, VertexB);
	const int32 MaxIndex = FMath::Max(VertexA, VertexB);
	
	return CachedIntersectingEdges.Contains(TPair<int32, int32>(MinIndex, MaxIndex));
}

void ManualSpriteEditorViewport::DrawTriangleEdge(FCanvas* Canvas, const FVector2D& StartPos, const FVector2D& EndPos, 
												 int32 VertexA, int32 VertexB, const FLinearColor& EdgeColorParam, float Thickness) const
{
	FLinearColor EdgeColor = EdgeColorParam;
	float LineThickness = Thickness;
	
	if (IsEdgeIntersecting(VertexA, VertexB))
	{
		const float Time = FApp::GetCurrentTime();
		const float PulseAlpha = 0.5f + 0.5f * FMath::Sin(Time * 8.0f);
		
		EdgeColor = FLinearColor(1.0f, 0.0f, 0.0f, FMath::Lerp(0.7f, 1.0f, PulseAlpha));
		LineThickness = FMath::Max(Thickness, 2.5f);
	}
	
	FCanvasLineItem LineItem(StartPos, EndPos);
	LineItem.SetColor(EdgeColor);
	LineItem.LineThickness = LineThickness;
	Canvas->DrawItem(LineItem);
	
	if (IsEdgeIntersecting(VertexA, VertexB))
	{
		const FVector2D MidPoint = (StartPos + EndPos) * 0.5f;
		const float SymbolSize = 8.0f;
		
		FCanvasLineItem CrossH(
			MidPoint - FVector2D(SymbolSize * 0.5f, 0), 
			MidPoint + FVector2D(SymbolSize * 0.5f, 0)
		);
		CrossH.SetColor(FLinearColor::Yellow);
		CrossH.LineThickness = 2.0f;
		Canvas->DrawItem(CrossH);
		
		FCanvasLineItem CrossV(
			MidPoint - FVector2D(0, SymbolSize * 0.5f), 
			MidPoint + FVector2D(0, SymbolSize * 0.5f)
		);
		CrossV.SetColor(FLinearColor::Yellow);
		CrossV.LineThickness = 2.0f;
		Canvas->DrawItem(CrossV);
	}
}