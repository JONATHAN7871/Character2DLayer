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
        UE_LOG(LogSpriteOptimizer, Log, TEXT("Created optimized texture: %s (%dx%d)"), *FullAssetPath, NewWidth, NewHeight);
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
        return PixelData;
    }
    
    // Проверяем что текстура может быть прочитана
    if (!Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Texture has no platform data or mips"));
        return PixelData;
    }
    
    FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
    const void* RawData = Mip.BulkData.LockReadOnly();
    
    if (!RawData)
    {
        UE_LOG(LogSpriteOptimizer, Warning, TEXT("Failed to lock texture data"));
        return PixelData;
    }
    
    int32 Width = Texture->GetSizeX();
    int32 Height = Texture->GetSizeY();
    
    // Предполагаем формат BGRA8
    const FColor* ColorData = static_cast<const FColor*>(RawData);
    PixelData.Reserve(Width * Height);
    
    for (int32 i = 0; i < Width * Height; i++)
    {
        PixelData.Add(ColorData[i]);
    }
    
    Mip.BulkData.Unlock();
    
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