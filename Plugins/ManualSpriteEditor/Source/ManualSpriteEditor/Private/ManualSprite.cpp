#include "ManualSprite.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"

UManualSprite::UManualSprite()
{
	bUseManualGeometry = false;
	bShowDebugGeometry = true;
	bShowTriangulationButton = true;
	bShowingPreview = false;
}

void UManualSprite::AddVertex(const FVector2D& Position, const FVector2D& UV)
{
	ManualGeometry.Vertices.Add(FManualSpriteVertex(Position, UV));
	MarkPackageDirty();
}

void UManualSprite::RemoveVertex(int32 VertexIndex)
{
	if (VertexIndex < 0 || VertexIndex >= ManualGeometry.Vertices.Num())
		return;

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
	if (TriangleIndex < 0 || TriangleIndex >= ManualGeometry.Triangles.Num())
		return;

	ManualGeometry.Triangles.RemoveAt(TriangleIndex);
	MarkPackageDirty();
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

#if WITH_EDITORONLY_DATA
bool UManualSprite::GenerateGeometryFromTexture()
{
	ManualGeometry.Clear();
	
	switch (AutoGeometrySettings.GeometryType)
	{
	case EAutoGeometryType::FromRenderShapes:
		return GenerateGeometryFromRenderShapes();
		
	case EAutoGeometryType::ShrinkWrapped:
		return GenerateGeometryFromAlphaChannel();
		
	case EAutoGeometryType::TightBoundingBox:
		return GenerateGeometryFromTightBounds();
		
	case EAutoGeometryType::SourceBoundingBox:
	default:
		return GenerateGeometryFromSourceBounds();
	}
}

bool UManualSprite::GenerateGeometryFromRenderShapes()
{
#if WITH_EDITORONLY_DATA
	// Получаем геометрию рендера напрямую из поля
	const FSpriteGeometryCollection& LocalRenderGeometry = RenderGeometry;
    
	if (LocalRenderGeometry.Shapes.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No RenderShapes found in Paper2D sprite. Please use 'Edit RenderGeom' mode first."));
		return false;
	}

	// Конвертируем RenderGeometry в нашу систему
	ConvertRenderGeometryToManual(LocalRenderGeometry);
    
	// Включаем использование ручной геометрии
	bUseManualGeometry = true;
	(void)MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("Successfully imported %d shapes from Paper2D RenderShapes"), 
		  LocalRenderGeometry.Shapes.Num());
    
	return true;
#else
	UE_LOG(LogTemp, Warning, TEXT("GenerateGeometryFromRenderShapes is only available in editor builds"));
	return false;
#endif
}

void UManualSprite::ConvertRenderGeometryToManual(const FSpriteGeometryCollection& SpriteRenderGeometry)
{
	// Очищаем старую геометрию
	ManualGeometry.Clear();

	// Конвертируем каждый Shape в вершины и треугольники
	for (const FSpriteGeometryShape& Shape : SpriteRenderGeometry.Shapes)
	{
		if (Shape.Vertices.Num() < 3)
			continue; // Пропускаем невалидные фигуры

		// Получаем вершины в текстурном пространстве
		TArray<FVector2D> TextureSpaceVertices;
		Shape.GetTextureSpaceVertices(TextureSpaceVertices);

		const int32 StartVertexIndex = ManualGeometry.Vertices.Num();

		// Конвертируем вершины в наш формат
		for (const FVector2D& TexturePos : TextureSpaceVertices)
		{
			// Конвертируем из текстурного пространства в локальное пространство спрайта
			const FVector2D LocalPos = ConvertTextureSpaceToPivotSpace(TexturePos);
			
			// Вычисляем UV координаты
			const FVector2D UV = CalculateUVFromTexturePosition(TexturePos);
			
			ManualGeometry.Vertices.Add(FManualSpriteVertex(LocalPos, UV));
		}

		// Простая веерная триангуляция для каждого shape
		const int32 VertexCount = TextureSpaceVertices.Num();
		for (int32 i = 1; i < VertexCount - 1; i++)
		{
			ManualGeometry.Triangles.Add(FManualSpriteTriangle(
				StartVertexIndex,
				StartVertexIndex + i,
				StartVertexIndex + i + 1
			));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Converted %d Paper2D shapes into Manual Sprite geometry"), 
		   SpriteRenderGeometry.Shapes.Num());
}

FVector2D UManualSprite::CalculateUVFromTexturePosition(const FVector2D& TexturePos) const
{
	// ИСПРАВЛЕНИЕ: Переименовываем локальные переменные
	const FVector2D LocalSourceSize = GetSourceSize();
	const FVector2D LocalSourceUV = GetSourceUV();
	
	if (LocalSourceSize.IsNearlyZero())
		return FVector2D(0.5f, 0.5f);

	// Нормализуем позицию относительно источника текстуры
	const FVector2D RelativePos = (TexturePos - LocalSourceUV) / LocalSourceSize;
	
	// Ограничиваем UV в диапазоне [0,1]
	return FVector2D(
		FMath::Clamp(RelativePos.X, 0.0f, 1.0f),
		FMath::Clamp(RelativePos.Y, 0.0f, 1.0f)
	);
}

bool UManualSprite::GenerateGeometryFromAlphaChannel()
{
	TArray<uint8> WritableAlpha = GetTextureAlphaData();
	if (WritableAlpha.IsEmpty())
		return false;

	TArray<TArray<FVector2D>> AllContours = FindAllContours(WritableAlpha);
	if (AllContours.IsEmpty())
		return false;

	ManualGeometry.Clear();
	int32 IslandCounter = 0;

	for (const auto& Contour : AllContours)
	{
		if (Contour.Num() < 3)
			continue;

		TArray<FVector2D> SimplifiedContour = SimplifyPoints(Contour, AutoGeometrySettings.SimplifyEpsilon);
		if (SimplifiedContour.Num() < 3)
			continue;

		const int32 StartVertexIndex = ManualGeometry.Vertices.Num();

		// Добавляем вершины
		for (const auto& Point : SimplifiedContour)
		{
			const FVector2D LocalPos = PixelToLocal(FIntPoint(Point.X, Point.Y));
			ManualGeometry.Vertices.Add(FManualSpriteVertex(LocalPos, LocalToUV(LocalPos)));
		}
		
		// Простая веерная триангуляция
		const int32 VertexCount = SimplifiedContour.Num();
		for (int32 i = 1; i < VertexCount - 1; i++)
		{
			ManualGeometry.Triangles.Add(FManualSpriteTriangle(
				StartVertexIndex,
				StartVertexIndex + i,
				StartVertexIndex + i + 1
			));
		}
	}

	bUseManualGeometry = true;
	MarkPackageDirty();
	return ManualGeometry.Vertices.Num() > 0;
}

bool UManualSprite::GenerateGeometryFromTightBounds()
{
	TArray<uint8> AlphaData = GetTextureAlphaData();
	if (AlphaData.IsEmpty())
		return false;
		
	FIntRect TightBox = CalculateTightBoundingBox(AlphaData);
	if (TightBox.Width() <= 0 || TightBox.Height() <= 0)
		return false;
	
	return GenerateQuadGeometry(TightBox);
}

bool UManualSprite::GenerateGeometryFromSourceBounds()
{
	const FVector2D SpriteSize = GetSourceSize();
	if (SpriteSize.IsNearlyZero())
		return false;

	const FIntRect SourceRect(0, 0, SpriteSize.X, SpriteSize.Y);
	return GenerateQuadGeometry(SourceRect);
}

bool UManualSprite::GenerateQuadGeometry(const FIntRect& Bounds)
{
	ManualGeometry.Clear();

	// Создаем 4 вершины
	const FVector2D V0 = PixelToLocal(FIntPoint(Bounds.Min.X, Bounds.Max.Y));
	const FVector2D V1 = PixelToLocal(FIntPoint(Bounds.Max.X, Bounds.Max.Y));
	const FVector2D V2 = PixelToLocal(FIntPoint(Bounds.Max.X, Bounds.Min.Y));
	const FVector2D V3 = PixelToLocal(FIntPoint(Bounds.Min.X, Bounds.Min.Y));
	
	ManualGeometry.Vertices.Add(FManualSpriteVertex(V0, LocalToUV(V0)));
	ManualGeometry.Vertices.Add(FManualSpriteVertex(V1, LocalToUV(V1)));
	ManualGeometry.Vertices.Add(FManualSpriteVertex(V2, LocalToUV(V2)));
	ManualGeometry.Vertices.Add(FManualSpriteVertex(V3, LocalToUV(V3)));
	
	// Создаем 2 треугольника
	ManualGeometry.Triangles.Add(FManualSpriteTriangle(0, 1, 2));
	ManualGeometry.Triangles.Add(FManualSpriteTriangle(0, 2, 3));

	bUseManualGeometry = true;
	MarkPackageDirty();
	return true;
}

TArray<uint8> UManualSprite::GetTextureAlphaData() const
{
	TArray<uint8> AlphaData;
	UTexture2D* MyTexture = GetSourceTexture();
	if (!MyTexture || !MyTexture->GetPlatformData())
		return AlphaData;

	const FTexture2DMipMap& Mip = MyTexture->GetPlatformData()->Mips[0];
	const FByteBulkData& BulkData = Mip.BulkData;
	
	if (BulkData.IsBulkDataLoaded() && MyTexture->GetPixelFormat() == PF_B8G8R8A8)
	{
		const uint8* RawData = static_cast<const uint8*>(BulkData.LockReadOnly());
		if (RawData)
		{
			const int32 NumPixels = MyTexture->GetSizeX() * MyTexture->GetSizeY();
			AlphaData.SetNumUninitialized(NumPixels);
			for (int32 i = 0; i < NumPixels; ++i)
			{
				AlphaData[i] = RawData[i * 4 + 3]; // Альфа канал
			}
		}
		BulkData.Unlock();
	}
	return AlphaData;
}

FIntRect UManualSprite::CalculateTightBoundingBox(const TArray<uint8>& AlphaData) const
{
	const int32 Width = GetSourceTexture()->GetSizeX();
	const int32 Height = GetSourceTexture()->GetSizeY();
	const uint8 AlphaThresholdByte = static_cast<uint8>(AutoGeometrySettings.AlphaThreshold * 255.0f);
	
	FIntRect TightBox(Width, Height, -1, -1);
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			if (AlphaData[Y * Width + X] > AlphaThresholdByte)
			{
				TightBox.Min.X = FMath::Min(TightBox.Min.X, X);
				TightBox.Min.Y = FMath::Min(TightBox.Min.Y, Y);
				TightBox.Max.X = FMath::Max(TightBox.Max.X, X);
				TightBox.Max.Y = FMath::Max(TightBox.Max.Y, Y);
			}
		}
	}
	TightBox.Max.X += 1;
	TightBox.Max.Y += 1;
	return TightBox;
}

TArray<TArray<FVector2D>> UManualSprite::FindAllContours(TArray<uint8>& WritableAlphaData) const
{
	TArray<TArray<FVector2D>> AllContours;
	
	const int32 Width = GetSourceTexture()->GetSizeX();
	const int32 Height = GetSourceTexture()->GetSizeY();
	const uint8 AlphaThresholdByte = static_cast<uint8>(AutoGeometrySettings.AlphaThreshold * 255.0f);
	
	const FIntPoint Offsets[8] = {{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};
	
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			if (WritableAlphaData[Y * Width + X] > AlphaThresholdByte)
			{
				FIntPoint StartPos(X, Y);
				FIntPoint CurrentPos = StartPos;
				FIntPoint PrevPos(X - 1, Y);
				TArray<FVector2D> CurrentContour;
				
				for (int32 i = 0; i < Width * Height * 2; ++i)
				{
					WritableAlphaData[CurrentPos.Y * Width + CurrentPos.X] = 0;
					CurrentContour.Add(FVector2D(CurrentPos));
					
					int32 StartOffset = 0;
					for (int32 j = 0; j < 8; ++j)
					{
						if (PrevPos == CurrentPos + Offsets[j])
						{
							StartOffset = (j + 1) % 8;
							break;
						}
					}
					
					bool bFoundNext = false;
					for (int32 j = 0; j < 8; ++j)
					{
						const FIntPoint NextPos = CurrentPos + Offsets[(StartOffset + j) % 8];
						if (NextPos.X >= 0 && NextPos.X < Width && NextPos.Y >= 0 && NextPos.Y < Height &&
							WritableAlphaData[NextPos.Y * Width + NextPos.X] > AlphaThresholdByte)
						{
							PrevPos = CurrentPos;
							CurrentPos = NextPos;
							bFoundNext = true;
							break;
						}
					}
					if (!bFoundNext || CurrentPos == StartPos) break;
				}
				
				if (CurrentContour.Num() > 2)
				{
					AllContours.Add(CurrentContour);
				}
			}
		}
	}
	return AllContours;
}

TArray<FVector2D> UManualSprite::SimplifyPoints(const TArray<FVector2D>& InPoints, float Epsilon) const
{
	if (InPoints.Num() < 3)
		return InPoints;

	TArray<int32> PointsToKeepIndices;
	PointsToKeepIndices.Add(0);

	const float EpsilonSq = Epsilon * Epsilon;

	for (int32 i = 1; i < InPoints.Num() - 1; ++i)
	{
		const FVector Point = FVector(InPoints[i].X, InPoints[i].Y, 0.0);
		const FVector StartPoint = FVector(InPoints[PointsToKeepIndices.Last()].X, InPoints[PointsToKeepIndices.Last()].Y, 0.0);
		const FVector EndPoint = FVector(InPoints[i+1].X, InPoints[i+1].Y, 0.0);
		
		if (FMath::PointDistToSegmentSquared(Point, StartPoint, EndPoint) > EpsilonSq)
		{
			PointsToKeepIndices.Add(i);
		}
	}
	PointsToKeepIndices.Add(InPoints.Num() - 1);

	TArray<FVector2D> Result;
	for (const int32 Index : PointsToKeepIndices)
	{
		Result.Add(InPoints[Index]);
	}

	return Result;
}

FVector2D UManualSprite::PixelToLocal(const FIntPoint& PixelPos) const
{
	const FVector2D HalfSize(GetSourceSize() * 0.5f);
	return FVector2D(static_cast<float>(PixelPos.X) - HalfSize.X, -(static_cast<float>(PixelPos.Y) - HalfSize.Y));
}

FVector2D UManualSprite::LocalToUV(const FVector2D& LocalPos) const
{
	const FVector2D Size(GetSourceSize());
	if (Size.IsNearlyZero())
		return FVector2D(0.5f, 0.5f);
		
	const FVector2D HalfSize = Size * 0.5f;
	return FVector2D(
		(LocalPos.X + HalfSize.X) / Size.X,
		1.0f - (LocalPos.Y + HalfSize.Y) / Size.Y
	);
}
#endif

void UManualSprite::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	
	// Если включили ручную геометрию, но её нет - создаём базовую
	if (bUseManualGeometry && ManualGeometry.Vertices.Num() == 0)
	{
		GenerateDefaultGeometry();
	}
	
	// Автогенерация при изменении настроек
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UManualSprite, AutoGeometrySettings))
	{
		// Автоматически применяем изменения только для определенных типов
		if (AutoGeometrySettings.GeometryType == EAutoGeometryType::FromRenderShapes)
		{
#if WITH_EDITORONLY_DATA
			GenerateGeometryFromRenderShapes();
#endif
		}
	}
	
	// Валидируем геометрию при любых изменениях
	ValidateGeometry();
}

void UManualSprite::GenerateDefaultGeometry()
{
	ManualGeometry.Clear();
	
	// Получаем размеры спрайта
	FVector2D SpriteSize = GetSourceSize();
	if (SpriteSize.IsNearlyZero())
	{
		SpriteSize = FVector2D(100.0f, 100.0f);
	}
	FVector2D HalfSize = SpriteSize * 0.5f;
	
	// Создаём 4 вершины по углам спрайта
	ManualGeometry.Vertices.Add(FManualSpriteVertex(FVector2D(-HalfSize.X, -HalfSize.Y), FVector2D(0, 1)));
	ManualGeometry.Vertices.Add(FManualSpriteVertex(FVector2D(HalfSize.X, -HalfSize.Y), FVector2D(1, 1)));
	ManualGeometry.Vertices.Add(FManualSpriteVertex(FVector2D(HalfSize.X, HalfSize.Y), FVector2D(1, 0)));
	ManualGeometry.Vertices.Add(FManualSpriteVertex(FVector2D(-HalfSize.X, HalfSize.Y), FVector2D(0, 0)));
	
	// Создаём 2 треугольника для квада
	ManualGeometry.Triangles.Add(FManualSpriteTriangle(0, 1, 2));
	ManualGeometry.Triangles.Add(FManualSpriteTriangle(0, 2, 3));

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
		
		// Если не найдено ни одного уха, прерываем
		if (!bFoundEar)
		{
			UE_LOG(LogTemp, Warning, TEXT("Ear clipping failed"));
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

bool UManualSprite::IsEar(int32 PrevIndex, int32 CurrentIndex, int32 NextIndex) const
{
	const FVector2D Prev = ManualGeometry.Vertices[PrevIndex].Position;
	const FVector2D Current = ManualGeometry.Vertices[CurrentIndex].Position;
	const FVector2D Next = ManualGeometry.Vertices[NextIndex].Position;
	
	// Проверяем, что вершина выпуклая
	const FVector2D Edge1 = Current - Prev;
	const FVector2D Edge2 = Next - Current;
	if (Cross2D(Edge1, Edge2) <= 0.0f)
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
	TArray<int32> HullIndices;
	const int32 VertexCount = ManualGeometry.Vertices.Num();
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