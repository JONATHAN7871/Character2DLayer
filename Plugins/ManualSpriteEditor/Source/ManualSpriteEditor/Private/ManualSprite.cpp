#include "ManualSprite.h"

#include "ManualSpriteMeshGeneratorOptions.h"
#include "SpriteEditorOnlyTypes.h"
#include "Engine/Texture2D.h"

UManualSprite::UManualSprite()
{
	bUseManualGeometry = true;
	
	// НОВОЕ: Инициализация настроек пивота
	PivotPlacement = EManualSpritePivotPlacement::Center;
	CustomPivotOffset = FVector::ZeroVector;
	MeshScale = 1.0f;
	bShowPivotPreview = true;
}

#if WITH_EDITOR
const FSpriteGeometryCollection& UManualSprite::GetRenderGeometry() const
{
	// Так как мы внутри реализации класса, который наследуется от UPaperSprite,
	// и этот код компилируется только в редакторе, мы имеем доступ к RenderGeometry.
	return RenderGeometry;
}

bool UManualSprite::ImportRenderGeometry(TArray<FManualSpriteVertex>& OutVertices, TArray<FManualSpriteTriangle>& OutTriangles) const
{
	OutVertices.Empty();
	OutTriangles.Empty();

#if WITH_EDITOR
	UE_LOG(LogTemp, Warning, TEXT("=== ImportRenderGeometry DEBUG ==="));
	
	// Убеждаемся, что данные актуальны
	const_cast<UManualSprite*>(this)->RebuildRenderData();

	const FSpriteGeometryCollection& GeomCollection = GetRenderGeometry();
	
	UE_LOG(LogTemp, Warning, TEXT("RenderGeometry has %d shapes"), GeomCollection.Shapes.Num());
	UE_LOG(LogTemp, Warning, TEXT("GeometryType: %d"), (int32)GeomCollection.GeometryType);
	
	if (GeomCollection.Shapes.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No shapes in RenderGeometry. Trying to access BakedRenderData..."));
		
		// Попробуем взять данные из BakedRenderData напрямую
		if (BakedRenderData.Num() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Found %d points in BakedRenderData"), BakedRenderData.Num());
			
			// BakedRenderData содержит готовые треугольники в формате XY UV
			if ((BakedRenderData.Num() % 3) == 0)
			{
				// Собираем уникальные вершины
				TArray<FVector2D> UniqueVertices;
				TArray<int32> VertexIndices;
				const float MergeTolerance = 0.1f;
				
				const FVector2D SpriteSize = GetSourceSize();
				const FVector2D HalfSize = SpriteSize * 0.5f;
				const float UnitsPerPixel = GetUnrealUnitsPerPixel();
				
				for (const FVector4& BakedVert : BakedRenderData)
				{
					// Конвертируем обратно в текстурное пространство
					// BakedVert.X и BakedVert.Y - это позиция в pivot space в UU
					const FVector2D PivotSpacePos(BakedVert.X / UnitsPerPixel, BakedVert.Y / UnitsPerPixel);
					const FVector2D TextureSpacePos = PivotSpacePos + HalfSize;
					
					// Ищем существующую близкую вершину
					int32 ExistingIndex = INDEX_NONE;
					for (int32 i = 0; i < UniqueVertices.Num(); i++)
					{
						if (FVector2D::DistSquared(UniqueVertices[i], TextureSpacePos) < MergeTolerance * MergeTolerance)
						{
							ExistingIndex = i;
							break;
						}
					}
					
					if (ExistingIndex == INDEX_NONE)
					{
						ExistingIndex = UniqueVertices.Add(TextureSpacePos);
					}
					
					VertexIndices.Add(ExistingIndex);
				}
				
				// Создаем вершины с UV
				for (const FVector2D& VertexPos : UniqueVertices)
				{
					FVector2D UV;
					if (!SpriteSize.IsNearlyZero())
					{
						UV.X = (VertexPos.X - GetSourceUV().X) / SpriteSize.X;
						UV.Y = 1.0f - ((VertexPos.Y - GetSourceUV().Y) / SpriteSize.Y);
					}
					else
					{
						UV = FVector2D(0.5f, 0.5f);
					}
					
					OutVertices.Add(FManualSpriteVertex(VertexPos, UV));
				}

				// ОТЛАДКА: Логируем первые несколько результирующих вершин
				for (int32 i = 0; i < FMath::Min(OutVertices.Num(), 5); i++)
				{
					const FManualSpriteVertex& Vertex = OutVertices[i];
					UE_LOG(LogTemp, Warning, TEXT("DEBUG: Final vertex %d = (%.2f, %.2f), UV = (%.3f, %.3f)"), 
						   i, Vertex.Position.X, Vertex.Position.Y, Vertex.UV.X, Vertex.UV.Y);
				}
				
				// Создаем треугольники
				for (int32 i = 0; i < VertexIndices.Num(); i += 3)
				{
					if (i + 2 < VertexIndices.Num())
					{
						const int32 Index0 = VertexIndices[i];
						const int32 Index1 = VertexIndices[i + 1];
						const int32 Index2 = VertexIndices[i + 2];
						
						if (Index0 < OutVertices.Num() && Index1 < OutVertices.Num() && Index2 < OutVertices.Num() &&
							Index0 != Index1 && Index1 != Index2 && Index0 != Index2)
						{
							OutTriangles.Add(FManualSpriteTriangle(Index0, Index1, Index2));
						}
					}
				}
				
				UE_LOG(LogTemp, Warning, TEXT("✅ Imported from BakedRenderData: %d vertices, %d triangles"), 
					   OutVertices.Num(), OutTriangles.Num());
				
				return OutVertices.Num() > 0 && OutTriangles.Num() > 0;
			}
		}
		
		UE_LOG(LogTemp, Warning, TEXT("❌ No usable geometry found in RenderGeometry or BakedRenderData"));
		return false;
	}

	// Отладочная информация о шейпах
	for (int32 i = 0; i < GeomCollection.Shapes.Num(); i++)
	{
		const FSpriteGeometryShape& Shape = GeomCollection.Shapes[i];
		UE_LOG(LogTemp, Warning, TEXT("Shape %d: Type=%d, Vertices=%d, BoxSize=(%.2f,%.2f)"), 
			   i, (int32)Shape.ShapeType, Shape.Vertices.Num(), Shape.BoxSize.X, Shape.BoxSize.Y);
	}

	// Триангулируем геометрию для получения треугольников
	TArray<FVector2D> TriangulatedPoints;
	GeomCollection.Triangulate(TriangulatedPoints, true); // включаем боксы

	UE_LOG(LogTemp, Warning, TEXT("Triangulation produced %d points"), TriangulatedPoints.Num());

	if (TriangulatedPoints.Num() < 3 || (TriangulatedPoints.Num() % 3) != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ Invalid triangulated data: %d points"), TriangulatedPoints.Num());
		return false;
	}

	// Собираем уникальные вершины
	TArray<FVector2D> UniqueVertices;
	TArray<int32> VertexIndices;
	
	const float VertexMergeTolerance = 0.1f; // Порог для слияния близких вершин
	
	for (const FVector2D& Point : TriangulatedPoints)
	{
		// Ищем существующую близкую вершину
		int32 ExistingIndex = INDEX_NONE;
		for (int32 i = 0; i < UniqueVertices.Num(); i++)
		{
			if (FVector2D::DistSquared(UniqueVertices[i], Point) < VertexMergeTolerance * VertexMergeTolerance)
			{
				ExistingIndex = i;
				break;
			}
		}
		
		if (ExistingIndex == INDEX_NONE)
		{
			// Добавляем новую уникальную вершину
			ExistingIndex = UniqueVertices.Add(Point);
		}
		
		VertexIndices.Add(ExistingIndex);
	}

	// Конвертируем вершины в наш формат с UV
	const FVector2D SpriteSize = GetSourceSize();

	// ОТЛАДКА: Логируем размер спрайта и первые несколько вершин
	UE_LOG(LogTemp, Warning, TEXT("DEBUG: SpriteSize = (%.2f, %.2f)"), SpriteSize.X, SpriteSize.Y);

	for (int32 i = 0; i < FMath::Min(UniqueVertices.Num(), 5); i++)
	{
		const FVector2D& VertexPos = UniqueVertices[i];
		UE_LOG(LogTemp, Warning, TEXT("DEBUG: Original vertex %d = (%.2f, %.2f)"), i, VertexPos.X, VertexPos.Y);
	}

	for (const FVector2D& VertexPos : UniqueVertices)
	{
		// ИСПРАВЛЕНИЕ: VertexPos находится в абсолютных координатах текстуры
		// Преобразуем в локальные координаты нашего редактора
		const FVector2D LocalPos = FVector2D(
			VertexPos.X - SpriteSize.X * 0.5f,  // Центрируем X
			VertexPos.Y - SpriteSize.Y * 0.5f   // Центрируем Y
		);
	
		FVector2D UV;
		if (!SpriteSize.IsNearlyZero())
		{
			// Вычисляем UV из абсолютных координат
			UV.X = VertexPos.X / SpriteSize.X;
			UV.Y = 1.0f - (VertexPos.Y / SpriteSize.Y); // Инвертируем Y
		}
		else
		{
			UV = FVector2D(0.5f, 0.5f);
		}

		OutVertices.Add(FManualSpriteVertex(LocalPos, UV));
	}

	// ОТЛАДКА: Логируем первые несколько результирующих вершин
	for (int32 i = 0; i < FMath::Min(OutVertices.Num(), 5); i++)
	{
		const FManualSpriteVertex& Vertex = OutVertices[i];
		UE_LOG(LogTemp, Warning, TEXT("DEBUG: Final vertex %d = (%.2f, %.2f), UV = (%.3f, %.3f)"), 
			   i, Vertex.Position.X, Vertex.Position.Y, Vertex.UV.X, Vertex.UV.Y);
	}
	
	// Создаем треугольники из индексов
	for (int32 i = 0; i < VertexIndices.Num(); i += 3)
	{
		if (i + 2 < VertexIndices.Num())
		{
			const int32 Index0 = VertexIndices[i];
			const int32 Index1 = VertexIndices[i + 1];
			const int32 Index2 = VertexIndices[i + 2];

			// Проверяем валидность индексов
			if (Index0 < OutVertices.Num() && Index1 < OutVertices.Num() && Index2 < OutVertices.Num() &&
				Index0 != Index1 && Index1 != Index2 && Index0 != Index2)
			{
				OutTriangles.Add(FManualSpriteTriangle(Index0, Index1, Index2));
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("✅ Final result: %d vertices, %d triangles from %d triangulated points"), 
		   OutVertices.Num(), OutTriangles.Num(), TriangulatedPoints.Num());

	return OutVertices.Num() > 0 && OutTriangles.Num() > 0;
#else
	return false;
#endif
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
	
	// НОВОЕ: Обновляем превью пивота при изменении настроек
	if (PropertyChangedEvent.Property)
	{
		const FName PropertyName = PropertyChangedEvent.Property->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UManualSprite, PivotPlacement) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UManualSprite, CustomPivotOffset) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UManualSprite, MeshScale) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UManualSprite, bShowPivotPreview))
		{
			// Уведомляем viewport о необходимости обновления
			OnPivotSettingsChanged();
		}
	}
	
	// Валидируем геометрию при любых изменениях
	ValidateGeometry();
}

void UManualSprite::OnPivotSettingsChanged()
{
	// Этот метод вызывается при изменении настроек пивота
	// Viewport будет отслеживать эти изменения для обновления превью
	UE_LOG(LogTemp, Log, TEXT("Pivot settings changed: Placement=%d, Scale=%.2f"), 
		   (int32)PivotPlacement, MeshScale);
}

FVector UManualSprite::CalculateCurrentPivotPosition() const
{
	if (!bUseManualGeometry || ManualGeometry.Vertices.Num() == 0)
	{
		return FVector::ZeroVector;
	}

	// Вычисляем границы геометрии в 3D пространстве
	FBox SpriteBounds(ForceInit);
	for (const FManualSpriteVertex& Vertex : ManualGeometry.Vertices)
	{
		// Конвертируем 2D в 3D (Y=0, Z инвертированный)
		SpriteBounds += FVector(Vertex.Position.X * MeshScale, 0.0f, -Vertex.Position.Y * MeshScale);
	}

	FVector PivotPos = FVector::ZeroVector;
	
	switch (PivotPlacement)
	{
	case EManualSpritePivotPlacement::Origin:
		PivotPos = FVector::ZeroVector;
		break;
		
	case EManualSpritePivotPlacement::Center:
		PivotPos = SpriteBounds.GetCenter();
		break;
		
	case EManualSpritePivotPlacement::BottomCenter:
		PivotPos = FVector(SpriteBounds.GetCenter().X, SpriteBounds.GetCenter().Y, SpriteBounds.Min.Z);
		break;
		
	case EManualSpritePivotPlacement::Custom:
		PivotPos = CustomPivotOffset;
		break;
	}

	return PivotPos;
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