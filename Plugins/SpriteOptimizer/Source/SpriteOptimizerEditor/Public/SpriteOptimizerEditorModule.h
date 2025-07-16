#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSpriteOptimizerEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Инициализация UI расширений */
	void InitializeMenuExtensions();
	
	/** Очистка UI расширений */
	void ShutdownMenuExtensions();
};