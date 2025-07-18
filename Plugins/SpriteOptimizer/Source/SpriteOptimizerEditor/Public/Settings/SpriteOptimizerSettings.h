// Copyright 2025, CRAFTCODE, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Materials/MaterialInterface.h"
#include "SpriteOptimizerSettings.generated.h"

/**
 * Settings for Sprite Optimizer plugin
 */
UCLASS(config=EditorPerProjectUserSettings, meta=(DisplayName="Sprite Optimizer"))
class SPRITEOPTIMIZEREDITOR_API USpriteOptimizerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USpriteOptimizerSettings();

	// === DEFAULT OPTIMIZATION SETTINGS ===
	
	// Default material for optimized sprites
	UPROPERTY(config, EditAnywhere, Category = "Default Settings", meta = (AllowedClasses = "/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UMaterialInterface> DefaultMaterial;

	// Default pixels per unit
	UPROPERTY(config, EditAnywhere, Category = "Default Settings", meta = (ClampMin = "0.1", ClampMax = "100.0"))
	float DefaultPixelsPerUnit = 1.0f;

	// Default padding in pixels
	UPROPERTY(config, EditAnywhere, Category = "Default Settings", meta = (ClampMin = "0", ClampMax = "20"))
	int32 DefaultPadding = 2;

	// Create backup by default
	UPROPERTY(config, EditAnywhere, Category = "Default Settings")
	bool bDefaultCreateBackup = true;

	// Replace originals by default
	UPROPERTY(config, EditAnywhere, Category = "Default Settings")
	bool bDefaultReplaceOriginals = false;

	// === AUTO SELECTION SETTINGS ===
	
	// Minimum savings percentage for auto-selection
	UPROPERTY(config, EditAnywhere, Category = "Auto Selection", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float MinimumSavingsForAutoSelect = 30.0f;

	// Auto-select only sprites with good optimization potential
	UPROPERTY(config, EditAnywhere, Category = "Auto Selection")
	bool bAutoSelectOptimalSpritesOnly = true;

	// === ATLAS DEFAULT SETTINGS ===
	
	// Default maximum atlas size
	UPROPERTY(config, EditAnywhere, Category = "Atlas Settings", meta = (ClampMin = "256", ClampMax = "8192"))
	FIntPoint DefaultMaxAtlasSize = FIntPoint(2048, 2048);

	// Default sprite padding in atlas
	UPROPERTY(config, EditAnywhere, Category = "Atlas Settings", meta = (ClampMin = "0", ClampMax = "20"))
	int32 DefaultAtlasSpritePadding = 2;

	// Optimize sprites before atlas creation by default
	UPROPERTY(config, EditAnywhere, Category = "Atlas Settings")
	bool bDefaultOptimizeSpritesForAtlas = true;

	// Create individual sprites from atlas by default
	UPROPERTY(config, EditAnywhere, Category = "Atlas Settings")
	bool bDefaultCreateIndividualSprites = true;

	// Default atlas suffix
	UPROPERTY(config, EditAnywhere, Category = "Atlas Settings")
	FString DefaultAtlasSuffix = TEXT("_Atlas");

	// === WORKFLOW SETTINGS ===
	
	// Show detailed logs in Output Log
	UPROPERTY(config, EditAnywhere, Category = "Workflow")
	bool bShowDetailedLogs = true;

	// Show optimization notifications
	UPROPERTY(config, EditAnywhere, Category = "Workflow")
	bool bShowOptimizationNotifications = true;

	// Auto-refresh Content Browser after optimization
	UPROPERTY(config, EditAnywhere, Category = "Workflow")
	bool bAutoRefreshContentBrowser = true;

	// Path for optimized assets (empty = next to original)
	UPROPERTY(config, EditAnywhere, Category = "Workflow")
	FString OptimizedAssetsPath;

	// Suffix for optimized asset names
	UPROPERTY(config, EditAnywhere, Category = "Workflow")
	FString OptimizedAssetsSuffix = TEXT("_Optimized");

public:
	// UDeveloperSettings interface
	virtual FName GetCategoryName() const override;
	virtual FText GetSectionText() const override;

#if WITH_EDITOR
	virtual FText GetSectionDescription() const override;
#endif
};