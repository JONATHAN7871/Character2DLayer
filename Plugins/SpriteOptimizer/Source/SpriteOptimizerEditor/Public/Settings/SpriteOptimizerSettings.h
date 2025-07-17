#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Materials/MaterialInterface.h"
#include "SpriteOptimizerSettings.generated.h"

/**
 * Настройки плагина Sprite Optimizer
 */
UCLASS(config=EditorPerProjectUserSettings, meta=(DisplayName="Sprite Optimizer"))
class SPRITEOPTIMIZEREDITOR_API USpriteOptimizerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USpriteOptimizerSettings();

	// Материал по умолчанию для оптимизированных спрайтов
	UPROPERTY(config, EditAnywhere, Category = "Default Settings", meta = (AllowedClasses = "/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UMaterialInterface> DefaultMaterial;

	// Pixels Per Unit по умолчанию
	UPROPERTY(config, EditAnywhere, Category = "Default Settings", meta = (ClampMin = "0.1", ClampMax = "100.0"))
	float DefaultPixelsPerUnit = 1.0f;

	// Padding по умолчанию (в пикселях)
	UPROPERTY(config, EditAnywhere, Category = "Default Settings", meta = (ClampMin = "0", ClampMax = "20"))
	int32 DefaultPadding = 2;

	// Создавать backup по умолчанию
	UPROPERTY(config, EditAnywhere, Category = "Default Settings")
	bool bDefaultCreateBackup = true;

	// Заменять оригиналы по умолчанию
	UPROPERTY(config, EditAnywhere, Category = "Default Settings")
	bool bDefaultReplaceOriginals = false;

	// Минимальная экономия для автоматического выбора спрайтов (в процентах)
	UPROPERTY(config, EditAnywhere, Category = "Auto Selection", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float MinimumSavingsForAutoSelect = 30.0f;

	// Автоматически выбирать только спрайты с хорошим потенциалом оптимизации
	UPROPERTY(config, EditAnywhere, Category = "Auto Selection")
	bool bAutoSelectOptimalSpritesOnly = true;

	// Показывать подробные логи в Output Log
	UPROPERTY(config, EditAnywhere, Category = "Debug")
	bool bShowDetailedLogs = true;

	// Показывать уведомления об успешной оптимизации
	UPROPERTY(config, EditAnywhere, Category = "Debug")
	bool bShowOptimizationNotifications = true;

	// Автоматически обновлять Content Browser после оптимизации
	UPROPERTY(config, EditAnywhere, Category = "Workflow")
	bool bAutoRefreshContentBrowser = true;

	// Путь для сохранения оптимизированных ассетов (если пустой - рядом с оригиналом)
	UPROPERTY(config, EditAnywhere, Category = "Workflow")
	FString OptimizedAssetsPath;

	// Суффикс для имен оптимизированных ассетов
	UPROPERTY(config, EditAnywhere, Category = "Workflow")
	FString OptimizedAssetsSuffix = TEXT("_Optimized");

public:
	// UDeveloperSettings interface
	virtual FName GetCategoryName() const override;
	virtual FText GetSectionText() const override;

	// === ATLAS SETTINGS ===
	UPROPERTY(config, EditAnywhere, Category = "Atlas Settings", meta = (ClampMin = "256", ClampMax = "8192"))
	FIntPoint DefaultMaxAtlasSize = FIntPoint(2048, 2048);

	UPROPERTY(config, EditAnywhere, Category = "Atlas Settings", meta = (ClampMin = "0", ClampMax = "20"))
	int32 DefaultAtlasSpritePadding = 2;

	UPROPERTY(config, EditAnywhere, Category = "Atlas Settings")
	bool bDefaultOptimizeSpritesForAtlas = true;

	UPROPERTY(config, EditAnywhere, Category = "Atlas Settings")
	bool bDefaultCreateIndividualSprites = true;

	UPROPERTY(config, EditAnywhere, Category = "Atlas Settings")
	FString DefaultAtlasSuffix = TEXT("_Atlas");

#if WITH_EDITOR
	virtual FText GetSectionDescription() const override;
#endif
};