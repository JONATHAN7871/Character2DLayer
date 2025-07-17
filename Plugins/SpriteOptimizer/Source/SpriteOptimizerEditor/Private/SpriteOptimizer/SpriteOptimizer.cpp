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
    
    // Список стандартных материалов Paper2D
    TArray<FString> Paper2DMaterialPaths = {
        TEXT("/Paper2D/DefaultSpriteMaterial.DefaultSpriteMaterial"),
        TEXT("/Paper2D/DefaultMaskedSpriteMaterial.DefaultMaskedSpriteMaterial"),
        TEXT("/Paper2D/DefaultTranslucentSpriteMaterial.DefaultTranslucentSpriteMaterial"),
        TEXT("/Paper2D/DefaultUnlitSpriteMaterial.DefaultUnlitSpriteMaterial"),
        TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"),
        TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Opaque.Widget3DPassThrough_Opaque"),
        TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent")
    };
    
    for (const FString& Path : Paper2DMaterialPaths)
    {
        UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *Path);
        if (Material)
        {
            Materials.Add(Material);
        }
    }
    
    return Materials;
}

UMaterialInterface* USpriteOptimizer::GetDefaultPaper2DMaterial()
{
    // Пытаемся загрузить стандартный материал спрайта
    UMaterialInterface* Material = LoadObject<UMaterialInterface>(
        nullptr, 
        TEXT("/Paper2D/DefaultSpriteMaterial.DefaultSpriteMaterial")
    );
    
    if (!Material)
    {
        // Если не найден, пытаемся загрузить другой
        Material = LoadObject<UMaterialInterface>(
            nullptr, 
            TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")
        );
    }
    
    return Material;
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
    
    // Настраиваем текстуру
    NewTexture->SetPlatformData(new FTexturePlatformData());
    NewTexture->GetPlatformData()->SizeX = NewWidth;
    NewTexture->GetPlatformData()->SizeY = NewHeight;
    NewTexture->GetPlatformData()->PixelFormat = PF_B8G8R8A8;
    
    // Создаем mip уровень
    FTexture2DMipMap* Mip = new FTexture2DMipMap();
    NewTexture->GetPlatformData()->Mips.Add(Mip);
    Mip->SizeX = NewWidth;
    Mip->SizeY = NewHeight;
    
    // Записываем данные
    Mip->BulkData.Lock(LOCK_READ_WRITE);
    void* TextureData = Mip->BulkData.Realloc(OptimizedPixels.Num() * sizeof(FColor));
    FMemory::Memcpy(TextureData, OptimizedPixels.GetData(), OptimizedPixels.Num() * sizeof(FColor));
    Mip->BulkData.Unlock();
    
    // Настраиваем источник
    NewTexture->Source.Init(NewWidth, NewHeight, 1, 1, TSF_BGRA8, (uint8*)OptimizedPixels.GetData());
    
    // Копируем настройки с оригинала
    NewTexture->SRGB = SourceTexture->SRGB;
    NewTexture->CompressionSettings = SourceTexture->CompressionSettings;
    NewTexture->Filter = SourceTexture->Filter;
    NewTexture->AddressX = SourceTexture->AddressX;
    NewTexture->AddressY = SourceTexture->AddressY;
    
    NewTexture->UpdateResource();
    (void)Package->MarkPackageDirty();
    
    // Сохраняем
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    bool bSaved = UPackage::SavePackage(Package, NewTexture, *PackageFileName, SaveArgs);
    
    if (bSaved)
    {
        FAssetRegistryModule::AssetCreated(NewTexture);
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Created optimized texture: %s (%dx%d)"), *FullAssetPath, NewWidth, NewHeight);
    }
    else
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Failed to save optimized texture: %s"), *FullAssetPath);
    }
    
    // В конце метода добавьте:
    if (NewTexture)
    {
        // Принудительно завершаем все операции с текстурой
        NewTexture->UpdateResource();
        NewTexture->PostEditChange();
        
        // Принудительный garbage collection для очистки временных данных
        if (GEngine)
        {
            GEngine->TrimMemory();
        }
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
    
    // Определяем материал
    UMaterialInterface* SpriteMaterial = Settings.Material;
    if (!SpriteMaterial)
    {
        SpriteMaterial = GetDefaultPaper2DMaterial();
    }
    
    // Настраиваем параметры инициализации
    FSpriteAssetInitParameters InitParams;
    InitParams.Texture = OptimizedTexture;
    InitParams.Offset = FIntPoint::ZeroValue;
    InitParams.Dimension = FIntPoint(OptimizedTexture->GetSizeX(), OptimizedTexture->GetSizeY());
    InitParams.DefaultMaterialOverride = SpriteMaterial;
    InitParams.bOverridePixelsPerUnrealUnit = true;
    InitParams.PixelsPerUnrealUnit = Settings.PixelsPerUnit;
    
    // Инициализируем спрайт
    NewSprite->InitializeSprite(InitParams, false);
    
    // Устанавливаем пивот
    NewSprite->SetPivotMode(ESpritePivotMode::Custom, NewPivotInPixels, true);
    
    (void)Package->MarkPackageDirty();
    
    // Сохраняем
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    bool bSaved = UPackage::SavePackage(Package, NewSprite, *PackageFileName, SaveArgs);
    
    if (bSaved)
    {
        FAssetRegistryModule::AssetCreated(NewSprite);
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Created optimized sprite: %s"), *FullAssetPath);
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
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("GetTexturePixelData: Texture is null"));
        return PixelData;
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Reading pixel data from texture: %s (%dx%d)"), 
           *Texture->GetName(), Texture->GetSizeX(), Texture->GetSizeY());
    
    // Проверяем что текстура может быть прочитана
    if (!Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Texture %s has no platform data or mips"), *Texture->GetName());
        return PixelData;
    }
    
    FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
    
    // ВАЖНО: Проверяем состояние блокировки перед попыткой заблокировать
    if (Mip.BulkData.IsLocked())
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("BulkData already locked for texture %s, skipping"), *Texture->GetName());
        return PixelData;
    }
    
    const void* RawData = nullptr;
    
    // Используем try-catch для безопасности
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
        // Не забываем разблокировать если lock вернул nullptr
        if (!Mip.BulkData.IsLocked())
        {
            try { Mip.BulkData.Unlock(); } catch (...) {}
        }
        return PixelData;
    }
    
    int32 Width = Texture->GetSizeX();
    int32 Height = Texture->GetSizeY();
    int32 ExpectedPixels = Width * Height;
    
    // Проверяем формат пикселей
    EPixelFormat PixelFormat = Texture->GetPlatformData()->PixelFormat;
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Texture %s format: %d, expected pixels: %d"), 
           *Texture->GetName(), (int32)PixelFormat, ExpectedPixels);
    
    // Предполагаем формат BGRA8
    const FColor* ColorData = static_cast<const FColor*>(RawData);
    PixelData.Reserve(ExpectedPixels);
    
    for (int32 i = 0; i < ExpectedPixels; i++)
    {
        PixelData.Add(ColorData[i]);
    }
    
    // ОБЯЗАТЕЛЬНО разблокируем
    try
    {
        Mip.BulkData.Unlock();
    }
    catch (...)
    {
        UE_LOG(LogSpriteOptimizer, Error, TEXT("Exception while unlocking texture data for %s"), *Texture->GetName());
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Successfully read %d pixels from %s"), 
           PixelData.Num(), *Texture->GetName());
    
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
    
    // Иначе возвращаем ту же директорию что и у оригинала
    FString Directory, Filename, Extension;
    FPaths::Split(OriginalPath, Directory, Filename, Extension);
    return Directory;
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
    
    if (Settings.MaxAtlasSize.X < 256 || Settings.MaxAtlasSize.Y < 256)
    {
        Result.ErrorMessage = TEXT("Atlas size must be at least 256x256 pixels");
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
    
    // Определяем путь для сохранения
    FString ActualAtlasPath = AtlasPath.IsEmpty() ? 
        GetOptimizedAssetPath(Sprites[0]->GetPackage()->GetName(), FSpriteOptimizationSettings()) : AtlasPath;
    
    // Создаем атласную текстуру
    Result.AtlasTexture = CreateAtlasTexture(Sprites, PackedRegions, AtlasSize, AtlasName, ActualAtlasPath);
    
    if (!Result.AtlasTexture)
    {
        Result.ErrorMessage = TEXT("Failed to create atlas texture");
        return Result;
    }
    
    Result.AtlasTexturePath = ActualAtlasPath + TEXT("/") + AtlasName;
    
    // Создаем отдельные спрайты если нужно
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
            }
        }
    }
    
    // Заполняем результаты
    Result.SpriteRegions = PackedRegions;
    Result.AtlasSize = AtlasSize;
    Result.PackingEfficiency = CalculatePackingEfficiency(SpriteSizes, AtlasSize);
    
    // Вычисляем экономию памяти
    float OriginalMemory = 0.0f;
    for (const FIntPoint& Size : SpriteSizes)
    {
        OriginalMemory += (Size.X * Size.Y * 4) / (1024.0f * 1024.0f); // RGBA, MB
    }
    float AtlasMemory = (AtlasSize.X * AtlasSize.Y * 4) / (1024.0f * 1024.0f);
    Result.MemorySavings = OriginalMemory > 0 ? ((OriginalMemory - AtlasMemory) / OriginalMemory) * 100.0f : 0.0f;
    
    Result.bSuccess = true;
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Atlas created successfully: %dx%d, %.1f%% efficiency, %.1f%% memory savings"), 
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
    for (const FIntPoint& Size : SpriteSizes)
    {
        OriginalMemory += (Size.X * Size.Y * 4) / (1024.0f * 1024.0f);
    }
    float AtlasMemory = (AtlasSize.X * AtlasSize.Y * 4) / (1024.0f * 1024.0f);
    Result.MemorySavings = OriginalMemory > 0 ? ((OriginalMemory - AtlasMemory) / OriginalMemory) * 100.0f : 0.0f;
    
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
    
    // Проверяем, что все спрайты помещаются в максимальный размер
    for (const FIntPoint& Size : SpriteSizes)
    {
        if (Size.X > Settings.MaxAtlasSize.X || Size.Y > Settings.MaxAtlasSize.Y)
        {
            UE_LOG(LogSpriteOptimizer, Warning, 
                   TEXT("Sprite size %dx%d exceeds max atlas size %dx%d"), 
                   Size.X, Size.Y, Settings.MaxAtlasSize.X, Settings.MaxAtlasSize.Y);
            
            // Возвращаем пустой результат
            OutAtlasSize = Settings.MaxAtlasSize;
            return PackedRegions;
        }
    }
    
    // Сортируем спрайты по площади (большие первыми) для лучшей упаковки
    TArray<TPair<int32, FIntPoint>> SortedSprites;
    for (int32 i = 0; i < SpriteSizes.Num(); i++)
    {
        SortedSprites.Add(TPair<int32, FIntPoint>(i, SpriteSizes[i]));
    }
    
    SortedSprites.Sort([](const TPair<int32, FIntPoint>& A, const TPair<int32, FIntPoint>& B)
    {
        int32 AreaA = A.Value.X * A.Value.Y;
        int32 AreaB = B.Value.X * B.Value.Y;
        if (AreaA == AreaB)
        {
            // При равной площади сортируем по максимальной стороне
            int32 MaxSideA = FMath::Max(A.Value.X, A.Value.Y);
            int32 MaxSideB = FMath::Max(B.Value.X, B.Value.Y);
            return MaxSideA > MaxSideB;
        }
        return AreaA > AreaB;
    });
    
    // Инициализируем результат правильным размером
    PackedRegions.Init(FIntRect(0, 0, 0, 0), SpriteSizes.Num());
    
    // Начинаем с минимального размера и увеличиваем по необходимости
    int32 CurrentWidth = 256;
    int32 CurrentHeight = 256;
    
    // Пытаемся упаковать с разными размерами атласа
    bool bPackingSuccessful = false;
    int32 Attempts = 0;
    const int32 MaxAttempts = 20;
    
    while (!bPackingSuccessful && Attempts < MaxAttempts)
    {
        // Список свободных прямоугольников
        TArray<FIntRect> FreeRects;
        FreeRects.Add(FIntRect(0, 0, CurrentWidth, CurrentHeight));
        
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
            
            // Добавляем padding к размеру спрайта
            FIntPoint PaddedSize(SpriteSize.X + Settings.SpritePadding, SpriteSize.Y + Settings.SpritePadding);
            
            // Ищем лучшее место для размещения
            int32 BestRectIndex = -1;
            int32 BestShortSideFit = INT_MAX;
            int32 BestLongSideFit = INT_MAX;
            
            for (int32 i = 0; i < FreeRects.Num(); i++)
            {
                const FIntRect& Rect = FreeRects[i];
                int32 RectWidth = Rect.Width();
                int32 RectHeight = Rect.Height();
                
                if (PaddedSize.X <= RectWidth && PaddedSize.Y <= RectHeight)
                {
                    int32 LeftoverHoriz = RectWidth - PaddedSize.X;
                    int32 LeftoverVert = RectHeight - PaddedSize.Y;
                    int32 ShortSideFit = FMath::Min(LeftoverHoriz, LeftoverVert);
                    int32 LongSideFit = FMath::Max(LeftoverHoriz, LeftoverVert);
                    
                    if (ShortSideFit < BestShortSideFit || 
                        (ShortSideFit == BestShortSideFit && LongSideFit < BestLongSideFit))
                    {
                        BestRectIndex = i;
                        BestShortSideFit = ShortSideFit;
                        BestLongSideFit = LongSideFit;
                    }
                }
            }
            
            if (BestRectIndex == -1)
            {
                bAllSpritesPlaced = false;
                break;
            }
            
            // Размещаем спрайт (без padding в финальной позиции)
            FIntRect& BestRect = FreeRects[BestRectIndex];
            FIntRect PlacedRect(BestRect.Min.X, BestRect.Min.Y, 
                               BestRect.Min.X + SpriteSize.X, BestRect.Min.Y + SpriteSize.Y);
            
            TempPackedRegions[OriginalIndex] = PlacedRect;
            
            // Обновляем используемую область
            UsedWidth = FMath::Max(UsedWidth, PlacedRect.Max.X);
            UsedHeight = FMath::Max(UsedHeight, PlacedRect.Max.Y);
            
            // Разбиваем использованный прямоугольник
            FIntRect PaddedPlacedRect(BestRect.Min.X, BestRect.Min.Y, 
                                     BestRect.Min.X + PaddedSize.X, BestRect.Min.Y + PaddedSize.Y);
            
            TArray<FIntRect> NewRects;
            
            // Правый остаток
            if (PaddedPlacedRect.Max.X < BestRect.Max.X)
            {
                NewRects.Add(FIntRect(PaddedPlacedRect.Max.X, BestRect.Min.Y, 
                                     BestRect.Max.X, BestRect.Max.Y));
            }
            
            // Нижний остаток
            if (PaddedPlacedRect.Max.Y < BestRect.Max.Y)
            {
                NewRects.Add(FIntRect(BestRect.Min.X, PaddedPlacedRect.Max.Y, 
                                     BestRect.Max.X, BestRect.Max.Y));
            }
            
            // Удаляем использованный прямоугольник
            FreeRects.RemoveAt(BestRectIndex);
            
            // Добавляем новые прямоугольники
            for (const FIntRect& NewRect : NewRects)
            {
                if (NewRect.Width() > 0 && NewRect.Height() > 0)
                {
                    FreeRects.Add(NewRect);
                }
            }
        }
        
        if (bAllSpritesPlaced)
        {
            PackedRegions = TempPackedRegions;
            OutAtlasSize = FIntPoint(UsedWidth, UsedHeight);
            bPackingSuccessful = true;
            
            UE_LOG(LogSpriteOptimizer, Log, 
                   TEXT("BestFit packing successful: %d sprites into %dx%d atlas (attempt %d)"), 
                   SpriteSizes.Num(), UsedWidth, UsedHeight, Attempts + 1);
        }
        else
        {
            // Увеличиваем размер атласа для следующей попытки
            if (CurrentWidth <= CurrentHeight)
            {
                CurrentWidth = FMath::Min(CurrentWidth * 2, Settings.MaxAtlasSize.X);
            }
            else
            {
                CurrentHeight = FMath::Min(CurrentHeight * 2, Settings.MaxAtlasSize.Y);
            }
            
            // Проверяем, не достигли ли максимального размера
            if (CurrentWidth >= Settings.MaxAtlasSize.X && CurrentHeight >= Settings.MaxAtlasSize.Y)
            {
                break;
            }
        }
        
        Attempts++;
    }
    
    if (!bPackingSuccessful)
    {
        UE_LOG(LogSpriteOptimizer, Error, 
               TEXT("Failed to pack %d sprites within max atlas size %dx%d after %d attempts"), 
               SpriteSizes.Num(), Settings.MaxAtlasSize.X, Settings.MaxAtlasSize.Y, Attempts);
        
        OutAtlasSize = Settings.MaxAtlasSize;
        PackedRegions.Empty();
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
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Creating atlas texture %dx%d with %d sprites"), 
           AtlasSize.X, AtlasSize.Y, Sprites.Num());
    
    // Создаем массив пикселей для атласа (инициализируем прозрачным)
    TArray<FColor> AtlasPixels;
    AtlasPixels.Init(FColor(0, 0, 0, 0), AtlasSize.X * AtlasSize.Y);
    
    // Обрабатываем каждый спрайт БЕЗ множественных вызовов GetTexturePixelData
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
        
        // Используем альтернативный метод чтения пикселей через Source
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
    
    // БЕЗОПАСНОЕ создание текстуры через Source API
    AtlasTexture->Source.Init(AtlasSize.X, AtlasSize.Y, 1, 1, TSF_BGRA8, (uint8*)AtlasPixels.GetData());
    
    // Копируем настройки с первого спрайта
    if (Sprites.Num() > 0 && Sprites[0] && Sprites[0]->GetSourceTexture())
    {
        UTexture2D* FirstTexture = Sprites[0]->GetSourceTexture();
        AtlasTexture->SRGB = FirstTexture->SRGB;
        AtlasTexture->CompressionSettings = TC_EditorIcon; // Используем безопасный формат
        AtlasTexture->Filter = FirstTexture->Filter;
        AtlasTexture->AddressX = FirstTexture->AddressX;
        AtlasTexture->AddressY = FirstTexture->AddressY;
        
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Copied settings from %s: SRGB=%d"), 
               *FirstTexture->GetName(), AtlasTexture->SRGB);
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
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Successfully created and saved atlas texture: %s (%dx%d)"), 
               *FullAssetPath, AtlasSize.X, AtlasSize.Y);
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
    
    // ИСПРАВЛЕНИЕ: Определяем, является ли спрайт уже оптимизированным
    bool bIsAlreadyOptimized = SpriteName.Contains(TEXT("_Optimized"));
    
    FVector2D CorrectPivot;
    
    if (bIsAlreadyOptimized)
    {
        // Для уже оптимизированных спрайтов используем центральный пивот
        // так как они уже обрезаны правильно
        CorrectPivot = FVector2D(Region.Width() * 0.5f, Region.Height() * 0.5f);
        
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Using center pivot for optimized sprite: (%f,%f)"), 
               CorrectPivot.X, CorrectPivot.Y);
    }
    else
    {
        // Для неоптимизированных спрайтов нужно найти исходную область
        FIntRect OriginalUsedRegion = FindUsedBounds(OriginalTexture, 2);
        
        CorrectPivot = CalculateAtlasPivotForLayering(
            OriginalTexture->GetSizeX(), OriginalTexture->GetSizeY(),
            OriginalUsedRegion,
            Region,
            OriginalSprite
        );
        
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Calculated layering pivot: (%f,%f)"), 
               CorrectPivot.X, CorrectPivot.Y);
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Atlas sprite creation for %s:"), *SpriteName);
    UE_LOG(LogSpriteOptimizer, Log, TEXT("  Original texture: %dx%d"), OriginalTexture->GetSizeX(), OriginalTexture->GetSizeY());
    UE_LOG(LogSpriteOptimizer, Log, TEXT("  Atlas region: (%d,%d) to (%d,%d)"), 
           Region.Min.X, Region.Min.Y, Region.Max.X, Region.Max.Y);
    UE_LOG(LogSpriteOptimizer, Log, TEXT("  Is optimized: %s"), bIsAlreadyOptimized ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogSpriteOptimizer, Log, TEXT("  Final pivot: (%f,%f)"), CorrectPivot.X, CorrectPivot.Y);
    
    // Безопасное получение материала
    UMaterialInterface* SpriteMaterial = OriginalSprite->GetDefaultMaterial();
    if (!SpriteMaterial)
    {
        SpriteMaterial = GetDefaultPaper2DMaterial();
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
    
    // Устанавливаем правильный пивот
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
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Created atlas sprite: %s with pivot (%f,%f)"), 
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
        return false;
    }
    
    // ИСПРАВЛЕНИЕ: Принудительно обновляем ресурс для свежесозданных текстур
    SourceTexture->UpdateResource();
    
    // Небольшая задержка для завершения операций
    FPlatformProcess::Sleep(0.1f);
    
    // Пытаемся использовать Source API (более безопасно)
    if (SourceTexture->Source.IsValid())
    {
        TArray64<uint8> RawData;
        if (SourceTexture->Source.GetMipData(RawData, 0) && RawData.Num() > 0)
        {
            int32 SourceWidth = SourceTexture->GetSizeX();
            int32 SourceHeight = SourceTexture->GetSizeY();
            
            // Предполагаем формат BGRA8
            const FColor* SourcePixels = reinterpret_cast<const FColor*>(RawData.GetData());
            
            // Копируем только нужную область
            int32 CopiedPixels = 0;
            for (int32 Y = 0; Y < SourceHeight && Y < Region.Height(); Y++)
            {
                for (int32 X = 0; X < SourceWidth && X < Region.Width(); X++)
                {
                    int32 SourceIndex = Y * SourceWidth + X;
                    int32 AtlasX = Region.Min.X + X;
                    int32 AtlasY = Region.Min.Y + Y;
                    int32 AtlasIndex = AtlasY * AtlasSize.X + AtlasX;
                    
                    if (SourceIndex < RawData.Num() / 4 && 
                        AtlasIndex < AtlasPixels.Num() &&
                        AtlasX >= 0 && AtlasX < AtlasSize.X &&
                        AtlasY >= 0 && AtlasY < AtlasSize.Y)
                    {
                        AtlasPixels[AtlasIndex] = SourcePixels[SourceIndex];
                        CopiedPixels++;
                    }
                }
            }
            
            UE_LOG(LogSpriteOptimizer, Log, TEXT("Copied %d pixels via Source API"), CopiedPixels);
            return CopiedPixels > 0;
        }
    }
    
    // Fallback: используем GetTexturePixelData, но с проверкой блокировки
    TArray<FColor> SourcePixels = GetTexturePixelData(SourceTexture);
    if (SourcePixels.Num() == 0)
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Failed to read texture pixel data for %s"), *SourceTexture->GetName());
        return false;
    }
    
    int32 SourceWidth = SourceTexture->GetSizeX();
    int32 SourceHeight = SourceTexture->GetSizeY();
    
    int32 CopiedPixels = 0;
    for (int32 Y = 0; Y < SourceHeight && Y < Region.Height(); Y++)
    {
        for (int32 X = 0; X < SourceWidth && X < Region.Width(); X++)
        {
            int32 SourceIndex = Y * SourceWidth + X;
            int32 AtlasX = Region.Min.X + X;
            int32 AtlasY = Region.Min.Y + Y;
            int32 AtlasIndex = AtlasY * AtlasSize.X + AtlasX;
            
            if (SourceIndex < SourcePixels.Num() && 
                AtlasIndex < AtlasPixels.Num() &&
                AtlasX >= 0 && AtlasX < AtlasSize.X &&
                AtlasY >= 0 && AtlasY < AtlasSize.Y)
            {
                AtlasPixels[AtlasIndex] = SourcePixels[SourceIndex];
                CopiedPixels++;
            }
        }
    }
    
    UE_LOG(LogSpriteOptimizer, Log, TEXT("Copied %d pixels via fallback method"), CopiedPixels);
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