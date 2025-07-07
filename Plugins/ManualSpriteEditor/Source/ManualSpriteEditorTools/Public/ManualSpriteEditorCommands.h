#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Styling/AppStyle.h"

/**
 * Команды для Manual Sprite Editor с поддержкой горячих клавиш и копирования/вставки
 * v1.1: Добавлены команды автоматической триангуляции
 */
class MANUALSPRITEEDITORTOOLS_API FManualSpriteEditorCommands : public TCommands<FManualSpriteEditorCommands>
{
public:
	FManualSpriteEditorCommands();

	// TCommands<> interface
	virtual void RegisterCommands() override;

	// Режимы редактирования
	TSharedPtr<FUICommandInfo> SelectMode;
	TSharedPtr<FUICommandInfo> AddVertexMode;
	TSharedPtr<FUICommandInfo> TriangleMode;
	TSharedPtr<FUICommandInfo> DeleteMode;

	// Undo/Redo
	TSharedPtr<FUICommandInfo> Undo;
	TSharedPtr<FUICommandInfo> Redo;

	// Удаление выделенного
	TSharedPtr<FUICommandInfo> DeleteSelected;

	// Копирование/вставка
	TSharedPtr<FUICommandInfo> Copy;
	TSharedPtr<FUICommandInfo> Paste;
	TSharedPtr<FUICommandInfo> Cut;
	TSharedPtr<FUICommandInfo> Duplicate;

	// Сетка
	TSharedPtr<FUICommandInfo> ToggleGrid;
	TSharedPtr<FUICommandInfo> ToggleSnap;

	// Утилиты
	TSharedPtr<FUICommandInfo> ClearAll;
	TSharedPtr<FUICommandInfo> ResetToDefault;

	// Выделение
	TSharedPtr<FUICommandInfo> SelectAll;
	TSharedPtr<FUICommandInfo> DeselectAll;

	// v1.1: Автоматическая триангуляция
	TSharedPtr<FUICommandInfo> AutoTriangulate;
	TSharedPtr<FUICommandInfo> ClearTriangles;
	TSharedPtr<FUICommandInfo> TriangulateFan;
	TSharedPtr<FUICommandInfo> TriangulateDelaunay;
	TSharedPtr<FUICommandInfo> TriangulateConvexHull;
	TSharedPtr<FUICommandInfo> TriangulateEarClipping;

	// v1.1: Утилиты для вершин
	TSharedPtr<FUICommandInfo> SortVerticesByAngle;
	TSharedPtr<FUICommandInfo> ReverseVertexOrder;
	TSharedPtr<FUICommandInfo> ShowPolygonInfo;
};