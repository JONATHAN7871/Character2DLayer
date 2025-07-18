// Copyright 2025, CRAFTCODE, All Rights Reserved.

#include "SpriteOptimizer/SpriteOptimizer.h"
#include "Settings/SpriteOptimizerSettings.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"
#include "PaperSprite.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/DateTime.h"
#include "Materials/Material.h"
#include "Engine/Engine.h"
#include "Misc/Paths.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

DEFINE_LOG_CATEGORY_STATIC(LogSpriteOptimizer, Log, All);

// === SETTINGS IMPLEMENTATION ===

void FSpriteOptimizationSettings::LoadFromProjectSettings()
{
    const USpriteOptimizerSettings* ProjectSettings = GetDefault<USpriteOptimizerSettings>();
    if (ProjectSettings && bUseProjectSettings)
    {
        if (ProjectSettings->DefaultMaterial.IsValid())
        {
            Material = ProjectSettings->DefaultMaterial.LoadSynchronous();
        }
        PixelsPerUnit = ProjectSettings->DefaultPixelsPerUnit;
        Padding = ProjectSettings->DefaultPadding;
        bCreateBackup = ProjectSettings->bDefaultCreateBackup;
        bReplaceOriginals = ProjectSettings->bDefaultReplaceOriginals;
    }
}

// === CORE OPTIMIZATION FUNCTIONS ===

TArray<FSpriteOptimizationResult> USpriteOptimizer::OptimizeSprites(
    const TArray<UPaperSprite*>& Sprites, 
    const FSpriteOptimizationSettings& Settings)
{
    TArray<FSpriteOptimizationResult> Results;
    
    // Load project settings if needed
    FSpriteOptimizationSettings WorkingSettings = Settings;
    WorkingSettings.LoadFromProjectSettings();
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Starting optimization of %d sprites"), Sprites.Num());
    
    for (UPaperSprite* Sprite : Sprites)
    {
        if (!Sprite)
        {
            UE_LOG(LogSpriteOptimizer, Warning, TEXT("Skipping null sprite"));
            continue;
        }
        
        FSpriteOptimizationResult Result = OptimizeSingleSprite(Sprite, WorkingSettings);
        Results.Add(Result);
        
        if (Result.bSuccess)
        {
            UE_LOG(LogSpriteOptimizer, Log, TEXT("Successfully optimized sprite: %s"), *Result.SpriteName);
        }
        else
        {
            UE_LOG(LogSpriteOptimizer, Warning, TEXT("Failed to optimize sprite %s: %s"), 
                   *Result.SpriteName, *Result.ErrorMessage);
        }
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Completed optimization of %d sprites"), Results.Num());
    
    // Show notification if enabled in settings
    const USpriteOptimizerSettings* ProjectSettings = GetDefault<USpriteOptimizerSettings>();
    if (ProjectSettings && ProjectSettings->bShowOptimizationNotifications)
    {
        int32 SuccessCount = 0;
        for (const auto& Result : Results)
        {
            if (Result.bSuccess) SuccessCount++;
        }
        
        FText NotificationText = FText::Format(
            FText::FromString(TEXT("Optimized {0}/{1} sprites successfully")), 
            SuccessCount, Results.Num()
        );
        ShowOptimizationNotification(NotificationText, SuccessCount > 0);
    }
    
    // Refresh Content Browser if enabled in settings
    if (ProjectSettings && ProjectSettings->bAutoRefreshContentBrowser)
    {
        RefreshContentBrowser();
    }
    
    return Results;
}

FSpriteOptimizationResult USpriteOptimizer::OptimizeSingleSprite(
    UPaperSprite* Sprite, 
    const FSpriteOptimizationSettings& Settings)
{
    FSpriteOptimizationResult Result;
    Result.OriginalSprite = Sprite;
    Result.SpriteName = Sprite ? Sprite->GetName() : TEXT("Unknown");
    
    if (!Sprite)
    {
        Result.ErrorMessage = TEXT("Sprite is null");
        return Result;
    }
    
    UTexture2D* SourceTexture = Sprite->GetSourceTexture();
    if (!SourceTexture)
    {
        Result.ErrorMessage = TEXT("Sprite has no source texture");
        return Result;
    }
    
    Result.OriginalTexture = SourceTexture;
    
    // Find used region
    Result.UsedRegion = FindUsedBounds(SourceTexture, Settings.Padding);
    
    if (Result.UsedRegion.Width() <= 0 || Result.UsedRegion.Height() <= 0)
    {
        Result.ErrorMessage = TEXT("No used region found in texture");
        return Result;
    }
    
    // Check if optimization is worthwhile
    float UsagePercent = (float)(Result.UsedRegion.Width() * Result.UsedRegion.Height()) / 
                        (SourceTexture->GetSizeX() * SourceTexture->GetSizeY()) * 100.0f;
    
    if (UsagePercent > 90.0f)
    {
        Result.ErrorMessage = TEXT("Sprite already well optimized (>90% usage)");
        return Result;
    }
    
    // Determine asset paths
    FString OriginalPackagePath = Sprite->GetPackage()->GetName();
    FString OptimizedPath = GetOptimizedAssetPath(OriginalPackagePath, Settings);
    FString OptimizedName = GetOptimizedAssetName(Sprite->GetName(), Settings);
    
    // Create backup if needed
    if (Settings.bCreateBackup && !Settings.bReplaceOriginals)
    {
        CreateBackupIfNeeded(Sprite, Settings.bCreateBackup);
    }
    
    // Create optimized texture
    Result.OptimizedTexture = CreateOptimizedTexture(
        SourceTexture, 
        Result.UsedRegion, 
        OptimizedName + TEXT("_Tex"), 
        OptimizedPath
    );
    
    if (!Result.OptimizedTexture)
    {
        Result.ErrorMessage = TEXT("Failed to create optimized texture");
        return Result;
    }
    
    Result.OptimizedTexturePath = OptimizedPath + TEXT("/") + OptimizedName + TEXT("_Tex");
    
    // Create optimized sprite
    Result.OptimizedSprite = CreateOptimizedSprite(
        Result.OptimizedTexture,
        Sprite,
        Result.UsedRegion,
        Settings,
        OptimizedName,
        OptimizedPath
    );
    
    if (!Result.OptimizedSprite)
    {
        Result.ErrorMessage = TEXT("Failed to create optimized sprite");
        return Result;
    }
    
    Result.OptimizedSpritePath = OptimizedPath + TEXT("/") + OptimizedName;
    
    // Calculate statistics
    Result.CalculateStats();
    Result.bSuccess = true;
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Optimized %s: %.1f%% savings (%.1fMB -> %.1fMB)"), 
           *Result.SpriteName, Result.SavingsPercent, Result.OriginalSizeMB, Result.OptimizedSizeMB);
    
    return Result;
}

FSpriteOptimizationResult USpriteOptimizer::AnalyzeSprite(UPaperSprite* Sprite)
{
    FSpriteOptimizationResult Result;
    Result.OriginalSprite = Sprite;
    Result.SpriteName = Sprite ? Sprite->GetName() : TEXT("Unknown");
    
    if (!Sprite)
    {
        Result.ErrorMessage = TEXT("Sprite is null");
        return Result;
    }
    
    UTexture2D* SourceTexture = Sprite->GetSourceTexture();
    if (!SourceTexture)
    {
        Result.ErrorMessage = TEXT("Sprite has no source texture");
        return Result;
    }
    
    Result.OriginalTexture = SourceTexture;
    Result.UsedRegion = FindUsedBounds(SourceTexture, 2);
    
    // Simulate optimized texture for calculations
    if (Result.UsedRegion.Width() > 0 && Result.UsedRegion.Height() > 0)
    {
        int32 OriginalPixels = SourceTexture->GetSizeX() * SourceTexture->GetSizeY();
        int32 OptimizedPixels = Result.UsedRegion.Width() * Result.UsedRegion.Height();
        
        Result.OriginalSizeMB = (OriginalPixels * 4) / (1024.0f * 1024.0f);
        Result.OptimizedSizeMB = (OptimizedPixels * 4) / (1024.0f * 1024.0f);
        Result.SavingsPercent = Result.OriginalSizeMB > 0 ? 
            ((Result.OriginalSizeMB - Result.OptimizedSizeMB) / Result.OriginalSizeMB) * 100.0f : 0.0f;
        Result.UsagePercent = OriginalPixels > 0 ? 
            (static_cast<float>(OptimizedPixels) / OriginalPixels) * 100.0f : 0.0f;
        
        Result.OriginalSize = FVector2D(SourceTexture->GetSizeX(), SourceTexture->GetSizeY());
        Result.OptimizedSize = FVector2D(Result.UsedRegion.Width(), Result.UsedRegion.Height());
        
        Result.bSuccess = true;
    }
    else
    {
        Result.ErrorMessage = TEXT("No used region found");
    }
    
    return Result;
}

// === ATLAS CREATION FUNCTIONS ===

FSpriteAtlasResult USpriteOptimizer::CreateSpriteAtlas(
    const TArray<UPaperSprite*>& Sprites,
    const FSpriteAtlasSettings& Settings,
    const FString& AtlasName,
    const FString& AtlasPath)
{
    FSpriteAtlasResult Result;
    Result.TotalSprites = Sprites.Num();
    
    // Validation
    if (Sprites.Num() == 0)
    {
        Result.ErrorMessage = TEXT("No sprites provided for atlas creation");
        return Result;
    }
    
    if (Sprites.Num() == 1)
    {
        Result.ErrorMessage = TEXT("Atlas requires at least 2 sprites. Use regular optimization for single sprites.");
        return Result;
    }
    
    if (AtlasName.IsEmpty())
    {
        Result.ErrorMessage = TEXT("Atlas name cannot be empty");
        return Result;
    }
    
    // Check sprite validity
    int32 ValidSprites = 0;
    for (UPaperSprite* Sprite : Sprites)
    {
        if (Sprite && Sprite->GetSourceTexture())
        {
            ValidSprites++;
        }
    }
    
    if (ValidSprites < 2)
    {
        Result.ErrorMessage = FString::Printf(TEXT("Only %d valid sprites found. Need at least 2."), ValidSprites);
        return Result;
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Creating atlas '%s' from %d sprites (%d valid)"), 
           *AtlasName, Sprites.Num(), ValidSprites);
    
    // Determine atlas path
    FString ActualAtlasPath;
    if (AtlasPath.IsEmpty())
    {
        FString FirstSpritePath = Sprites[0]->GetPackage()->GetName();
        ActualAtlasPath = GetAtlasAssetPath(FirstSpritePath);
    }
    else
    {
        ActualAtlasPath = AtlasPath;
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Atlas will be created in: %s"), *ActualAtlasPath);
    
    // Get sprite sizes (optimized or original)
    TArray<FIntPoint> SpriteSizes;
    if (Settings.bOptimizeSpritesFirst)
    {
        SpriteSizes = GetOptimizedSpriteSizes(Sprites, Settings.SpritePadding);
    }
    else
    {
        for (UPaperSprite* Sprite : Sprites)
        {
            if (Sprite && Sprite->GetSourceTexture())
            {
                UTexture2D* SourceTexture = Sprite->GetSourceTexture();
                SpriteSizes.Add(FIntPoint(SourceTexture->GetSizeX(), SourceTexture->GetSizeY()));
            }
        }
    }
    
    if (SpriteSizes.Num() == 0)
    {
        Result.ErrorMessage = TEXT("No valid sprites found");
        return Result;
    }
    
    // Pack sprites
    FIntPoint AtlasSize;
    TArray<FIntRect> PackedRegions;
    
    switch (Settings.PackingAlgorithm)
    {
        case EAtlasPackingAlgorithm::Simple:
            PackedRegions = PackSprites_Simple(SpriteSizes, Settings, AtlasSize);
            break;
        case EAtlasPackingAlgorithm::BestFit:
            PackedRegions = PackSprites_BestFit(SpriteSizes, Settings, AtlasSize);
            break;
        case EAtlasPackingAlgorithm::MaxRects:
            PackedRegions = PackSprites_MaxRects(SpriteSizes, Settings, AtlasSize);
            break;
        default:
            PackedRegions = PackSprites_Simple(SpriteSizes, Settings, AtlasSize);
            break;
    }
    
    // Check atlas size limits
    if (AtlasSize.X > Settings.MaxAtlasSize.X || AtlasSize.Y > Settings.MaxAtlasSize.Y)
    {
        Result.ErrorMessage = FString::Printf(TEXT("Atlas size (%dx%d) exceeds maximum (%dx%d)"), 
                                            AtlasSize.X, AtlasSize.Y, 
                                            Settings.MaxAtlasSize.X, Settings.MaxAtlasSize.Y);
        return Result;
    }
    
    // Create atlas texture
    Result.AtlasTexture = CreateAtlasTexture(Sprites, PackedRegions, AtlasSize, AtlasName, ActualAtlasPath);
    
    if (!Result.AtlasTexture)
    {
        Result.ErrorMessage = TEXT("Failed to create atlas texture");
        return Result;
    }
    
    Result.AtlasTexturePath = ActualAtlasPath + TEXT("/") + AtlasName;
    
    // Create individual sprites if needed
    if (Settings.bCreateIndividualSprites)
    {
        for (int32 i = 0; i < Sprites.Num() && i < PackedRegions.Num(); i++)
        {
            UPaperSprite* OriginalSprite = Sprites[i];
            const FIntRect& Region = PackedRegions[i];
            
            FString SpriteName = OriginalSprite->GetName() + Settings.AtlasSuffix;
            UPaperSprite* AtlasSprite = CreateSpriteFromAtlas(
                Result.AtlasTexture, 
                Region, 
                OriginalSprite,
                SpriteName,
                ActualAtlasPath
            );
            
            if (AtlasSprite)
            {
                Result.CreatedSprites.Add(AtlasSprite);
                UE_LOG(LogSpriteOptimizer, Log, TEXT("Created atlas sprite: %s in folder: Atlas"), *SpriteName);
            }
        }
    }
    
    // Fill results
    Result.SpriteRegions = PackedRegions;
    Result.AtlasSize = AtlasSize;
    Result.PackingEfficiency = CalculatePackingEfficiency(SpriteSizes, AtlasSize);
    
    // Calculate memory savings
    float OriginalMemory = 0.0f;
    for (UPaperSprite* Sprite : Sprites)
    {
        if (Sprite && Sprite->GetSourceTexture())
        {
            UTexture2D* OriginalTexture = Sprite->GetSourceTexture();
            int32 OriginalPixels = OriginalTexture->GetSizeX() * OriginalTexture->GetSizeY();
            OriginalMemory += (OriginalPixels * 4) / (1024.0f * 1024.0f);
        }
    }
    float AtlasMemory = (AtlasSize.X * AtlasSize.Y * 4) / (1024.0f * 1024.0f);
    Result.MemorySavings = OriginalMemory > 0 ? ((OriginalMemory - AtlasMemory) / OriginalMemory) * 100.0f : 0.0f;
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Memory calculation: Original=%.2fMB, Atlas=%.2fMB, Savings=%.1f%%"), 
           OriginalMemory, AtlasMemory, Result.MemorySavings);
    
    Result.bSuccess = true;
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Atlas created successfully in folder 'Atlas': %dx%d, %.1f%% efficiency, %.1f%% memory savings"), 
           AtlasSize.X, AtlasSize.Y, Result.PackingEfficiency, Result.MemorySavings);
    
    return Result;
}

FSpriteAtlasResult USpriteOptimizer::AnalyzeSpriteAtlas(
    const TArray<UPaperSprite*>& Sprites,
    const FSpriteAtlasSettings& Settings)
{
    FSpriteAtlasResult Result;
    Result.TotalSprites = Sprites.Num();
    
    if (Sprites.Num() == 0)
    {
        Result.ErrorMessage = TEXT("No sprites provided for analysis");
        return Result;
    }
    
    // Get sprite sizes
    TArray<FIntPoint> SpriteSizes = Settings.bOptimizeSpritesFirst ?
        GetOptimizedSpriteSizes(Sprites, Settings.SpritePadding) :
        TArray<FIntPoint>();
    
    if (!Settings.bOptimizeSpritesFirst)
    {
        for (UPaperSprite* Sprite : Sprites)
        {
            if (Sprite && Sprite->GetSourceTexture())
            {
                UTexture2D* SourceTexture = Sprite->GetSourceTexture();
                SpriteSizes.Add(FIntPoint(SourceTexture->GetSizeX(), SourceTexture->GetSizeY()));
            }
        }
    }
    
    if (SpriteSizes.Num() == 0)
    {
        Result.ErrorMessage = TEXT("No valid sprites found");
        return Result;
    }
    
    // Simulate packing
    FIntPoint AtlasSize;
    TArray<FIntRect> PackedRegions;
    
    switch (Settings.PackingAlgorithm)
    {
        case EAtlasPackingAlgorithm::Simple:
            PackedRegions = PackSprites_Simple(SpriteSizes, Settings, AtlasSize);
            break;
        case EAtlasPackingAlgorithm::BestFit:
            PackedRegions = PackSprites_BestFit(SpriteSizes, Settings, AtlasSize);
            break;
        case EAtlasPackingAlgorithm::MaxRects:
            PackedRegions = PackSprites_MaxRects(SpriteSizes, Settings, AtlasSize);
            break;
        default:
            PackedRegions = PackSprites_Simple(SpriteSizes, Settings, AtlasSize);
            break;
    }
    
    // Fill analysis results
    Result.AtlasSize = AtlasSize;
    Result.SpriteRegions = PackedRegions;
    Result.PackingEfficiency = CalculatePackingEfficiency(SpriteSizes, AtlasSize);
    
    // Calculate memory savings
    float OriginalMemory = 0.0f;
    for (UPaperSprite* Sprite : Sprites)
    {
        if (Sprite && Sprite->GetSourceTexture())
        {
            UTexture2D* OriginalTexture = Sprite->GetSourceTexture();
            int32 OriginalPixels = OriginalTexture->GetSizeX() * OriginalTexture->GetSizeY();
            OriginalMemory += (OriginalPixels * 4) / (1024.0f * 1024.0f);
        }
    }
    float AtlasMemory = (AtlasSize.X * AtlasSize.Y * 4) / (1024.0f * 1024.0f);
    Result.MemorySavings = OriginalMemory > 0 ? ((OriginalMemory - AtlasMemory) / OriginalMemory) * 100.0f : 0.0f;
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Analysis memory calculation: Original=%.2fMB, Atlas=%.2fMB, Savings=%.1f%%"), 
           OriginalMemory, AtlasMemory, Result.MemorySavings);
    
    // Check limits
    if (AtlasSize.X > Settings.MaxAtlasSize.X || AtlasSize.Y > Settings.MaxAtlasSize.Y)
    {
        Result.ErrorMessage = FString::Printf(TEXT("Atlas size (%dx%d) exceeds maximum (%dx%d)"), 
                                            AtlasSize.X, AtlasSize.Y, 
                                            Settings.MaxAtlasSize.X, Settings.MaxAtlasSize.Y);
        Result.bSuccess = false;
    }
    else
    {
        Result.bSuccess = true;
    }
    
    return Result;
}

// === UTILITY FUNCTIONS ===

FIntRect USpriteOptimizer::FindUsedBounds(UTexture2D* Texture, int32 Padding)
{
    if (!Texture)
    {
        return FIntRect(0, 0, 0, 0);
    }
    
    TArray<FColor> PixelData = GetTexturePixelData(Texture);
    if (PixelData.Num() == 0)
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Failed to read texture pixel data"));
        return FIntRect(0, 0, Texture->GetSizeX(), Texture->GetSizeY());
    }
    
    int32 Width = Texture->GetSizeX();
    int32 Height = Texture->GetSizeY();
    
    // Find bounds of non-transparent pixels
    int32 MinX = Width, MaxX = 0;
    int32 MinY = Height, MaxY = 0;
    bool FoundContent = false;
    
    for (int32 Y = 0; Y < Height; Y++)
    {
        for (int32 X = 0; X < Width; X++)
        {
            const FColor& Pixel = PixelData[Y * Width + X];
            
            // If pixel is not fully transparent
            if (Pixel.A > 10) // Small threshold for anti-aliasing
            {
                MinX = FMath::Min(MinX, X);
                MaxX = FMath::Max(MaxX, X);
                MinY = FMath::Min(MinY, Y);
                MaxY = FMath::Max(MaxY, Y);
                FoundContent = true;
            }
        }
    }
    
    if (!FoundContent)
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("No non-transparent content found"));
        return FIntRect(0, 0, 0, 0);
    }
    
    // Add padding
    MinX = FMath::Max(0, MinX - Padding);
    MinY = FMath::Max(0, MinY - Padding);
    MaxX = FMath::Min(Width - 1, MaxX + Padding);
    MaxY = FMath::Min(Height - 1, MaxY + Padding);
    
    return FIntRect(MinX, MinY, MaxX + 1, MaxY + 1);
}

TArray<UMaterialInterface*> USpriteOptimizer::GetAvailablePaper2DMaterials()
{
    TArray<UMaterialInterface*> Materials;
    
    // List of correct Paper2D materials
    TArray<FString> Paper2DMaterialPaths = {
        TEXT("/Paper2D/TranslucentLitSpriteMaterial.TranslucentLitSpriteMaterial"),
        TEXT("/Paper2D/MaskedUnlitSpriteMaterial.MaskedUnlitSpriteMaterial"),
        TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"),
        TEXT("/Paper2D/OpaqueUnlitSpriteMaterial.OpaqueUnlitSpriteMaterial"),
        TEXT("/Paper2D/MaskedLitSpriteMaterial.MaskedLitSpriteMaterial"),
        TEXT("/Paper2D/OpaqueLitSpriteMaterial.OpaqueLitSpriteMaterial")
    };
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Loading Paper2D materials..."));
    
    for (const FString& Path : Paper2DMaterialPaths)
    {
        UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *Path);
        if (Material)
        {
            Materials.Add(Material);
            UE_LOG(LogSpriteOptimizer, Log, TEXT("Loaded material: %s"), *Material->GetName());
        }
        else
        {
            UE_LOG(LogSpriteOptimizer, Warning, TEXT("Failed to load material: %s"), *Path);
        }
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Loaded %d Paper2D materials"), Materials.Num());
    
    return Materials;
}

UMaterialInterface* USpriteOptimizer::GetDefaultPaper2DMaterial()
{
    // Use TranslucentLitSpriteMaterial as default
    UMaterialInterface* Material = LoadObject<UMaterialInterface>(
        nullptr, 
        TEXT("/Paper2D/TranslucentLitSpriteMaterial.TranslucentLitSpriteMaterial")
    );
    
    if (Material)
    {
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Using default material: %s"), *Material->GetName());
        return Material;
    }
    
    // Fallback materials
    TArray<FString> FallbackPaths = {
        TEXT("/Paper2D/MaskedLitSpriteMaterial.MaskedLitSpriteMaterial"),
        TEXT("/Paper2D/OpaqueUnlitSpriteMaterial.OpaqueUnlitSpriteMaterial"),
        TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")
    };
    
    for (const FString& Path : FallbackPaths)
    {
        Material = LoadObject<UMaterialInterface>(nullptr, *Path);
        if (Material)
        {
            UE_LOG(LogSpriteOptimizer, Warning, TEXT("Using fallback material: %s"), *Material->GetName());
            return Material;
        }
    }
    
    UE_LOG(LogSpriteOptimizer, Error, TEXT("Failed to load any Paper2D materials!"));
    return nullptr;
}

void USpriteOptimizer::RefreshContentBrowser()
{
    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    IContentBrowserSingleton& ContentBrowserSingleton = ContentBrowserModule.Get();
    ContentBrowserSingleton.SyncBrowserToAssets(TArray<FAssetData>());
}

void USpriteOptimizer::ShowOptimizationNotification(const FText& Message, bool bSuccess)
{
    FNotificationInfo Info(Message);
    Info.bFireAndForget = true;
    Info.FadeOutDuration = 3.0f;
    Info.ExpireDuration = 5.0f;
    
    if (bSuccess)
    {
        Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.SuccessImage"));
    }
    else
    {
        Info.Image = FAppStyle::GetBrush(TEXT("NotificationList.FailImage"));
    }
    
    FSlateNotificationManager::Get().AddNotification(Info);
}

// === OPTIMIZATION HELPERS ===

UTexture2D* USpriteOptimizer::CreateOptimizedTexture(
    UTexture2D* SourceTexture, 
    FIntRect UsedRegion, 
    const FString& AssetName, 
    const FString& AssetPath)
{
    if (!SourceTexture)
    {
        return nullptr;
    }
    
    int32 NewWidth = UsedRegion.Width();
    int32 NewHeight = UsedRegion.Height();
    
    if (NewWidth <= 0 || NewHeight <= 0)
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Invalid texture size: %dx%d"), NewWidth, NewHeight);
        return nullptr;
    }
    
    // Create directory if needed
    if (!EnsureDirectoryExists(AssetPath))
    {
        UE_LOG(LogSpriteOptimizer, Error, TEXT("Failed to create directory: %s"), *AssetPath);
        return nullptr;
    }
    
    // Get pixels from original texture
    TArray<FColor> SourcePixels = GetTexturePixelData(SourceTexture);
    if (SourcePixels.Num() == 0)
    {
        return nullptr;
    }
    
    // Extract used region
    TArray<FColor> OptimizedPixels;
    OptimizedPixels.Reserve(NewWidth * NewHeight);
    
    int32 SourceWidth = SourceTexture->GetSizeX();
    
    for (int32 Y = UsedRegion.Min.Y; Y < UsedRegion.Max.Y; Y++)
    {
        for (int32 X = UsedRegion.Min.X; X < UsedRegion.Max.X; X++)
        {
            if (Y * SourceWidth + X < SourcePixels.Num())
            {
                OptimizedPixels.Add(SourcePixels[Y * SourceWidth + X]);
            }
            else
            {
                OptimizedPixels.Add(FColor::Transparent);
            }
        }
    }
    
    // Create package for new texture
    FString FullAssetPath = AssetPath + TEXT("/") + AssetName;
    FString PackageName = FullAssetPath;
    
    UPackage* Package = CreatePackage(*PackageName);
    Package->FullyLoad();
    
    UTexture2D* NewTexture = NewObject<UTexture2D>(Package, FName(*AssetName), RF_Public | RF_Standalone);
    
    if (!NewTexture)
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Failed to create texture object"));
        return nullptr;
    }
    
    // Set up texture via Source API
    NewTexture->Source.Init(NewWidth, NewHeight, 1, 1, TSF_BGRA8, (uint8*)OptimizedPixels.GetData());
    
    // Copy settings from original
    NewTexture->SRGB = SourceTexture->SRGB;
    NewTexture->CompressionSettings = SourceTexture->CompressionSettings;
    NewTexture->Filter = SourceTexture->Filter;
    NewTexture->AddressX = SourceTexture->AddressX;
    NewTexture->AddressY = SourceTexture->AddressY;
    
    NewTexture->UpdateResource();
    NewTexture->PostEditChange();
    (void)Package->MarkPackageDirty();
    
    // Save
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    bool bSaved = UPackage::SavePackage(Package, NewTexture, *PackageFileName, SaveArgs);
    
    if (bSaved)
    {
        FAssetRegistryModule::AssetCreated(NewTexture);
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Created optimized texture: %s (%dx%d) in folder: %s"), 
               *AssetName, NewWidth, NewHeight, *AssetPath);
    }
    else
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Failed to save optimized texture: %s"), *FullAssetPath);
    }
    
    return NewTexture;
}

UPaperSprite* USpriteOptimizer::CreateOptimizedSprite(
    UTexture2D* OptimizedTexture,
    UPaperSprite* OriginalSprite,
    const FIntRect& UsedRegion,
    const FSpriteOptimizationSettings& Settings,
    const FString& AssetName,
    const FString& AssetPath)
{
    if (!OptimizedTexture || !OriginalSprite)
    {
        return nullptr;
    }
    
    // Create package for sprite
    FString FullAssetPath = AssetPath + TEXT("/") + AssetName;
    FString PackageName = FullAssetPath;
    
    UPackage* Package = CreatePackage(*PackageName);
    Package->FullyLoad();
    
    UPaperSprite* NewSprite = NewObject<UPaperSprite>(Package, FName(*AssetName), RF_Public | RF_Standalone);
    
    if (!NewSprite)
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Failed to create sprite object"));
        return nullptr;
    }
    
    // Calculate new pivot
    UTexture2D* OriginalTexture = OriginalSprite->GetSourceTexture();
    const FVector2D OriginalTextureSize(OriginalTexture->GetSizeX(), OriginalTexture->GetSizeY());
    const FVector2D OriginalPivotInPixels = OriginalTextureSize * 0.5f;
    const FVector2D CroppedTextureTopLeftInPixels(UsedRegion.Min.X, UsedRegion.Min.Y);
    const FVector2D NewPivotInPixels = OriginalPivotInPixels - CroppedTextureTopLeftInPixels;
    
    // Determine material
    UMaterialInterface* SpriteMaterial;
    
    if (Settings.Material)
    {
        SpriteMaterial = Settings.Material;
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Using material from settings: %s"), *SpriteMaterial->GetName());
    }
    else if (OriginalSprite->GetDefaultMaterial())
    {
        SpriteMaterial = OriginalSprite->GetDefaultMaterial();
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Using original sprite material: %s"), *SpriteMaterial->GetName());
    }
    else
    {
        SpriteMaterial = GetDefaultPaper2DMaterial();
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Using default Paper2D material: %s"), 
               SpriteMaterial ? *SpriteMaterial->GetName() : TEXT("NULL"));
    }
    
    // Set up initialization parameters
    FSpriteAssetInitParameters InitParams;
    InitParams.Texture = OptimizedTexture;
    InitParams.Offset = FIntPoint::ZeroValue;
    InitParams.Dimension = FIntPoint(OptimizedTexture->GetSizeX(), OptimizedTexture->GetSizeY());
    InitParams.DefaultMaterialOverride = SpriteMaterial;
    InitParams.bOverridePixelsPerUnrealUnit = true;
    InitParams.PixelsPerUnrealUnit = Settings.PixelsPerUnit;
    
    // Initialize sprite
    NewSprite->InitializeSprite(InitParams, false);
    
    // Set pivot
    NewSprite->SetPivotMode(ESpritePivotMode::Custom, NewPivotInPixels, true);
    
    (void)Package->MarkPackageDirty();
    
    // Save
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    bool bSaved = UPackage::SavePackage(Package, NewSprite, *PackageFileName, SaveArgs);
    
    if (bSaved)
    {
        FAssetRegistryModule::AssetCreated(NewSprite);
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Created optimized sprite: %s with material: %s"), 
               *FullAssetPath, SpriteMaterial ? *SpriteMaterial->GetName() : TEXT("None"));
    }
    else
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Failed to save optimized sprite: %s"), *FullAssetPath);
    }
    
    return NewSprite;
}

// === ATLAS PACKING ALGORITHMS ===

TArray<FIntRect> USpriteOptimizer::PackSprites_Simple(
    const TArray<FIntPoint>& SpriteSizes, 
    const FSpriteAtlasSettings& Settings, 
    FIntPoint& OutAtlasSize)
{
    TArray<FIntRect> PackedRegions;
    
    if (SpriteSizes.Num() == 0)
    {
        OutAtlasSize = FIntPoint::ZeroValue;
        return PackedRegions;
    }
    
    // Simple grid layout
    int32 SpritesPerRow = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(SpriteSizes.Num())));
    
    int32 CurrentX = 0;
    int32 CurrentY = 0;
    int32 MaxRowHeight = 0;
    int32 MaxAtlasWidth = 0;
    
    for (int32 i = 0; i < SpriteSizes.Num(); i++)
    {
        const FIntPoint& SpriteSize = SpriteSizes[i];
        
        // Check if we need a new row
        if (i > 0 && i % SpritesPerRow == 0)
        {
            CurrentY += MaxRowHeight + Settings.SpritePadding;
            CurrentX = 0;
            MaxRowHeight = 0;
        }
        
        // Add region
        FIntRect Region(CurrentX, CurrentY, CurrentX + SpriteSize.X, CurrentY + SpriteSize.Y);
        PackedRegions.Add(Region);
        
        // Update positions and sizes
        CurrentX += SpriteSize.X + Settings.SpritePadding;
        MaxRowHeight = FMath::Max(MaxRowHeight, SpriteSize.Y);
        MaxAtlasWidth = FMath::Max(MaxAtlasWidth, CurrentX);
    }
    
    OutAtlasSize = FIntPoint(MaxAtlasWidth, CurrentY + MaxRowHeight);
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Simple packing: %d sprites into %dx%d atlas"), 
           SpriteSizes.Num(), OutAtlasSize.X, OutAtlasSize.Y);
    
    return PackedRegions;
}

TArray<FIntRect> USpriteOptimizer::PackSprites_BestFit(
    const TArray<FIntPoint>& SpriteSizes, 
    const FSpriteAtlasSettings& Settings, 
    FIntPoint& OutAtlasSize)
{
    TArray<FIntRect> PackedRegions;
    
    if (SpriteSizes.Num() == 0)
    {
        OutAtlasSize = FIntPoint::ZeroValue;
        return PackedRegions;
    }
    
    // Sort sprites by height, then width, then area (largest first)
    TArray<TPair<int32, FIntPoint>> SortedSprites;
    for (int32 i = 0; i < SpriteSizes.Num(); i++)
    {
        SortedSprites.Add(TPair<int32, FIntPoint>(i, SpriteSizes[i]));
    }
    
    SortedSprites.Sort([](const TPair<int32, FIntPoint>& A, const TPair<int32, FIntPoint>& B)
    {
        if (A.Value.Y != B.Value.Y)
        {
            return A.Value.Y > B.Value.Y;
        }
        if (A.Value.X != B.Value.X)
        {
            return A.Value.X > B.Value.X;
        }
        int32 AreaA = A.Value.X * A.Value.Y;
        int32 AreaB = B.Value.X * B.Value.Y;
        return AreaA > AreaB;
    });
    
    // Initialize result with correct size
    PackedRegions.Init(FIntRect(0, 0, 0, 0), SpriteSizes.Num());
    
    // Calculate initial atlas size estimate
    int32 TotalArea = 0;
    int32 MaxWidth = 0;
    int32 MaxHeight = 0;
    
    for (const FIntPoint& Size : SpriteSizes)
    {
        TotalArea += Size.X * Size.Y;
        MaxWidth = FMath::Max(MaxWidth, Size.X);
        MaxHeight = FMath::Max(MaxHeight, Size.Y);
    }
    
    float PackingEfficiencyFactor = 1.3f;
    int32 EstimatedSize = FMath::CeilToInt(FMath::Sqrt(TotalArea * PackingEfficiencyFactor));
    
    int32 CurrentWidth = FMath::Max(EstimatedSize, MaxWidth);
    int32 CurrentHeight = FMath::Max(EstimatedSize, MaxHeight);
    
    CurrentWidth = FMath::Max(CurrentWidth, 256);
    CurrentHeight = FMath::Max(CurrentHeight, 256);
    
    bool bPackingSuccessful = false;
    int32 Attempts = 0;
    const int32 MaxAttempts = 15;
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("BestFit: Starting with estimated size %dx%d for %d sprites (total area: %d)"), 
           CurrentWidth, CurrentHeight, SpriteSizes.Num(), TotalArea);
    
    while (!bPackingSuccessful && Attempts < MaxAttempts)
    {
        TArray<FIntRect> OccupiedRects;
        TArray<FIntRect> TempPackedRegions;
        TempPackedRegions.Init(FIntRect(0, 0, 0, 0), SpriteSizes.Num());
        
        bool bAllSpritesPlaced = true;
        int32 UsedWidth = 0;
        int32 UsedHeight = 0;
        
        // Try to place all sprites
        for (const auto& SpriteData : SortedSprites)
        {
            int32 OriginalIndex = SpriteData.Key;
            FIntPoint SpriteSize = SpriteData.Value;
            
            FIntPoint BestPosition(-1, -1);
            int32 BestScore = INT_MAX;
            
            // Scan for best position
            for (int32 Y = 0; Y <= CurrentHeight - SpriteSize.Y; Y += 2)
            {
                for (int32 X = 0; X <= CurrentWidth - SpriteSize.X; X += 2)
                {
                    FIntRect TestRect(X, Y, X + SpriteSize.X, Y + SpriteSize.Y);
                    
                    // Check for overlaps
                    bool bOverlaps = false;
                    for (const FIntRect& Occupied : OccupiedRects)
                    {
                        if (!(TestRect.Max.X <= Occupied.Min.X || 
                              TestRect.Min.X >= Occupied.Max.X || 
                              TestRect.Max.Y <= Occupied.Min.Y || 
                              TestRect.Min.Y >= Occupied.Max.Y))
                        {
                            bOverlaps = true;
                            break;
                        }
                    }
                    
                    if (!bOverlaps)
                    {
                        // Score: prefer top-left positions
                        int32 Score = Y * 1000 + X;
                        
                        // Bonus for compactness
                        int32 CompactnessBonus = 0;
                        for (const FIntRect& Occupied : OccupiedRects)
                        {
                            bool bIsNeighbor = false;
                            
                            if (FMath::Abs(TestRect.Min.X - Occupied.Max.X) <= Settings.SpritePadding && 
                                !(TestRect.Max.Y <= Occupied.Min.Y || TestRect.Min.Y >= Occupied.Max.Y))
                            {
                                bIsNeighbor = true;
                            }
                            else if (FMath::Abs(TestRect.Min.Y - Occupied.Max.Y) <= Settings.SpritePadding && 
                                     !(TestRect.Max.X <= Occupied.Min.X || TestRect.Min.X >= Occupied.Max.X))
                            {
                                bIsNeighbor = true;
                            }
                            
                            if (bIsNeighbor)
                            {
                                CompactnessBonus += 100;
                            }
                        }
                        
                        Score -= CompactnessBonus;
                        
                        if (Score < BestScore)
                        {
                            BestPosition = FIntPoint(X, Y);
                            BestScore = Score;
                        }
                    }
                }
            }
            
            if (BestPosition.X == -1)
            {
                bAllSpritesPlaced = false;
                break;
            }
            
            // Place sprite with padding
            FIntRect PlacedRect(
                BestPosition.X, 
                BestPosition.Y, 
                BestPosition.X + SpriteSize.X, 
                BestPosition.Y + SpriteSize.Y
            );
            
            FIntRect OccupiedRect(
                BestPosition.X, 
                BestPosition.Y, 
                BestPosition.X + SpriteSize.X + Settings.SpritePadding, 
                BestPosition.Y + SpriteSize.Y + Settings.SpritePadding
            );
            
            TempPackedRegions[OriginalIndex] = PlacedRect;
            OccupiedRects.Add(OccupiedRect);
            
            UsedWidth = FMath::Max(UsedWidth, PlacedRect.Max.X);
            UsedHeight = FMath::Max(UsedHeight, PlacedRect.Max.Y);
            
            UE_LOG(LogSpriteOptimizer, Verbose, TEXT("Placed sprite %d at (%d,%d) size (%d,%d)"), 
                   OriginalIndex, BestPosition.X, BestPosition.Y, SpriteSize.X, SpriteSize.Y);
        }
        
        if (bAllSpritesPlaced)
        {
            PackedRegions = TempPackedRegions;
            OutAtlasSize = FIntPoint(UsedWidth, UsedHeight);
            bPackingSuccessful = true;
            
            UE_LOG(LogSpriteOptimizer, Log, 
                   TEXT("BestFit packing successful: %d sprites into %dx%d atlas (attempt %d, efficiency: %.1f%%)"), 
                   SpriteSizes.Num(), UsedWidth, UsedHeight, Attempts + 1,
                   (float(TotalArea) / float(UsedWidth * UsedHeight)) * 100.0f);
        }
        else
        {
            // Increase size for next attempt
            if (CurrentWidth <= CurrentHeight)
            {
                CurrentWidth = FMath::Min(CurrentWidth + 64, Settings.MaxAtlasSize.X);
            }
            else
            {
                CurrentHeight = FMath::Min(CurrentHeight + 64, Settings.MaxAtlasSize.Y);
            }
            
            if (CurrentWidth >= Settings.MaxAtlasSize.X && CurrentHeight >= Settings.MaxAtlasSize.Y)
            {
                UE_LOG(LogSpriteOptimizer, Warning, TEXT("Reached maximum atlas size, trying Simple algorithm"));
                return PackSprites_Simple(SpriteSizes, Settings, OutAtlasSize);
            }
        }
        
        Attempts++;
    }
    
    if (!bPackingSuccessful)
    {
        UE_LOG(LogSpriteOptimizer, Error, 
               TEXT("Failed to pack %d sprites within max atlas size %dx%d after %d attempts"), 
               SpriteSizes.Num(), Settings.MaxAtlasSize.X, Settings.MaxAtlasSize.Y, Attempts);
        
        return PackSprites_Simple(SpriteSizes, Settings, OutAtlasSize);
    }
    
    return PackedRegions;
}

TArray<FIntRect> USpriteOptimizer::PackSprites_MaxRects(
    const TArray<FIntPoint>& SpriteSizes, 
    const FSpriteAtlasSettings& Settings, 
    FIntPoint& OutAtlasSize)
{
    // For simplicity, use BestFit algorithm
    UE_LOG(LogSpriteOptimizer, Log, TEXT("MaxRects algorithm using BestFit implementation"));
    return PackSprites_BestFit(SpriteSizes, Settings, OutAtlasSize);
}

// === ATLAS CREATION HELPERS ===

UTexture2D* USpriteOptimizer::CreateAtlasTexture(
    const TArray<UPaperSprite*>& Sprites,
    const TArray<FIntRect>& SpriteRegions,
    const FIntPoint& AtlasSize,
    const FString& AssetName,
    const FString& AssetPath)
{
    if (Sprites.Num() != SpriteRegions.Num())
    {
        UE_LOG(LogSpriteOptimizer, Error, TEXT("Sprites count (%d) doesn't match regions count (%d)"), 
               Sprites.Num(), SpriteRegions.Num());
        return nullptr;
    }
    
    if (AtlasSize.X <= 0 || AtlasSize.Y <= 0)
    {
        UE_LOG(LogSpriteOptimizer, Error, TEXT("Invalid atlas size: %dx%d"), AtlasSize.X, AtlasSize.Y);
        return nullptr;
    }
    
    // Create Atlas directory if needed
    if (!EnsureDirectoryExists(AssetPath))
    {
        UE_LOG(LogSpriteOptimizer, Error, TEXT("Failed to create atlas directory: %s"), *AssetPath);
        return nullptr;
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Creating atlas texture %dx%d with %d sprites in: %s"), 
           AtlasSize.X, AtlasSize.Y, Sprites.Num(), *AssetPath);
    
    // Create pixel array for atlas (initialize with transparent black)
    TArray<FColor> AtlasPixels;
    int32 TotalPixels = AtlasSize.X * AtlasSize.Y;
    AtlasPixels.Init(FColor(0, 0, 0, 0), TotalPixels);
    
    // Process each sprite
    for (int32 i = 0; i < Sprites.Num(); i++)
    {
        UPaperSprite* Sprite = Sprites[i];
        const FIntRect& Region = SpriteRegions[i];
        
        if (!Sprite || !Sprite->GetSourceTexture())
        {
            UE_LOG(LogSpriteOptimizer, Warning, TEXT("Skipping invalid sprite at index %d"), i);
            continue;
        }
        
        UTexture2D* SourceTexture = Sprite->GetSourceTexture();
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Processing sprite %d: %s (%dx%d) -> Region(%d,%d,%d,%d)"), 
               i, *Sprite->GetName(), 
               SourceTexture->GetSizeX(), SourceTexture->GetSizeY(),
               Region.Min.X, Region.Min.Y, Region.Max.X, Region.Max.Y);
        
        // Copy pixels
        if (!CopyPixelsFromSourceToAtlas(SourceTexture, AtlasPixels, Region, AtlasSize))
        {
            UE_LOG(LogSpriteOptimizer, Warning, TEXT("Failed to copy pixels from sprite: %s"), *Sprite->GetName());
        }
    }
    
    // Create package for new texture
    FString FullAssetPath = AssetPath + TEXT("/") + AssetName;
    FString PackageName = FullAssetPath;
    
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        UE_LOG(LogSpriteOptimizer, Error, TEXT("Failed to create package: %s"), *PackageName);
        return nullptr;
    }
    
    Package->FullyLoad();
    
    UTexture2D* AtlasTexture = NewObject<UTexture2D>(Package, FName(*AssetName), RF_Public | RF_Standalone);
    if (!AtlasTexture)
    {
        UE_LOG(LogSpriteOptimizer, Error, TEXT("Failed to create atlas texture object"));
        return nullptr;
    }
    
    // Create texture via Source API
    AtlasTexture->Source.Init(AtlasSize.X, AtlasSize.Y, 1, 1, TSF_BGRA8, (uint8*)AtlasPixels.GetData());
    
    // Copy settings from first sprite to preserve quality
    if (Sprites.Num() > 0 && Sprites[0] && Sprites[0]->GetSourceTexture())
    {
        UTexture2D* FirstTexture = Sprites[0]->GetSourceTexture();
        
        AtlasTexture->SRGB = FirstTexture->SRGB;
        AtlasTexture->CompressionSettings = FirstTexture->CompressionSettings;
        AtlasTexture->Filter = FirstTexture->Filter;
        AtlasTexture->AddressX = FirstTexture->AddressX;
        AtlasTexture->AddressY = FirstTexture->AddressY;
        AtlasTexture->MipGenSettings = FirstTexture->MipGenSettings;
        
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Copied texture settings from %s: SRGB=%d, Compression=%d, Filter=%d"), 
               *FirstTexture->GetName(), AtlasTexture->SRGB, (int32)AtlasTexture->CompressionSettings, (int32)AtlasTexture->Filter);
    }
    else
    {
        // Fallback settings
        AtlasTexture->SRGB = true;
        AtlasTexture->CompressionSettings = TC_Default;
        AtlasTexture->Filter = TF_Bilinear;
        AtlasTexture->AddressX = TA_Clamp;
        AtlasTexture->AddressY = TA_Clamp;
        AtlasTexture->MipGenSettings = TMGS_FromTextureGroup;
        
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Using fallback texture settings for atlas"));
    }
    
    AtlasTexture->UpdateResource();
    AtlasTexture->PostEditChange();
    (void)Package->MarkPackageDirty();
    
    // Save
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    bool bSaved = UPackage::SavePackage(Package, AtlasTexture, *PackageFileName, SaveArgs);
    
    if (bSaved)
    {
        FAssetRegistryModule::AssetCreated(AtlasTexture);
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Successfully created atlas texture: %s (%dx%d) in folder: Atlas"), 
               *AssetName, AtlasSize.X, AtlasSize.Y);
    }
    else
    {
        UE_LOG(LogSpriteOptimizer, Error, TEXT("Failed to save atlas texture: %s"), *FullAssetPath);
    }
    
    return AtlasTexture;
}

UPaperSprite* USpriteOptimizer::CreateSpriteFromAtlas(
    UTexture2D* AtlasTexture,
    const FIntRect& Region,
    UPaperSprite* OriginalSprite,
    const FString& SpriteName,
    const FString& AssetPath)
{
    if (!AtlasTexture || !OriginalSprite)
    {
        return nullptr;
    }
    
    // Create package for sprite
    FString FullAssetPath = AssetPath + TEXT("/") + SpriteName;
    FString PackageName = FullAssetPath;
    
    UPackage* Package = CreatePackage(*PackageName);
    Package->FullyLoad();
    
    UPaperSprite* AtlasSprite = NewObject<UPaperSprite>(Package, FName(*SpriteName), RF_Public | RF_Standalone);
    
    if (!AtlasSprite)
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Failed to create atlas sprite object"));
        return nullptr;
    }
    
    // Get original sprite data
    UTexture2D* OriginalTexture = OriginalSprite->GetSourceTexture();
    if (!OriginalTexture)
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Original sprite has no source texture"));
        return nullptr;
    }
    
    // Calculate correct pivot for layering
    FVector2D CorrectPivot;
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("=== ATLAS PIVOT COMPENSATION FOR: %s ==="), *SpriteName);
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Atlas region: (%d,%d,%d,%d)"), Region.Min.X, Region.Min.Y, Region.Max.X, Region.Max.Y);
    
    bool bWasOptimized = OriginalTexture->GetName().Contains(TEXT("_Optimized"));
    
    if (bWasOptimized)
    {
        // Case 1: Sprite was already optimized
        FVector2D OriginalPivot = OriginalSprite->GetPivotPosition();
        FVector2D OptimizedTextureSize(OriginalTexture->GetSizeX(), OriginalTexture->GetSizeY());
        FVector2D AtlasRegionSize(Region.Width(), Region.Height());
        
        FVector2D ScaleFactor(
            AtlasRegionSize.X / OptimizedTextureSize.X,
            AtlasRegionSize.Y / OptimizedTextureSize.Y
        );
        
        CorrectPivot = FVector2D(
            OriginalPivot.X * ScaleFactor.X,
            OriginalPivot.Y * ScaleFactor.Y
        );
        
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Optimized sprite: original pivot (%f,%f), final pivot (%f,%f)"), 
               OriginalPivot.X, OriginalPivot.Y, CorrectPivot.X, CorrectPivot.Y);
    }
    else
    {
        // Case 2: Compensate for atlas offset
        
        // Find used region in original texture
        FIntRect OriginalUsedRegion = FindUsedBounds(OriginalTexture, 2);
        if (OriginalUsedRegion.Width() <= 0 || OriginalUsedRegion.Height() <= 0)
        {
            OriginalUsedRegion = FIntRect(0, 0, OriginalTexture->GetSizeX(), OriginalTexture->GetSizeY());
        }
        
        // Get original pivot
        FVector2D OriginalPivot = OriginalSprite->GetPivotPosition();
        
        // Pivot relative to used region
        FVector2D PivotRelativeToUsedRegion = OriginalPivot - FVector2D(OriginalUsedRegion.Min.X, OriginalUsedRegion.Min.Y);
        
        // Scale to atlas region size
        FVector2D ScaleFactor(
            float(Region.Width()) / float(OriginalUsedRegion.Width()),
            float(Region.Height()) / float(OriginalUsedRegion.Height())
        );
        
        CorrectPivot = FVector2D(
            PivotRelativeToUsedRegion.X * ScaleFactor.X,
            PivotRelativeToUsedRegion.Y * ScaleFactor.Y
        );
        
        // Add atlas region offset for proper alignment
        CorrectPivot.X += Region.Min.X;
        CorrectPivot.Y += Region.Min.Y;
        
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Original sprite compensation:"));
        UE_LOG(LogSpriteOptimizer, Log, TEXT("  Original pivot: (%f,%f)"), OriginalPivot.X, OriginalPivot.Y);
        UE_LOG(LogSpriteOptimizer, Log, TEXT("  Used region: (%d,%d,%d,%d)"), 
               OriginalUsedRegion.Min.X, OriginalUsedRegion.Min.Y, OriginalUsedRegion.Max.X, OriginalUsedRegion.Max.Y);
        UE_LOG(LogSpriteOptimizer, Log, TEXT("  Pivot relative to used region: (%f,%f)"), 
               PivotRelativeToUsedRegion.X, PivotRelativeToUsedRegion.Y);
        UE_LOG(LogSpriteOptimizer, Log, TEXT("  Scale factor: (%f,%f)"), ScaleFactor.X, ScaleFactor.Y);
        UE_LOG(LogSpriteOptimizer, Log, TEXT("  Atlas region offset: (%d,%d)"), Region.Min.X, Region.Min.Y);
        UE_LOG(LogSpriteOptimizer, Log, TEXT("  Final compensated pivot: (%f,%f)"), CorrectPivot.X, CorrectPivot.Y);
    }
    
    // Get material for atlas sprite
    UMaterialInterface* SpriteMaterial = nullptr;
    
    if (OriginalSprite->GetDefaultMaterial())
    {
        SpriteMaterial = OriginalSprite->GetDefaultMaterial();
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Using original sprite material for atlas: %s"), *SpriteMaterial->GetName());
    }
    else
    {
        SpriteMaterial = GetDefaultPaper2DMaterial();
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Using default material for atlas sprite: %s"), 
               SpriteMaterial ? *SpriteMaterial->GetName() : TEXT("NULL"));
    }
    
    // Set up initialization parameters
    FSpriteAssetInitParameters InitParams;
    InitParams.Texture = AtlasTexture;
    InitParams.Offset = FIntPoint(Region.Min.X, Region.Min.Y);
    InitParams.Dimension = FIntPoint(Region.Width(), Region.Height());
    InitParams.DefaultMaterialOverride = SpriteMaterial;
    InitParams.bOverridePixelsPerUnrealUnit = true;
    InitParams.PixelsPerUnrealUnit = OriginalSprite->GetPixelsPerUnrealUnit();
    
    // Initialize sprite
    AtlasSprite->InitializeSprite(InitParams, false);
    
    // Set correct compensated pivot
    AtlasSprite->SetPivotMode(ESpritePivotMode::Custom, CorrectPivot, true);
    
    (void)Package->MarkPackageDirty();
    
    // Save
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    bool bSaved = UPackage::SavePackage(Package, AtlasSprite, *PackageFileName, SaveArgs);
    
    if (bSaved)
    {
        FAssetRegistryModule::AssetCreated(AtlasSprite);
        UE_LOG(LogSpriteOptimizer, Log, TEXT("✅ Created atlas sprite: %s with compensated pivot (%f,%f)"), 
               *FullAssetPath, CorrectPivot.X, CorrectPivot.Y);
    }
    else
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Failed to save atlas sprite: %s"), *FullAssetPath);
    }
    
    return AtlasSprite;
}

bool USpriteOptimizer::CopyPixelsFromSourceToAtlas(
    UTexture2D* SourceTexture, 
    TArray<FColor>& AtlasPixels, 
    const FIntRect& Region, 
    const FIntPoint& AtlasSize)
{
    if (!SourceTexture)
    {
        UE_LOG(LogSpriteOptimizer, Error, TEXT("Source texture is null"));
        return false;
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Copying pixels from %s (%dx%d) to region (%d,%d,%d,%d)"), 
           *SourceTexture->GetName(),
           SourceTexture->GetSizeX(), SourceTexture->GetSizeY(),
           Region.Min.X, Region.Min.Y, Region.Max.X, Region.Max.Y);
    
    // Get source texture dimensions
    int32 SourceWidth = SourceTexture->GetSizeX();
    int32 SourceHeight = SourceTexture->GetSizeY();
    
    // Determine copy region
    FIntRect SourceCopyRegion;
    
    bool bIsOptimizedTexture = SourceTexture->GetName().Contains(TEXT("_Optimized"));
    
    if (bIsOptimizedTexture)
    {
        // For optimized textures, copy entire texture
        SourceCopyRegion = FIntRect(0, 0, SourceWidth, SourceHeight);
        
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Source is optimized texture - using full texture region: (%d,%d,%d,%d)"), 
               SourceCopyRegion.Min.X, SourceCopyRegion.Min.Y, SourceCopyRegion.Max.X, SourceCopyRegion.Max.Y);
    }
    else
    {
        // For original textures, find used area
        SourceCopyRegion = FindUsedBounds(SourceTexture, 2);
        
        if (SourceCopyRegion.Width() <= 0 || SourceCopyRegion.Height() <= 0)
        {
            UE_LOG(LogSpriteOptimizer, Warning, TEXT("No used area found in texture %s, using full texture"), *SourceTexture->GetName());
            SourceCopyRegion = FIntRect(0, 0, SourceWidth, SourceHeight);
        }
        
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Source is original texture - using used bounds region: (%d,%d,%d,%d)"), 
               SourceCopyRegion.Min.X, SourceCopyRegion.Min.Y, SourceCopyRegion.Max.X, SourceCopyRegion.Max.Y);
    }
    
    // Get pixels from source texture
    TArray<FColor> SourcePixels = GetTexturePixelData(SourceTexture);
    if (SourcePixels.Num() == 0)
    {
        UE_LOG(LogSpriteOptimizer, Error, TEXT("Failed to read pixel data from %s"), *SourceTexture->GetName());
        return false;
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Read %d pixels from source texture"), SourcePixels.Num());
    
    // Calculate copy dimensions
    int32 SourceCopyWidth = SourceCopyRegion.Width();
    int32 SourceCopyHeight = SourceCopyRegion.Height();
    int32 AtlasRegionWidth = Region.Width();
    int32 AtlasRegionHeight = Region.Height();
    
    // Determine if scaling is needed
    bool bNeedsScaling = (SourceCopyWidth != AtlasRegionWidth) || (SourceCopyHeight != AtlasRegionHeight);
    
    if (bNeedsScaling)
    {
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Scaling needed: source (%dx%d) -> atlas region (%dx%d)"), 
               SourceCopyWidth, SourceCopyHeight, AtlasRegionWidth, AtlasRegionHeight);
        
        return CopyPixelsWithScaling(SourcePixels, SourceWidth, SourceCopyRegion, 
                                   AtlasPixels, AtlasSize, Region);
    }
    else
    {
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Direct copy: source (%dx%d) == atlas region (%dx%d)"), 
               SourceCopyWidth, SourceCopyHeight, AtlasRegionWidth, AtlasRegionHeight);
        
        return CopyPixelsDirect(SourcePixels, SourceWidth, SourceCopyRegion, 
                              AtlasPixels, AtlasSize, Region);
    }
}

bool USpriteOptimizer::CopyPixelsDirect(
    const TArray<FColor>& SourcePixels,
    int32 SourceWidth,
    const FIntRect& SourceRegion,
    TArray<FColor>& AtlasPixels,
    const FIntPoint& AtlasSize,
    const FIntRect& AtlasRegion)
{
    int32 CopiedPixels = 0;
    int32 CopyWidth = FMath::Min(SourceRegion.Width(), AtlasRegion.Width());
    int32 CopyHeight = FMath::Min(SourceRegion.Height(), AtlasRegion.Height());
    
    // Copy pixels row by row
    for (int32 Y = 0; Y < CopyHeight; Y++)
    {
        for (int32 X = 0; X < CopyWidth; X++)
        {
            // Source texture coordinates
            int32 SourceX = SourceRegion.Min.X + X;
            int32 SourceY = SourceRegion.Min.Y + Y;
            int32 SourceIndex = SourceY * SourceWidth + SourceX;
            
            // Atlas coordinates
            int32 AtlasX = AtlasRegion.Min.X + X;
            int32 AtlasY = AtlasRegion.Min.Y + Y;
            int32 AtlasIndex = AtlasY * AtlasSize.X + AtlasX;
            
            // Bounds checking
            if (SourceIndex >= 0 && SourceIndex < SourcePixels.Num() && 
                AtlasIndex >= 0 && AtlasIndex < AtlasPixels.Num() &&
                AtlasX >= 0 && AtlasX < AtlasSize.X &&
                AtlasY >= 0 && AtlasY < AtlasSize.Y)
            {
                AtlasPixels[AtlasIndex] = SourcePixels[SourceIndex];
                
                // Count only non-transparent pixels
                if (SourcePixels[SourceIndex].A > 0)
                {
                    CopiedPixels++;
                }
            }
        }
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Direct copy completed: %d visible pixels copied"), CopiedPixels);
    return CopiedPixels > 0;
}

bool USpriteOptimizer::CopyPixelsWithScaling(
    const TArray<FColor>& SourcePixels,
    int32 SourceWidth,
    const FIntRect& SourceRegion,
    TArray<FColor>& AtlasPixels,
    const FIntPoint& AtlasSize,
    const FIntRect& AtlasRegion)
{
    int32 CopiedPixels = 0;
    
    float ScaleX = float(SourceRegion.Width()) / float(AtlasRegion.Width());
    float ScaleY = float(SourceRegion.Height()) / float(AtlasRegion.Height());
    
    // Copy with scaling
    for (int32 AtlasY = 0; AtlasY < AtlasRegion.Height(); AtlasY++)
    {
        for (int32 AtlasX = 0; AtlasX < AtlasRegion.Width(); AtlasX++)
        {
            // Find corresponding pixel in source texture
            int32 SourceX = SourceRegion.Min.X + FMath::RoundToInt(AtlasX * ScaleX);
            int32 SourceY = SourceRegion.Min.Y + FMath::RoundToInt(AtlasY * ScaleY);
            
            // Check source texture bounds
            if (SourceX >= SourceRegion.Min.X && SourceX < SourceRegion.Max.X &&
                SourceY >= SourceRegion.Min.Y && SourceY < SourceRegion.Max.Y)
            {
                int32 SourceIndex = SourceY * SourceWidth + SourceX;
                
                // Atlas coordinates
                int32 FinalAtlasX = AtlasRegion.Min.X + AtlasX;
                int32 FinalAtlasY = AtlasRegion.Min.Y + AtlasY;
                int32 AtlasIndex = FinalAtlasY * AtlasSize.X + FinalAtlasX;
                
                // Check atlas bounds
                if (SourceIndex >= 0 && SourceIndex < SourcePixels.Num() &&
                    AtlasIndex >= 0 && AtlasIndex < AtlasPixels.Num() &&
                    FinalAtlasX >= 0 && FinalAtlasX < AtlasSize.X &&
                    FinalAtlasY >= 0 && FinalAtlasY < AtlasSize.Y)
                {
                    AtlasPixels[AtlasIndex] = SourcePixels[SourceIndex];
                    
                    if (SourcePixels[SourceIndex].A > 0)
                    {
                        CopiedPixels++;
                    }
                }
            }
        }
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Scaled copy completed: %d visible pixels copied"), CopiedPixels);
    return CopiedPixels > 0;
}

// === UTILITY HELPERS ===

TArray<FColor> USpriteOptimizer::GetTexturePixelData(UTexture2D* Texture)
{
    TArray<FColor> PixelData;
    
    if (!Texture)
    {
        UE_LOG(LogSpriteOptimizer, Error, TEXT("GetTexturePixelData: Texture is null"));
        return PixelData;
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Reading pixel data from texture: %s (%dx%d)"), 
           *Texture->GetName(), Texture->GetSizeX(), Texture->GetSizeY());
    
    // Method 1: Try Source API (most reliable)
    if (Texture->Source.IsValid())
    {
        TArray64<uint8> RawData;
        if (Texture->Source.GetMipData(RawData, 0) && RawData.Num() > 0)
        {
            int32 Width = Texture->GetSizeX();
            int32 Height = Texture->GetSizeY();
            int32 ExpectedPixels = Width * Height;
            
            if (RawData.Num() >= ExpectedPixels * 4) // BGRA = 4 bytes per pixel
            {
                const FColor* SourcePixels = reinterpret_cast<const FColor*>(RawData.GetData());
                PixelData.Reserve(ExpectedPixels);
                
                for (int32 i = 0; i < ExpectedPixels; i++)
                {
                    PixelData.Add(SourcePixels[i]);
                }
                
                UE_LOG(LogSpriteOptimizer, Log, TEXT("Successfully read %d pixels via Source API"), PixelData.Num());
                return PixelData;
            }
        }
    }
    
    // Method 2: Platform Data fallback
    if (!Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Texture %s has no platform data or mips"), *Texture->GetName());
        return PixelData;
    }
    
    FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
    
    // Check lock state
    if (Mip.BulkData.IsLocked())
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("BulkData already locked for texture %s"), *Texture->GetName());
        return PixelData;
    }
    
    const void* RawData;
    
    try
    {
        RawData = Mip.BulkData.LockReadOnly();
    }
    catch (...)
    {
        UE_LOG(LogSpriteOptimizer, Error, TEXT("Exception while locking texture data for %s"), *Texture->GetName());
        return PixelData;
    }
    
    if (!RawData)
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Failed to lock texture data for %s"), *Texture->GetName());
        try { Mip.BulkData.Unlock(); } catch (...) {}
        return PixelData;
    }
    
    int32 Width = Texture->GetSizeX();
    int32 Height = Texture->GetSizeY();
    int32 ExpectedPixels = Width * Height;
    
    const FColor* ColorData = static_cast<const FColor*>(RawData);
    PixelData.Reserve(ExpectedPixels);
    
    for (int32 i = 0; i < ExpectedPixels; i++)
    {
        PixelData.Add(ColorData[i]);
    }
    
    // Always unlock
    try
    {
        Mip.BulkData.Unlock();
    }
    catch (...)
    {
        UE_LOG(LogSpriteOptimizer, Error, TEXT("Exception while unlocking texture data for %s"), *Texture->GetName());
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Successfully read %d pixels via Platform Data"), PixelData.Num());
    
    return PixelData;
}

float USpriteOptimizer::CalculatePackingEfficiency(
    const TArray<FIntPoint>& SpriteSizes, 
    const FIntPoint& AtlasSize)
{
    if (AtlasSize.X <= 0 || AtlasSize.Y <= 0)
    {
        return 0.0f;
    }
    
    // Calculate total sprite area
    int32 TotalSpriteArea = 0;
    for (const FIntPoint& Size : SpriteSizes)
    {
        TotalSpriteArea += Size.X * Size.Y;
    }
    
    // Calculate atlas area
    int32 AtlasArea = AtlasSize.X * AtlasSize.Y;
    
    // Return efficiency percentage
    return AtlasArea > 0 ? (static_cast<float>(TotalSpriteArea) / AtlasArea) * 100.0f : 0.0f;
}

TArray<FIntPoint> USpriteOptimizer::GetOptimizedSpriteSizes(
    const TArray<UPaperSprite*>& Sprites,
    int32 Padding)
{
    TArray<FIntPoint> OptimizedSizes;
    
    for (UPaperSprite* Sprite : Sprites)
    {
        if (!Sprite || !Sprite->GetSourceTexture())
        {
            continue;
        }
        
        // Analyze sprite and get optimized size
        FSpriteOptimizationResult Analysis = AnalyzeSprite(Sprite);
        if (Analysis.bSuccess && Analysis.OptimizedSize.X > 0 && Analysis.OptimizedSize.Y > 0)
        {
            OptimizedSizes.Add(FIntPoint(Analysis.OptimizedSize.X, Analysis.OptimizedSize.Y));
        }
        else
        {
            // If analysis failed, use original size
            UTexture2D* SourceTexture = Sprite->GetSourceTexture();
            OptimizedSizes.Add(FIntPoint(SourceTexture->GetSizeX(), SourceTexture->GetSizeY()));
        }
    }
    
    return OptimizedSizes;
}

FVector2D USpriteOptimizer::CalculateAtlasPivotForLayering(
    int32 OriginalTextureWidth,
    int32 OriginalTextureHeight,
    const FIntRect& OriginalUsedRegion,
    const FIntRect& AtlasRegion,
    UPaperSprite* OriginalSprite)
{
    // Calculate center of original texture (assume central pivot)
    FVector2D OriginalTextureCenter(OriginalTextureWidth * 0.5f, OriginalTextureHeight * 0.5f);
    
    // Find center of used region in original texture
    FVector2D UsedRegionCenter(
        OriginalUsedRegion.Min.X + OriginalUsedRegion.Width() * 0.5f,
        OriginalUsedRegion.Min.Y + OriginalUsedRegion.Height() * 0.5f
    );
    
    // Calculate offset from original texture center to used region center
    FVector2D OffsetFromOriginalCenter = UsedRegionCenter - OriginalTextureCenter;
    
    // Center of new atlas sprite
    FVector2D AtlasSpriteSizeHalf(AtlasRegion.Width() * 0.5f, AtlasRegion.Height() * 0.5f);
    
    // Calculate pivot that compensates for offset
    FVector2D CorrectPivot = AtlasSpriteSizeHalf - OffsetFromOriginalCenter;
    
    UE_LOG(LogSpriteOptimizer, Verbose, TEXT("Pivot calculation:"));
    UE_LOG(LogSpriteOptimizer, Verbose, TEXT("  Original texture center: (%f,%f)"), OriginalTextureCenter.X, OriginalTextureCenter.Y);
    UE_LOG(LogSpriteOptimizer, Verbose, TEXT("  Used region center: (%f,%f)"), UsedRegionCenter.X, UsedRegionCenter.Y);
    UE_LOG(LogSpriteOptimizer, Verbose, TEXT("  Offset from center: (%f,%f)"), OffsetFromOriginalCenter.X, OffsetFromOriginalCenter.Y);
    UE_LOG(LogSpriteOptimizer, Verbose, TEXT("  Atlas sprite half-size: (%f,%f)"), AtlasSpriteSizeHalf.X, AtlasSpriteSizeHalf.Y);
    UE_LOG(LogSpriteOptimizer, Verbose, TEXT("  Final pivot: (%f,%f)"), CorrectPivot.X, CorrectPivot.Y);
    
    return CorrectPivot;
}

// === PATH AND ASSET HELPERS ===

FString USpriteOptimizer::GetOptimizedAssetPath(const FString& OriginalPath, const FSpriteOptimizationSettings& Settings)
{
    const USpriteOptimizerSettings* ProjectSettings = GetDefault<USpriteOptimizerSettings>();
    
    // Use custom path from project settings if specified
    if (ProjectSettings && !ProjectSettings->OptimizedAssetsPath.IsEmpty())
    {
        return ProjectSettings->OptimizedAssetsPath;
    }
    
    // Create "Optimized" folder next to original files
    FString Directory, Filename, Extension;
    FPaths::Split(OriginalPath, Directory, Filename, Extension);
    
    FString OptimizedDirectory = Directory + TEXT("/Optimized");
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Creating optimized assets in: %s"), *OptimizedDirectory);
    
    return OptimizedDirectory;
}

FString USpriteOptimizer::GetAtlasAssetPath(const FString& FirstSpritePath)
{
    FString Directory, Filename, Extension;
    FPaths::Split(FirstSpritePath, Directory, Filename, Extension);
    
    FString AtlasDirectory = Directory + TEXT("/Atlas");
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Creating atlas assets in: %s"), *AtlasDirectory);
    
    return AtlasDirectory;
}

FString USpriteOptimizer::GetOptimizedAssetName(const FString& OriginalName, const FSpriteOptimizationSettings& Settings)
{
    const USpriteOptimizerSettings* ProjectSettings = GetDefault<USpriteOptimizerSettings>();
    
    // Use suffix from project settings
    FString Suffix = TEXT("_Optimized");
    if (ProjectSettings && !ProjectSettings->OptimizedAssetsSuffix.IsEmpty())
    {
        Suffix = ProjectSettings->OptimizedAssetsSuffix;
    }
    
    return OriginalName + Suffix;
}

bool USpriteOptimizer::EnsureDirectoryExists(const FString& DirectoryPath)
{
    // Convert package path to file path
    FString PackageFilename;
    if (FPackageName::TryConvertLongPackageNameToFilename(DirectoryPath, PackageFilename))
    {
        FString PhysicalPath = FPaths::GetPath(PackageFilename);
        
        if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*PhysicalPath))
        {
            bool bCreated = FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*PhysicalPath);
            UE_LOG(LogSpriteOptimizer, Log, TEXT("Created directory: %s (Success: %s)"), *PhysicalPath, bCreated ? TEXT("Yes") : TEXT("No"));
            return bCreated;
        }
        return true;
    }
    
    UE_LOG(LogSpriteOptimizer, Warning, TEXT("Failed to convert package path to file path: %s"), *DirectoryPath);
    return false;
}

void USpriteOptimizer::CreateBackupIfNeeded(UObject* Asset, bool bCreateBackup)
{
    if (!bCreateBackup || !Asset)
    {
        return;
    }
    
    // In this simple implementation, just log
    // Full version could create copy with "_Backup" suffix
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Backup requested for asset: %s"), *Asset->GetName());
}