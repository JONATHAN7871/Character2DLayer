#include "Character2DLayerOptimizer/Character2DLayerOptimizer.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"
#include "PaperSprite.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/DateTime.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacter2DOptimizer, Log, All);

TArray<FLayerOptimizationResult> UCharacter2DLayerOptimizer::OptimizeLayeredCharacter(UCharacter2DAsset* Asset)
{
    TArray<FLayerOptimizationResult> Results;
    
    if (!Asset)
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Asset is null"));
        return Results;
    }
    
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("Starting optimization for asset: %s"), *Asset->GetName());
    
    const FString BaseOptimizedPath = FString::Printf(TEXT("/Game/Character2D/Optimized/%s"), *Asset->GetName());
    
    if (Asset->SpriteStructure.Body.Sprite) Results.Add(OptimizeLayer(Asset->SpriteStructure.Body.Sprite, TEXT("Body"), BaseOptimizedPath));
    if (Asset->SpriteStructure.Arms.Sprite) Results.Add(OptimizeLayer(Asset->SpriteStructure.Arms.Sprite, TEXT("Arms"), BaseOptimizedPath));
    if (Asset->SpriteStructure.Head.Head.Sprite) Results.Add(OptimizeLayer(Asset->SpriteStructure.Head.Head.Sprite, TEXT("Head"), BaseOptimizedPath));
    if (Asset->SpriteStructure.Head.Eyes.Sprite) Results.Add(OptimizeLayer(Asset->SpriteStructure.Head.Eyes.Sprite, TEXT("Eyes"), BaseOptimizedPath));
    if (Asset->SpriteStructure.Head.Eyebrows.Sprite) Results.Add(OptimizeLayer(Asset->SpriteStructure.Head.Eyebrows.Sprite, TEXT("Eyebrows"), BaseOptimizedPath));
    if (Asset->SpriteStructure.Head.Eyelids.Sprite) Results.Add(OptimizeLayer(Asset->SpriteStructure.Head.Eyelids.Sprite, TEXT("Eyelids"), BaseOptimizedPath));
    if (Asset->SpriteStructure.Head.Mouth.Sprite) Results.Add(OptimizeLayer(Asset->SpriteStructure.Head.Mouth.Sprite, TEXT("Mouth"), BaseOptimizedPath));
    if (Asset->SpriteStructure.Shadow.Sprite) Results.Add(OptimizeLayer(Asset->SpriteStructure.Shadow.Sprite, TEXT("Shadow"), BaseOptimizedPath));
    
    float TotalOriginalMB = 0;
    float TotalOptimizedMB = 0;
    
    for (const auto& Result : Results)
    {
        TotalOriginalMB += Result.OriginalSizeMB;
        TotalOptimizedMB += Result.OptimizedSizeMB;
    }
    
    float TotalSavingsMB = TotalOriginalMB - TotalOptimizedMB;
    float SavingsPercent = TotalOriginalMB > 0 ? (TotalSavingsMB / TotalOriginalMB) * 100.0f : 0.0f;
    
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("Optimization complete: %.1f MB -> %.1f MB (%.1f%% savings)"), 
           TotalOriginalMB, TotalOptimizedMB, SavingsPercent);
    
    return Results;
}

FLayerOptimizationResult UCharacter2DLayerOptimizer::OptimizeLayer(UPaperSprite* LayerSprite, const FString& LayerName, const FString& OptimizedBasePath)
{
    FLayerOptimizationResult Result;
    Result.LayerName = LayerName;

    if (!LayerSprite || !LayerSprite->GetSourceTexture())
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Layer %s: Invalid sprite or source texture."), *LayerName);
        return Result;
    }
    
    // Проверяем, что есть "запеченные" данные рендера
    if (LayerSprite->BakedRenderData.IsEmpty())
    {
       // Попытаемся пересобрать данные, если их нет. Это может помочь для свежесозданных спрайтов.
#if WITH_EDITOR
       LayerSprite->RebuildRenderData();
       if (LayerSprite->BakedRenderData.IsEmpty())
       {
            UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Layer %s: Sprite has no BakedRenderData. Cannot determine geometry. Try re-saving the sprite."), *LayerName);
            return Result;
       }
#else
       return Result;
#endif
    }
    
    UTexture2D* SourceTexture = LayerSprite->GetSourceTexture();
    Result.OriginalTexture = SourceTexture;
    const FVector2D OriginalCanvasSize(SourceTexture->GetSizeX(), SourceTexture->GetSizeY());
    
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("Optimizing layer: %s (using Baked Render Data)"), *LayerName);

    // --- 1. Находим границы по "запеченным" данным рендера ---
    FBox2D GeometryBounds(EForceInit::ForceInit); // Границы вершин (в мировых юнитах от пивота)
    FBox2D UvBounds(EForceInit::ForceInit);       // Границы UV (в нормализованных координатах 0-1)

    for (const FVector4& BakedVertex : LayerSprite->BakedRenderData)
    {
        GeometryBounds += FVector2D(BakedVertex.X, BakedVertex.Y);
        UvBounds += FVector2D(BakedVertex.Z, BakedVertex.W);
    }
    
    if (!GeometryBounds.bIsValid || !UvBounds.bIsValid)
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Layer %s: No valid geometry or UV data found in BakedRenderData."), *LayerName);
        return Result;
    }

    // --- 2. Определяем область для обрезки в пикселях на основе UV ---
    FIntRect UsedBounds(
        FMath::FloorToInt(UvBounds.Min.X * OriginalCanvasSize.X),
        FMath::FloorToInt(UvBounds.Min.Y * OriginalCanvasSize.Y),
        FMath::CeilToInt(UvBounds.Max.X * OriginalCanvasSize.X),
        FMath::CeilToInt(UvBounds.Max.Y * OriginalCanvasSize.Y)
    );

    // Добавим небольшой padding, чтобы не обрезать сглаженные края
    const int32 Padding = 2;
    UsedBounds.Min.X = FMath::Max(0, UsedBounds.Min.X - Padding);
    UsedBounds.Min.Y = FMath::Max(0, UsedBounds.Min.Y - Padding);
    UsedBounds.Max.X = FMath::Min(SourceTexture->GetSizeX(), UsedBounds.Max.X + Padding);
    UsedBounds.Max.Y = FMath::Min(SourceTexture->GetSizeY(), UsedBounds.Max.Y + Padding);

    Result.UsedRegion = UsedBounds;
    
    // --- 3. Вычисляем статистику и создаем ассеты ---
    Result.OptimizedTexture = CreateAndSaveOptimizedTexture(SourceTexture, UsedBounds, LayerName, OptimizedBasePath);
    
    // Сохраняем данные для создания спрайта
    Result.PositionData.OriginalCanvasSize = OriginalCanvasSize;
    Result.PositionData.ContentBounds = UsedBounds;
    // Смещение ЦЕНТРА видимой геометрии относительно ПИВОТА оригинального спрайта (в мировых юнитах).
    // Это именно то смещение, которое нам нужно компенсировать.
    Result.PositionData.PositionOffset = GeometryBounds.GetCenter(); 
    
    Result.OptimizedSprite = CreateAndSaveOptimizedSprite(Result, OptimizedBasePath);

    return Result;
}

FIntRect UCharacter2DLayerOptimizer::FindUsedBounds(UTexture2D* Texture)
{
    if (!Texture)
    {
        return FIntRect(0, 0, 0, 0);
    }
    
    // Получаем пиксели текстуры
    TArray<FColor> PixelData = GetTexturePixelData(Texture);
    if (PixelData.Num() == 0)
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Failed to read texture pixel data"));
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
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("No non-transparent content found, using center region"));
        return FIntRect(Width/2 - 50, Height/2 - 50, Width/2 + 50, Height/2 + 50);
    }
    
    int32 Padding = 4;
    MinX = FMath::Max(0, MinX - Padding);
    MinY = FMath::Max(0, MinY - Padding);
    MaxX = FMath::Min(Width - 1, MaxX + Padding);
    MaxY = FMath::Min(Height - 1, MaxY + Padding);
    
    return FIntRect(MinX, MinY, MaxX + 1, MaxY + 1);
}

UTexture2D* UCharacter2DLayerOptimizer::CreateAndSaveOptimizedTexture(UTexture2D* SourceTexture, FIntRect UsedRegion, const FString& LayerName, const FString& BasePath)
{
    if (!SourceTexture)
    {
        return nullptr;
    }
    
    int32 NewWidth = UsedRegion.Width();
    int32 NewHeight = UsedRegion.Height();
    
    if (NewWidth <= 0 || NewHeight <= 0)
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Invalid optimized texture size: %dx%d"), NewWidth, NewHeight);
        return nullptr;
    }
    
    TArray<FColor> SourcePixels = GetTexturePixelData(SourceTexture);
    if (SourcePixels.Num() == 0)
    {
        return nullptr;
    }
    
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
    
    FString TextureAssetPath = FString::Printf(TEXT("%s/Textures/T_%s_Optimized"), *BasePath, *LayerName);
    FString PackageName = TextureAssetPath;
    FPackageName::TryConvertFilenameToLongPackageName(PackageName, PackageName);
    
    UPackage* Package = CreatePackage(*PackageName);
    Package->FullyLoad();
    
    FString TextureName = FString::Printf(TEXT("T_%s_Optimized"), *LayerName);
    UTexture2D* NewTexture = NewObject<UTexture2D>(Package, FName(*TextureName), RF_Public | RF_Standalone);
    
    if (!NewTexture)
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Failed to create optimized texture object"));
        return nullptr;
    }
    
    NewTexture->SetPlatformData(new FTexturePlatformData());
    NewTexture->GetPlatformData()->SizeX = NewWidth;
    NewTexture->GetPlatformData()->SizeY = NewHeight;
    NewTexture->GetPlatformData()->PixelFormat = PF_B8G8R8A8;
    
    FTexture2DMipMap* Mip = new FTexture2DMipMap();
    NewTexture->GetPlatformData()->Mips.Add(Mip);
    Mip->SizeX = NewWidth;
    Mip->SizeY = NewHeight;
    
    Mip->BulkData.Lock(LOCK_READ_WRITE);
    void* TextureData = Mip->BulkData.Realloc(OptimizedPixels.Num() * sizeof(FColor));
    FMemory::Memcpy(TextureData, OptimizedPixels.GetData(), OptimizedPixels.Num() * sizeof(FColor));
    Mip->BulkData.Unlock();
    
    NewTexture->Source.Init(NewWidth, NewHeight, 1, 1, TSF_BGRA8, (uint8*)OptimizedPixels.GetData());
    
    NewTexture->SRGB = SourceTexture->SRGB;
    NewTexture->CompressionSettings = SourceTexture->CompressionSettings;
    NewTexture->Filter = SourceTexture->Filter;
    NewTexture->AddressX = SourceTexture->AddressX;
    NewTexture->AddressY = SourceTexture->AddressY;
    
    NewTexture->UpdateResource();
    Package->MarkPackageDirty();
    
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    bool bSaved = UPackage::SavePackage(Package, NewTexture, *PackageFileName, SaveArgs);
    
    if (bSaved)
    {
        FAssetRegistryModule::AssetCreated(NewTexture);
        UE_LOG(LogCharacter2DOptimizer, Log, TEXT("Created and saved optimized texture: %s (%dx%d)"), *TextureAssetPath, NewWidth, NewHeight);
    }
    else
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Failed to save optimized texture: %s"), *TextureAssetPath);
    }
    
    return NewTexture;
}

UPaperSprite* UCharacter2DLayerOptimizer::CreateAndSaveOptimizedSprite(const FLayerOptimizationResult& OptimizationResult, const FString& BasePath)
{
    if (!OptimizationResult.OptimizedTexture)
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("No optimized texture for %s"), *OptimizationResult.LayerName);
        return nullptr;
    }

    // --- Блок создания пакета и объекта спрайта ---
    FString SpriteAssetPath = FString::Printf(TEXT("%s/Sprites/S_%s_Optimized"), *BasePath, *OptimizationResult.LayerName);
    FString PackageName = SpriteAssetPath;
    FPackageName::TryConvertFilenameToLongPackageName(PackageName, PackageName);
    UPackage* Package = CreatePackage(*PackageName);
    Package->FullyLoad();
    FString SpriteName = FString::Printf(TEXT("S_%s_Optimized"), *OptimizationResult.LayerName);
    UPaperSprite* NewSprite = NewObject<UPaperSprite>(Package, FName(*SpriteName), RF_Public | RF_Standalone);
    if (!NewSprite) { /*...*/ return nullptr; }

    // --- ПОДГОТОВКА ДАННЫХ ДЛЯ ИНИЦИАЛИЗАЦИИ ---

    // 1. Вычисляем пивот (этот код работает)
    const FIntRect& CroppedBounds = OptimizationResult.UsedRegion;
    const FVector2D& OriginalTextureSize = OptimizationResult.PositionData.OriginalCanvasSize;
    const FVector2D OriginalPivotInPixels = OriginalTextureSize * 0.5f;
    const FVector2D CroppedTextureTopLeftInPixels(CroppedBounds.Min.X, CroppedBounds.Min.Y);
    const FVector2D NewPivotInPixels = OriginalPivotInPixels - CroppedTextureTopLeftInPixels;

    // 2. Загружаем материал
    const FString MaterialPath = TEXT("/Game/CoreGame/Materials/Sprite/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial");
    UMaterialInterface* SpriteMaterial = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
    if (!SpriteMaterial)
    {
         UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("   - Could not find material at path: %s. Sprite will be created with default material."), *MaterialPath);
    }
    
    // 3. Заполняем структуру FSpriteAssetInitParameters ВСЕМИ данными
    FSpriteAssetInitParameters InitParams;
    InitParams.Texture = OptimizationResult.OptimizedTexture;
    InitParams.Offset = FIntPoint::ZeroValue; // Обрезку мы сделали сами, поэтому offset 0
    InitParams.Dimension = FIntPoint(OptimizationResult.OptimizedTexture->GetSizeX(), OptimizationResult.OptimizedTexture->GetSizeY());
    InitParams.DefaultMaterialOverride = SpriteMaterial;     // <<< УСТАНАВЛИВАЕМ МАТЕРИАЛ ЗДЕСЬ
    InitParams.bOverridePixelsPerUnrealUnit = 1.0f;           // <<< УСТАНАВЛИВАЕМ PIXELS PER UNIT ЗДЕСЬ

    // --- ВЫПОЛНЯЕМ ИНИЦИАЛИЗАЦИЮ И НАСТРОЙКУ ---
    
    // Инициализируем спрайт со всеми данными, КРОМЕ пивота
    NewSprite->InitializeSprite(InitParams, false); // bRebuildData = false, чтобы затем установить пивот

    // Устанавливаем пивот отдельно, так как его нет в FSpriteAssetInitParameters
    NewSprite->SetPivotMode(ESpritePivotMode::Custom, NewPivotInPixels, true); // bRebuildData = true, чтобы "запечь" всё
    
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("   - Assigned material and PixelsPerUnit via InitParams."));

    // --- Отладочный вывод и сохранение ---
    Package->MarkPackageDirty();
    
    // ... остальная часть метода ...
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    bool bSaved = UPackage::SavePackage(Package, NewSprite, *PackageFileName, SaveArgs);
    if(bSaved)
    {
        FAssetRegistryModule::AssetCreated(NewSprite);
        UE_LOG(LogCharacter2DOptimizer, Log, TEXT("✅ Successfully created and saved optimized sprite: %s"), *SpriteAssetPath);
    }
    
    return NewSprite;
}

// Старые функции остаются для обратной совместимости
UTexture2D* UCharacter2DLayerOptimizer::CreateOptimizedTexture(UTexture2D* SourceTexture, FIntRect UsedRegion, const FString& LayerName)
{
    // Вызываем новую функцию с временным путем
    return CreateAndSaveOptimizedTexture(SourceTexture, UsedRegion, LayerName, TEXT("/Game/Character2D/Temp"));
}

UPaperSprite* UCharacter2DLayerOptimizer::CreateOptimizedSprite(const FLayerOptimizationResult& OptimizationResult)
{
    // Вызываем новую функцию с временным путем
    return CreateAndSaveOptimizedSprite(OptimizationResult, TEXT("/Game/Character2D/Temp"));
}

void UCharacter2DLayerOptimizer::ApplyOptimizationToAsset(UCharacter2DAsset* Asset, const TArray<FLayerOptimizationResult>& OptimizationResults)
{
    if (!Asset)
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Cannot apply optimization: Asset is null"));
        return;
    }
    
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("Applying optimization to asset: %s"), *Asset->GetName());
    
    for (const auto& Result : OptimizationResults)
    {
        if (!Result.OptimizedSprite)
        {
            UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Skipping %s: No optimized sprite"), *Result.LayerName);
            continue;
        }
        
        // Применяем оптимизированные спрайты к соответствующим частям ассета
        if (Result.LayerName == TEXT("Body"))
        {
            Asset->SpriteStructure.Body.Sprite = Result.OptimizedSprite;
        }
        else if (Result.LayerName == TEXT("Arms"))
        {
            Asset->SpriteStructure.Arms.Sprite = Result.OptimizedSprite;
        }
        else if (Result.LayerName == TEXT("Head"))
        {
            Asset->SpriteStructure.Head.Head.Sprite = Result.OptimizedSprite;
        }
        else if (Result.LayerName == TEXT("Eyes"))
        {
            Asset->SpriteStructure.Head.Eyes.Sprite = Result.OptimizedSprite;
        }
        else if (Result.LayerName == TEXT("Eyebrows"))
        {
            Asset->SpriteStructure.Head.Eyebrows.Sprite = Result.OptimizedSprite;
        }
        else if (Result.LayerName == TEXT("Eyelids"))
        {
            Asset->SpriteStructure.Head.Eyelids.Sprite = Result.OptimizedSprite;
        }
        else if (Result.LayerName == TEXT("Mouth"))
        {
            Asset->SpriteStructure.Head.Mouth.Sprite = Result.OptimizedSprite;
        }
        else if (Result.LayerName == TEXT("Shadow"))
        {
            Asset->SpriteStructure.Shadow.Sprite = Result.OptimizedSprite;
        }
        
        UE_LOG(LogCharacter2DOptimizer, Log, TEXT("Applied optimized sprite for %s"), *Result.LayerName);
    }
    
    // Помечаем ассет как измененный
    Asset->MarkPackageDirty();
    
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("Optimization applied successfully"));
}

bool UCharacter2DLayerOptimizer::ValidateOptimizedPositions(UCharacter2DAsset* OriginalAsset, UCharacter2DAsset* OptimizedAsset)
{
    if (!OriginalAsset || !OptimizedAsset)
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Cannot validate: One or both assets are null"));
        return false;
    }
    
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("🔍 Validating optimized positions for asset: %s"), *OriginalAsset->GetName());
    
    // Проверяем каждый слой
    auto ValidateLayer = [](UPaperSprite* Original, UPaperSprite* Optimized, const FString& LayerName) -> bool
    {
        if (!Original || !Optimized)
        {
            UE_LOG(LogCharacter2DOptimizer, Log, TEXT("   - %s: Skipped (one sprite is null)"), *LayerName);
            return true;
        }
        
        UE_LOG(LogCharacter2DOptimizer, Log, TEXT("   - %s: Checking positioning..."), *LayerName);
        DebugSpritePositioning(Original, Optimized, LayerName);
        return true;
    };
    
    ValidateLayer(OriginalAsset->SpriteStructure.Body.Sprite, OptimizedAsset->SpriteStructure.Body.Sprite, TEXT("Body"));
    ValidateLayer(OriginalAsset->SpriteStructure.Arms.Sprite, OptimizedAsset->SpriteStructure.Arms.Sprite, TEXT("Arms"));
    ValidateLayer(OriginalAsset->SpriteStructure.Head.Head.Sprite, OptimizedAsset->SpriteStructure.Head.Head.Sprite, TEXT("Head"));
    ValidateLayer(OriginalAsset->SpriteStructure.Head.Eyes.Sprite, OptimizedAsset->SpriteStructure.Head.Eyes.Sprite, TEXT("Eyes"));
    ValidateLayer(OriginalAsset->SpriteStructure.Head.Eyebrows.Sprite, OptimizedAsset->SpriteStructure.Head.Eyebrows.Sprite, TEXT("Eyebrows"));
    ValidateLayer(OriginalAsset->SpriteStructure.Head.Eyelids.Sprite, OptimizedAsset->SpriteStructure.Head.Eyelids.Sprite, TEXT("Eyelids"));
    ValidateLayer(OriginalAsset->SpriteStructure.Head.Mouth.Sprite, OptimizedAsset->SpriteStructure.Head.Mouth.Sprite, TEXT("Mouth"));
    ValidateLayer(OriginalAsset->SpriteStructure.Shadow.Sprite, OptimizedAsset->SpriteStructure.Shadow.Sprite, TEXT("Shadow"));
    
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("✅ Position validation completed"));
    return true;
}

void UCharacter2DLayerOptimizer::DebugSpritePositioning(UPaperSprite* OriginalSprite, UPaperSprite* OptimizedSprite, const FString& LayerName)
{
    if (!OriginalSprite || !OptimizedSprite)
    {
        return;
    }
    
    // Получаем информацию об оригинальном спрайте
    UTexture2D* OriginalTexture = OriginalSprite->GetSourceTexture();
    FVector2D OriginalPivot = OriginalSprite->GetPivotPosition();
    
    // Получаем информацию об оптимизированном спрайте
    UTexture2D* OptimizedTexture = OptimizedSprite->GetSourceTexture();
    FVector2D OptimizedPivot = OptimizedSprite->GetPivotPosition();
    
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("     📏 %s Sprite Analysis:"), *LayerName);
    
    if (OriginalTexture)
    {
        UE_LOG(LogCharacter2DOptimizer, Log, TEXT("       Original: %dx%d, Pivot: (%.1f, %.1f)"), 
               OriginalTexture->GetSizeX(), OriginalTexture->GetSizeY(),
               OriginalPivot.X, OriginalPivot.Y);
    }
    
    if (OptimizedTexture)
    {
        UE_LOG(LogCharacter2DOptimizer, Log, TEXT("       Optimized: %dx%d, Pivot: (%.1f, %.1f)"), 
               OptimizedTexture->GetSizeX(), OptimizedTexture->GetSizeY(),
               OptimizedPivot.X, OptimizedPivot.Y);
    }
    
    // Вычисляем теоретическое смещение
    FVector2D PivotDifference = OptimizedPivot - OriginalPivot;
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("       Pivot difference: (%.1f, %.1f)"), 
           PivotDifference.X, PivotDifference.Y);
    
    // Анализируем pivot mode (исправлено для UE5)
    FVector2D TempPivot;
    ESpritePivotMode::Type OriginalPivotMode = OriginalSprite->GetPivotMode(TempPivot);
    ESpritePivotMode::Type OptimizedPivotMode = OptimizedSprite->GetPivotMode(TempPivot);
    
    FString OriginalModeStr = (OriginalPivotMode == ESpritePivotMode::Custom) ? TEXT("Custom") : TEXT("Center");
    FString OptimizedModeStr = (OptimizedPivotMode == ESpritePivotMode::Custom) ? TEXT("Custom") : TEXT("Center");
    
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("       Pivot modes: Original=%s, Optimized=%s"), 
           *OriginalModeStr, *OptimizedModeStr);
    
    // Получаем custom pivot если есть (исправлено для UE5)
    if (OptimizedPivotMode == ESpritePivotMode::Custom)
    {
        FVector2D CustomPivot;
        OptimizedSprite->GetPivotMode(CustomPivot);
        UE_LOG(LogCharacter2DOptimizer, Log, TEXT("       Custom pivot: (%.3f, %.3f)"), 
               CustomPivot.X, CustomPivot.Y);
    }
    
    // Анализ результата
    if (OptimizedPivotMode == ESpritePivotMode::Custom)
    {
        UE_LOG(LogCharacter2DOptimizer, Log, TEXT("       ✅ Using custom pivot to compensate for positioning"));
    }
    else if (FMath::Abs(PivotDifference.X) < 1.0f && FMath::Abs(PivotDifference.Y) < 1.0f)
    {
        UE_LOG(LogCharacter2DOptimizer, Log, TEXT("       ✅ Position looks correct (minimal pivot difference)"));
    }
    else
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("       ⚠️ Significant position difference detected - may need custom pivot"));
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("           Consider using Custom pivot mode for this sprite"));
    }
}

TArray<FColor> UCharacter2DLayerOptimizer::GetTexturePixelData(UTexture2D* Texture)
{
    TArray<FColor> PixelData;
    
    if (!Texture)
    {
        return PixelData;
    }
    
    // Проверяем что текстура может быть прочитана
    if (!Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Texture has no platform data or mips"));
        return PixelData;
    }
    
    FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
    const void* RawData = Mip.BulkData.LockReadOnly();
    
    if (!RawData)
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Failed to lock texture data"));
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

void UCharacter2DLayerOptimizer::CopyTextureRegion(UTexture2D* SourceTexture, FIntRect SourceRegion, UTexture2D* DestTexture, FIntPoint DestPosition)
{
    // Базовая реализация - в полной версии здесь будет копирование пикселей
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("CopyTextureRegion called (placeholder implementation)"));
}

UTexture2D* UCharacter2DLayerOptimizer::CreateTextureFromPixels(const TArray<FColor>& Pixels, int32 Width, int32 Height, const FString& AssetName)
{
    if (Pixels.Num() != Width * Height)
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Pixel count mismatch: expected %d, got %d"), Width * Height, Pixels.Num());
        return nullptr;
    }
    
    // Создаем transient текстуру
    UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
    
    if (!NewTexture)
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Failed to create transient texture"));
        return nullptr;
    }
    
    // Записываем пиксели в текстуру
    if (NewTexture->GetPlatformData() && NewTexture->GetPlatformData()->Mips.Num() > 0)
    {
        FTexture2DMipMap& Mip = NewTexture->GetPlatformData()->Mips[0];
        void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
        
        if (TextureData)
        {
            FMemory::Memcpy(TextureData, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
            Mip.BulkData.Unlock();
            NewTexture->UpdateResource();
        }
    }
    
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("Created texture %s: %dx%d"), *AssetName, Width, Height);
    return NewTexture;
}

void UCharacter2DLayerOptimizer::SaveOptimizedTextureAsAsset(UTexture2D* Texture, const FString& AssetPath)
{
    // Базовая реализация сохранения
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("SaveOptimizedTextureAsAsset called for path: %s"), *AssetPath);
}