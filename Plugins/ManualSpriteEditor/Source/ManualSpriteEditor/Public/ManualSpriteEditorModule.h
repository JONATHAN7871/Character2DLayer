#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IAssetTypeActions.h"

class FManualSpriteAssetTypeActions;

/**
 * Главный модуль плагина ManualSpriteEditor
 */
class MANUALSPRITEEDITOR_API FManualSpriteEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Регистрация типов ассетов */
	void RegisterAssetTypeActions();
	void UnregisterAssetTypeActions();

	/** Asset type actions для Manual Sprite */
	TSharedPtr<FManualSpriteAssetTypeActions> ManualSpriteAssetTypeActions;
};