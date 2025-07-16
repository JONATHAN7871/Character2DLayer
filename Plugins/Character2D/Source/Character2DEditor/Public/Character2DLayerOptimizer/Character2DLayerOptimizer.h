#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "PaperSprite.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Character2DAsset.h"
#include "SpriteEditorOnlyTypes.h"  // для FSpriteAssetInitParameters
#include "Character2DLayerOptimizer.generated.h"

USTRUCT(BlueprintType)
struct FCharacter2DLayerPositionData
{
    GENERATED_BODY()
    
    // Оригинальная позиция на холсте
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D OriginalCanvasPosition;
    
    // Размер оригинального холста
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D OriginalCanvasSize = FVector2D(2048, 2048);
    
    // Область с реальным содержимым
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntRect ContentBounds;
    
    // Оптимизированная текстура
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UTexture2D> OptimizedTexture = nullptr;
    
    // Вычисленный offset для правильного позиционирования
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D PositionOffset;
    
    FCharacter2DLayerPositionData()
    {
        OriginalCanvasPosition = FVector2D::ZeroVector;
        OriginalCanvasSize = FVector2D(2048, 2048);
        ContentBounds = FIntRect(0, 0, 0, 0);
        PositionOffset = FVector2D::ZeroVector;
    }
    
    // Вычисляет правильную позицию для оптимизированного спрайта
    FVector2D GetCorrectedPosition() const
    {
        return FVector2D(ContentBounds.Min.X, ContentBounds.Min.Y);
    }
    
    // Вычисляет смещение центра обрезанной области относительно центра оригинала
    FVector2D GetCenterOffset() const
    {
        FVector2D OriginalCenter = OriginalCanvasSize * 0.5f;
        FVector2D UsedCenter = FVector2D(
            ContentBounds.Min.X + ContentBounds.Width() * 0.5f,
            ContentBounds.Min.Y + ContentBounds.Height() * 0.5f
        );
        return UsedCenter - OriginalCenter;
    }
};

USTRUCT(BlueprintType)
struct FLayerOptimizationResult
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString LayerName;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UTexture2D> OriginalTexture = nullptr;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntRect UsedRegion;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float OriginalSizeMB = 0.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float OptimizedSizeMB = 0.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SavingsPercent = 0.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UTexture2D> OptimizedTexture = nullptr;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FCharacter2DLayerPositionData PositionData;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UPaperSprite> OptimizedSprite = nullptr;
    
    FLayerOptimizationResult()
    {
        LayerName = TEXT("");
        UsedRegion = FIntRect(0, 0, 0, 0);
    }
};

UCLASS()
class CHARACTER2DEDITOR_API UCharacter2DLayerOptimizer : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
    
public:
    // Основная функция оптимизации
    UFUNCTION(CallInEditor, Category="Character2D Optimization", BlueprintCallable)
    static TArray<FLayerOptimizationResult> OptimizeLayeredCharacter(UCharacter2DAsset* Asset);
    
    // Оптимизация отдельного слоя (новая версия с сохранением)
    UFUNCTION(BlueprintCallable, Category="Character2D Optimization")
    static FLayerOptimizationResult OptimizeLayer(UPaperSprite* LayerSprite, const FString& LayerName, const FString& OptimizedBasePath = TEXT("/Game/Character2D/Optimized"));
    
    // Поиск границ содержимого
    UFUNCTION(BlueprintCallable, Category="Character2D Optimization")
    static FIntRect FindUsedBounds(UTexture2D* Texture);
    
    // Создание и сохранение оптимизированной текстуры как ассета
    UFUNCTION(BlueprintCallable, Category="Character2D Optimization")
    static UTexture2D* CreateAndSaveOptimizedTexture(UTexture2D* SourceTexture, FIntRect UsedRegion, const FString& LayerName, const FString& BasePath);
    
    // Создание и сохранение оптимизированного спрайта как ассета (исправленное позиционирование)
    UFUNCTION(BlueprintCallable, Category="Character2D Optimization")
    static UPaperSprite* CreateAndSaveOptimizedSprite(const FLayerOptimizationResult& OptimizationResult, const FString& BasePath);
    
    // Старые функции для обратной совместимости (создают временные объекты)
    UFUNCTION(BlueprintCallable, Category="Character2D Optimization")
    static UTexture2D* CreateOptimizedTexture(UTexture2D* SourceTexture, FIntRect UsedRegion, const FString& LayerName);
    
    UFUNCTION(BlueprintCallable, Category="Character2D Optimization")
    static UPaperSprite* CreateOptimizedSprite(const FLayerOptimizationResult& OptimizationResult);
    
    // Применение оптимизации к ассету
    UFUNCTION(CallInEditor, Category="Character2D Optimization", BlueprintCallable)
    static void ApplyOptimizationToAsset(UCharacter2DAsset* Asset, const TArray<FLayerOptimizationResult>& OptimizationResults);
    
    // Валидация позиций (с подробным логированием)
    UFUNCTION(BlueprintCallable, Category="Character2D Optimization")
    static bool ValidateOptimizedPositions(UCharacter2DAsset* OriginalAsset, UCharacter2DAsset* OptimizedAsset);
    
    // Отладочная функция для проверки позиционирования спрайта
    UFUNCTION(BlueprintCallable, Category="Character2D Optimization", CallInEditor)
    static void DebugSpritePositioning(UPaperSprite* OriginalSprite, UPaperSprite* OptimizedSprite, const FString& LayerName);
    
    // Вспомогательные функции
    static TArray<FColor> GetTexturePixelData(UTexture2D* Texture);
    static void CopyTextureRegion(UTexture2D* SourceTexture, FIntRect SourceRegion, UTexture2D* DestTexture, FIntPoint DestPosition);
    static UTexture2D* CreateTextureFromPixels(const TArray<FColor>& Pixels, int32 Width, int32 Height, const FString& AssetName);
    static void SaveOptimizedTextureAsAsset(UTexture2D* Texture, const FString& AssetPath);
};