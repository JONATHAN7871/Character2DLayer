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
};