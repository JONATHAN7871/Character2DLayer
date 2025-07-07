#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IAssetTypeActions.h"

class FManualSpriteAssetTypeActions;

/**
 * Manual Sprite Editor Tools Module - ОЧИЩЕННАЯ ВЕРСИЯ
 * 
 * Предоставляет полнофункциональный редактор Manual Sprite:
 * - Независимый редактор ассетов для Manual Sprite
 * - Профессиональный UI с тулбаром и viewport
 * - Полные возможности редактирования геометрии
 * - Автотриангуляция с 4 различными алгоритмами
 * - Copy/Paste, Undo/Redo функциональность
 * - Выделение и манипуляция множественных вершин
 * - Сетка с привязкой
 * - Превью спрайта в реальном времени и расчет UV
 * 
 * Использование:
 * 1. Content Browser → Right Click → Manual Sprite
 * 2. Двойной клик по ассету Manual Sprite для открытия редактора
 * 3. Установите текстуру и включите 'Use Manual Geometry'
 * 4. Используйте горячие клавиши Q/W/E/R/T для редактирования
 */
class MANUALSPRITEEDITORTOOLS_API FManualSpriteEditorToolsModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Регистрация типов ассетов Manual Sprite в Unreal Editor */
	void RegisterAssetTypeActions();
	
	/** Отмена регистрации типов ассетов при закрытии модуля */
	void UnregisterAssetTypeActions();

	/** Показать сообщение об успешной инициализации */
	void ShowSuccessMessage();
	
	/** Отобразить подробные инструкции по использованию в логе */
	void ShowUsageInstructions();

	/** Asset type actions для ассетов Manual Sprite */
	TSharedPtr<FManualSpriteAssetTypeActions> ManualSpriteAssetTypeActions;
};