// Copyright 2025, CRAFTCODE, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PaperSprite.h"
#include "Engine/Texture2D.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Runtime/Engine/Classes/Kismet/BlueprintFunctionLibrary.h"
#include "SpriteOptimizer.generated.h"

class UPaperSprite;

// Settings structure for sprite optimization operations
USTRUCT(BlueprintType)
struct FSpriteOptimizationSettings
{
    GENERATED_BODY()
    
    // Material to apply to optimized sprites
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
    TObjectPtr<UMaterialInterface> Material = nullptr;
    
    // Pixels per unit value for optimized sprites
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ClampMin = "0.1", ClampMax = "100.0"))
    float PixelsPerUnit = 1.0f;
    
    // Padding around sprite content in pixels
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ClampMin = "0", ClampMax = "20"))
    int32 Padding = 2;
    
    // Whether to create backup copies of original files
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bCreateBackup = true;
    
    // Whether to replace original files with optimized versions
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bReplaceOriginals = false;
    
    // Whether to use project-wide settings
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
    
    // Loads settings from project configuration
    void LoadFromProjectSettings();
};

// Result of sprite optimization operation
USTRUCT(BlueprintType)
struct FSpriteOptimizationResult
{
    GENERATED_BODY()
    
    // Name of the processed sprite
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString SpriteName;
    
    // Reference to original sprite
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UPaperSprite> OriginalSprite = nullptr;
    
    // Reference to created optimized sprite
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UPaperSprite> OptimizedSprite = nullptr;
    
    // Reference to original texture
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UTexture2D> OriginalTexture = nullptr;
    
    // Reference to created optimized texture
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UTexture2D> OptimizedTexture = nullptr;
    
    // Bounds of used area in original texture
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntRect UsedRegion;
    
    // Size of original texture
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D OriginalSize;
    
    // Size of optimized texture
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D OptimizedSize;
    
    // Memory usage of original texture in MB
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float OriginalSizeMB = 0.0f;
    
    // Memory usage of optimized texture in MB
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float OptimizedSizeMB = 0.0f;
    
    // Memory savings percentage
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SavingsPercent = 0.0f;
    
    // Percentage of original texture actually used
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float UsagePercent = 0.0f;
    
    // Whether optimization was successful
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bSuccess = false;
    
    // Error message if optimization failed
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ErrorMessage;
    
    // Path to created optimized texture asset
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString OptimizedTexturePath;
    
    // Path to created optimized sprite asset
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString OptimizedSpritePath;
    
    FSpriteOptimizationResult()
    {
        OriginalSize = FVector2D::ZeroVector;
        OptimizedSize = FVector2D::ZeroVector;
        UsedRegion = FIntRect(0, 0, 0, 0);
    }
    
    // Calculates statistics from texture data
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

// Available atlas packing algorithms
UENUM(BlueprintType)
enum class EAtlasPackingAlgorithm : uint8
{
    Simple      UMETA(DisplayName = "Simple Grid"),
    BestFit     UMETA(DisplayName = "Best Fit"),
    MaxRects    UMETA(DisplayName = "MaxRects Algorithm")
};

// Settings for sprite atlas creation
USTRUCT(BlueprintType)
struct SPRITEOPTIMIZEREDITOR_API FSpriteAtlasSettings
{
    GENERATED_BODY()
    
    // Maximum size of generated atlas texture
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas", meta = (ClampMin = "256", ClampMax = "8192"))
    FIntPoint MaxAtlasSize = FIntPoint(2048, 2048);
    
    // Padding between sprites in atlas
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas", meta = (ClampMin = "0", ClampMax = "20"))
    int32 SpritePadding = 2;
    
    // Algorithm used for packing sprites into atlas
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas")
    EAtlasPackingAlgorithm PackingAlgorithm = EAtlasPackingAlgorithm::Simple;
    
    // Whether to create individual sprite assets from atlas
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas")
    bool bCreateIndividualSprites = true;
    
    // Suffix for atlas asset names
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas")
    FString AtlasSuffix = TEXT("_Atlas");
    
    // Whether to optimize sprites before creating atlas
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas")
    bool bOptimizeSpritesFirst = true;
    
    // Whether to preserve original texture quality settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas Quality")
    bool bPreserveOriginalQuality = true;
    
    // Whether to force smooth filtering
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas Quality")
    bool bForceSmoothing = false;
    
    FSpriteAtlasSettings()
    {
        MaxAtlasSize = FIntPoint(2048, 2048);
        SpritePadding = 2;
        PackingAlgorithm = EAtlasPackingAlgorithm::Simple;
        bCreateIndividualSprites = true;
        AtlasSuffix = TEXT("_Atlas");
        bOptimizeSpritesFirst = true;
        bPreserveOriginalQuality = true;
        bForceSmoothing = false;
    }
};

// Result of sprite atlas creation
USTRUCT(BlueprintType)
struct SPRITEOPTIMIZEREDITOR_API FSpriteAtlasResult
{
    GENERATED_BODY()
    
    // Generated atlas texture
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UTexture2D> AtlasTexture = nullptr;
    
    // Individual sprites created from atlas
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<TObjectPtr<UPaperSprite>> CreatedSprites;
    
    // Regions occupied by each sprite in atlas
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FIntRect> SpriteRegions;
    
    // Final size of atlas texture
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntPoint AtlasSize;
    
    // Packing efficiency percentage
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PackingEfficiency = 0.0f;
    
    // Number of sprites processed
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TotalSprites = 0;
    
    // Memory savings percentage
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MemorySavings = 0.0f;
    
    // Whether atlas creation was successful
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bSuccess = false;
    
    // Error message if creation failed
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ErrorMessage;
    
    // Path to created atlas texture
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

// Main sprite optimization and atlas creation utility class
UCLASS()
class SPRITEOPTIMIZEREDITOR_API USpriteOptimizer : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // === CORE OPTIMIZATION FUNCTIONS ===
    
    // Optimizes multiple sprites by removing transparent areas
    UFUNCTION(CallInEditor, Category="Sprite Optimization", BlueprintCallable)
    static TArray<FSpriteOptimizationResult> OptimizeSprites(
        const TArray<UPaperSprite*>& Sprites, 
        const FSpriteOptimizationSettings& Settings = FSpriteOptimizationSettings()
    );
    
    // Optimizes a single sprite
    UFUNCTION(BlueprintCallable, Category="Sprite Optimization")
    static FSpriteOptimizationResult OptimizeSingleSprite(
        UPaperSprite* Sprite, 
        const FSpriteOptimizationSettings& Settings = FSpriteOptimizationSettings()
    );
    
    // Analyzes sprite without creating optimized version
    UFUNCTION(BlueprintCallable, Category="Sprite Optimization")
    static FSpriteOptimizationResult AnalyzeSprite(UPaperSprite* Sprite);
    
    // === ATLAS CREATION FUNCTIONS ===
    
    // Creates sprite atlas from multiple sprites
    UFUNCTION(CallInEditor, Category="Sprite Atlas", BlueprintCallable)
    static FSpriteAtlasResult CreateSpriteAtlas(
        const TArray<UPaperSprite*>& Sprites,
        const FSpriteAtlasSettings& Settings = FSpriteAtlasSettings(),
        const FString& AtlasName = TEXT("SpriteAtlas"),
        const FString& AtlasPath = TEXT("")
    );
    
    // Analyzes atlas creation without actually creating it
    UFUNCTION(BlueprintCallable, Category="Sprite Atlas")
    static FSpriteAtlasResult AnalyzeSpriteAtlas(
        const TArray<UPaperSprite*>& Sprites,
        const FSpriteAtlasSettings& Settings = FSpriteAtlasSettings()
    );
    
    // === UTILITY FUNCTIONS ===
    
    // Finds used (non-transparent) bounds in texture
    UFUNCTION(BlueprintCallable, Category="Sprite Optimization")
    static FIntRect FindUsedBounds(UTexture2D* Texture, int32 Padding = 2);
    
    // Gets available Paper2D materials
    UFUNCTION(BlueprintCallable, Category="Sprite Optimization")
    static TArray<UMaterialInterface*> GetAvailablePaper2DMaterials();
    
    // Gets default Paper2D material
    UFUNCTION(BlueprintCallable, Category="Sprite Optimization")
    static UMaterialInterface* GetDefaultPaper2DMaterial();
    
    // Refreshes Content Browser view
    static void RefreshContentBrowser();
    
    // Shows optimization notification to user
    static void ShowOptimizationNotification(const FText& Message, bool bSuccess = true);

private:
    // === OPTIMIZATION HELPERS ===
    
    // Creates optimized texture from source texture
    static UTexture2D* CreateOptimizedTexture(
        UTexture2D* SourceTexture, 
        FIntRect UsedRegion, 
        const FString& AssetName, 
        const FString& AssetPath
    );
    
    // Creates optimized sprite from optimized texture
    static UPaperSprite* CreateOptimizedSprite(
        UTexture2D* OptimizedTexture,
        UPaperSprite* OriginalSprite,
        const FIntRect& UsedRegion,
        const FSpriteOptimizationSettings& Settings,
        const FString& AssetName,
        const FString& AssetPath
    );
    
    // === ATLAS PACKING ALGORITHMS ===
    
    // Simple grid-based packing algorithm
    static TArray<FIntRect> PackSprites_Simple(
        const TArray<FIntPoint>& SpriteSizes, 
        const FSpriteAtlasSettings& Settings, 
        FIntPoint& OutAtlasSize
    );
    
    // Best-fit packing algorithm
    static TArray<FIntRect> PackSprites_BestFit(
        const TArray<FIntPoint>& SpriteSizes, 
        const FSpriteAtlasSettings& Settings, 
        FIntPoint& OutAtlasSize
    );
    
    // MaxRects packing algorithm
    static TArray<FIntRect> PackSprites_MaxRects(
        const TArray<FIntPoint>& SpriteSizes, 
        const FSpriteAtlasSettings& Settings, 
        FIntPoint& OutAtlasSize
    );
    
    // === ATLAS CREATION HELPERS ===
    
    // Creates atlas texture from multiple sprites
    static UTexture2D* CreateAtlasTexture(
        const TArray<UPaperSprite*>& Sprites,
        const TArray<FIntRect>& SpriteRegions,
        const FIntPoint& AtlasSize,
        const FString& AssetName,
        const FString& AssetPath
    );
    
    // Creates individual sprite from atlas texture
    static UPaperSprite* CreateSpriteFromAtlas(
        UTexture2D* AtlasTexture,
        const FIntRect& Region,
        UPaperSprite* OriginalSprite,
        const FString& SpriteName,
        const FString& AssetPath
    );
    
    // Copies pixels from source texture to atlas
    static bool CopyPixelsFromSourceToAtlas(
        UTexture2D* SourceTexture, 
        TArray<FColor>& AtlasPixels, 
        const FIntRect& Region, 
        const FIntPoint& AtlasSize
    );
    
    // Direct pixel copying without scaling
    static bool CopyPixelsDirect(
        const TArray<FColor>& SourcePixels,
        int32 SourceWidth,
        const FIntRect& SourceRegion,
        TArray<FColor>& AtlasPixels,
        const FIntPoint& AtlasSize,
        const FIntRect& AtlasRegion
    );
    
    // Pixel copying with scaling
    static bool CopyPixelsWithScaling(
        const TArray<FColor>& SourcePixels,
        int32 SourceWidth,
        const FIntRect& SourceRegion,
        TArray<FColor>& AtlasPixels,
        const FIntPoint& AtlasSize,
        const FIntRect& AtlasRegion
    );
    
    // === UTILITY HELPERS ===
    
    // Reads pixel data from texture
    static TArray<FColor> GetTexturePixelData(UTexture2D* Texture);
    
    // Calculates packing efficiency percentage
    static float CalculatePackingEfficiency(
        const TArray<FIntPoint>& SpriteSizes, 
        const FIntPoint& AtlasSize
    );
    
    // Gets optimized sizes for sprites
    static TArray<FIntPoint> GetOptimizedSpriteSizes(
        const TArray<UPaperSprite*>& Sprites,
        int32 Padding = 2
    );
    
    // Calculates atlas pivot for proper layering
    static FVector2D CalculateAtlasPivotForLayering(
        int32 OriginalTextureWidth,
        int32 OriginalTextureHeight, 
        const FIntRect& OriginalUsedRegion,
        const FIntRect& AtlasRegion,
        UPaperSprite* OriginalSprite
    );
    
    // === PATH AND ASSET HELPERS ===
    
    // Gets path for optimized assets
    static FString GetOptimizedAssetPath(const FString& OriginalPath, const FSpriteOptimizationSettings& Settings);
    
    // Gets path for atlas assets
    static FString GetAtlasAssetPath(const FString& FirstSpritePath);
    
    // Gets name for optimized assets
    static FString GetOptimizedAssetName(const FString& OriginalName, const FSpriteOptimizationSettings& Settings);
    
    // Ensures directory exists
    static bool EnsureDirectoryExists(const FString& DirectoryPath);
    
    // Creates backup if needed
    static void CreateBackupIfNeeded(UObject* Asset, bool bCreateBackup);
};