#include "ManualSpriteEditorCommands.h"

#define LOCTEXT_NAMESPACE "ManualSpriteEditorCommands"

FManualSpriteEditorCommands::FManualSpriteEditorCommands()
	: TCommands<FManualSpriteEditorCommands>(
		TEXT("ManualSpriteEditor"),
		LOCTEXT("ManualSpriteEditor", "Manual Sprite Editor"),
		NAME_None,
		FAppStyle::GetAppStyleSetName())
{
}

void FManualSpriteEditorCommands::RegisterCommands()
{
	// Режимы редактирования с горячими клавишами
	UI_COMMAND(SelectMode, "Select", "Select and move vertices", EUserInterfaceActionType::RadioButton, FInputChord(EKeys::Q));
	UI_COMMAND(AddVertexMode, "Add Vertex", "Click to add new vertices", EUserInterfaceActionType::RadioButton, FInputChord(EKeys::W));
	UI_COMMAND(TriangleMode, "Triangle", "Select 3 vertices to create a triangle", EUserInterfaceActionType::RadioButton, FInputChord(EKeys::E));
	UI_COMMAND(DeleteMode, "Delete", "Click to delete vertices or triangles", EUserInterfaceActionType::RadioButton, FInputChord(EKeys::R));

	// Undo/Redo с стандартными горячими клавишами
	UI_COMMAND(Undo, "Undo", "Undo the last operation", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::Z));
	UI_COMMAND(Redo, "Redo", "Redo the last undone operation", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::Y));

	// Удаление выделенного
	UI_COMMAND(DeleteSelected, "Delete", "Delete selected vertices or triangles", EUserInterfaceActionType::Button, FInputChord(EKeys::Delete));

	// Копирование/вставка с стандартными горячими клавишами
	UI_COMMAND(Copy, "Copy", "Copy selected vertices to clipboard", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::C));
	UI_COMMAND(Paste, "Paste", "Paste vertices from clipboard", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::V));
	UI_COMMAND(Cut, "Cut", "Cut selected vertices to clipboard", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::X));
	UI_COMMAND(Duplicate, "Duplicate", "Duplicate selected vertices", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::D));

	// Сетка
	UI_COMMAND(ToggleGrid, "Grid", "Show/Hide grid", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::G));
	UI_COMMAND(ToggleSnap, "Snap", "Snap to grid", EUserInterfaceActionType::ToggleButton, FInputChord(EModifierKey::Control, EKeys::G));

	// Утилиты
	UI_COMMAND(ClearAll, "Clear", "Clear all manual geometry", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ResetToDefault, "Reset", "Reset to default quad geometry", EUserInterfaceActionType::Button, FInputChord());

	// Выделение
	UI_COMMAND(SelectAll, "Select All", "Select all vertices", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::A));
	UI_COMMAND(DeselectAll, "Deselect", "Clear selection", EUserInterfaceActionType::Button, FInputChord(EKeys::A, false, true, false, true)); // Shift+Ctrl+A

	// Генерация мешей
	UI_COMMAND(GenerateMesh, "Generate Mesh", "Generate Static or Skeletal Mesh from Manual Sprite geometry", 
			  EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::M));

	// НОВОЕ: Автоматическая триангуляция
	UI_COMMAND(AutoTriangulate, "Auto Triangulate", "Automatically triangulate selected vertices using Delaunay triangulation", 
			  EUserInterfaceActionType::Button, FInputChord(EKeys::Three));

	// Команда для импорта геометрии
	UI_COMMAND(ImportRenderGeometry, "Import Geometry", "Import vertices and triangles from the sprite's Edit Source Region geometry", EUserInterfaceActionType::Button, FInputChord());

	// НОВОЕ: Валидация триангуляции
	UI_COMMAND(ValidateTriangulation, "Validate", "Check triangulation for intersecting edges", 
			  EUserInterfaceActionType::Button, FInputChord(EKeys::V));

	// Удаление треугольников
	UI_COMMAND(DeleteTriangles, "Delete Triangles", "Delete all triangles connected to selected vertices", 
			  EUserInterfaceActionType::Button, FInputChord(EKeys::Four));
}

#undef LOCTEXT_NAMESPACE