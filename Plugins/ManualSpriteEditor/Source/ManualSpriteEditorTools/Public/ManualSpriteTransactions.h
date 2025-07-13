#pragma once

#include "CoreMinimal.h"
#include "ManualSprite.h"
#include "SpriteEditorOnlyTypes.h"
#include "Editor.h"

/**
 * Базовый класс для транзакций Manual Sprite
 */
class MANUALSPRITEEDITORTOOLS_API FManualSpriteTransaction
{
public:
	explicit FManualSpriteTransaction(UManualSprite* InSprite, const FText& InDescription);
	virtual ~FManualSpriteTransaction();

	// Начать транзакцию
	void BeginTransaction();
	
	// Завершить транзакцию
	void EndTransaction();
	
	// Отменить транзакцию
	void CancelTransaction();

protected:
	UManualSprite* ManualSprite;
	FText Description;
	bool bTransactionActive;
};

/**
 * Транзакция для добавления вершины
 */
class MANUALSPRITEEDITORTOOLS_API FAddVertexTransaction : public FManualSpriteTransaction
{
public:
	FAddVertexTransaction(UManualSprite* InSprite, const FVector2D& Position, const FVector2D& UV);
	
	// Выполнить операцию
	void Execute();
	
private:
	FVector2D VertexPosition;
	FVector2D VertexUV;
};

/**
 * Транзакция для удаления вершины
 */
class MANUALSPRITEEDITORTOOLS_API FRemoveVertexTransaction : public FManualSpriteTransaction
{
public:
	FRemoveVertexTransaction(UManualSprite* InSprite, int32 InVertexIndex);
	
	// Выполнить операцию
	void Execute();
	
private:
	int32 VertexIndex;
};

/**
 * Транзакция для добавления треугольника
 */
class MANUALSPRITEEDITORTOOLS_API FAddTriangleTransaction : public FManualSpriteTransaction
{
public:
	FAddTriangleTransaction(UManualSprite* InSprite, int32 Index0, int32 Index1, int32 Index2);
	
	// Выполнить операцию
	void Execute();
	
private:
	int32 VertexIndex0, VertexIndex1, VertexIndex2;
};

/**
 * Транзакция для удаления треугольника
 */
class MANUALSPRITEEDITORTOOLS_API FRemoveTriangleTransaction : public FManualSpriteTransaction
{
public:
	FRemoveTriangleTransaction(UManualSprite* InSprite, int32 InTriangleIndex);
	
	// Выполнить операцию
	void Execute();
	
private:
	int32 TriangleIndex;
};

/**
 * Транзакция для перемещения вершины
 */
class MANUALSPRITEEDITORTOOLS_API FMoveVertexTransaction : public FManualSpriteTransaction
{
public:
	FMoveVertexTransaction(UManualSprite* InSprite, int32 InVertexIndex, const FVector2D& NewPosition, const FVector2D& NewUV);
	
	// Выполнить операцию
	void Execute();
	
private:
	int32 VertexIndex;
	FVector2D NewVertexPosition;
	FVector2D NewVertexUV;
};

/**
 * Транзакция для перемещения множественных вершин
 */
class MANUALSPRITEEDITORTOOLS_API FMoveVerticesTransaction : public FManualSpriteTransaction
{
public:
	FMoveVerticesTransaction(UManualSprite* InSprite, const TArray<int32>& InVertexIndices, const TArray<FVector2D>& NewPositions, const TArray<FVector2D>& NewUVs);
	
	// Выполнить операцию
	void Execute();
	
private:
	TArray<int32> VertexIndices;
	TArray<FVector2D> NewVertexPositions;
	TArray<FVector2D> NewVertexUVs;
};

/**
 * Транзакция для вставки множественных вершин (для Copy/Paste)
 */
class MANUALSPRITEEDITORTOOLS_API FPasteVerticesTransaction : public FManualSpriteTransaction
{
public:
	FPasteVerticesTransaction(UManualSprite* InSprite, const TArray<FVector2D>& InPositions, const TArray<FVector2D>& InUVs);
	
	// Выполнить операцию
	void Execute();
	
private:
	TArray<FVector2D> Positions;
	TArray<FVector2D> UVs;
};

/**
 * Транзакция для очистки всей геометрии
 */
class MANUALSPRITEEDITORTOOLS_API FClearGeometryTransaction : public FManualSpriteTransaction
{
public:
	explicit FClearGeometryTransaction(UManualSprite* InSprite);
	
	// Выполнить операцию
	void Execute();
};

/**
 * Транзакция для сброса к базовой геометрии
 */
class MANUALSPRITEEDITORTOOLS_API FResetGeometryTransaction : public FManualSpriteTransaction
{
public:
	explicit FResetGeometryTransaction(UManualSprite* InSprite);
	
	// Выполнить операцию
	void Execute();
};

/**
 * Транзакция для импорта геометрии из RenderShapes в ManualGeometry.
 */
class FImportGeometryTransaction : public FScopedTransaction
{
public: // <-- Убедитесь, что это public
	FTextKey LLOCTEXT_NAMESPACE;

	FImportGeometryTransaction(UManualSprite* InSprite)
		: FScopedTransaction(LOCTEXT("ImportRenderGeometryTransaction", "Import Render Geometry"))
		, Sprite(InSprite)
	{
		check(Sprite);
		OldGeometry = Sprite->ManualGeometry;
	}

	// ДЕСТРУКТОР ДОЛЖЕН БЫТЬ PUBLIC
	virtual ~FImportGeometryTransaction()
	{
		if (IsOutstanding())
		{
			Sprite->ManualGeometry = OldGeometry;
			Sprite->PostEditChange();
		}
	}

	void Execute()
	{
#if WITH_EDITOR
		Sprite->Modify();

		// Явно вызываем наш метод Clear(), который очищает оба массива
		Sprite->ManualGeometry.Clear();

		const FVector2D SourceSize = Sprite->GetSourceSize();
		if (SourceSize.IsNearlyZero())
		{
			// ...
			return;
		}

		TArray<FVector2D> ImportedVertices;
		Sprite->GetRenderGeometryVertices(ImportedVertices);

		if (ImportedVertices.Num() == 0)
		{
			// ...
			return;
		}

		for (const FVector2D& VertexPos : ImportedVertices)
		{
			const FVector2D UV = (VertexPos + SourceSize * 0.5f) / SourceSize;
			Sprite->ManualGeometry.Vertices.Add(FManualSpriteVertex(VertexPos, UV));
		}

		// Важно! После импорта вершин массив треугольников должен быть пуст.
		// Убедимся в этом еще раз (хотя Clear() уже должен был это сделать).
		// Sprite->ManualGeometry.Triangles.Empty(); // Эта строка для параноиков, но не повредит
#endif
	}

private: // <-- А вот эти поля могут быть private
	UManualSprite* Sprite;
	FManualSpriteGeometry OldGeometry; 
};

/**
 * Транзакция для импорта ПОЛНОЙ геометрии (вершины + треугольники) из RenderGeometry
 */
class MANUALSPRITEEDITORTOOLS_API FImportGeometryRenderTransaction : public FManualSpriteTransaction
{
public:
	explicit FImportGeometryRenderTransaction(UManualSprite* InSprite);
	
	// Выполнить операцию импорта
	void Execute();
};