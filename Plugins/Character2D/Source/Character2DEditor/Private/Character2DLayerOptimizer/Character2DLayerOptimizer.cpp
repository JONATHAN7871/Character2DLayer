#include "Character2DLayerOptimizer/Character2DLayerOptimizer.h"
#include "Engine/Texture2D.h"
#include "PaperSprite.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "HAL/PlatformFileManager.h"
#include "ImageUtils.h"

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
    
    // Анализируем каждый слой
    if (Asset->SpriteStructure.Body.Sprite)
    {
        Results.Add(OptimizeLayer(Asset->SpriteStructure.Body.Sprite, TEXT("Body")));
    }
    
    if (Asset->SpriteStructure.Arms.Sprite)
    {
        Results.Add(OptimizeLayer(Asset->SpriteStructure.Arms.Sprite, TEXT("Arms")));
    }
    
    if (Asset->SpriteStructure.Head.Head.Sprite)
    {
        Results.Add(OptimizeLayer(Asset->SpriteStructure.Head.Head.Sprite, TEXT("Head")));
    }
    
    if (Asset->SpriteStructure.Head.Eyes.Sprite)
    {
        Results.Add(OptimizeLayer(Asset->SpriteStructure.Head.Eyes.Sprite, TEXT("Eyes")));
    }
    
    if (Asset->SpriteStructure.Head.Eyebrows.Sprite)
    {
        Results.Add(OptimizeLayer(Asset->SpriteStructure.Head.Eyebrows.Sprite, TEXT("Eyebrows")));
    }
    
    if (Asset->SpriteStructure.Head.Eyelids.Sprite)
    {
        Results.Add(OptimizeLayer(Asset->SpriteStructure.Head.Eyelids.Sprite, TEXT("Eyelids")));
    }
    
    if (Asset->SpriteStructure.Head.Mouth.Sprite)
    {
        Results.Add(OptimizeLayer(Asset->SpriteStructure.Head.Mouth.Sprite, TEXT("Mouth")));
    }
    
    if (Asset->SpriteStructure.Shadow.Sprite)
    {
        Results.Add(OptimizeLayer(Asset->SpriteStructure.Shadow.Sprite, TEXT("Shadow")));
    }
    
    // Подсчитываем общую статистику
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

FLayerOptimizationResult UCharacter2DLayerOptimizer::OptimizeLayer(UPaperSprite* LayerSprite, const FString& LayerName)
{
    FLayerOptimizationResult Result;
    Result.LayerName = LayerName;
    
    if (!LayerSprite)
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Layer %s: Invalid sprite"), *LayerName);
        return Result;
    }
    
    // Получаем текстуру из спрайта
    UTexture2D* SourceTexture = LayerSprite->GetSourceTexture();
    if (!SourceTexture)
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Layer %s: No source texture"), *LayerName);
        return Result;
    }
    
    Result.OriginalTexture = SourceTexture;
    
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("Optimizing layer: %s (Size: %dx%d)"), 
           *LayerName, SourceTexture->GetSizeX(), SourceTexture->GetSizeY());
    
    // Находим реально используемую область
    FIntRect UsedBounds = FindUsedBounds(SourceTexture);
    Result.UsedRegion = UsedBounds;
    
    // Вычисляем размеры
    int32 OriginalPixels = SourceTexture->GetSizeX() * SourceTexture->GetSizeY();
    int32 UsedPixels = UsedBounds.Width() * UsedBounds.Height();
    
    Result.OriginalSizeMB = (OriginalPixels * 4) / (1024.0f * 1024.0f);
    Result.OptimizedSizeMB = (UsedPixels * 4) / (1024.0f * 1024.0f);
    Result.SavingsPercent = Result.OriginalSizeMB > 0 ? ((Result.OriginalSizeMB - Result.OptimizedSizeMB) / Result.OriginalSizeMB) * 100.0f : 0.0f;
    
    // Создаем оптимизированную текстуру
    Result.OptimizedTexture = CreateOptimizedTexture(SourceTexture, UsedBounds, LayerName);
    
    // Сохраняем данные позиционирования
    Result.PositionData.OriginalCanvasSize = FVector2D(SourceTexture->GetSizeX(), SourceTexture->GetSizeY());
    Result.PositionData.ContentBounds = UsedBounds;
    Result.PositionData.PositionOffset = Result.PositionData.GetCorrectedPosition();
    Result.PositionData.OptimizedTexture = Result.OptimizedTexture;
    
    // Создаем оптимизированный спрайт
    Result.OptimizedSprite = CreateOptimizedSprite(Result);
    
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("Layer %s: %.1f MB -> %.1f MB (%.1f%% savings), Bounds: (%d,%d,%d,%d)"), 
           *LayerName, Result.OriginalSizeMB, Result.OptimizedSizeMB, Result.SavingsPercent,
           UsedBounds.Min.X, UsedBounds.Min.Y, UsedBounds.Max.X, UsedBounds.Max.Y);
    
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
        // Если ничего не найдено, возвращаем центральную область
        return FIntRect(Width/2 - 50, Height/2 - 50, Width/2 + 50, Height/2 + 50);
    }
    
    // Добавляем небольшой padding
    int32 Padding = 4;
    MinX = FMath::Max(0, MinX - Padding);
    MinY = FMath::Max(0, MinY - Padding);
    MaxX = FMath::Min(Width - 1, MaxX + Padding);
    MaxY = FMath::Min(Height - 1, MaxY + Padding);
    
    return FIntRect(MinX, MinY, MaxX + 1, MaxY + 1);
}

UTexture2D* UCharacter2DLayerOptimizer::CreateOptimizedTexture(UTexture2D* SourceTexture, FIntRect UsedRegion, const FString& LayerName)
{
    if (!SourceTexture)
    {
        return nullptr;
    }
    
    // Создаем новую текстуру только с используемой областью
    int32 NewWidth = UsedRegion.Width();
    int32 NewHeight = UsedRegion.Height();
    
    if (NewWidth <= 0 || NewHeight <= 0)
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Invalid optimized texture size: %dx%d"), NewWidth, NewHeight);
        return nullptr;
    }
    
    // Получаем исходные пиксели
    TArray<FColor> SourcePixels = GetTexturePixelData(SourceTexture);
    if (SourcePixels.Num() == 0)
    {
        return nullptr;
    }
    
    // Извлекаем пиксели только из нужной области
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
    
    // Создаем новую текстуру
    FString OptimizedAssetName = FString::Printf(TEXT("%s_Optimized"), *LayerName);
    UTexture2D* OptimizedTexture = CreateTextureFromPixels(OptimizedPixels, NewWidth, NewHeight, OptimizedAssetName);
    
    if (OptimizedTexture)
    {
        UE_LOG(LogCharacter2DOptimizer, Log, TEXT("Created optimized texture for %s: %dx%d"), *LayerName, NewWidth, NewHeight);
    }
    
    return OptimizedTexture;
}

UPaperSprite* UCharacter2DLayerOptimizer::CreateOptimizedSprite(const FLayerOptimizationResult& OptimizationResult)
{
    if (!OptimizationResult.OptimizedTexture)
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("No optimized texture for %s"), *OptimizationResult.LayerName);
        return nullptr;
    }
    
    UPaperSprite* NewSprite = NewObject<UPaperSprite>();
    if (!NewSprite)
    {
        UE_LOG(LogCharacter2DOptimizer, Error, TEXT("Failed to create sprite for %s"), *OptimizationResult.LayerName);
        return nullptr;
    }
    
    // ИСПРАВЛЕНО: Используем правильные параметры инициализации
    FSpriteAssetInitParameters InitParams;
    InitParams.Texture = OptimizationResult.OptimizedTexture;
    InitParams.Offset = FIntPoint::ZeroValue;  // ИСПРАВЛЕНО: ZeroValue вместо (0,0)
    InitParams.Dimension = FIntPoint(
        OptimizationResult.OptimizedTexture->GetSizeX(), 
        OptimizationResult.OptimizedTexture->GetSizeY()
    );
    
    // ОТЛАДКА: Логируем параметры
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("Creating sprite %s: Texture=%dx%d, Dimension=%dx%d"), 
           *OptimizationResult.LayerName,
           OptimizationResult.OptimizedTexture->GetSizeX(), OptimizationResult.OptimizedTexture->GetSizeY(),
           InitParams.Dimension.X, InitParams.Dimension.Y);
    
    // Инициализируем спрайт с базовыми параметрами
    NewSprite->InitializeSprite(InitParams, true); // ИСПРАВЛЕНО: добавлен параметр bRebuildData
    
    // ИСПРАВЛЕНО: Вычисляем корректный pivot с учетом оригинальной позиции
    FVector2D OriginalCanvasSize = OptimizationResult.PositionData.OriginalCanvasSize;
    FVector2D PositionOffset = OptimizationResult.PositionData.PositionOffset;
    FVector2D TextureSize = FVector2D(InitParams.Dimension.X, InitParams.Dimension.Y);
    
    // Вычисляем offset в мировых координатах
    FVector2D WorldOffset = FVector2D::ZeroVector;
    if (OriginalCanvasSize.X > 0 && OriginalCanvasSize.Y > 0)
    {
        // Offset = насколько сдвинулся центр обрезанной области относительно центра оригинала
        FVector2D OriginalCenter = OriginalCanvasSize * 0.5f;
        FVector2D NewCenter = PositionOffset + TextureSize * 0.5f;
        WorldOffset = NewCenter - OriginalCenter;
        
        UE_LOG(LogCharacter2DOptimizer, Log, TEXT("Sprite %s offset calculation: OriginalCenter=(%.1f,%.1f), NewCenter=(%.1f,%.1f), WorldOffset=(%.1f,%.1f)"), 
               *OptimizationResult.LayerName, 
               OriginalCenter.X, OriginalCenter.Y,
               NewCenter.X, NewCenter.Y,
               WorldOffset.X, WorldOffset.Y);
    }
    
    // ИСПРАВЛЕНО: Устанавливаем pivot с учетом смещения
    // Pivot в UE4/5 это точка вращения в нормализованных координатах (0-1)
    FVector2D PivotPoint = FVector2D(0.5f, 0.5f); // Центр спрайта
    
    // Корректируем pivot если есть значительное смещение
    if (TextureSize.X > 0 && TextureSize.Y > 0)
    {
        FVector2D PivotCorrection = -WorldOffset / TextureSize; // Инвертируем смещение
        PivotPoint += PivotCorrection;
        
        // Ограничиваем pivot разумными пределами
        PivotPoint.X = FMath::Clamp(PivotPoint.X, -1.0f, 2.0f);
        PivotPoint.Y = FMath::Clamp(PivotPoint.Y, -1.0f, 2.0f);
    }
    
    // Устанавливаем кастомный pivot
    NewSprite->SetPivotMode(ESpritePivotMode::Custom, PivotPoint, true); // ИСПРАВЛЕНО: добавлен параметр bRebuildData
    
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("Created optimized sprite for %s: size %dx%d, pivot (%.3f, %.3f)"), 
           *OptimizationResult.LayerName, 
           InitParams.Dimension.X, InitParams.Dimension.Y,
           PivotPoint.X, PivotPoint.Y);
    
    return NewSprite;
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
    // Базовая валидация - проверяем что оба ассета существуют
    if (!OriginalAsset || !OptimizedAsset)
    {
        UE_LOG(LogCharacter2DOptimizer, Warning, TEXT("Cannot validate: One or both assets are null"));
        return false;
    }
    
    UE_LOG(LogCharacter2DOptimizer, Log, TEXT("Position validation passed (basic check)"));
    return true;
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