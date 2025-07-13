#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Styling/AppStyle.h"

/**
 * Команды для Manual Sprite Editor с поддержкой горячих клавиш и копирования/вставки
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

	// Генерация мешей
	TSharedPtr<FUICommandInfo> GenerateMesh;

	// НОВОЕ: Автоматическая триангуляция
	TSharedPtr<FUICommandInfo> AutoTriangulate;

	// Команда для импорта геометрии
	TSharedPtr<FUICommandInfo> ImportRenderGeometry;

	// Команда валидации пересечений
	TSharedPtr<FUICommandInfo> ValidateTriangulation;

	// Команда удаления треугольников
	TSharedPtr<FUICommandInfo> DeleteTriangles;
};