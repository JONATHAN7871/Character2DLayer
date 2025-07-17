#pragma once

#include "CoreMinimal.h"
#include "PaperSprite.h"
#include "SpriteEditorOnlyTypes.h"
#include "Engine/Texture2D.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Runtime/Engine/Classes/Kismet/BlueprintFunctionLibrary.h"
#include "SpriteOptimizer.generated.h"

class UPaperSprite;

USTRUCT(BlueprintType)
struct FSpriteOptimizationSettings
{
    GENERATED_BODY()
    
    // Материал для оптимизированных спрайтов
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
    TObjectPtr<UMaterialInterface> Material = nullptr;
    
    // Pixels per unit для оптимизированных спрайтов
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ClampMin = "0.1", ClampMax = "100.0"))
    float PixelsPerUnit = 1.0f;
    
    // Padding для обрезки (в пикселях)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ClampMin = "0", ClampMax = "20"))
    int32 Padding = 2;
    
    // Создавать backup оригинальных файлов
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bCreateBackup = true;
    
    // Заменить оригинальные файлы оптимизированными
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bReplaceOriginals = false;
    
    // Использовать настройки из проекта
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bUseProjectSettings = true;
    
    FSpriteOptimizationSettings()
    {
        PixelsPerUnit = 1.0f;
        Padding = 2;
        bCreateBackup = true;
        bReplaceOriginals = false;
        bUseProjectSettings = true;
    }
    
    // Загружает настройки из конфигурации проекта
    void LoadFromProjectSettings();
};

USTRUCT(BlueprintType)
struct FSpriteOptimizationResult
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString SpriteName;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UPaperSprite> OriginalSprite = nullptr;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UPaperSprite> OptimizedSprite = nullptr;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UTexture2D> OriginalTexture = nullptr;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UTexture2D> OptimizedTexture = nullptr;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntRect UsedRegion;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D OriginalSize;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D OptimizedSize;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float OriginalSizeMB = 0.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float OptimizedSizeMB = 0.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SavingsPercent = 0.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float UsagePercent = 0.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bSuccess = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ErrorMessage;
    
    // Пути к созданным ассетам
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString OptimizedTexturePath;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString OptimizedSpritePath;
    
    FSpriteOptimizationResult()
    {
        OriginalSize = FVector2D::ZeroVector;
        OptimizedSize = FVector2D::ZeroVector;
        UsedRegion = FIntRect(0, 0, 0, 0);
    }
    
    void CalculateStats()
    {
        if (OriginalTexture && OptimizedTexture)
        {
            int32 OriginalPixels = OriginalTexture->GetSizeX() * OriginalTexture->GetSizeY();
            int32 OptimizedPixels = OptimizedTexture->GetSizeX() * OptimizedTexture->GetSizeY();
            
            OriginalSizeMB = (OriginalPixels * 4) / (1024.0f * 1024.0f);
            OptimizedSizeMB = (OptimizedPixels * 4) / (1024.0f * 1024.0f);
            SavingsPercent = OriginalSizeMB > 0 ? ((OriginalSizeMB - OptimizedSizeMB) / OriginalSizeMB) * 100.0f : 0.0f;
            UsagePercent = OriginalPixels > 0 ? (static_cast<float>(OptimizedPixels) / OriginalPixels) * 100.0f : 0.0f;
            
            OriginalSize = FVector2D(OriginalTexture->GetSizeX(), OriginalTexture->GetSizeY());
            OptimizedSize = FVector2D(OptimizedTexture->GetSizeX(), OptimizedTexture->GetSizeY());
        }
    }
};

UENUM(BlueprintType)
enum class EAtlasPackingAlgorithm : uint8
{
    Simple      UMETA(DisplayName = "Simple Grid"),
    BestFit     UMETA(DisplayName = "Best Fit"),
    MaxRects    UMETA(DisplayName = "MaxRects Algorithm")
};

USTRUCT(BlueprintType)
struct SPRITEOPTIMIZEREDITOR_API FSpriteAtlasSettings
{
    GENERATED_BODY()
    
    // Максимальный размер атласа
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas", meta = (ClampMin = "256", ClampMax = "8192"))
    FIntPoint MaxAtlasSize = FIntPoint(2048, 2048);
    
    // Отступ между спрайтами в атласе
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas", meta = (ClampMin = "0", ClampMax = "20"))
    int32 SpritePadding = 2;
    
    // Алгоритм упаковки
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas")
    EAtlasPackingAlgorithm PackingAlgorithm = EAtlasPackingAlgorithm::Simple;
    
    // Создавать отдельные спрайты или один большой
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas")
    bool bCreateIndividualSprites = true;
    
    // Суффикс для атласа
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas")
    FString AtlasSuffix = TEXT("_Atlas");
    
    // Оптимизировать спрайты перед созданием атласа
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas")
    bool bOptimizeSpritesFirst = true;
    
    FSpriteAtlasSettings()
    {
        MaxAtlasSize = FIntPoint(2048, 2048);
        SpritePadding = 2;
        PackingAlgorithm = EAtlasPackingAlgorithm::Simple;
        bCreateIndividualSprites = true;
        AtlasSuffix = TEXT("_Atlas");
        bOptimizeSpritesFirst = true;
    }
};

USTRUCT(BlueprintType)
struct SPRITEOPTIMIZEREDITOR_API FSpriteAtlasResult
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UTexture2D> AtlasTexture = nullptr;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<TObjectPtr<UPaperSprite>> CreatedSprites;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FIntRect> SpriteRegions;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntPoint AtlasSize;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PackingEfficiency = 0.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TotalSprites = 0;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MemorySavings = 0.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bSuccess = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ErrorMessage;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString AtlasTexturePath;
    
    FSpriteAtlasResult()
    {
        AtlasSize = FIntPoint::ZeroValue;
        PackingEfficiency = 0.0f;
        TotalSprites = 0;
        MemorySavings = 0.0f;
        bSuccess = false;
    }
};

UCLASS()
class SPRITEOPTIMIZEREDITOR_API USpriteOptimizer : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Основная функция оптимизации множественных спрайтов
    UFUNCTION(CallInEditor, Category="Sprite Optimization", BlueprintCallable)
    static TArray<FSpriteOptimizationResult> OptimizeSprites(
        const TArray<UPaperSprite*>& Sprites, 
        const FSpriteOptimizationSettings& Settings = FSpriteOptimizationSettings()
    );
    
    // Оптимизация одного спрайта
    UFUNCTION(BlueprintCallable, Category="Sprite Optimization")
    static FSpriteOptimizationResult OptimizeSingleSprite(
        UPaperSprite* Sprite, 
        const FSpriteOptimizationSettings& Settings = FSpriteOptimizationSettings()
    );
    
    // Анализ спрайта без создания оптимизированной версии
    UFUNCTION(BlueprintCallable, Category="Sprite Optimization")
    static FSpriteOptimizationResult AnalyzeSprite(UPaperSprite* Sprite);
    
    // Поиск использованной области в текстуре
    UFUNCTION(BlueprintCallable, Category="Sprite Optimization")
    static FIntRect FindUsedBounds(UTexture2D* Texture, int32 Padding = 2);
    
    // Получение доступных материалов Paper2D
    UFUNCTION(BlueprintCallable, Category="Sprite Optimization")
    static TArray<UMaterialInterface*> GetAvailablePaper2DMaterials();
    
    // Получение стандартного материала Paper2D
    UFUNCTION(BlueprintCallable, Category="Sprite Optimization")
    static UMaterialInterface* GetDefaultPaper2DMaterial();
    
    // Создание оптимизированной текстуры
    static UTexture2D* CreateOptimizedTexture(
        UTexture2D* SourceTexture, 
        FIntRect UsedRegion, 
        const FString& AssetName, 
        const FString& AssetPath
    );
    
    // Создание оптимизированного спрайта
    static UPaperSprite* CreateOptimizedSprite(
        UTexture2D* OptimizedTexture,
        UPaperSprite* OriginalSprite,
        const FIntRect& UsedRegion,
        const FSpriteOptimizationSettings& Settings,
        const FString& AssetName,
        const FString& AssetPath
    );
    
    // Вспомогательные функции
    static TArray<FColor> GetTexturePixelData(UTexture2D* Texture);
    static FString GetOptimizedAssetPath(const FString& OriginalPath, const FSpriteOptimizationSettings& Settings);
    static FString GetOptimizedAssetName(const FString& OriginalName, const FSpriteOptimizationSettings& Settings);
    static void CreateBackupIfNeeded(UObject* Asset, bool bCreateBackup);
    static void RefreshContentBrowser();
    static void ShowOptimizationNotification(const FText& Message, bool bSuccess = true);
    
    // НОВЫЕ МЕТОДЫ ДЛЯ РАБОТЫ С ПАПКАМИ
    static FString GetAtlasAssetPath(const FString& FirstSpritePath);
    static bool EnsureDirectoryExists(const FString& DirectoryPath);

public:
    // === ATLAS METHODS ===
    
    // Создание атласа из множества спрайтов
    UFUNCTION(CallInEditor, Category="Sprite Atlas", BlueprintCallable)
    static FSpriteAtlasResult CreateSpriteAtlas(
        const TArray<UPaperSprite*>& Sprites,
        const FSpriteAtlasSettings& Settings = FSpriteAtlasSettings(),
        const FString& AtlasName = TEXT("SpriteAtlas"),
        const FString& AtlasPath = TEXT("")
    );
    
    // Анализ возможности создания атласа
    UFUNCTION(BlueprintCallable, Category="Sprite Atlas")
    static FSpriteAtlasResult AnalyzeSpriteAtlas(
        const TArray<UPaperSprite*>& Sprites,
        const FSpriteAtlasSettings& Settings = FSpriteAtlasSettings()
    );

private:
    // === ATLAS HELPER METHODS ===
    
    // Алгоритмы упаковки
    static TArray<FIntRect> PackSprites_Simple(
        const TArray<FIntPoint>& SpriteSizes, 
        const FSpriteAtlasSettings& Settings, 
        FIntPoint& OutAtlasSize
    );
    
    static TArray<FIntRect> PackSprites_BestFit(
        const TArray<FIntPoint>& SpriteSizes, 
        const FSpriteAtlasSettings& Settings, 
        FIntPoint& OutAtlasSize
    );
    
    static TArray<FIntRect> PackSprites_MaxRects(
        const TArray<FIntPoint>& SpriteSizes, 
        const FSpriteAtlasSettings& Settings, 
        FIntPoint& OutAtlasSize
    );
    
    // Создание атласной текстуры
    static UTexture2D* CreateAtlasTexture(
        const TArray<UPaperSprite*>& Sprites,
        const TArray<FIntRect>& SpriteRegions,
        const FIntPoint& AtlasSize,
        const FString& AssetName,
        const FString& AssetPath
    );
    
    // Создание спрайта из атласа
    static UPaperSprite* CreateSpriteFromAtlas(
        UTexture2D* AtlasTexture,
        const FIntRect& Region,
        UPaperSprite* OriginalSprite,
        const FString& SpriteName,
        const FString& AssetPath
    );
    
    // Вычисление эффективности упаковки
    static float CalculatePackingEfficiency(
        const TArray<FIntPoint>& SpriteSizes, 
        const FIntPoint& AtlasSize
    );
    
    // Получение оптимизированных размеров спрайтов
    static TArray<FIntPoint> GetOptimizedSpriteSizes(
        const TArray<UPaperSprite*>& Sprites,
        int32 Padding = 2
    );

    static bool CopyPixelsFromSourceToAtlas(
        UTexture2D* SourceTexture, 
        TArray<FColor>& AtlasPixels, 
        const FIntRect& Region, 
        const FIntPoint& AtlasSize
    );

    static FVector2D CalculateAtlasPivotForLayering(
        int32 OriginalTextureWidth,
        int32 OriginalTextureHeight, 
        const FIntRect& OriginalUsedRegion,
        const FIntRect& AtlasRegion,
        UPaperSprite* OriginalSprite
    );
};