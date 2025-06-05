#pragma once

#include "CoreMinimal.h"
#include "Character2DBuilderWindow/AssetData/Character2DLayerData.h"
#include "Character2DMeshGeneratorOptions.h"
#include "MeshDescription.h"
#include "MeshDescriptionBuilder.h"

class UTexture2D;
class UPaperSprite;
class UStaticMesh;
class FMeshDescriptionBuilder;
struct FPolygonGroupID;

/**
 * Опции генерации меша:
 * - per-category (bUseGridMesh, GridCellSize, AlphaThreshold) задаются в SCharacter2DBuilderWindow
 * - глобальные (OutputType, PivotPlacement, AssetName, SavePath, MeshScale) — через UCharacter2DMeshGeneratorOptions
 */
struct FCharacter2DMeshGenerationOptions
{
    /** Тип ассета на выходе */
    ECharacter2DMeshOutputType       OutputType      = ECharacter2DMeshOutputType::StaticMesh;
    /** Размещение пивота или корневой кости */
    ECharacter2DRootBonePlacement    PivotPlacement  = ECharacter2DRootBonePlacement::BottomCenter;
    /** Имя создаваемого ассета */
    FString                           AssetName       = TEXT("GeneratedMesh");
    /** Путь для сохранения ассета (Content Browser) */
    FString                           SavePath        = TEXT("/Game/Character2DBuilder/Generated");

    // Перенесённые из SCharacter2DBuilderWindow: per-category
    bool    bUseGridMesh   = true;
    int32   GridCellSize   = 32;
    uint8   AlphaThreshold = 64;

    /** Глобальный масштаб меша (1 UU = 1 см) */
    float   MeshScale      = 1.0f;
};

namespace Character2DMeshGenerator
{
    /**
     * Генерация "grid" меша для одного спрайта:
     * разбивает область на ячейки CellSize, отфильтровывает по AlphaThreshold,
     * масштабирует вершины по MeshScale.
     */
    static bool GenerateGridMeshFromSprite(
        UPaperSprite* Sprite,
        const FVector& Offset,
        const FPolygonGroupID& Group,
        FMeshDescriptionBuilder& Builder,
        TArray<FVector2D>& OutUVs,
        int32 CellSize,
        uint8 AlphaThreshold,
        float MeshScale
    );

    /**
     * Генерирует меш (Static или Skeletal) по списку категорий,
     * применяя per-category настройки из Categories и глобальные из Options.
     */
    void GenerateMeshFromOptions(
        const TArray<TSharedPtr<FCharacter2DLayerCategory>>& Categories,
        const FCharacter2DMeshGenerationOptions& Options
    );

    /**
     * Запуск генерации "из коробки": читает глобальные опции из UCharacter2DMeshGeneratorOptions
     * и вызывает GenerateMeshFromOptions.
     */
    void ShowMeshGenerationDialog(
        const TArray<TSharedPtr<FCharacter2DLayerCategory>>& Categories
    );

    /**
     * Собирает MeshDescription и массив текстур из категорий,
     * используя переданные Options для контроля grid/переменных и масштаба.
     */
    void BuildMeshDescriptionAndTextures(
        const TArray<TSharedPtr<FCharacter2DLayerCategory>>& Categories,
        FMeshDescription& OutDesc,
        TArray<UTexture*>& OutTextures,
        TArray<UPaperSprite*>& OutUniqueSprites,
        const FCharacter2DMeshGenerationOptions& Options
    );

    /** Проверяет, поддерживает ли текстура формат PF_B8G8R8A8 */
    bool IsTextureFormatSupported(UTexture2D* Texture);
}
