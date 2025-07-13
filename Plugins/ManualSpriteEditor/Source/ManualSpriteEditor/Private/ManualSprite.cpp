#include "ManualSprite.h"
#include "SpriteEditorOnlyTypes.h"
#include "Engine/Texture2D.h"

UManualSprite::UManualSprite()
{
	bUseManualGeometry = true;
}

#if WITH_EDITOR
const FSpriteGeometryCollection& UManualSprite::GetRenderGeometry() const
{
	// Так как мы внутри реализации класса, который наследуется от UPaperSprite,
	// и этот код компилируется только в редакторе, мы имеем доступ к RenderGeometry.
	return RenderGeometry;
}
#endif

void UManualSprite::AddVertex(const FVector2D& Position, const FVector2D& UV)
{
	ManualGeometry.Vertices.Add(FManualSpriteVertex(Position, UV));
	
	// Помечаем объект как изменённый для сохранения
	(void)MarkPackageDirty();
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
		
		(void)MarkPackageDirty();
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
		(void)MarkPackageDirty();
	}
}

void UManualSprite::RemoveTriangle(int32 TriangleIndex)
{
	if (TriangleIndex >= 0 && TriangleIndex < ManualGeometry.Triangles.Num())
	{
		ManualGeometry.Triangles.RemoveAt(TriangleIndex);
		(void)MarkPackageDirty();
	}
}

void UManualSprite::ClearManualGeometry()
{
	ManualGeometry.Clear();
	(void)MarkPackageDirty();
}

bool UManualSprite::IsManualGeometryValid() const
{
	return ManualGeometry.IsValid();
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

	(void)MarkPackageDirty();
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

#if WITH_EDITOR
void UManualSprite::GetRenderGeometryVertices(TArray<FVector2D>& OutVertices) const
{
	OutVertices.Empty();

	// Убеждаемся, что данные актуальны.
	// const_cast нужен, потому что RebuildRenderData не является const-методом.
	// Это безопасно в данном контексте, так как метод логически не меняет состояние объекта для пользователя.
	const_cast<UManualSprite*>(this)->RebuildRenderData();

	// Так как мы внутри метода класса-наследника, у нас есть доступ к protected-полю RenderGeometry.
	for (const FSpriteGeometryShape& Shape : RenderGeometry.Shapes)
	{
		OutVertices.Append(Shape.Vertices);
	}
}
#endif