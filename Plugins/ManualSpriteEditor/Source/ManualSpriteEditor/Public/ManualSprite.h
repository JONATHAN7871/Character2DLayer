#pragma once

#include "CoreMinimal.h"
#include "PaperSprite.h"
#include "Engine/Texture2D.h"
#if WITH_EDITORONLY_DATA
#include "SpriteEditorOnlyTypes.h"
#endif
#include "ManualSprite.generated.h"

// Структура для хранения вершины с UV координатами
USTRUCT(BlueprintType)
struct MANUALSPRITEEDITOR_API FManualSpriteVertex
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertex")
	FVector2D Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertex")
	FVector2D UV;

	FManualSpriteVertex()
		: Position(FVector2D::ZeroVector)
		, UV(FVector2D::ZeroVector)
	{
	}

	FManualSpriteVertex(const FVector2D& InPosition, const FVector2D& InUV)
		: Position(InPosition)
		, UV(InUV)
	{
	}
};

// Структура для хранения треугольника (3 индекса вершин)
USTRUCT(BlueprintType)
struct MANUALSPRITEEDITOR_API FManualSpriteTriangle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Triangle")
	int32 VertexIndex0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Triangle")
	int32 VertexIndex1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Triangle")
	int32 VertexIndex2;

	FManualSpriteTriangle()
		: VertexIndex0(0)
		, VertexIndex1(0)
		, VertexIndex2(0)
	{
	}

	FManualSpriteTriangle(int32 Index0, int32 Index1, int32 Index2)
		: VertexIndex0(Index0)
		, VertexIndex1(Index1)
		, VertexIndex2(Index2)
	{
	}
};

// Перечисление для алгоритмов триангуляции
UENUM(BlueprintType)
enum class ETriangulationMethod : uint8
{
	Fan UMETA(DisplayName = "Fan Triangulation"),
	Delaunay UMETA(DisplayName = "Delaunay Triangulation"),
	ConvexHull UMETA(DisplayName = "Convex Hull"),
	EarClipping UMETA(DisplayName = "Ear Clipping")
};

// Типы автогенерации геометрии
UENUM(BlueprintType)
enum class EAutoGeometryType : uint8
{
	SourceBoundingBox UMETA(DisplayName = "Source Bounding Box"),
	TightBoundingBox UMETA(DisplayName = "Tight Bounding Box"),
	ShrinkWrapped UMETA(DisplayName = "Shrink Wrapped"),
	FromRenderShapes UMETA(DisplayName = "From Paper2D Render Shapes")
};

// Настройки автогенерации
USTRUCT(BlueprintType)
struct MANUALSPRITEEDITOR_API FAutoGeometrySettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Geometry")
	EAutoGeometryType GeometryType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Geometry", 
		meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "GeometryType == EAutoGeometryType::ShrinkWrapped"))
	float AlphaThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Geometry", 
		meta = (ClampMin = "0.0", ClampMax = "10.0", EditCondition = "GeometryType == EAutoGeometryType::ShrinkWrapped"))
	float SimplifyEpsilon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Geometry",
		meta = (EditCondition = "GeometryType == EAutoGeometryType::FromRenderShapes"))
	bool bImportCollisionAsGeometry;

	FAutoGeometrySettings()
		: GeometryType(EAutoGeometryType::FromRenderShapes)
		, AlphaThreshold(0.1f)
		, SimplifyEpsilon(1.5f)
		, bImportCollisionAsGeometry(false)
	{
	}
};

// УПРОЩЕННАЯ структура геометрии - ТОЛЬКО основные массивы
USTRUCT(BlueprintType)
struct MANUALSPRITEEDITOR_API FManualSpriteGeometry
{
	GENERATED_BODY()

	// Основные массивы - ЕДИНСТВЕННЫЙ источник данных
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manual Geometry")
	TArray<FManualSpriteVertex> Vertices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manual Geometry")
	TArray<FManualSpriteTriangle> Triangles;

	// Предпочитаемый метод автоматической триангуляции
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Triangulation")
	ETriangulationMethod PreferredTriangulationMethod;

	FManualSpriteGeometry()
		: PreferredTriangulationMethod(ETriangulationMethod::EarClipping)
	{
	}

	// Проверка валидности геометрии
	bool IsValid() const
	{
		if (Vertices.Num() < 3 || Triangles.Num() < 1)
			return false;

		for (const FManualSpriteTriangle& Triangle : Triangles)
		{
			if (Triangle.VertexIndex0 >= Vertices.Num() || Triangle.VertexIndex0 < 0 ||
				Triangle.VertexIndex1 >= Vertices.Num() || Triangle.VertexIndex1 < 0 ||
				Triangle.VertexIndex2 >= Vertices.Num() || Triangle.VertexIndex2 < 0)
			{
				return false;
			}
		}
		return true;
	}

	// Очистка геометрии
	void Clear()
	{
		Vertices.Empty();
		Triangles.Empty();
	}

	// Очистка только треугольников
	void ClearTriangles()
	{
		Triangles.Empty();
	}

	// Получение количества элементов
	int32 GetVertexCount() const { return Vertices.Num(); }
	int32 GetTriangleCount() const { return Triangles.Num(); }
};

/**
 * Кастомный класс спрайта с поддержкой ручной триангуляции
 * УПРОЩЕННАЯ ВЕРСИЯ - только необходимый функционал
 */
UCLASS(BlueprintType, Blueprintable)
class MANUALSPRITEEDITOR_API UManualSprite : public UPaperSprite
{
	GENERATED_BODY()

public:
	UManualSprite();

	// Использовать ли ручную геометрию вместо автоматической триангуляции
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manual Geometry")
	bool bUseManualGeometry;

	// Ручная геометрия спрайта
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manual Geometry", meta = (EditCondition = "bUseManualGeometry"))
	FManualSpriteGeometry ManualGeometry;

	// Показывать ли отладочную информацию в редакторе
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebugGeometry;

	// Настройки автоматической триангуляции
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Triangulation", meta = (EditCondition = "bUseManualGeometry"))
	bool bShowTriangulationButton;

	// Настройки автогенерации геометрии
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Geometry Generation", 
		meta = (ShowOnlyInnerProperties))
	FAutoGeometrySettings AutoGeometrySettings;

	// ОСНОВНЫЕ ФУНКЦИИ для работы с ручной геометрией
	void AddVertex(const FVector2D& Position, const FVector2D& UV);
	void RemoveVertex(int32 VertexIndex);
	void AddTriangle(int32 VertexIndex0, int32 VertexIndex1, int32 VertexIndex2);
	void RemoveTriangle(int32 TriangleIndex);
	void ClearManualGeometry();
	bool IsManualGeometryValid() const;

	// Функции автоматической триангуляции
	void AutoTriangulate();
	void AutoTriangulateWithMethod(ETriangulationMethod Method);
	void ClearTriangles();
	bool CanAutoTriangulate() const;

	// Утилиты для работы с контуром
	void SortVerticesByAngle();
	bool IsConvexPolygon() const;
	void ReverseVertexOrder();
	FVector2D GetCentroid() const;

	// Функции автогенерации (только для редактора)
#if WITH_EDITORONLY_DATA
	bool GenerateGeometryFromTexture();
	bool GenerateGeometryFromRenderShapes();
#endif

	// Переопределяем для обновления геометрии при изменениях
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

protected:
	// Генерация базовой геометрии (4 вершины по углам спрайта)
	void GenerateDefaultGeometry();

	// Валидация и очистка некорректных треугольников
	void ValidateGeometry();

	// Различные алгоритмы триангуляции
	void TriangulateFan();
	void TriangulateDelaunay();
	void TriangulateConvexHull();
	void TriangulateEarClipping();

	// Вспомогательные функции для триангуляции
	bool IsEar(int32 PrevIndex, int32 CurrentIndex, int32 NextIndex) const;
	bool IsPointInTriangle(const FVector2D& Point, const FVector2D& A, const FVector2D& B, const FVector2D& C) const;
	float Cross2D(const FVector2D& A, const FVector2D& B) const;
	bool IsConvexVertex(int32 PrevIndex, int32 CurrentIndex, int32 NextIndex) const;
	TArray<int32> GetConvexHull() const;

#if WITH_EDITORONLY_DATA
	// Функции для работы с альфа-каналом текстуры
	TArray<uint8> GetTextureAlphaData() const;
	FIntRect CalculateTightBoundingBox(const TArray<uint8>& AlphaData) const;
	TArray<TArray<FVector2D>> FindAllContours(TArray<uint8>& WritableAlphaData) const;
	TArray<FVector2D> SimplifyPoints(const TArray<FVector2D>& InPoints, float Epsilon) const;
	
	// Конвертация координат для автогенерации
	FVector2D PixelToLocal(const FIntPoint& PixelPos) const;
	FVector2D LocalToUV(const FVector2D& LocalPos) const;

	// Импорт из Paper2D RenderShapes
	void ConvertRenderGeometryToManual(const FSpriteGeometryCollection& SpriteRenderGeometry);
	FVector2D CalculateUVFromTexturePosition(const FVector2D& TexturePos) const;
	
	// Функции для различных типов автогенерации
	bool GenerateGeometryFromAlphaChannel();
	bool GenerateGeometryFromTightBounds();
	bool GenerateGeometryFromSourceBounds();
	bool GenerateQuadGeometry(const FIntRect& Bounds);
#endif

private:
	// Превью для автогенерации (transient - не сохраняется)
	UPROPERTY(Transient)
	TArray<FManualSpriteVertex> PreviewVertices;
	
	UPROPERTY(Transient)
	bool bShowingPreview;
};