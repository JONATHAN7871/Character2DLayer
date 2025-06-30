#include "ManualSpriteEditorViewport.h"
#include "ManualSpriteEditorToolkit.h"
#include "ManualSpriteTransactions.h"
#include "ManualSprite.h"
#include "Widgets/SViewport.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Canvas.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SceneViewport.h"
#include "ScopedTransaction.h"

// Константы для рендеринга
const float FManualSpriteEditorViewportClient::VertexSize = 8.0f;
const float FManualSpriteEditorViewportClient::VertexSelectSize = 12.0f;
const FLinearColor FManualSpriteEditorViewportClient::VertexColor = FLinearColor::Red;
const FLinearColor FManualSpriteEditorViewportClient::SelectedVertexColor = FLinearColor::Yellow;
const FLinearColor FManualSpriteEditorViewportClient::TriangleColor = FLinearColor(0.0f, 1.0f, 0.0f, 0.3f);
const FLinearColor FManualSpriteEditorViewportClient::SelectedTriangleColor = FLinearColor(1.0f, 1.0f, 0.0f, 0.5f);

void SManualSpriteEditorViewport::Construct(const FArguments& InArgs)
{
	ManualSpriteEditorPtr = InArgs._ManualSpriteEditor;

	// Создаём viewport client
	ViewportClient = MakeShareable(new FManualSpriteEditorViewportClient(ManualSpriteEditorPtr));

	// Создаём viewport widget
	ChildSlot
	[
		SAssignNew(ViewportWidget, SViewport)
		.EnableGammaCorrection(false)
		.IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute())
		.ShowEffectWhenDisabled(false)
	];

	// Создаём scene viewport
	SceneViewport = MakeShareable(new FSceneViewport(ViewportClient.Get(), ViewportWidget));
	ViewportWidget->SetViewportInterface(SceneViewport.ToSharedRef());
}

void SManualSpriteEditorViewport::RefreshViewport()
{
	if (SceneViewport.IsValid())
	{
		SceneViewport->Invalidate();
	}
}

// Реализация FManualSpriteEditorViewportClient
FManualSpriteEditorViewportClient::FManualSpriteEditorViewportClient(TWeakPtr<FManualSpriteEditorToolkit> InManualSpriteEditor)
	: ManualSpriteEditorPtr(InManualSpriteEditor)
	, ZoomFactor(1.0f)
	, ViewOffset(FVector2D::ZeroVector)
	, bIsPanning(false)
	, LastMousePosition(FVector2D::ZeroVector)
	, HoveredVertex(-1)
	, HoveredTriangle(-1)
	, bIsDraggingVertices(false)
	, DragStartPosition(FVector2D::ZeroVector)
	, bDragTransactionStarted(false)
	, LastViewportSize(FIntPoint::ZeroValue)
{
}

void FManualSpriteEditorViewportClient::Draw(FViewport* Viewport, FCanvas* Canvas)
{
	// Сохраняем размер viewport для использования в других функциях
	LastViewportSize = Viewport->GetSizeXY();

	// Очищаем фон
	Canvas->Clear(FLinearColor(0.2f, 0.2f, 0.2f, 1.0f));

	// Рисуем сетку (первой, чтобы была позади всего)
	DrawGrid(Canvas, Viewport);

	// Рисуем спрайт
	DrawSprite(Canvas, Viewport);

	// Рисуем отладочную геометрию
	DrawDebugGeometry(Canvas, Viewport);
	
	// Рисуем рамку выделения поверх всего
	DrawBoxSelection(Canvas, Viewport);
}

bool FManualSpriteEditorViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
	const FKey Key = EventArgs.Key;
	const EInputEvent Event = EventArgs.Event;
	FViewport* Viewport = EventArgs.Viewport;

	// Проверяем, обрабатываются ли команды в редакторе
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (Editor.IsValid())
	{
		// Передаем ввод в систему команд редактора
		if (const TSharedPtr<FUICommandList> CommandList = Editor->GetCommandList())
		{
			if (CommandList->ProcessCommandBindings(Key, FSlateApplication::Get().GetModifierKeys(), Event == IE_Repeat))
			{
				return true;
			}
		}
	}

	// Обработка клавиши Escape для отмены операций
	if (Key == EKeys::Escape && Event == IE_Pressed)
	{
		bool bHandled = false;
		
		// Отменяем выделение рамкой
		if (BoxSelection.bIsActive)
		{
			CancelBoxSelection();
			Viewport->Invalidate();
			bHandled = true;
		}
		
		// Отменяем перетаскивание вершин
		if (bIsDraggingVertices)
		{
			CancelVerticesDrag();
			Viewport->Invalidate();
			bHandled = true;
		}
		
		// Если ничего не отменяли, очищаем выделение
		if (!bHandled && SelectedVertices.Num() > 0)
		{
			SelectedVertices.Empty();
			Viewport->Invalidate();
			bHandled = true;
		}
		
		if (bHandled)
		{
			UE_LOG(LogTemp, Log, TEXT("Escape pressed - cancelled current operation"));
			return true;
		}
	}

	// Обработка кликов мыши
	if (Key == EKeys::LeftMouseButton || Key == EKeys::RightMouseButton)
	{
		const FVector2D ViewportMousePos = FVector2D(Viewport->GetMouseX(), Viewport->GetMouseY());
		HandleMouseClick(Viewport, Key, Event, ViewportMousePos);
		return true;
	}

	// Обработка масштабирования колесом мыши
	if (Key == EKeys::MouseScrollUp)
	{
		SetZoom(FMath::Clamp(ZoomFactor * 1.1f, 0.1f, 10.0f));
		Viewport->Invalidate();
		return true;
	}
	if (Key == EKeys::MouseScrollDown)
	{
		SetZoom(FMath::Clamp(ZoomFactor * 0.9f, 0.1f, 10.0f));
		Viewport->Invalidate();
		return true;
	}

	// Обработка панорамирования средней кнопкой мыши
	if (Key == EKeys::MiddleMouseButton)
	{
		if (Event == IE_Pressed)
		{
			bIsPanning = true;
			LastMousePosition = FVector2D(Viewport->GetMouseX(), Viewport->GetMouseY());
		}
		else if (Event == IE_Released)
		{
			bIsPanning = false;
		}
		return true;
	}

	return false;
}

bool FManualSpriteEditorViewportClient::InputAxis(FViewport* Viewport, FInputDeviceId DeviceId, FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	return false;
}

void FManualSpriteEditorViewportClient::MouseMove(FViewport* Viewport, int32 X, int32 Y)
{
	const FVector2D CurrentMousePos(X, Y);
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();

	// Обработка панорамирования
	if (bIsPanning)
	{
		const FVector2D Delta = (CurrentMousePos - LastMousePosition) / ZoomFactor;
		SetViewOffset(ViewOffset + Delta);
		Viewport->Invalidate();
	}
	// Обработка выделения рамкой
	else if (BoxSelection.bIsActive)
	{
		UpdateBoxSelection(CurrentMousePos);
		Viewport->Invalidate();
	}
	// Обработка множественного перетаскивания вершин
	else if (bIsDraggingVertices)
	{
		const FVector2D MouseDelta = CurrentMousePos - DragStartPosition;
		UpdateVerticesDrag(MouseDelta);
		Viewport->Invalidate();
	}
	// Обработка превью вставки
	else if (Editor.IsValid() && Editor->GetEditMode() == FManualSpriteEditorToolkit::EEditMode::Paste)
	{
		const FVector2D WorldPos = ScreenToWorld(CurrentMousePos, Viewport);
		const FVector2D SnappedWorldPos = SnapToGrid(WorldPos);
		Editor->SetPastePreviewPosition(SnappedWorldPos);
		Viewport->Invalidate();
	}

	LastMousePosition = CurrentMousePos;

	// Обновление hover состояния (только если не перетаскиваем, не выделяем рамкой и не в режиме вставки)
	if (!bIsDraggingVertices && !BoxSelection.bIsActive && 
		(!Editor.IsValid() || Editor->GetEditMode() != FManualSpriteEditorToolkit::EEditMode::Paste))
	{
		const FVector2D WorldPos = ScreenToWorld(CurrentMousePos, Viewport);
		HoveredVertex = FindVertexAtPosition(WorldPos);
		HoveredTriangle = FindTriangleAtPosition(WorldPos);
		
		Viewport->Invalidate();
	}
}

void FManualSpriteEditorViewportClient::CapturedMouseMove(FViewport* Viewport, int32 InMouseX, int32 InMouseY)
{
	MouseMove(Viewport, InMouseX, InMouseY);
}

// ========== МНОЖЕСТВЕННОЕ ПЕРЕТАСКИВАНИЕ ВЕРШИН ==========

void FManualSpriteEditorViewportClient::BeginVerticesDrag()
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid() || SelectedVertices.Num() == 0)
		return;

	UManualSprite* ManualSprite = Editor->GetManualSprite();
	if (!ManualSprite)
		return;

	bIsDraggingVertices = true;
	DragStartPosition = LastMousePosition;
	bDragTransactionStarted = false;

	// Сохраняем исходные позиции и UV всех выделенных вершин
	OriginalVertexPositions.Empty();
	OriginalVertexUVs.Empty();

	const FManualSpriteGeometry& Geometry = ManualSprite->ManualGeometry;
	for (int32 VertexIndex : SelectedVertices)
	{
		if (VertexIndex >= 0 && VertexIndex < Geometry.Vertices.Num())
		{
			OriginalVertexPositions.Add(Geometry.Vertices[VertexIndex].Position);
			OriginalVertexUVs.Add(Geometry.Vertices[VertexIndex].UV);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Started dragging %d vertices"), SelectedVertices.Num());
}

void FManualSpriteEditorViewportClient::UpdateVerticesDrag(const FVector2D& MouseDelta)
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid() || !bIsDraggingVertices || SelectedVertices.Num() == 0)
		return;

	UManualSprite* ManualSprite = Editor->GetManualSprite();
	if (!ManualSprite || OriginalVertexPositions.Num() != SelectedVertices.Num())
		return;

	// Начинаем транзакцию только при первом движении
	if (!bDragTransactionStarted)
	{
		GEditor->BeginTransaction(FText::FromString(TEXT("Move Vertices")));
		ManualSprite->Modify();
		bDragTransactionStarted = true;
	}

	// Вычисляем дельту в мировых координатах
	const FVector2D WorldDelta = MouseDelta / ZoomFactor;

	// Обновляем позиции всех выделенных вершин
	for (int32 i = 0; i < SelectedVertices.Num(); i++)
	{
		const int32 VertexIndex = SelectedVertices[i];
		if (VertexIndex >= 0 && VertexIndex < ManualSprite->ManualGeometry.Vertices.Num())
		{
			// Вычисляем новую позицию с привязкой к сетке
			const FVector2D NewPosition = SnapToGrid(OriginalVertexPositions[i] + WorldDelta);
			
			// Пересчитываем UV координаты для новой позиции
			const FVector2D NewUV = CalculateUVFromWorldPosition(NewPosition, ManualSprite);
			
			// Обновляем вершину
			ManualSprite->ManualGeometry.Vertices[VertexIndex].Position = NewPosition;
			ManualSprite->ManualGeometry.Vertices[VertexIndex].UV = NewUV;
		}
	}
	
	// Помечаем объект как изменённый
	ManualSprite->MarkPackageDirty();
}

void FManualSpriteEditorViewportClient::EndVerticesDrag()
{
	if (bIsDraggingVertices && bDragTransactionStarted)
	{
		GEditor->EndTransaction();
		bDragTransactionStarted = false;
		
		UE_LOG(LogTemp, Log, TEXT("Finished dragging %d vertices"), SelectedVertices.Num());
	}
	
	bIsDraggingVertices = false;
	OriginalVertexPositions.Empty();
	OriginalVertexUVs.Empty();
}

void FManualSpriteEditorViewportClient::CancelVerticesDrag()
{
	if (bIsDraggingVertices && bDragTransactionStarted)
	{
		GEditor->CancelTransaction(0);
		bDragTransactionStarted = false;
		
		UE_LOG(LogTemp, Log, TEXT("Cancelled dragging %d vertices"), SelectedVertices.Num());
	}
	
	bIsDraggingVertices = false;
	OriginalVertexPositions.Empty();
	OriginalVertexUVs.Empty();
}

// ========== ОБРАБОТКА КЛИКОВ МЫШИ ==========

void FManualSpriteEditorViewportClient::HandleMouseClick(FViewport* Viewport, FKey Key, EInputEvent Event, FVector2D MousePos)
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid())
		return;

	UManualSprite* ManualSprite = Editor->GetManualSprite();
	if (!ManualSprite)
		return;

	const FVector2D WorldPos = ScreenToWorld(MousePos, Viewport);

	// Обработка в зависимости от режима редактирования
	switch (Editor->GetEditMode())
	{
	case FManualSpriteEditorToolkit::EEditMode::AddVertex:
		if (Key == EKeys::LeftMouseButton && Event == IE_Pressed)
		{
			const FVector2D SnappedWorldPos = SnapToGrid(WorldPos);
			const FVector2D UV = CalculateUVFromWorldPosition(SnappedWorldPos, ManualSprite);
			
			Editor->AddVertexWithTransaction(SnappedWorldPos, UV);
			Viewport->Invalidate();
		}
		break;

	case FManualSpriteEditorToolkit::EEditMode::Paste:
		if (Key == EKeys::LeftMouseButton && Event == IE_Pressed)
		{
			// Вставляем в позицию клика
			const FVector2D SnappedWorldPos = SnapToGrid(WorldPos);
			Editor->PasteVertices(SnappedWorldPos);
			Viewport->Invalidate();
		}
		else if (Key == EKeys::RightMouseButton && Event == IE_Pressed)
		{
			// Отменяем режим вставки правой кнопкой
			Editor->SetEditMode(FManualSpriteEditorToolkit::EEditMode::Select);
			Viewport->Invalidate();
		}
		break;

	case FManualSpriteEditorToolkit::EEditMode::Select:
		if (Key == EKeys::LeftMouseButton)
		{
			if (Event == IE_Pressed)
			{
				const int32 VertexIndex = FindVertexAtPosition(WorldPos);
				const bool bCtrlPressed = FSlateApplication::Get().GetModifierKeys().IsControlDown();
				
				if (VertexIndex != -1)
				{
					// Клик по вершине - обработка выделения
					if (!bCtrlPressed)
					{
						// Если вершина уже выделена и Ctrl не нажат, не очищаем выделение
						// Это позволяет перетаскивать группу вершин
						if (!SelectedVertices.Contains(VertexIndex))
						{
							SelectedVertices.Empty();
							SelectedVertices.Add(VertexIndex);
						}
					}
					else
					{
						// Ctrl нажат - переключаем выделение вершины
						if (SelectedVertices.Contains(VertexIndex))
						{
							SelectedVertices.Remove(VertexIndex);
						}
						else
						{
							SelectedVertices.Add(VertexIndex);
						}
					}
					
					// Начинаем drag операцию если есть выделенные вершины
					if (SelectedVertices.Num() > 0)
					{
						BeginVerticesDrag();
					}
				}
				else
				{
					// Клик в пустое место - начинаем выделение рамкой
					StartBoxSelection(MousePos, bCtrlPressed);
				}
				
				Viewport->Invalidate();
			}
			else if (Event == IE_Released)
			{
				// Заканчиваем операции
				if (bIsDraggingVertices)
				{
					EndVerticesDrag();
				}
				else if (BoxSelection.bIsActive)
				{
					EndBoxSelection();
					Viewport->Invalidate();
				}
			}
		}
		break;

	case FManualSpriteEditorToolkit::EEditMode::Triangle:
		if (Key == EKeys::LeftMouseButton && Event == IE_Pressed)
		{
			if (const int32 VertexIndex = FindVertexAtPosition(WorldPos); VertexIndex != -1)
			{
				if (!SelectedVertices.Contains(VertexIndex))
				{
					SelectedVertices.Add(VertexIndex);
				}

				if (SelectedVertices.Num() == 3)
				{
					Editor->AddTriangleWithTransaction(SelectedVertices[0], SelectedVertices[1], SelectedVertices[2]);
					SelectedVertices.Empty();
					Viewport->Invalidate();
				}
			}
		}
		break;

	case FManualSpriteEditorToolkit::EEditMode::Delete:
		if (Key == EKeys::LeftMouseButton && Event == IE_Pressed)
		{
			if (const int32 VertexIndex = FindVertexAtPosition(WorldPos); VertexIndex != -1)
			{
				Editor->RemoveVertexWithTransaction(VertexIndex);
				SelectedVertices.Empty();
				Viewport->Invalidate();
			}
			else if (const int32 TriangleIndex = FindTriangleAtPosition(WorldPos); TriangleIndex != -1)
			{
				Editor->RemoveTriangleWithTransaction(TriangleIndex);
				Viewport->Invalidate();
			}
		}
		break;
	}
}

// ========== ФУНКЦИИ ДЛЯ РАБОТЫ С ВЫДЕЛЕНИЕМ ==========

void FManualSpriteEditorViewportClient::SetSelectedVertices(const TArray<int32>& NewSelection)
{
	SelectedVertices = NewSelection;
}

void FManualSpriteEditorViewportClient::SelectAllVertices()
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid())
		return;

	const UManualSprite* ManualSprite = Editor->GetManualSprite();
	if (!ManualSprite)
		return;

	SelectedVertices.Empty();
	for (int32 i = 0; i < ManualSprite->ManualGeometry.Vertices.Num(); i++)
	{
		SelectedVertices.Add(i);
	}
}

void FManualSpriteEditorViewportClient::ClearSelection()
{
	SelectedVertices.Empty();
}

void FManualSpriteEditorViewportClient::DeleteSelectedVertices()
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid() || SelectedVertices.Num() == 0)
		return;

	// Сортируем индексы в убывающем порядке для правильного удаления
	SelectedVertices.Sort([](const int32& A, const int32& B) {
		return A > B;
	});

	// Удаляем вершины с помощью транзакций
	for (const int32 VertexIndex : SelectedVertices)
	{
		Editor->RemoveVertexWithTransaction(VertexIndex);
	}

	SelectedVertices.Empty();
}

// ========== ФУНКЦИИ ДЛЯ ВЫДЕЛЕНИЯ РАМКОЙ ==========

void FManualSpriteEditorViewportClient::StartBoxSelection(const FVector2D& StartPos, bool bAdditive)
{
	BoxSelection.bIsActive = true;
	BoxSelection.StartPosition = StartPos;
	BoxSelection.CurrentPosition = StartPos;
	BoxSelection.bIsAdditive = bAdditive;
	
	// Если не добавляем к существующему выделению, очищаем текущее
	if (!bAdditive)
	{
		SelectedVertices.Empty();
	}
	
	UE_LOG(LogTemp, Log, TEXT("Started box selection at (%.1f, %.1f), additive: %s"), 
		   StartPos.X, StartPos.Y, bAdditive ? TEXT("true") : TEXT("false"));
}

void FManualSpriteEditorViewportClient::UpdateBoxSelection(const FVector2D& CurrentPos)
{
	if (!BoxSelection.bIsActive)
		return;
		
	BoxSelection.CurrentPosition = CurrentPos;
}

void FManualSpriteEditorViewportClient::EndBoxSelection()
{
	if (!BoxSelection.bIsActive)
		return;
		
	// Проверяем, достаточно ли большая рамка для валидного выделения
	if (BoxSelection.IsValidSelection())
	{
		// Получаем все вершины в рамке выделения
		TArray<int32> VerticesInBox = GetVerticesInSelectionBox();
		
		if (BoxSelection.bIsAdditive)
		{
			// Добавляем к существующему выделению
			for (int32 VertexIndex : VerticesInBox)
			{
				if (!SelectedVertices.Contains(VertexIndex))
				{
					SelectedVertices.Add(VertexIndex);
				}
			}
		}
		else
		{
			// Заменяем текущее выделение
			SelectedVertices = VerticesInBox;
		}
		
		UE_LOG(LogTemp, Log, TEXT("Box selection completed. Selected %d vertices"), SelectedVertices.Num());
	}
	else
	{
		// Рамка слишком маленькая - очищаем выделение если не добавляем
		if (!BoxSelection.bIsAdditive)
		{
			SelectedVertices.Empty();
		}
		
		UE_LOG(LogTemp, Log, TEXT("Box selection too small, selection cleared"));
	}
	
	BoxSelection.bIsActive = false;
}

void FManualSpriteEditorViewportClient::CancelBoxSelection()
{
	if (BoxSelection.bIsActive)
	{
		BoxSelection.bIsActive = false;
		UE_LOG(LogTemp, Log, TEXT("Box selection cancelled"));
	}
}

// ========== ОТРИСОВКА И ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========

void FManualSpriteEditorViewportClient::DrawBoxSelection(FCanvas* Canvas, const FViewport* Viewport) const
{
	if (!BoxSelection.bIsActive)
		return;

	const FVector2D TopLeft = BoxSelection.GetTopLeft();
	const FVector2D Size = BoxSelection.GetSize();

	// Рисуем заливку рамки
	FCanvasBoxItem FillItem(TopLeft, Size);
	FillItem.SetColor(FLinearColor(0.0f, 0.5f, 1.0f, 0.1f)); // Полупрозрачный синий
	Canvas->DrawItem(FillItem);

	// Рисуем границы рамки
	FCanvasLineItem LineItem;
	LineItem.SetColor(FLinearColor(0.0f, 0.7f, 1.0f, 0.8f)); // Более яркий синий для границ

	const FVector2D BottomRight = BoxSelection.GetBottomRight();

	// Верхняя линия
	LineItem.Origin = FVector(TopLeft, 0.0);
	LineItem.EndPos = FVector(BottomRight.X, TopLeft.Y, 0.0);
	Canvas->DrawItem(LineItem);

	// Правая линия
	LineItem.Origin = FVector(BottomRight.X, TopLeft.Y, 0.0);
	LineItem.EndPos = FVector(BottomRight, 0.0);
	Canvas->DrawItem(LineItem);

	// Нижняя линия
	LineItem.Origin = FVector(BottomRight, 0.0);
	LineItem.EndPos = FVector(TopLeft.X, BottomRight.Y, 0.0);
	Canvas->DrawItem(LineItem);

	// Левая линия
	LineItem.Origin = FVector(TopLeft.X, BottomRight.Y, 0.0);
	LineItem.EndPos = FVector(TopLeft, 0.0);
	Canvas->DrawItem(LineItem);
}

TArray<int32> FManualSpriteEditorViewportClient::GetVerticesInSelectionBox() const
{
	TArray<int32> VerticesInBox;
	
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid() || !BoxSelection.bIsActive)
		return VerticesInBox;

	const UManualSprite* ManualSprite = Editor->GetManualSprite();
	if (!ManualSprite)
		return VerticesInBox;

	const FManualSpriteGeometry& Geometry = ManualSprite->ManualGeometry;
	
	// Используем простой подход без фиктивного viewport
	// Применяем ту же трансформацию, что и в WorldToScreen
	const FVector2D ViewportSize = FVector2D(LastViewportSize.X, LastViewportSize.Y);
	const FVector2D ViewportCenter = ViewportSize * 0.5f;
	
	// Проверяем каждую вершину на попадание в рамку
	for (int32 i = 0; i < Geometry.Vertices.Num(); i++)
	{
		// Конвертируем позицию вершины в экранные координаты вручную
		const FVector2D VertexWorldPos = Geometry.Vertices[i].Position;
		const FVector2D VertexScreenPos = ViewportCenter + ViewOffset * ZoomFactor + VertexWorldPos * ZoomFactor;
		
		// Проверяем, попадает ли вершина в рамку выделения
		if (BoxSelection.ContainsPoint(VertexScreenPos))
		{
			VerticesInBox.Add(i);
		}
	}
	
	return VerticesInBox;
}

int32 FManualSpriteEditorViewportClient::FindVertexAtPosition(FVector2D WorldPos, float Tolerance) const
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid())
		return -1;

	const UManualSprite* ManualSprite = Editor->GetManualSprite();
	if (!ManualSprite)
		return -1;

	const FManualSpriteGeometry& Geometry = ManualSprite->ManualGeometry;
	const float ToleranceSquared = Tolerance * Tolerance;

	for (int32 i = 0; i < Geometry.Vertices.Num(); i++)
	{
		const FVector2D VertexPos = Geometry.Vertices[i].Position;
		if (const float DistanceSquared = FVector2D::DistSquared(WorldPos, VertexPos); 
			DistanceSquared <= ToleranceSquared)
		{
			return i;
		}
	}

	return -1;
}

int32 FManualSpriteEditorViewportClient::FindTriangleAtPosition(FVector2D WorldPos) const
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid())
		return -1;

	const UManualSprite* ManualSprite = Editor->GetManualSprite();
	if (!ManualSprite)
		return -1;

	const FManualSpriteGeometry& Geometry = ManualSprite->ManualGeometry;

	// Простая проверка - находится ли точка внутри треугольника
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

		// Алгоритм проверки точки в треугольнике (barycentric coordinates)
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

void FManualSpriteEditorViewportClient::SetZoom(const float NewZoom)
{
	ZoomFactor = NewZoom;
}

void FManualSpriteEditorViewportClient::SetViewOffset(const FVector2D NewOffset)
{
	ViewOffset = NewOffset;
}

FVector2D FManualSpriteEditorViewportClient::ScreenToWorld(const FVector2D ScreenPos, const FViewport* Viewport) const
{
	const FVector2D ViewportSize = FVector2D(Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y);
	const FVector2D ViewportCenter = ViewportSize * 0.5f;
	
	return (ScreenPos - ViewportCenter - ViewOffset * ZoomFactor) / ZoomFactor;
}

FVector2D FManualSpriteEditorViewportClient::WorldToScreen(const FVector2D WorldPos, const FViewport* Viewport) const
{
	const FVector2D ViewportSize = FVector2D(Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y);
	const FVector2D ViewportCenter = ViewportSize * 0.5f;
	
	return ViewportCenter + ViewOffset * ZoomFactor + WorldPos * ZoomFactor;
}

FVector2D FManualSpriteEditorViewportClient::CalculateUVFromWorldPosition(const FVector2D& WorldPos, const UManualSprite* ManualSprite) const
{
	if (!ManualSprite)
	{
		return FVector2D(0.5f, 0.5f); // Центр текстуры как fallback
	}

	// Получаем размеры спрайта
	const FVector2D SpriteSize = GetSpriteSize(ManualSprite);
	
	if (SpriteSize.IsNearlyZero())
	{
		return FVector2D(0.5f, 0.5f);
	}

	// Конвертируем мировые координаты в UV
	const FVector2D HalfSize = SpriteSize * 0.5f;
	
	// Нормализуем позицию: от -1 до +1
	const FVector2D NormalizedPos = FVector2D(
		WorldPos.X / HalfSize.X,
		WorldPos.Y / HalfSize.Y
	);
	
	// Конвертируем в UV координаты: от 0 до 1
	// Внимание: Y координата инвертирована для UV (0 вверху, 1 внизу)
	const FVector2D UV = FVector2D(
		FMath::Clamp((NormalizedPos.X + 1.0f) * 0.5f, 0.0f, 1.0f),
		FMath::Clamp((1.0f - NormalizedPos.Y) * 0.5f, 0.0f, 1.0f)
	);
	
	return UV;
}

FVector2D FManualSpriteEditorViewportClient::GetSpriteSize(const UManualSprite* ManualSprite) const
{
	if (!ManualSprite)
	{
		return FVector2D(100.0f, 100.0f); // Размер по умолчанию
	}

	// Пытаемся получить размер из источника (текстуры)
	const FVector2D SourceSize = ManualSprite->GetSourceSize();
	
	if (!SourceSize.IsNearlyZero())
	{
		return SourceSize;
	}

	// Если размер источника недоступен, пытаемся получить из текстуры напрямую
	if (UTexture2D* Texture = ManualSprite->GetSourceTexture())
	{
		return FVector2D(static_cast<float>(Texture->GetSizeX()), static_cast<float>(Texture->GetSizeY()));
	}

	// Fallback размер
	return FVector2D(100.0f, 100.0f);
}

void FManualSpriteEditorViewportClient::DrawGrid(FCanvas* Canvas, const FViewport* Viewport) const
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid())
		return;

	const auto& GridSettings = Editor->GetGridSettings();
	if (!GridSettings.bShowGrid)
		return;

	const FVector2D ViewportSize = FVector2D(Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y);
	const float GridSize = GridSettings.GridSize * ZoomFactor;
	
	// Не рисуем сетку если она слишком мелкая
	if (GridSize < 4.0f)
		return;

	// Вычисляем границы сетки
	const FVector2D StartWorld = ScreenToWorld(FVector2D::ZeroVector, Viewport);
	const FVector2D EndWorld = ScreenToWorld(ViewportSize, Viewport);
	
	const float GridWorldSize = GridSettings.GridSize;
	
	// Находим стартовые и конечные индексы линий сетки
	const int32 StartX = FMath::FloorToInt(StartWorld.X / GridWorldSize) - 1;
	const int32 EndX = FMath::CeilToInt(EndWorld.X / GridWorldSize) + 1;
	const int32 StartY = FMath::FloorToInt(StartWorld.Y / GridWorldSize) - 1;
	const int32 EndY = FMath::CeilToInt(EndWorld.Y / GridWorldSize) + 1;

	// Рисуем вертикальные линии
	for (int32 X = StartX; X <= EndX; ++X)
	{
		const float WorldX = X * GridWorldSize;
		const FVector2D StartScreen = WorldToScreen(FVector2D(WorldX, StartWorld.Y), Viewport);
		const FVector2D EndScreen = WorldToScreen(FVector2D(WorldX, EndWorld.Y), Viewport);
		
		FCanvasLineItem LineItem(StartScreen, EndScreen);
		LineItem.SetColor(GridSettings.GridColor);
		Canvas->DrawItem(LineItem);
	}

	// Рисуем горизонтальные линии
	for (int32 Y = StartY; Y <= EndY; ++Y)
	{
		const float WorldY = Y * GridWorldSize;
		const FVector2D StartScreen = WorldToScreen(FVector2D(StartWorld.X, WorldY), Viewport);
		const FVector2D EndScreen = WorldToScreen(FVector2D(EndWorld.X, WorldY), Viewport);
		
		FCanvasLineItem LineItem(StartScreen, EndScreen);
		LineItem.SetColor(GridSettings.GridColor);
		Canvas->DrawItem(LineItem);
	}
}

FVector2D FManualSpriteEditorViewportClient::SnapToGrid(const FVector2D& Position) const
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

void FManualSpriteEditorViewportClient::DrawSprite(FCanvas* Canvas, const FViewport* Viewport) const
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid())
		return;

	const UManualSprite* ManualSprite = Editor->GetManualSprite();
	if (!ManualSprite || !ManualSprite->GetSourceTexture())
		return;

	// Получаем текстуру спрайта
	UTexture2D* Texture = ManualSprite->GetSourceTexture();
	const FVector2D TextureSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
	
	// Вычисляем позицию и размер для отображения
	const FVector2D ViewportSize = FVector2D(Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y);
	const FVector2D ViewportCenter = ViewportSize * 0.5f;
	
	const FVector2D SpriteScreenPos = ViewportCenter + ViewOffset * ZoomFactor;
	const FVector2D SpriteScreenSize = TextureSize * ZoomFactor;

	// Рисуем спрайт
	FCanvasTileItem TileItem(
		SpriteScreenPos - SpriteScreenSize * 0.5f,
		Texture->GetResource(),
		SpriteScreenSize,
		FLinearColor::White
	);
	Canvas->DrawItem(TileItem);
}

void FManualSpriteEditorViewportClient::DrawDebugGeometry(FCanvas* Canvas, const FViewport* Viewport) const
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid())
		return;

	const UManualSprite* ManualSprite = Editor->GetManualSprite();
	if (!ManualSprite)
		return;

	// Рисуем геометрию только если используется ручная геометрия
	if (ManualSprite->bUseManualGeometry)
	{
		// Рисуем треугольники
		DrawTriangles(Canvas, Viewport);
		
		// Рисуем вершины
		DrawVertices(Canvas, Viewport);
	}

	// Рисуем превью вставки
	if (Editor->GetEditMode() == FManualSpriteEditorToolkit::EEditMode::Paste)
	{
		DrawPastePreview(Canvas, Viewport);
	}
}

void FManualSpriteEditorViewportClient::DrawPastePreview(FCanvas* Canvas, const FViewport* Viewport) const
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid())
		return;

	// Получаем превью вершин
	const TArray<FVector2D> PreviewVertices = Editor->GetPastePreviewVertices();
	
	if (PreviewVertices.Num() == 0)
		return;

	// Цвет превью - полупрозрачный синий
	const FLinearColor PreviewColor = FLinearColor(0.0f, 0.5f, 1.0f, 0.6f);
	const float PreviewVertexSize = VertexSize * 1.2f; // Немного больше обычных вершин

	// Рисуем каждую вершину превью
	for (int32 i = 0; i < PreviewVertices.Num(); i++)
	{
		const FVector2D WorldPos = PreviewVertices[i];
		const FVector2D ScreenPos = WorldToScreen(WorldPos, Viewport);

		// Рисуем вершину как круг
		FCanvasBoxItem BoxItem(
			ScreenPos - FVector2D(PreviewVertexSize * 0.5f),
			FVector2D(PreviewVertexSize)
		);
		BoxItem.SetColor(PreviewColor);
		Canvas->DrawItem(BoxItem);

		// Рисуем номер вершины
		const FString VertexNumber = FString::Printf(TEXT("%d"), i);
		FCanvasTextItem TextItem(
			ScreenPos + FVector2D(PreviewVertexSize * 0.5f, -PreviewVertexSize * 0.5f),
			FText::FromString(VertexNumber),
			GEngine->GetSmallFont(),
			PreviewColor
		);
		Canvas->DrawItem(TextItem);
	}

	// Рисуем соединительные линии между вершинами превью (если их больше 1)
	if (PreviewVertices.Num() > 1)
	{
		for (int32 i = 0; i < PreviewVertices.Num() - 1; i++)
		{
			const FVector2D StartPos = WorldToScreen(PreviewVertices[i], Viewport);
			const FVector2D EndPos = WorldToScreen(PreviewVertices[i + 1], Viewport);
			
			FCanvasLineItem LineItem(StartPos, EndPos);
			LineItem.SetColor(FLinearColor(0.0f, 0.3f, 0.8f, 0.4f)); // Более темный синий для линий
			Canvas->DrawItem(LineItem);
		}
	}

	// Показываем подсказку
	const FString HintText = FString::Printf(TEXT("Paste Preview (%d vertices) - Click to paste, Right-click to cancel"), PreviewVertices.Num());
	FCanvasTextItem HintTextItem(
		FVector2D(10, 10), // Левый верхний угол
		FText::FromString(HintText),
		GEngine->GetMediumFont(),
		FLinearColor::Yellow
	);
	Canvas->DrawItem(HintTextItem);
}

void FManualSpriteEditorViewportClient::DrawVertices(FCanvas* Canvas, const FViewport* Viewport) const
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid())
		return;

	const UManualSprite* ManualSprite = Editor->GetManualSprite();
	if (!ManualSprite)
		return;

	const FManualSpriteGeometry& Geometry = ManualSprite->ManualGeometry;

	// Рисуем каждую вершину
	for (int32 i = 0; i < Geometry.Vertices.Num(); i++)
	{
		const FVector2D WorldPos = Geometry.Vertices[i].Position;
		const FVector2D ScreenPos = WorldToScreen(WorldPos, Viewport);

		// Определяем цвет и размер
		FLinearColor Color = VertexColor;
		float Size = VertexSize;

		if (SelectedVertices.Contains(i))
		{
			Color = SelectedVertexColor;
			Size = VertexSelectSize;
		}
		else if (i == HoveredVertex)
		{
			Color = FLinearColor::White;
			Size = VertexSelectSize;
		}
		else if (bIsDraggingVertices && SelectedVertices.Contains(i))
		{
			Color = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f); // Голубой цвет для перетаскиваемых вершин
			Size = VertexSelectSize;
		}

		// Рисуем вершину как круг
		FCanvasBoxItem BoxItem(
			ScreenPos - FVector2D(Size * 0.5f),
			FVector2D(Size)
		);
		BoxItem.SetColor(Color);
		Canvas->DrawItem(BoxItem);

		// Рисуем номер вершины
		const FString VertexNumber = FString::Printf(TEXT("%d"), i);
		FCanvasTextItem TextItem(
			ScreenPos + FVector2D(Size * 0.5f, -Size * 0.5f),
			FText::FromString(VertexNumber),
			GEngine->GetSmallFont(),
			FLinearColor::White
		);
		Canvas->DrawItem(TextItem);
		
		// Показываем UV координаты при наведении
		if (i == HoveredVertex)
		{
			const FVector2D UV = Geometry.Vertices[i].UV;
			const FString UVText = FString::Printf(TEXT("UV: (%.3f, %.3f)"), UV.X, UV.Y);
			const FString PosText = FString::Printf(TEXT("Pos: (%.1f, %.1f)"), WorldPos.X, WorldPos.Y);
			
			// UV координаты
			FCanvasTextItem UVTextItem(
				ScreenPos + FVector2D(Size * 0.5f, Size * 0.5f),
				FText::FromString(UVText),
				GEngine->GetSmallFont(),
				FLinearColor::Yellow
			);
			Canvas->DrawItem(UVTextItem);
			
			// Позиция
			FCanvasTextItem PosTextItem(
				ScreenPos + FVector2D(Size * 0.5f, Size * 0.5f + 12.0f),
				FText::FromString(PosText),
				GEngine->GetSmallFont(),
				FLinearColor::Green
			);
			Canvas->DrawItem(PosTextItem);
		}
	}
}

void FManualSpriteEditorViewportClient::DrawTriangles(FCanvas* Canvas, const FViewport* Viewport) const
{
	const TSharedPtr<FManualSpriteEditorToolkit> Editor = ManualSpriteEditorPtr.Pin();
	if (!Editor.IsValid())
		return;

	const UManualSprite* ManualSprite = Editor->GetManualSprite();
	if (!ManualSprite)
		return;

	const FManualSpriteGeometry& Geometry = ManualSprite->ManualGeometry;

	// Рисуем каждый треугольник
	for (int32 i = 0; i < Geometry.Triangles.Num(); i++)
	{
		const FManualSpriteTriangle& Triangle = Geometry.Triangles[i];

		// Проверяем валидность индексов
		if (Triangle.VertexIndex0 >= Geometry.Vertices.Num() ||
			Triangle.VertexIndex1 >= Geometry.Vertices.Num() ||
			Triangle.VertexIndex2 >= Geometry.Vertices.Num())
			continue;

		// Получаем позиции вершин
		const FVector2D Pos0 = WorldToScreen(Geometry.Vertices[Triangle.VertexIndex0].Position, Viewport);
		const FVector2D Pos1 = WorldToScreen(Geometry.Vertices[Triangle.VertexIndex1].Position, Viewport);
		const FVector2D Pos2 = WorldToScreen(Geometry.Vertices[Triangle.VertexIndex2].Position, Viewport);

		// Определяем цвет
		const FLinearColor Color = (i == HoveredTriangle) ? SelectedTriangleColor : TriangleColor;

		// Рисуем рёбра треугольника
		FCanvasLineItem LineItem0(Pos0, Pos1);
		LineItem0.SetColor(Color);
		Canvas->DrawItem(LineItem0);

		FCanvasLineItem LineItem1(Pos1, Pos2);
		LineItem1.SetColor(Color);
		Canvas->DrawItem(LineItem1);

		FCanvasLineItem LineItem2(Pos2, Pos0);
		LineItem2.SetColor(Color);
		Canvas->DrawItem(LineItem2);
	}
}