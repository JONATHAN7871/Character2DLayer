#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Runtime модуль для Manual Sprite Editor
 * Содержит только основные классы, которые нужны в игре
 */
class MANUALSPRITEEDITOR_API FManualSpriteEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};