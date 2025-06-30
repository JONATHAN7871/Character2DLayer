#include "ManualSprite.h"
#include "Engine/Texture2D.h"

UManualSprite::UManualSprite()
{
	bUseManualGeometry = false;
	bShowDebugGeometry = true;
	bShowTriangulationButton = true; // v1.1
}

void UManualSprite::AddVertex(const FVector2D& Position, const FVector2D& UV)
{
	ManualGeometry.Vertices.Add(FManualSpriteVertex(Position, UV));
	
	// Помечаем объект как изменённый для сохранения
	MarkPackageDirty();
}

void UManualSprite::RemoveVertex(int32 VertexIndex)
{
	if (VertexIndex >= 0 && VertexIndex < ManualGeometry.Vertices.Num())
	{
		// Удаляем вершину
		ManualGeometry.Vertices.RemoveAt(VertexIndex);
		
		// Обновляем индексы в треугольниках и удаляем невалидные
		for (int32 i = ManualGeometry.Triangles.Num() - 1; i >= 0; i--)
		{
			FManualSpriteTriangle& Triangle = ManualGeometry.Triangles[i];
			
			// Если треугольник использует удалённую вершину - удаляем треугольник
			if (Triangle.VertexIndex0 == VertexIndex || 
				Triangle.VertexIndex1 == VertexIndex || 
				Triangle.VertexIndex2 == VertexIndex)
			{
				ManualGeometry.Triangles.RemoveAt(i);
				continue;
			}
			
			// Сдвигаем индексы больше удалённого
			if (Triangle.VertexIndex0 > VertexIndex) Triangle.VertexIndex0--;
			if (Triangle.VertexIndex1 > VertexIndex) Triangle.VertexIndex1--;
			if (Triangle.VertexIndex2 > VertexIndex) Triangle.VertexIndex2--;
		}
		
		MarkPackageDirty();
	}
}

void UManualSprite::AddTriangle(int32 VertexIndex0, int32 VertexIndex1, int32 VertexIndex2)
{
	// Проверяем валидность индексов
	if (VertexIndex0 >= 0 && VertexIndex0 < ManualGeometry.Vertices.Num() &&
		VertexIndex1 >= 0 && VertexIndex1 < ManualGeometry.Vertices.Num() &&
		VertexIndex2 >= 0 && VertexIndex2 < ManualGeometry.Vertices.Num() &&
		VertexIndex0 != VertexIndex1 && VertexIndex1 != VertexIndex2 && VertexIndex0 != VertexIndex2)
	{
		ManualGeometry.Triangles.Add(FManualSpriteTriangle(VertexIndex0, VertexIndex1, VertexIndex2));
		MarkPackageDirty();
	}
}

void UManualSprite::RemoveTriangle(int32 TriangleIndex)
{
	if (TriangleIndex >= 0 && TriangleIndex < ManualGeometry.Triangles.Num())
	{
		ManualGeometry.Triangles.RemoveAt(TriangleIndex);
		MarkPackageDirty();
	}
}

void UManualSprite::ClearManualGeometry()
{
	ManualGeometry.Clear();
	MarkPackageDirty();
}

bool UManualSprite::IsManualGeometryValid() const
{
	return ManualGeometry.IsValid();
}

// v1.1: Автоматическая триангуляция
void UManualSprite::AutoTriangulate()
{
	AutoTriangulateWithMethod(ManualGeometry.PreferredTriangulationMethod);
}

void UManualSprite::AutoTriangulateWithMethod(ETriangulationMethod Method)
{
	if (!CanAutoTriangulate())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot auto-triangulate: need at least 3 vertices"));
		return;
	}

	// Очищаем существующие треугольники
	ClearTriangles();

	// Выполняем триангуляцию выбранным методом
	switch (Method)
	{
	case ETriangulationMethod::Fan:
		TriangulateFan();
		break;
	case ETriangulationMethod::Delaunay:
		TriangulateDelaunay();
		break;
	case ETriangulationMethod::ConvexHull:
		TriangulateConvexHull();
		break;
	case ETriangulationMethod::EarClipping:
		TriangulateEarClipping();
		break;
	}

	MarkPackageDirty();
	
	UE_LOG(LogTemp, Log, TEXT("Auto-triangulation completed using method %d. Created %d triangles"), 
		   static_cast<int32>(Method), ManualGeometry.Triangles.Num());
}

void UManualSprite::ClearTriangles()
{
	ManualGeometry.ClearTriangles();
	MarkPackageDirty();
}

bool UManualSprite::CanAutoTriangulate() const
{
	return ManualGeometry.Vertices.Num() >= 3;
}

void UManualSprite::SortVerticesByAngle()
{
	if (ManualGeometry.Vertices.Num() < 3)
		return;

	// Находим центроид
	const FVector2D Centroid = GetCentroid();

	// Сортируем вершины по углу относительно центроида
	ManualGeometry.Vertices.Sort([&Centroid](const FManualSpriteVertex& A, const FManualSpriteVertex& B)
	{
		const FVector2D VecA = A.Position - Centroid;
		const FVector2D VecB = B.Position - Centroid;
		
		const float AngleA = FMath::Atan2(VecA.Y, VecA.X);
		const float AngleB = FMath::Atan2(VecB.Y, VecB.X);
		
		return AngleA < AngleB;
	});

	MarkPackageDirty();
	UE_LOG(LogTemp, Log, TEXT("Vertices sorted by angle around centroid"));
}

bool UManualSprite::IsConvexPolygon() const
{
	const int32 VertexCount = ManualGeometry.Vertices.Num();
	if (VertexCount < 3)
		return false;

	bool bPositive = false;
	bool bNegative = false;

	for (int32 i = 0; i < VertexCount; i++)
	{
		const int32 PrevIndex = (i - 1 + VertexCount) % VertexCount;
		const int32 NextIndex = (i + 1) % VertexCount;
		
		const FVector2D Prev = ManualGeometry.Vertices[PrevIndex].Position;
		const FVector2D Current = ManualGeometry.Vertices[i].Position;
		const FVector2D Next = ManualGeometry.Vertices[NextIndex].Position;
		
		const FVector2D Edge1 = Current - Prev;
		const FVector2D Edge2 = Next - Current;
		const float CrossProduct = Cross2D(Edge1, Edge2);
		
		if (CrossProduct > 0.0f)
			bPositive = true;
		else if (CrossProduct < 0.0f)
			bNegative = true;
		
		// Если есть и положительные, и отрицательные произведения, многоугольник не выпуклый
		if (bPositive && bNegative)
			return false;
	}

	return true;
}

void UManualSprite::ReverseVertexOrder()
{
	Algo::Reverse(ManualGeometry.Vertices);
	MarkPackageDirty();
	UE_LOG(LogTemp, Log, TEXT("Vertex order reversed"));
}

void UManualSprite::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	// Если включили ручную геометрию, но её нет - создаём базовую
	if (bUseManualGeometry && ManualGeometry.Vertices.Num() == 0)
	{
		GenerateDefaultGeometry();
	}
	
	// Валидируем геометрию при любых изменениях
	ValidateGeometry();
}

void UManualSprite::GenerateDefaultGeometry()
{
	ManualGeometry.Clear();
	
	// Получаем размеры спрайта. Если текстуры нет, используем размер по умолчанию.
	FVector2D SpriteSize = GetSourceSize();
	if (SpriteSize.IsNearlyZero())
	{
		SpriteSize = FVector2D(100.0f, 100.0f); // Размер по умолчанию
	}
	FVector2D HalfSize = SpriteSize * 0.5f;
	
	// Создаём 4 вершины по углам спрайта
	// Координаты UV должны быть (0,1) и (1,0), а не (0,0) и (1,1) для стандартного рендера Paper2D
	ManualGeometry.Vertices.Add(FManualSpriteVertex(FVector2D(-HalfSize.X, -HalfSize.Y), FVector2D(0, 1))); // Левый нижний
	ManualGeometry.Vertices.Add(FManualSpriteVertex(FVector2D(HalfSize.X, -HalfSize.Y), FVector2D(1, 1)));  // Правый нижний
	ManualGeometry.Vertices.Add(FManualSpriteVertex(FVector2D(HalfSize.X, HalfSize.Y), FVector2D(1, 0)));   // Правый верхний
	ManualGeometry.Vertices.Add(FManualSpriteVertex(FVector2D(-HalfSize.X, HalfSize.Y), FVector2D(0, 0)));  // Левый верхний
	
	// Создаём 2 треугольника для квада
	ManualGeometry.Triangles.Add(FManualSpriteTriangle(0, 1, 2)); // Первый треугольник
	ManualGeometry.Triangles.Add(FManualSpriteTriangle(0, 2, 3)); // Второй треугольник

	MarkPackageDirty();
}

void UManualSprite::ValidateGeometry()
{
	// Удаляем треугольники с некорректными индексами
	for (int32 i = ManualGeometry.Triangles.Num() - 1; i >= 0; i--)
	{
		const FManualSpriteTriangle& Triangle = ManualGeometry.Triangles[i];
		
		if (Triangle.VertexIndex0 >= ManualGeometry.Vertices.Num() || Triangle.VertexIndex0 < 0 ||
			Triangle.VertexIndex1 >= ManualGeometry.Vertices.Num() || Triangle.VertexIndex1 < 0 ||
			Triangle.VertexIndex2 >= ManualGeometry.Vertices.Num() || Triangle.VertexIndex2 < 0 ||
			Triangle.VertexIndex0 == Triangle.VertexIndex1 || 
			Triangle.VertexIndex1 == Triangle.VertexIndex2 || 
			Triangle.VertexIndex0 == Triangle.VertexIndex2)
		{
			ManualGeometry.Triangles.RemoveAt(i);
		}
	}
}

// v1.1: Реализация алгоритмов триангуляции

void UManualSprite::TriangulateFan()
{
	const int32 VertexCount = ManualGeometry.Vertices.Num();
	if (VertexCount < 3)
		return;

	// Простая веерная триангуляция от первой вершины
	for (int32 i = 1; i < VertexCount - 1; i++)
	{
		ManualGeometry.Triangles.Add(FManualSpriteTriangle(0, i, i + 1));
	}

	UE_LOG(LogTemp, Log, TEXT("Fan triangulation: created %d triangles"), ManualGeometry.Triangles.Num());
}

void UManualSprite::TriangulateDelaunay()
{
	// Упрощённая версия триангуляции Делоне
	// Для сложной реализации потребуется внешняя библиотека, пока используем Fan с улучшениями
	const int32 VertexCount = ManualGeometry.Vertices.Num();
	if (VertexCount < 3)
		return;

	if (VertexCount == 3)
	{
		ManualGeometry.Triangles.Add(FManualSpriteTriangle(0, 1, 2));
		return;
	}

	// Для выпуклых многоугольников используем улучшенную веерную триангуляцию
	if (IsConvexPolygon())
	{
		TriangulateFan();
	}
	else
	{
		// Для невыпуклых - используем ear clipping
		TriangulateEarClipping();
	}

	UE_LOG(LogTemp, Log, TEXT("Delaunay-like triangulation: created %d triangles"), ManualGeometry.Triangles.Num());
}

void UManualSprite::TriangulateConvexHull()
{
	// Получаем выпуклую оболочку
	TArray<int32> HullIndices = GetConvexHull();
	
	if (HullIndices.Num() < 3)
		return;

	// Триангулируем выпуклую оболочку веерным методом
	for (int32 i = 1; i < HullIndices.Num() - 1; i++)
	{
		ManualGeometry.Triangles.Add(FManualSpriteTriangle(HullIndices[0], HullIndices[i], HullIndices[i + 1]));
	}

	UE_LOG(LogTemp, Log, TEXT("Convex hull triangulation: created %d triangles from %d hull points"), 
		   ManualGeometry.Triangles.Num(), HullIndices.Num());
}

void UManualSprite::TriangulateEarClipping()
{
	const int32 VertexCount = ManualGeometry.Vertices.Num();
	if (VertexCount < 3)
		return;

	// Создаём список активных вершин
	TArray<int32> ActiveVertices;
	for (int32 i = 0; i < VertexCount; i++)
	{
		ActiveVertices.Add(i);
	}

	// Алгоритм отсечения ушей
	while (ActiveVertices.Num() > 3)
	{
		bool bFoundEar = false;
		
		for (int32 i = 0; i < ActiveVertices.Num() && !bFoundEar; i++)
		{
			const int32 PrevIndex = ActiveVertices[(i - 1 + ActiveVertices.Num()) % ActiveVertices.Num()];
			const int32 CurrentIndex = ActiveVertices[i];
			const int32 NextIndex = ActiveVertices[(i + 1) % ActiveVertices.Num()];
			
			// Проверяем, является ли эта вершина "ухом"
			if (IsEar(PrevIndex, CurrentIndex, NextIndex))
			{
				// Создаём треугольник
				ManualGeometry.Triangles.Add(FManualSpriteTriangle(PrevIndex, CurrentIndex, NextIndex));
				
				// Удаляем текущую вершину из активного списка
				ActiveVertices.RemoveAt(i);
				bFoundEar = true;
			}
		}
		
		// Если не найдено ни одного уха, прерываем (защита от бесконечного цикла)
		if (!bFoundEar)
		{
			UE_LOG(LogTemp, Warning, TEXT("Ear clipping failed: no ears found. Remaining vertices: %d"), ActiveVertices.Num());
			break;
		}
	}

	// Добавляем последний треугольник
	if (ActiveVertices.Num() == 3)
	{
		ManualGeometry.Triangles.Add(FManualSpriteTriangle(ActiveVertices[0], ActiveVertices[1], ActiveVertices[2]));
	}

	UE_LOG(LogTemp, Log, TEXT("Ear clipping triangulation: created %d triangles"), ManualGeometry.Triangles.Num());
}

// v1.1: Вспомогательные функции

bool UManualSprite::IsEar(int32 PrevIndex, int32 CurrentIndex, int32 NextIndex) const
{
	const FVector2D Prev = ManualGeometry.Vertices[PrevIndex].Position;
	const FVector2D Current = ManualGeometry.Vertices[CurrentIndex].Position;
	const FVector2D Next = ManualGeometry.Vertices[NextIndex].Position;
	
	// Проверяем, что вершина выпуклая
	if (!IsConvexVertex(PrevIndex, CurrentIndex, NextIndex))
		return false;
	
	// Проверяем, что внутри треугольника нет других вершин
	for (int32 i = 0; i < ManualGeometry.Vertices.Num(); i++)
	{
		if (i == PrevIndex || i == CurrentIndex || i == NextIndex)
			continue;
			
		if (IsPointInTriangle(ManualGeometry.Vertices[i].Position, Prev, Current, Next))
			return false;
	}
	
	return true;
}

bool UManualSprite::IsPointInTriangle(const FVector2D& Point, const FVector2D& A, const FVector2D& B, const FVector2D& C) const
{
	// Используем барицентрические координаты
	const FVector2D v0 = C - A;
	const FVector2D v1 = B - A;
	const FVector2D v2 = Point - A;

	const float dot00 = FVector2D::DotProduct(v0, v0);
	const float dot01 = FVector2D::DotProduct(v0, v1);
	const float dot02 = FVector2D::DotProduct(v0, v2);
	const float dot11 = FVector2D::DotProduct(v1, v1);
	const float dot12 = FVector2D::DotProduct(v1, v2);

	const float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
	const float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	const float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	return (u >= 0) && (v >= 0) && (u + v <= 1);
}

float UManualSprite::Cross2D(const FVector2D& A, const FVector2D& B) const
{
	return A.X * B.Y - A.Y * B.X;
}

bool UManualSprite::IsConvexVertex(int32 PrevIndex, int32 CurrentIndex, int32 NextIndex) const
{
	const FVector2D Prev = ManualGeometry.Vertices[PrevIndex].Position;
	const FVector2D Current = ManualGeometry.Vertices[CurrentIndex].Position;
	const FVector2D Next = ManualGeometry.Vertices[NextIndex].Position;
	
	const FVector2D Edge1 = Current - Prev;
	const FVector2D Edge2 = Next - Current;
	
	// Положительное векторное произведение означает поворот против часовой стрелки (выпуклая вершина)
	return Cross2D(Edge1, Edge2) > 0.0f;
}

TArray<int32> UManualSprite::GetConvexHull() const
{
	const int32 VertexCount = ManualGeometry.Vertices.Num();
	TArray<int32> HullIndices;
	
	if (VertexCount < 3)
		return HullIndices;
	
	// Алгоритм Грэхема для построения выпуклой оболочки
	TArray<int32> SortedIndices;
	for (int32 i = 0; i < VertexCount; i++)
	{
		SortedIndices.Add(i);
	}
	
	// Находим нижнюю левую точку
	int32 BottomLeftIndex = 0;
	for (int32 i = 1; i < VertexCount; i++)
	{
		const FVector2D Current = ManualGeometry.Vertices[i].Position;
		const FVector2D BottomLeft = ManualGeometry.Vertices[BottomLeftIndex].Position;
		
		if (Current.Y < BottomLeft.Y || (FMath::IsNearlyEqual(Current.Y, BottomLeft.Y) && Current.X < BottomLeft.X))
		{
			BottomLeftIndex = i;
		}
	}
	
	// Сортируем точки по полярному углу относительно нижней левой точки
	const FVector2D Pivot = ManualGeometry.Vertices[BottomLeftIndex].Position;
	SortedIndices.Sort([this, &Pivot](const int32& A, const int32& B)
	{
		if (A == B)
			return false;
			
		const FVector2D VecA = ManualGeometry.Vertices[A].Position - Pivot;
		const FVector2D VecB = ManualGeometry.Vertices[B].Position - Pivot;
		
		const float CrossProduct = Cross2D(VecA, VecB);
		if (FMath::Abs(CrossProduct) < KINDA_SMALL_NUMBER)
		{
			// Коллинеарные точки - сортируем по расстоянию
			return VecA.SizeSquared() < VecB.SizeSquared();
		}
		
		return CrossProduct > 0.0f;
	});
	
	// Строим оболочку
	for (int32 i = 0; i < SortedIndices.Num(); i++)
	{
		// Удаляем точки, которые создают правый поворот
		while (HullIndices.Num() > 1)
		{
			const FVector2D P1 = ManualGeometry.Vertices[HullIndices[HullIndices.Num() - 2]].Position;
			const FVector2D P2 = ManualGeometry.Vertices[HullIndices[HullIndices.Num() - 1]].Position;
			const FVector2D P3 = ManualGeometry.Vertices[SortedIndices[i]].Position;
			
			const FVector2D Edge1 = P2 - P1;
			const FVector2D Edge2 = P3 - P2;
			
			if (Cross2D(Edge1, Edge2) < 0.0f) // Правый поворот
			{
				HullIndices.RemoveAt(HullIndices.Num() - 1);
			}
			else
			{
				break;
			}
		}
		
		HullIndices.Add(SortedIndices[i]);
	}
	
	return HullIndices;
}

FVector2D UManualSprite::GetCentroid() const
{
	if (ManualGeometry.Vertices.Num() == 0)
		return FVector2D::ZeroVector;
	
	FVector2D Sum = FVector2D::ZeroVector;
	for (const FManualSpriteVertex& Vertex : ManualGeometry.Vertices)
	{
		Sum += Vertex.Position;
	}
	
	return Sum / static_cast<float>(ManualGeometry.Vertices.Num());
}