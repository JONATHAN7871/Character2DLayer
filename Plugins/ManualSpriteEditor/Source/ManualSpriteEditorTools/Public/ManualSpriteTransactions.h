#pragma once

#include "CoreMinimal.h"
#include "ManualSprite.h"
#include "Editor.h"

/**
 * Базовый класс для транзакций Manual Sprite
 * v1.1: Добавлены транзакции для автоматической триангуляции
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

// v1.1: Новые транзакции для автоматической триангуляции

/**
 * Транзакция для автоматической триангуляции
 */
class MANUALSPRITEEDITORTOOLS_API FAutoTriangulateTransaction : public FManualSpriteTransaction
{
public:
	FAutoTriangulateTransaction(UManualSprite* InSprite, ETriangulationMethod InMethod);
	
	// Выполнить операцию
	void Execute();
	
private:
	ETriangulationMethod TriangulationMethod;
};

/**
 * Транзакция для очистки треугольников (оставляя вершины)
 */
class MANUALSPRITEEDITORTOOLS_API FClearTrianglesTransaction : public FManualSpriteTransaction
{
public:
	explicit FClearTrianglesTransaction(UManualSprite* InSprite);
	
	// Выполнить операцию
	void Execute();
};

/**
 * Транзакция для сортировки вершин по углу
 */
class MANUALSPRITEEDITORTOOLS_API FSortVerticesByAngleTransaction : public FManualSpriteTransaction
{
public:
	explicit FSortVerticesByAngleTransaction(UManualSprite* InSprite);
	
	// Выполнить операцию
	void Execute();
};

/**
 * Транзакция для обращения порядка вершин
 */
class MANUALSPRITEEDITORTOOLS_API FReverseVertexOrderTransaction : public FManualSpriteTransaction
{
public:
	explicit FReverseVertexOrderTransaction(UManualSprite* InSprite);
	
	// Выполнить операцию
	void Execute();
};