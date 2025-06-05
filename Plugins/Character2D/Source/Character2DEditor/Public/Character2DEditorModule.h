#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FAssetTypeActions_Character2DAsset;

class FCharacter2DEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void OpenCharacter2DBuilder();

	TSharedPtr<FAssetTypeActions_Character2DAsset> Character2DAssetActions;
	uint32 Character2DAssetCategory;

private:
	void RegisterMenus();
	void RegisterMenus_Internal();

	TSharedPtr<class FUICommandList> PluginCommands;
};
