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

TArray<FSpriteOptimizationResult> USpriteOptimizer::OptimizeSprites(
    const TArray<UPaperSprite*>& Sprites, 
    const FSpriteOptimizationSettings& Settings)
{
    TArray<FSpriteOptimizationResult> Results;
    
    // Загружаем настройки из проекта если нужно
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
    
    // Показываем уведомление если включено в настройках
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
    
    // Обновляем Content Browser если включено в настройках
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
    
    // Находим использованную область
    Result.UsedRegion = FindUsedBounds(SourceTexture, Settings.Padding);
    
    if (Result.UsedRegion.Width() <= 0 || Result.UsedRegion.Height() <= 0)
    {
        Result.ErrorMessage = TEXT("No used region found in texture");
        return Result;
    }
    
    // Проверяем, стоит ли оптимизировать
    float UsagePercent = (float)(Result.UsedRegion.Width() * Result.UsedRegion.Height()) / 
                        (SourceTexture->GetSizeX() * SourceTexture->GetSizeY()) * 100.0f;
    
    if (UsagePercent > 90.0f)
    {
        Result.ErrorMessage = TEXT("Sprite already well optimized (>90% usage)");
        return Result;
    }
    
    // Определяем пути для сохранения
    FString OriginalPackagePath = Sprite->GetPackage()->GetName();
    FString OptimizedPath = GetOptimizedAssetPath(OriginalPackagePath, Settings);
    FString OptimizedName = GetOptimizedAssetName(Sprite->GetName(), Settings);
    
    // Создаем backup если нужно
    if (Settings.bCreateBackup && !Settings.bReplaceOriginals)
    {
        CreateBackupIfNeeded(Sprite, Settings.bCreateBackup);
    }
    
    // Создаем оптимизированную текстуру
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
    
    // Сохраняем путь к текстуре
    Result.OptimizedTexturePath = OptimizedPath + TEXT("/") + OptimizedName + TEXT("_Tex");
    
    // Создаем оптимизированный спрайт
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
    
    // Сохраняем путь к спрайту
    Result.OptimizedSpritePath = OptimizedPath + TEXT("/") + OptimizedName;
    
    // Вычисляем статистику
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
    
    // Симулируем оптимизированную текстуру для подсчетов
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
    
    // Находим границы непрозрачных пикселей
    int32 MinX = Width, MaxX = 0;
    int32 MinY = Height, MaxY = 0;
    bool FoundContent = false;
    
    for (int32 Y = 0; Y < Height; Y++)
    {
        for (int32 X = 0; X < Width; X++)
        {
            const FColor& Pixel = PixelData[Y * Width + X];
            
            // Если пиксель не полностью прозрачный
            if (Pixel.A > 10) // небольшой порог для сглаживания
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
    
    // Добавляем padding
    MinX = FMath::Max(0, MinX - Padding);
    MinY = FMath::Max(0, MinY - Padding);
    MaxX = FMath::Min(Width - 1, MaxX + Padding);
    MaxY = FMath::Min(Height - 1, MaxY + Padding);
    
    return FIntRect(MinX, MinY, MaxX + 1, MaxY + 1);
}

TArray<UMaterialInterface*> USpriteOptimizer::GetAvailablePaper2DMaterials()
{
    TArray<UMaterialInterface*> Materials;
    
    // Список правильных материалов Paper2D (по вашим путям)
    TArray<FString> Paper2DMaterialPaths = {
        TEXT("/Paper2D/TranslucentLitSpriteMaterial.TranslucentLitSpriteMaterial"), // ПО УМОЛЧАНИЮ
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
    // По умолчанию используем TranslucentLitSpriteMaterial как вы просили
    UMaterialInterface* Material = LoadObject<UMaterialInterface>(
        nullptr, 
        TEXT("/Paper2D/TranslucentLitSpriteMaterial.TranslucentLitSpriteMaterial")
    );
    
    if (Material)
    {
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Using default material: %s"), *Material->GetName());
        return Material;
    }
    
    // Fallback - пытаемся загрузить любой другой Paper2D материал
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
    
    // ВАЖНО: Создаем директорию если её нет
    if (!EnsureDirectoryExists(AssetPath))
    {
        UE_LOG(LogSpriteOptimizer, Error, TEXT("Failed to create directory: %s"), *AssetPath);
        return nullptr;
    }
    
    // Получаем пиксели из оригинальной текстуры
    TArray<FColor> SourcePixels = GetTexturePixelData(SourceTexture);
    if (SourcePixels.Num() == 0)
    {
        return nullptr;
    }
    
    // Извлекаем нужную область
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
    
    // Создаем пакет для новой текстуры
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
    
    // Настраиваем текстуру через Source API
    NewTexture->Source.Init(NewWidth, NewHeight, 1, 1, TSF_BGRA8, (uint8*)OptimizedPixels.GetData());
    
    // Копируем настройки с оригинала
    NewTexture->SRGB = SourceTexture->SRGB;
    NewTexture->CompressionSettings = SourceTexture->CompressionSettings;
    NewTexture->Filter = SourceTexture->Filter;
    NewTexture->AddressX = SourceTexture->AddressX;
    NewTexture->AddressY = SourceTexture->AddressY;
    
    NewTexture->UpdateResource();
    NewTexture->PostEditChange();
    Package->MarkPackageDirty();
    
    // Сохраняем
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
    
    // Создаем пакет для спрайта
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
    
    // Вычисляем новый пивот
    UTexture2D* OriginalTexture = OriginalSprite->GetSourceTexture();
    const FVector2D OriginalTextureSize(OriginalTexture->GetSizeX(), OriginalTexture->GetSizeY());
    const FVector2D OriginalPivotInPixels = OriginalTextureSize * 0.5f; // Предполагаем центральный пивот
    const FVector2D CroppedTextureTopLeftInPixels(UsedRegion.Min.X, UsedRegion.Min.Y);
    const FVector2D NewPivotInPixels = OriginalPivotInPixels - CroppedTextureTopLeftInPixels;
    
    // ИСПРАВЛЕНО: Определяем материал правильно
    UMaterialInterface* SpriteMaterial = nullptr;
    
    // 1. Сначала проверяем материал из настроек
    if (Settings.Material)
    {
        SpriteMaterial = Settings.Material;
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Using material from settings: %s"), *SpriteMaterial->GetName());
    }
    // 2. Если материал не задан, используем материал оригинального спрайта
    else if (OriginalSprite->GetDefaultMaterial())
    {
        SpriteMaterial = OriginalSprite->GetDefaultMaterial();
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Using original sprite material: %s"), *SpriteMaterial->GetName());
    }
    // 3. Если и у оригинала нет материала, используем дефолтный Paper2D
    else
    {
        SpriteMaterial = GetDefaultPaper2DMaterial();
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Using default Paper2D material: %s"), 
               SpriteMaterial ? *SpriteMaterial->GetName() : TEXT("NULL"));
    }
    
    // Настраиваем параметры инициализации
    FSpriteAssetInitParameters InitParams;
    InitParams.Texture = OptimizedTexture;
    InitParams.Offset = FIntPoint::ZeroValue;
    InitParams.Dimension = FIntPoint(OptimizedTexture->GetSizeX(), OptimizedTexture->GetSizeY());
    InitParams.DefaultMaterialOverride = SpriteMaterial; // ИСПРАВЛЕНО: правильный материал
    InitParams.bOverridePixelsPerUnrealUnit = true;
    InitParams.PixelsPerUnrealUnit = Settings.PixelsPerUnit;
    
    // Инициализируем спрайт
    NewSprite->InitializeSprite(InitParams, false);
    
    // Устанавливаем пивот
    NewSprite->SetPivotMode(ESpritePivotMode::Custom, NewPivotInPixels, true);
    
    Package->MarkPackageDirty();
    
    // Сохраняем
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
    
    // МЕТОД 1: Попытка через Source API (самый надежный)
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
    
    // МЕТОД 2: Через Platform Data (fallback)
    if (!Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Texture %s has no platform data or mips"), *Texture->GetName());
        return PixelData;
    }
    
    FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
    
    // Проверяем состояние блокировки
    if (Mip.BulkData.IsLocked())
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("BulkData already locked for texture %s"), *Texture->GetName());
        return PixelData;
    }
    
    const void* RawData = nullptr;
    
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
    
    // Обязательно разблокируем
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

FString USpriteOptimizer::GetOptimizedAssetPath(const FString& OriginalPath, const FSpriteOptimizationSettings& Settings)
{
    const USpriteOptimizerSettings* ProjectSettings = GetDefault<USpriteOptimizerSettings>();
    
    // Если указан кастомный путь в настройках проекта, используем его
    if (ProjectSettings && !ProjectSettings->OptimizedAssetsPath.IsEmpty())
    {
        return ProjectSettings->OptimizedAssetsPath;
    }
    
    // Создаем папку "Optimized" рядом с оригинальными файлами
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

bool USpriteOptimizer::EnsureDirectoryExists(const FString& DirectoryPath)
{
    // Конвертируем путь пакета в файловый путь
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

FString USpriteOptimizer::GetOptimizedAssetName(const FString& OriginalName, const FSpriteOptimizationSettings& Settings)
{
    const USpriteOptimizerSettings* ProjectSettings = GetDefault<USpriteOptimizerSettings>();
    
    // Используем суффикс из настроек проекта
    FString Suffix = TEXT("_Optimized");
    if (ProjectSettings && !ProjectSettings->OptimizedAssetsSuffix.IsEmpty())
    {
        Suffix = ProjectSettings->OptimizedAssetsSuffix;
    }
    
    return OriginalName + Suffix;
}

void USpriteOptimizer::CreateBackupIfNeeded(UObject* Asset, bool bCreateBackup)
{
    if (!bCreateBackup || !Asset)
    {
        return;
    }
    
    // В этой простой реализации просто логируем
    // В полной версии можно создать копию с суффиксом "_Backup"
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Backup requested for asset: %s"), *Asset->GetName());
}

void USpriteOptimizer::RefreshContentBrowser()
{
    // Обновляем Content Browser
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

// === ATLAS IMPLEMENTATION ===

FSpriteAtlasResult USpriteOptimizer::CreateSpriteAtlas(
    const TArray<UPaperSprite*>& Sprites,
    const FSpriteAtlasSettings& Settings,
    const FString& AtlasName,
    const FString& AtlasPath)
{
    FSpriteAtlasResult Result;
    Result.TotalSprites = Sprites.Num();
    
    // Расширенная валидация
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
    
    // Проверяем валидность спрайтов
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
    
    // ВАЖНО: Определяем путь для сохранения атласа в папке Atlas
    FString ActualAtlasPath;
    if (AtlasPath.IsEmpty())
    {
        // Получаем путь первого спрайта и создаем папку Atlas рядом
        FString FirstSpritePath = Sprites[0]->GetPackage()->GetName();
        ActualAtlasPath = GetAtlasAssetPath(FirstSpritePath);
    }
    else
    {
        ActualAtlasPath = AtlasPath;
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Atlas will be created in: %s"), *ActualAtlasPath);
    
    // Получаем размеры спрайтов (оптимизированные или оригинальные)
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
    
    // Упаковываем спрайты
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
    
    // Проверяем размер атласа
    if (AtlasSize.X > Settings.MaxAtlasSize.X || AtlasSize.Y > Settings.MaxAtlasSize.Y)
    {
        Result.ErrorMessage = FString::Printf(TEXT("Atlas size (%dx%d) exceeds maximum (%dx%d)"), 
                                            AtlasSize.X, AtlasSize.Y, 
                                            Settings.MaxAtlasSize.X, Settings.MaxAtlasSize.Y);
        return Result;
    }
    
    // Создаем атласную текстуру В ПАПКЕ ATLAS
    Result.AtlasTexture = CreateAtlasTexture(Sprites, PackedRegions, AtlasSize, AtlasName, ActualAtlasPath);
    
    if (!Result.AtlasTexture)
    {
        Result.ErrorMessage = TEXT("Failed to create atlas texture");
        return Result;
    }
    
    Result.AtlasTexturePath = ActualAtlasPath + TEXT("/") + AtlasName;
    
    // Создаем отдельные спрайты если нужно В ТОЙ ЖЕ ПАПКЕ ATLAS
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
                ActualAtlasPath  // ТА ЖЕ ПАПКА ДЛЯ СПРАЙТОВ
            );
            
            if (AtlasSprite)
            {
                Result.CreatedSprites.Add(AtlasSprite);
                UE_LOG(LogSpriteOptimizer, Log, TEXT("Created atlas sprite: %s in folder: Atlas"), *SpriteName);
            }
        }
    }
    
    // Заполняем результаты
    Result.SpriteRegions = PackedRegions;
    Result.AtlasSize = AtlasSize;
    Result.PackingEfficiency = CalculatePackingEfficiency(SpriteSizes, AtlasSize);
    
    // Вычисляем экономию памяти
    float OriginalMemory = 0.0f;
    for (UPaperSprite* Sprite : Sprites)
    {
        if (Sprite && Sprite->GetSourceTexture())
        {
            UTexture2D* OriginalTexture = Sprite->GetSourceTexture();
            int32 OriginalPixels = OriginalTexture->GetSizeX() * OriginalTexture->GetSizeY();
            OriginalMemory += (OriginalPixels * 4) / (1024.0f * 1024.0f); // RGBA, MB
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
    
    // Получаем размеры спрайтов
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
    
    // Симулируем упаковку
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
    
    // Заполняем результаты анализа
    Result.AtlasSize = AtlasSize;
    Result.SpriteRegions = PackedRegions;
    Result.PackingEfficiency = CalculatePackingEfficiency(SpriteSizes, AtlasSize);
    
    // Вычисляем экономию памяти
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
    
    // Проверяем ограничения
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
    
    // Простая сетка - вычисляем оптимальную компоновку
    int32 SpritesPerRow = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(SpriteSizes.Num())));
    
    int32 CurrentX = 0;
    int32 CurrentY = 0;
    int32 MaxRowHeight = 0;
    int32 MaxAtlasWidth = 0;
    
    for (int32 i = 0; i < SpriteSizes.Num(); i++)
    {
        const FIntPoint& SpriteSize = SpriteSizes[i];
        
        // Проверяем, помещается ли спрайт в текущую строку
        if (i > 0 && i % SpritesPerRow == 0)
        {
            // Переходим на новую строку
            CurrentY += MaxRowHeight + Settings.SpritePadding;
            CurrentX = 0;
            MaxRowHeight = 0;
        }
        
        // Добавляем регион
        FIntRect Region(CurrentX, CurrentY, CurrentX + SpriteSize.X, CurrentY + SpriteSize.Y);
        PackedRegions.Add(Region);
        
        // Обновляем позицию и размеры
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
    
    // Сортируем спрайты по площади (большие первыми) для лучшей упаковки
    TArray<TPair<int32, FIntPoint>> SortedSprites;
    for (int32 i = 0; i < SpriteSizes.Num(); i++)
    {
        SortedSprites.Add(TPair<int32, FIntPoint>(i, SpriteSizes[i]));
    }
    
    // УЛУЧШЕННАЯ СОРТИРОВКА: сначала по высоте, потом по площади
    SortedSprites.Sort([](const TPair<int32, FIntPoint>& A, const TPair<int32, FIntPoint>& B)
    {
        // Сначала сортируем по высоте (высокие первыми)
        if (A.Value.Y != B.Value.Y)
        {
            return A.Value.Y > B.Value.Y;
        }
        // Потом по ширине
        if (A.Value.X != B.Value.X)
        {
            return A.Value.X > B.Value.X;
        }
        // И наконец по площади
        int32 AreaA = A.Value.X * A.Value.Y;
        int32 AreaB = B.Value.X * B.Value.Y;
        return AreaA > AreaB;
    });
    
    // Инициализируем результат правильным размером
    PackedRegions.Init(FIntRect(0, 0, 0, 0), SpriteSizes.Num());
    
    // УЛУЧШЕННЫЙ АЛГОРИТМ: начинаем с более компактного размера
    int32 TotalArea = 0;
    int32 MaxWidth = 0;
    int32 MaxHeight = 0;
    
    for (const FIntPoint& Size : SpriteSizes)
    {
        TotalArea += Size.X * Size.Y;
        MaxWidth = FMath::Max(MaxWidth, Size.X);
        MaxHeight = FMath::Max(MaxHeight, Size.Y);
    }
    
    // Начальный размер основан на общей площади с коэффициентом упаковки
    float PackingEfficiencyFactor = 1.3f; // 30% запас на неидеальную упаковку
    int32 EstimatedSize = FMath::CeilToInt(FMath::Sqrt(TotalArea * PackingEfficiencyFactor));
    
    int32 CurrentWidth = FMath::Max(EstimatedSize, MaxWidth);
    int32 CurrentHeight = FMath::Max(EstimatedSize, MaxHeight);
    
    // Убеждаемся что размер не меньше минимального
    CurrentWidth = FMath::Max(CurrentWidth, 256);
    CurrentHeight = FMath::Max(CurrentHeight, 256);
    
    bool bPackingSuccessful = false;
    int32 Attempts = 0;
    const int32 MaxAttempts = 15;
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("BestFit: Starting with estimated size %dx%d for %d sprites (total area: %d)"), 
           CurrentWidth, CurrentHeight, SpriteSizes.Num(), TotalArea);
    
    while (!bPackingSuccessful && Attempts < MaxAttempts)
    {
        // Список занятых прямоугольников (для избежания перекрытий)
        TArray<FIntRect> OccupiedRects;
        TArray<FIntRect> TempPackedRegions;
        TempPackedRegions.Init(FIntRect(0, 0, 0, 0), SpriteSizes.Num());
        
        bool bAllSpritesPlaced = true;
        int32 UsedWidth = 0;
        int32 UsedHeight = 0;
        
        // Пытаемся разместить все спрайты
        for (const auto& SpriteData : SortedSprites)
        {
            int32 OriginalIndex = SpriteData.Key;
            FIntPoint SpriteSize = SpriteData.Value;
            
            // УЛУЧШЕННЫЙ ПОИСК ПОЗИЦИИ: предпочитаем левый верхний угол и движемся вправо-вниз
            FIntPoint BestPosition(-1, -1);
            int32 BestScore = INT_MAX;
            
            // Сканируем возможные позиции - ИЗМЕНЕН ПОРЯДОК: сначала Y, потом X
            for (int32 Y = 0; Y <= CurrentHeight - SpriteSize.Y; Y += 2) // Уменьшили шаг для точности
            {
                for (int32 X = 0; X <= CurrentWidth - SpriteSize.X; X += 2)
                {
                    FIntRect TestRect(X, Y, X + SpriteSize.X, Y + SpriteSize.Y);
                    
                    // Проверяем пересечение с уже размещенными спрайтами
                    bool bOverlaps = false;
                    for (const FIntRect& Occupied : OccupiedRects)
                    {
                        // Проверка пересечения прямоугольников
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
                        // УЛУЧШЕННАЯ ФУНКЦИЯ ОЦЕНКИ: предпочитаем верхний левый угол
                        int32 Score = Y * 1000 + X; // Сначала минимизируем Y, потом X
                        
                        // Бонус за компактность - если спрайт близко к другим
                        int32 CompactnessBonus = 0;
                        for (const FIntRect& Occupied : OccupiedRects)
                        {
                            // Проверяем соседство
                            bool bIsNeighbor = false;
                            
                            // Сосед справа
                            if (FMath::Abs(TestRect.Min.X - Occupied.Max.X) <= Settings.SpritePadding && 
                                !(TestRect.Max.Y <= Occupied.Min.Y || TestRect.Min.Y >= Occupied.Max.Y))
                            {
                                bIsNeighbor = true;
                            }
                            // Сосед снизу
                            else if (FMath::Abs(TestRect.Min.Y - Occupied.Max.Y) <= Settings.SpritePadding && 
                                     !(TestRect.Max.X <= Occupied.Min.X || TestRect.Min.X >= Occupied.Max.X))
                            {
                                bIsNeighbor = true;
                            }
                            
                            if (bIsNeighbor)
                            {
                                CompactnessBonus += 100; // Бонус за соседство
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
            
            // Размещаем спрайт с учетом padding
            FIntRect PlacedRect(
                BestPosition.X, 
                BestPosition.Y, 
                BestPosition.X + SpriteSize.X, 
                BestPosition.Y + SpriteSize.Y
            );
            
            // Добавляем padding к занятому пространству
            FIntRect OccupiedRect(
                BestPosition.X, 
                BestPosition.Y, 
                BestPosition.X + SpriteSize.X + Settings.SpritePadding, 
                BestPosition.Y + SpriteSize.Y + Settings.SpritePadding
            );
            
            TempPackedRegions[OriginalIndex] = PlacedRect;
            OccupiedRects.Add(OccupiedRect);
            
            // Обновляем используемую область
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
            // Увеличиваем размер для следующей попытки
            if (CurrentWidth <= CurrentHeight)
            {
                CurrentWidth = FMath::Min(CurrentWidth + 64, Settings.MaxAtlasSize.X);
            }
            else
            {
                CurrentHeight = FMath::Min(CurrentHeight + 64, Settings.MaxAtlasSize.Y);
            }
            
            // Проверяем, не достигли ли максимального размера
            if (CurrentWidth >= Settings.MaxAtlasSize.X && CurrentHeight >= Settings.MaxAtlasSize.Y)
            {
                UE_LOG(LogSpriteOptimizer, Warning, TEXT("Reached maximum atlas size, trying Simple algorithm"));
                // Fallback к простому алгоритму
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
        
        // Fallback к простому алгоритму
        return PackSprites_Simple(SpriteSizes, Settings, OutAtlasSize);
    }
    
    return PackedRegions;
}

TArray<FIntRect> USpriteOptimizer::PackSprites_MaxRects(
    const TArray<FIntPoint>& SpriteSizes, 
    const FSpriteAtlasSettings& Settings, 
    FIntPoint& OutAtlasSize)
{
    // Для простоты используем BestFit алгоритм
    // В полной реализации здесь был бы настоящий MaxRects
    UE_LOG(LogSpriteOptimizer, Log, TEXT("MaxRects algorithm using BestFit implementation"));
    return PackSprites_BestFit(SpriteSizes, Settings, OutAtlasSize);
}

float USpriteOptimizer::CalculatePackingEfficiency(
    const TArray<FIntPoint>& SpriteSizes, 
    const FIntPoint& AtlasSize)
{
    if (AtlasSize.X <= 0 || AtlasSize.Y <= 0)
    {
        return 0.0f;
    }
    
    // Вычисляем общую площадь спрайтов
    int32 TotalSpriteArea = 0;
    for (const FIntPoint& Size : SpriteSizes)
    {
        TotalSpriteArea += Size.X * Size.Y;
    }
    
    // Вычисляем площадь атласа
    int32 AtlasArea = AtlasSize.X * AtlasSize.Y;
    
    // Возвращаем эффективность в процентах
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
        
        // Анализируем спрайт и получаем оптимизированный размер
        FSpriteOptimizationResult Analysis = AnalyzeSprite(Sprite);
        if (Analysis.bSuccess && Analysis.OptimizedSize.X > 0 && Analysis.OptimizedSize.Y > 0)
        {
            OptimizedSizes.Add(FIntPoint(Analysis.OptimizedSize.X, Analysis.OptimizedSize.Y));
        }
        else
        {
            // Если анализ не удался, используем оригинальный размер
            UTexture2D* SourceTexture = Sprite->GetSourceTexture();
            OptimizedSizes.Add(FIntPoint(SourceTexture->GetSizeX(), SourceTexture->GetSizeY()));
        }
    }
    
    return OptimizedSizes;
}

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
    
    // ВАЖНО: Создаем директорию Atlas если её нет
    if (!EnsureDirectoryExists(AssetPath))
    {
        UE_LOG(LogSpriteOptimizer, Error, TEXT("Failed to create atlas directory: %s"), *AssetPath);
        return nullptr;
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Creating atlas texture %dx%d with %d sprites in: %s"), 
           AtlasSize.X, AtlasSize.Y, Sprites.Num(), *AssetPath);
    
    // Создаем массив пикселей для атласа (инициализируем прозрачным черным)
    TArray<FColor> AtlasPixels;
    int32 TotalPixels = AtlasSize.X * AtlasSize.Y;
    AtlasPixels.Init(FColor(0, 0, 0, 0), TotalPixels);
    
    // Обрабатываем каждый спрайт
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
        
        // Копируем пиксели
        if (!CopyPixelsFromSourceToAtlas(SourceTexture, AtlasPixels, Region, AtlasSize))
        {
            UE_LOG(LogSpriteOptimizer, Warning, TEXT("Failed to copy pixels from sprite: %s"), *Sprite->GetName());
        }
    }
    
    // Создаем пакет для новой текстуры
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
    
    // Создание текстуры через Source API
    AtlasTexture->Source.Init(AtlasSize.X, AtlasSize.Y, 1, 1, TSF_BGRA8, (uint8*)AtlasPixels.GetData());
    
    // ИСПРАВЛЕНО: Копируем настройки с первого спрайта для сохранения качества
    if (Sprites.Num() > 0 && Sprites[0] && Sprites[0]->GetSourceTexture())
    {
        UTexture2D* FirstTexture = Sprites[0]->GetSourceTexture();
        
        // Копируем настройки фильтрации и сжатия с оригинала
        AtlasTexture->SRGB = FirstTexture->SRGB;
        AtlasTexture->CompressionSettings = FirstTexture->CompressionSettings; // ИЗМЕНЕНО: копируем с оригинала
        AtlasTexture->Filter = FirstTexture->Filter; // ИЗМЕНЕНО: копируем фильтрацию с оригинала
        AtlasTexture->AddressX = FirstTexture->AddressX;
        AtlasTexture->AddressY = FirstTexture->AddressY;
        AtlasTexture->MipGenSettings = FirstTexture->MipGenSettings; // ИЗМЕНЕНО: копируем настройки мипмапов
        
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Copied texture settings from %s: SRGB=%d, Compression=%d, Filter=%d"), 
               *FirstTexture->GetName(), AtlasTexture->SRGB, (int32)AtlasTexture->CompressionSettings, (int32)AtlasTexture->Filter);
    }
    else
    {
        // Fallback настройки для сглаженных изображений
        AtlasTexture->SRGB = true;
        AtlasTexture->CompressionSettings = TC_Default; // Стандартное сжатие
        AtlasTexture->Filter = TF_Bilinear; // Сглаженная фильтрация
        AtlasTexture->AddressX = TA_Clamp;
        AtlasTexture->AddressY = TA_Clamp;
        AtlasTexture->MipGenSettings = TMGS_FromTextureGroup; // Стандартные мипмапы
        
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Using fallback texture settings for atlas"));
    }
    
    // Принудительно обновляем все данные
    AtlasTexture->UpdateResource();
    AtlasTexture->PostEditChange();
    Package->MarkPackageDirty();
    
    // Сохраняем
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
    
    // Создаем пакет для спрайта
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
    
    // Получаем данные оригинального спрайта
    UTexture2D* OriginalTexture = OriginalSprite->GetSourceTexture();
    if (!OriginalTexture)
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Original sprite has no source texture"));
        return nullptr;
    }
    
    // ПРАВИЛЬНЫЙ РАСЧЕТ ПИВОТА ДЛЯ ПОСЛОЙНОЙ КОМПОЗИЦИИ
    FVector2D CorrectPivot;
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("=== ATLAS PIVOT COMPENSATION FOR: %s ==="), *SpriteName);
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Atlas region: (%d,%d,%d,%d)"), Region.Min.X, Region.Min.Y, Region.Max.X, Region.Max.Y);
    
    // Определяем, использовался ли спрайт оптимизированный или оригинальный
    bool bWasOptimized = OriginalTexture->GetName().Contains(TEXT("_Optimized"));
    
    if (bWasOptimized)
    {
        // СЛУЧАЙ 1: Спрайт уже был оптимизирован
        // Пивот уже правильный, просто масштабируем
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
        // СЛУЧАЙ 2: КЛЮЧЕВОЕ РЕШЕНИЕ - Компенсация смещения в атласе
        
        // 1. Находим используемую область в оригинальной текстуре
        FIntRect OriginalUsedRegion = FindUsedBounds(OriginalTexture, 2);
        if (OriginalUsedRegion.Width() <= 0 || OriginalUsedRegion.Height() <= 0)
        {
            OriginalUsedRegion = FIntRect(0, 0, OriginalTexture->GetSizeX(), OriginalTexture->GetSizeY());
        }
        
        // 2. Получаем оригинальный пивот
        FVector2D OriginalPivot = OriginalSprite->GetPivotPosition();
        
        // 3. КЛЮЧЕВАЯ ФОРМУЛА: Пивот в атласе = пивот относительно используемой области
        FVector2D PivotRelativeToUsedRegion = OriginalPivot - FVector2D(OriginalUsedRegion.Min.X, OriginalUsedRegion.Min.Y);
        
        // 4. Масштабируем к размеру региона в атласе (если отличается)
        FVector2D ScaleFactor(
            float(Region.Width()) / float(OriginalUsedRegion.Width()),
            float(Region.Height()) / float(OriginalUsedRegion.Height())
        );
        
        CorrectPivot = FVector2D(
            PivotRelativeToUsedRegion.X * ScaleFactor.X,
            PivotRelativeToUsedRegion.Y * ScaleFactor.Y
        );
        
        // 5. ВАЖНО: Добавляем смещение чтобы компенсировать разное расположение в атласе
        // Это гарантирует, что все спрайты будут выровнены одинаково при размещении в одной точке
        
        // Находим "базовую позицию" - где должен быть первый спрайт
        // Все остальные спрайты будут выровнены относительно этой позиции
        
        // Для этого используем смещение от левого верхнего угла атласа
        // Все спрайты должны иметь одинаковое смещение от их позиции в атласе
        
        // Добавляем смещение позиции региона в атласе к пивоту
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
    
    // Безопасное получение материала для атласного спрайта
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
    
    // Настраиваем параметры инициализации
    FSpriteAssetInitParameters InitParams;
    InitParams.Texture = AtlasTexture;
    InitParams.Offset = FIntPoint(Region.Min.X, Region.Min.Y);
    InitParams.Dimension = FIntPoint(Region.Width(), Region.Height());
    InitParams.DefaultMaterialOverride = SpriteMaterial;
    InitParams.bOverridePixelsPerUnrealUnit = true;
    InitParams.PixelsPerUnrealUnit = OriginalSprite->GetPixelsPerUnrealUnit();
    
    // Инициализируем спрайт
    AtlasSprite->InitializeSprite(InitParams, false);
    
    // Устанавливаем правильный компенсированный пивот
    AtlasSprite->SetPivotMode(ESpritePivotMode::Custom, CorrectPivot, true);
    
    Package->MarkPackageDirty();
    
    // Сохраняем
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
    
    // Получаем размеры исходной текстуры
    int32 SourceWidth = SourceTexture->GetSizeX();
    int32 SourceHeight = SourceTexture->GetSizeY();
    
    // УЛУЧШЕННОЕ ОПРЕДЕЛЕНИЕ ОБЛАСТИ КОПИРОВАНИЯ
    FIntRect SourceCopyRegion;
    
    // Проверяем, является ли это оптимизированной текстурой
    bool bIsOptimizedTexture = SourceTexture->GetName().Contains(TEXT("_Optimized"));
    
    if (bIsOptimizedTexture)
    {
        // Для оптимизированных текстур копируем всю текстуру целиком
        // так как она уже обрезана до нужного размера
        SourceCopyRegion = FIntRect(0, 0, SourceWidth, SourceHeight);
        
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Source is optimized texture - using full texture region: (%d,%d,%d,%d)"), 
               SourceCopyRegion.Min.X, SourceCopyRegion.Min.Y, SourceCopyRegion.Max.X, SourceCopyRegion.Max.Y);
    }
    else
    {
        // Для неоптимизированных текстур находим используемую область
        SourceCopyRegion = FindUsedBounds(SourceTexture, 2);
        
        if (SourceCopyRegion.Width() <= 0 || SourceCopyRegion.Height() <= 0)
        {
            UE_LOG(LogSpriteOptimizer, Warning, TEXT("No used area found in texture %s, using full texture"), *SourceTexture->GetName());
            SourceCopyRegion = FIntRect(0, 0, SourceWidth, SourceHeight);
        }
        
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Source is original texture - using used bounds region: (%d,%d,%d,%d)"), 
               SourceCopyRegion.Min.X, SourceCopyRegion.Min.Y, SourceCopyRegion.Max.X, SourceCopyRegion.Max.Y);
    }
    
    // Получаем пиксели из исходной текстуры
    TArray<FColor> SourcePixels = GetTexturePixelData(SourceTexture);
    if (SourcePixels.Num() == 0)
    {
        UE_LOG(LogSpriteOptimizer, Error, TEXT("Failed to read pixel data from %s"), *SourceTexture->GetName());
        return false;
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Read %d pixels from source texture"), SourcePixels.Num());
    
    // Вычисляем размеры для копирования
    int32 SourceCopyWidth = SourceCopyRegion.Width();
    int32 SourceCopyHeight = SourceCopyRegion.Height();
    int32 AtlasRegionWidth = Region.Width();
    int32 AtlasRegionHeight = Region.Height();
    
    // ВАЖНО: Определяем способ копирования
    bool bNeedsScaling = (SourceCopyWidth != AtlasRegionWidth) || (SourceCopyHeight != AtlasRegionHeight);
    
    if (bNeedsScaling)
    {
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Scaling needed: source (%dx%d) -> atlas region (%dx%d)"), 
               SourceCopyWidth, SourceCopyHeight, AtlasRegionWidth, AtlasRegionHeight);
        
        // Копирование с масштабированием (может быть нужно для особых случаев)
        return CopyPixelsWithScaling(SourcePixels, SourceWidth, SourceCopyRegion, 
                                   AtlasPixels, AtlasSize, Region);
    }
    else
    {
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Direct copy: source (%dx%d) == atlas region (%dx%d)"), 
               SourceCopyWidth, SourceCopyHeight, AtlasRegionWidth, AtlasRegionHeight);
        
        // Прямое копирование без масштабирования
        return CopyPixelsDirect(SourcePixels, SourceWidth, SourceCopyRegion, 
                              AtlasPixels, AtlasSize, Region);
    }
}

// Вспомогательный метод для прямого копирования
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
    
    // Копируем пиксели построчно
    for (int32 Y = 0; Y < CopyHeight; Y++)
    {
        for (int32 X = 0; X < CopyWidth; X++)
        {
            // Координаты в исходной текстуре
            int32 SourceX = SourceRegion.Min.X + X;
            int32 SourceY = SourceRegion.Min.Y + Y;
            int32 SourceIndex = SourceY * SourceWidth + SourceX;
            
            // Координаты в атласе
            int32 AtlasX = AtlasRegion.Min.X + X;
            int32 AtlasY = AtlasRegion.Min.Y + Y;
            int32 AtlasIndex = AtlasY * AtlasSize.X + AtlasX;
            
            // Проверяем границы
            if (SourceIndex >= 0 && SourceIndex < SourcePixels.Num() && 
                AtlasIndex >= 0 && AtlasIndex < AtlasPixels.Num() &&
                AtlasX >= 0 && AtlasX < AtlasSize.X &&
                AtlasY >= 0 && AtlasY < AtlasSize.Y)
            {
                AtlasPixels[AtlasIndex] = SourcePixels[SourceIndex];
                
                // Считаем только непрозрачные пиксели
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

// Вспомогательный метод для копирования с масштабированием
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
    
    // Копируем с масштабированием
    for (int32 AtlasY = 0; AtlasY < AtlasRegion.Height(); AtlasY++)
    {
        for (int32 AtlasX = 0; AtlasX < AtlasRegion.Width(); AtlasX++)
        {
            // Находим соответствующий пиксель в исходной текстуре
            int32 SourceX = SourceRegion.Min.X + FMath::RoundToInt(AtlasX * ScaleX);
            int32 SourceY = SourceRegion.Min.Y + FMath::RoundToInt(AtlasY * ScaleY);
            
            // Проверяем границы исходной текстуры
            if (SourceX >= SourceRegion.Min.X && SourceX < SourceRegion.Max.X &&
                SourceY >= SourceRegion.Min.Y && SourceY < SourceRegion.Max.Y)
            {
                int32 SourceIndex = SourceY * SourceWidth + SourceX;
                
                // Координаты в атласе
                int32 FinalAtlasX = AtlasRegion.Min.X + AtlasX;
                int32 FinalAtlasY = AtlasRegion.Min.Y + AtlasY;
                int32 AtlasIndex = FinalAtlasY * AtlasSize.X + FinalAtlasX;
                
                // Проверяем границы атласа
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

FVector2D USpriteOptimizer::CalculateAtlasPivotForLayering(
    int32 OriginalTextureWidth,
    int32 OriginalTextureHeight,
    const FIntRect& OriginalUsedRegion,
    const FIntRect& AtlasRegion,
    UPaperSprite* OriginalSprite)
{
    // Шаг 1: Определяем центр оригинальной текстуры (предполагаем центральный пивот)
    FVector2D OriginalTextureCenter(OriginalTextureWidth * 0.5f, OriginalTextureHeight * 0.5f);
    
    // Шаг 2: Находим центр использованной области в оригинальной текстуре
    FVector2D UsedRegionCenter(
        OriginalUsedRegion.Min.X + OriginalUsedRegion.Width() * 0.5f,
        OriginalUsedRegion.Min.Y + OriginalUsedRegion.Height() * 0.5f
    );
    
    // Шаг 3: Вычисляем смещение от центра оригинальной текстуры до центра используемой области
    FVector2D OffsetFromOriginalCenter = UsedRegionCenter - OriginalTextureCenter;
    
    // Шаг 4: Центр нового спрайта из атласа
    FVector2D AtlasSpriteSizeHalf(AtlasRegion.Width() * 0.5f, AtlasRegion.Height() * 0.5f);
    
    // Шаг 5: Вычисляем пивот который компенсирует смещение
    // Пивот должен быть в центре атласного спрайта МИНУС смещение от оригинала
    FVector2D CorrectPivot = AtlasSpriteSizeHalf - OffsetFromOriginalCenter;
    
    UE_LOG(LogSpriteOptimizer, Verbose, TEXT("Pivot calculation:"));
    UE_LOG(LogSpriteOptimizer, Verbose, TEXT("  Original texture center: (%f,%f)"), OriginalTextureCenter.X, OriginalTextureCenter.Y);
    UE_LOG(LogSpriteOptimizer, Verbose, TEXT("  Used region center: (%f,%f)"), UsedRegionCenter.X, UsedRegionCenter.Y);
    UE_LOG(LogSpriteOptimizer, Verbose, TEXT("  Offset from center: (%f,%f)"), OffsetFromOriginalCenter.X, OffsetFromOriginalCenter.Y);
    UE_LOG(LogSpriteOptimizer, Verbose, TEXT("  Atlas sprite half-size: (%f,%f)"), AtlasSpriteSizeHalf.X, AtlasSpriteSizeHalf.Y);
    UE_LOG(LogSpriteOptimizer, Verbose, TEXT("  Final pivot: (%f,%f)"), CorrectPivot.X, CorrectPivot.Y);
    
    return CorrectPivot;
}