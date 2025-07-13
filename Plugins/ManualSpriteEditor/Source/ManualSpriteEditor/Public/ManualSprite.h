#pragma once

#include "CoreMinimal.h"
#include "PaperSprite.h"
#include "Engine/Texture2D.h"
#include "ManualSprite.generated.h"

// Структура для хранения вершины с UV координатами
USTRUCT(BlueprintType)
struct MANUALSPRITEEDITOR_API FManualSpriteVertex
{
	GENERATED_BODY()

	// Позиция вершины в локальных координатах спрайта
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertex")
	FVector2D Position;

	// UV координаты для текстуры
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

// Структура для хранения всей ручной геометрии
USTRUCT(BlueprintType)
struct MANUALSPRITEEDITOR_API FManualSpriteGeometry
{
	GENERATED_BODY()

	// Массив всех вершин
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manual Geometry")
	TArray<FManualSpriteVertex> Vertices;

	// Массив треугольников (каждый треугольник содержит 3 индекса вершин)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manual Geometry")
	TArray<FManualSpriteTriangle> Triangles;

	// Проверка валидности геометрии
	bool IsValid() const
	{
		// Проверяем, что все индексы в треугольниках корректны
		for (const FManualSpriteTriangle& Triangle : Triangles)
		{
			if (Triangle.VertexIndex0 >= Vertices.Num() || Triangle.VertexIndex0 < 0 ||
				Triangle.VertexIndex1 >= Vertices.Num() || Triangle.VertexIndex1 < 0 ||
				Triangle.VertexIndex2 >= Vertices.Num() || Triangle.VertexIndex2 < 0)
			{
				return false;
			}
		}
		return Vertices.Num() >= 3 && Triangles.Num() >= 1;
	}

	// Очистка геометрии
	void Clear()
	{
		Vertices.Empty();
		Triangles.Empty();
	}
};

/**
 * Кастомный класс спрайта с поддержкой ручной триангуляции
 */
UCLASS(BlueprintType, Blueprintable)
class MANUALSPRITEEDITOR_API UManualSprite : public UPaperSprite
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	// Возвращает коллекцию фигур рендер-геометрии
	const FSpriteGeometryCollection& GetRenderGeometry() const;
#endif

public:
#if WITH_EDITOR
	/**
	 * Получает вершины из стандартной Render Geometry спрайта.
	 * Этот метод существует только в редакторе.
	 * @param OutVertices - Массив, в который будут записаны позиции вершин.
	 */
	void GetRenderGeometryVertices(TArray<FVector2D>& OutVertices) const;
#endif
	
public:
	UManualSprite();

	// Использовать ли ручную геометрию вместо автоматической триангуляции
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manual Geometry")
	bool bUseManualGeometry;

	// Ручная геометрия спрайта
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manual Geometry", meta = (EditCondition = "bUseManualGeometry"))
	FManualSpriteGeometry ManualGeometry;

	// Функции для работы с ручной геометрией
	UFUNCTION(BlueprintCallable, Category = "Manual Geometry")
	void AddVertex(const FVector2D& Position, const FVector2D& UV);

	UFUNCTION(BlueprintCallable, Category = "Manual Geometry")
	void RemoveVertex(int32 VertexIndex);

	UFUNCTION(BlueprintCallable, Category = "Manual Geometry")
	void AddTriangle(int32 VertexIndex0, int32 VertexIndex1, int32 VertexIndex2);

	UFUNCTION(BlueprintCallable, Category = "Manual Geometry")
	void RemoveTriangle(int32 TriangleIndex);

	UFUNCTION(BlueprintCallable, Category = "Manual Geometry")
	void ClearManualGeometry();

	UFUNCTION(BlueprintCallable, Category = "Manual Geometry")
	bool IsManualGeometryValid() const;

	// Переопределяем для обновления геометрии при изменениях
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

protected:
	// Генерация базовой геометрии (4 вершины по углам спрайта)
	void GenerateDefaultGeometry();

	// Валидация и очистка некорректных треугольников
	void ValidateGeometry();
};