#include "Settings/SpriteOptimizerSettings.h"

#define LOCTEXT_NAMESPACE "SpriteOptimizerSettings"

USpriteOptimizerSettings::USpriteOptimizerSettings()
{
	// Set default settings
	DefaultMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Paper2D/DefaultSpriteMaterial.DefaultSpriteMaterial")));
	DefaultPixelsPerUnit = 1.0f;
	DefaultPadding = 2;
	bDefaultCreateBackup = true;
	bDefaultReplaceOriginals = false;
	MinimumSavingsForAutoSelect = 30.0f;
	bAutoSelectOptimalSpritesOnly = true;
	bShowDetailedLogs = true;
	bShowOptimizationNotifications = true;
	bAutoRefreshContentBrowser = true;
	OptimizedAssetsPath = TEXT("");
	OptimizedAssetsSuffix = TEXT("_Optimized");
}

FName USpriteOptimizerSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

FText USpriteOptimizerSettings::GetSectionText() const
{
	return LOCTEXT("SpriteOptimizerSettingsSection", "Sprite Optimizer");
}

#if WITH_EDITOR
FText USpriteOptimizerSettings::GetSectionDescription() const
{
	return LOCTEXT("SpriteOptimizerSettingsDescription", "Configure settings for the Sprite Optimizer plugin");
}
#endif

#undef LOCTEXT_NAMESPACE