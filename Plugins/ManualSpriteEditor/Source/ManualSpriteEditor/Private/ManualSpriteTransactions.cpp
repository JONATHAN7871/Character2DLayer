#include "ManualSpriteTransactions.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "ManualSpriteTransactions"

// Базовый класс транзакций
FManualSpriteTransaction::FManualSpriteTransaction(UManualSprite* InSprite, const FText& InDescription)
	: ManualSprite(InSprite)
	, Description(InDescription)
	, bTransactionActive(false)
{
}

FManualSpriteTransaction::~FManualSpriteTransaction()
{
	if (bTransactionActive)
	{
		CancelTransaction();
	}
}

void FManualSpriteTransaction::BeginTransaction()
{
	if (!bTransactionActive && ManualSprite)
	{
		// Создаем scoped transaction для автоматического управления
		GEditor->BeginTransaction(Description);
		
		// Помечаем объект для модификации
		ManualSprite->Modify();
		
		bTransactionActive = true;
	}
}

void FManualSpriteTransaction::EndTransaction()
{
	if (bTransactionActive)
	{
		GEditor->EndTransaction();
		bTransactionActive = false;
	}
}

void FManualSpriteTransaction::CancelTransaction()
{
	if (bTransactionActive)
	{
		GEditor->CancelTransaction(0);
		bTransactionActive = false;
	}
}

// Транзакция добавления вершины
FAddVertexTransaction::FAddVertexTransaction(UManualSprite* InSprite, const FVector2D& Position, const FVector2D& UV)
	: FManualSpriteTransaction(InSprite, LOCTEXT("AddVertex", "Add Vertex"))
	, VertexPosition(Position)
	, VertexUV(UV)
{
}

void FAddVertexTransaction::Execute()
{
	if (!ManualSprite) return;
	
	BeginTransaction();
	ManualSprite->AddVertex(VertexPosition, VertexUV);
	EndTransaction();
}

// Транзакция удаления вершины
FRemoveVertexTransaction::FRemoveVertexTransaction(UManualSprite* InSprite, int32 InVertexIndex)
	: FManualSpriteTransaction(InSprite, LOCTEXT("RemoveVertex", "Remove Vertex"))
	, VertexIndex(InVertexIndex)
{
}

void FRemoveVertexTransaction::Execute()
{
	if (!ManualSprite) return;
	
	BeginTransaction();
	ManualSprite->RemoveVertex(VertexIndex);
	EndTransaction();
}

// Транзакция добавления треугольника
FAddTriangleTransaction::FAddTriangleTransaction(UManualSprite* InSprite, int32 Index0, int32 Index1, int32 Index2)
	: FManualSpriteTransaction(InSprite, LOCTEXT("AddTriangle", "Add Triangle"))
	, VertexIndex0(Index0)
	, VertexIndex1(Index1)
	, VertexIndex2(Index2)
{
}

void FAddTriangleTransaction::Execute()
{
	if (!ManualSprite) return;
	
	BeginTransaction();
	ManualSprite->AddTriangle(VertexIndex0, VertexIndex1, VertexIndex2);
	EndTransaction();
}

// Транзакция удаления треугольника
FRemoveTriangleTransaction::FRemoveTriangleTransaction(UManualSprite* InSprite, int32 InTriangleIndex)
	: FManualSpriteTransaction(InSprite, LOCTEXT("RemoveTriangle", "Remove Triangle"))
	, TriangleIndex(InTriangleIndex)
{
}

void FRemoveTriangleTransaction::Execute()
{
	if (!ManualSprite) return;
	
	BeginTransaction();
	ManualSprite->RemoveTriangle(TriangleIndex);
	EndTransaction();
}

// Транзакция перемещения вершины
FMoveVertexTransaction::FMoveVertexTransaction(UManualSprite* InSprite, int32 InVertexIndex, const FVector2D& NewPosition, const FVector2D& NewUV)
	: FManualSpriteTransaction(InSprite, LOCTEXT("MoveVertex", "Move Vertex"))
	, VertexIndex(InVertexIndex)
	, NewVertexPosition(NewPosition)
	, NewVertexUV(NewUV)
{
}

void FMoveVertexTransaction::Execute()
{
	if (!ManualSprite || VertexIndex < 0 || VertexIndex >= ManualSprite->ManualGeometry.Vertices.Num()) 
		return;
	
	BeginTransaction();
	ManualSprite->ManualGeometry.Vertices[VertexIndex].Position = NewVertexPosition;
	ManualSprite->ManualGeometry.Vertices[VertexIndex].UV = NewVertexUV;
	ManualSprite->MarkPackageDirty();
	EndTransaction();
}

// Транзакция перемещения множественных вершин
FMoveVerticesTransaction::FMoveVerticesTransaction(UManualSprite* InSprite, const TArray<int32>& InVertexIndices, const TArray<FVector2D>& NewPositions, const TArray<FVector2D>& NewUVs)
	: FManualSpriteTransaction(InSprite, LOCTEXT("MoveVertices", "Move Vertices"))
	, VertexIndices(InVertexIndices)
	, NewVertexPositions(NewPositions)
	, NewVertexUVs(NewUVs)
{
}

void FMoveVerticesTransaction::Execute()
{
	if (!ManualSprite || VertexIndices.Num() != NewVertexPositions.Num() || VertexIndices.Num() != NewVertexUVs.Num()) 
		return;
	
	BeginTransaction();
	
	// Обновляем позиции всех указанных вершин
	for (int32 i = 0; i < VertexIndices.Num(); i++)
	{
		const int32 VertexIndex = VertexIndices[i];
		if (VertexIndex >= 0 && VertexIndex < ManualSprite->ManualGeometry.Vertices.Num())
		{
			ManualSprite->ManualGeometry.Vertices[VertexIndex].Position = NewVertexPositions[i];
			ManualSprite->ManualGeometry.Vertices[VertexIndex].UV = NewVertexUVs[i];
		}
	}
	
	ManualSprite->MarkPackageDirty();
	EndTransaction();
}

// Транзакция вставки множественных вершин
FPasteVerticesTransaction::FPasteVerticesTransaction(UManualSprite* InSprite, const TArray<FVector2D>& InPositions, const TArray<FVector2D>& InUVs)
	: FManualSpriteTransaction(InSprite, LOCTEXT("PasteVertices", "Paste Vertices"))
	, Positions(InPositions)
	, UVs(InUVs)
{
}

void FPasteVerticesTransaction::Execute()
{
	if (!ManualSprite || Positions.Num() != UVs.Num()) 
		return;
	
	BeginTransaction();
	
	// Добавляем все вершины
	for (int32 i = 0; i < Positions.Num(); i++)
	{
		ManualSprite->AddVertex(Positions[i], UVs[i]);
	}
	
	EndTransaction();
}

// Транзакция очистки геометрии
FClearGeometryTransaction::FClearGeometryTransaction(UManualSprite* InSprite)
	: FManualSpriteTransaction(InSprite, LOCTEXT("ClearGeometry", "Clear All Geometry"))
{
}

void FClearGeometryTransaction::Execute()
{
	if (!ManualSprite) return;
	
	BeginTransaction();
	ManualSprite->ClearManualGeometry();
	EndTransaction();
}

// Транзакция сброса геометрии
FResetGeometryTransaction::FResetGeometryTransaction(UManualSprite* InSprite)
	: FManualSpriteTransaction(InSprite, LOCTEXT("ResetGeometry", "Reset to Default Geometry"))
{
}

void FResetGeometryTransaction::Execute()
{
	if (!ManualSprite) return;
	
	BeginTransaction();
	FPropertyChangedEvent DummyEvent(nullptr);
	ManualSprite->PostEditChangeProperty(DummyEvent);
	EndTransaction();
}

// v1.1: Новые транзакции для автоматической триангуляции

FAutoTriangulateTransaction::FAutoTriangulateTransaction(UManualSprite* InSprite, ETriangulationMethod InMethod)
	: FManualSpriteTransaction(InSprite, LOCTEXT("AutoTriangulate", "Auto Triangulate"))
	, TriangulationMethod(InMethod)
{
}

void FAutoTriangulateTransaction::Execute()
{
	if (!ManualSprite) return;
	
	BeginTransaction();
	ManualSprite->AutoTriangulateWithMethod(TriangulationMethod);
	EndTransaction();
}

FClearTrianglesTransaction::FClearTrianglesTransaction(UManualSprite* InSprite)
	: FManualSpriteTransaction(InSprite, LOCTEXT("ClearTriangles", "Clear Triangles"))
{
}

void FClearTrianglesTransaction::Execute()
{
	if (!ManualSprite) return;
	
	BeginTransaction();
	ManualSprite->ClearTriangles();
	EndTransaction();
}

FSortVerticesByAngleTransaction::FSortVerticesByAngleTransaction(UManualSprite* InSprite)
	: FManualSpriteTransaction(InSprite, LOCTEXT("SortVerticesByAngle", "Sort Vertices by Angle"))
{
}

void FSortVerticesByAngleTransaction::Execute()
{
	if (!ManualSprite) return;
	
	BeginTransaction();
	ManualSprite->SortVerticesByAngle();
	EndTransaction();
}

FReverseVertexOrderTransaction::FReverseVertexOrderTransaction(UManualSprite* InSprite)
	: FManualSpriteTransaction(InSprite, LOCTEXT("ReverseVertexOrder", "Reverse Vertex Order"))
{
}

void FReverseVertexOrderTransaction::Execute()
{
	if (!ManualSprite) return;
	
	BeginTransaction();
	ManualSprite->ReverseVertexOrder();
	EndTransaction();
}

#undef LOCTEXT_NAMESPACE