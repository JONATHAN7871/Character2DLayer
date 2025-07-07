#include "ManualSpriteTransactions.h"
#include "Editor.h" // ИСПРАВЛЕНО: убираем полный путь

#define LOCTEXT_NAMESPACE "ManualSpriteTransactions"

// Base transaction class
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
		// Create scoped transaction for automatic management
		GEditor->BeginTransaction(Description);
		
		// Mark object for modification
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

// Add vertex transaction
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

// Remove vertex transaction
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

// Add triangle transaction
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

// Remove triangle transaction
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

// Move vertex transaction
FMoveVertexTransaction::FMoveVertexTransaction(UManualSprite* InSprite, int32 InVertexIndex, const FVector2D& NewPosition, const FVector2D& NewUV)
	: FManualSpriteTransaction(InSprite, LOCTEXT("MoveVertex", "Move Vertex"))
	, VertexIndex(InVertexIndex)
	, NewVertexPosition(NewPosition)
	, NewVertexUV(NewUV)
{
}

void FMoveVertexTransaction::Execute()
{
	if (!ManualSprite || VertexIndex < 0) 
		return;
	
	TArray<FManualSpriteVertex>& Vertices = ManualSprite->ManualGeometry.Vertices;
	if (VertexIndex >= Vertices.Num())
		return;
	
	BeginTransaction();
	Vertices[VertexIndex].Position = NewVertexPosition;
	Vertices[VertexIndex].UV = NewVertexUV;
	(void)ManualSprite->MarkPackageDirty();
	EndTransaction();
}

// Move multiple vertices transaction
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
	
	TArray<FManualSpriteVertex>& Vertices = ManualSprite->ManualGeometry.Vertices;
	
	for (int32 i = 0; i < VertexIndices.Num(); i++)
	{
		const int32 VertexIndex = VertexIndices[i];
		if (VertexIndex >= 0 && VertexIndex < Vertices.Num())
		{
			Vertices[VertexIndex].Position = NewVertexPositions[i];
			Vertices[VertexIndex].UV = NewVertexUVs[i];
		}
	}
	
	(void)ManualSprite->MarkPackageDirty();
	EndTransaction();
}

// Paste multiple vertices transaction
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
	
	// Add all vertices
	for (int32 i = 0; i < Positions.Num(); i++)
	{
		ManualSprite->AddVertex(Positions[i], UVs[i]);
	}
	
	EndTransaction();
}

// Clear geometry transaction
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

// Reset geometry transaction
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

// v1.1: New transactions for automatic triangulation

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