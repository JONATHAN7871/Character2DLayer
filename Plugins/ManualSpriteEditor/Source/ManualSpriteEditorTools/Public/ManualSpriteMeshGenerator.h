#pragma once

#include "CoreMinimal.h"
#include "ManualSprite.h"
#include "ManualSpriteMeshGeneratorOptions.h"
#include "MeshDescription.h"

class UStaticMesh;
class USkeletalMesh;
class USkeleton;
class UMaterialInterface;

/**
 * Опции генерации меша из Manual Sprite
 */
struct FManualSpriteMeshGenerationParams
{
    EManualSpriteMeshType MeshType = EManualSpriteMeshType::StaticMesh;
    EManualSpritePivotPlacement PivotPlacement = EManualSpritePivotPlacement::Center;
    FVector CustomPivotOffset = FVector::ZeroVector;
    float MeshScale = 1.0f;
    FVector MeshOffset = FVector::ZeroVector;
    FString AssetName = TEXT("ManualSpriteMesh");
    FString SavePath = TEXT("/Game/GeneratedMeshes");
    bool bCreateMaterial = true;
    bool bCreateUnlitMaterial = true;
    bool bTwoSidedMaterial = true;
};

namespace ManualSpriteMeshGenerator
{
    /**
     * Показать диалог генерации меша
     */
    void ShowMeshGenerationDialog(UManualSprite* ManualSprite);

    /**
     * Сгенерировать меш из Manual Sprite с заданными параметрами
     */
    bool GenerateMeshFromSprite(UManualSprite* ManualSprite, const FManualSpriteMeshGenerationParams& Params);

    /**
     * Создать MeshDescription из геометрии Manual Sprite
     */
    bool CreateMeshDescriptionFromSprite(UManualSprite* ManualSprite, const FManualSpriteMeshGenerationParams& Params, FMeshDescription& OutMeshDesc);

    /**
     * Создать материал для спрайта
     */
    UMaterialInterface* CreateSpriteMaterial(UTexture* Texture, const FString& AssetPath, const FString& MaterialName, bool bUnlit = true, bool bTwoSided = true);

    /**
     * Создать StaticMesh из MeshDescription
     */
    UStaticMesh* CreateStaticMesh(const FMeshDescription& MeshDesc, const FString& AssetPath, const FString& MeshName);

    /**
     * Создать SkeletalMesh из StaticMesh
     */
    USkeletalMesh* CreateSkeletalMesh(UStaticMesh* StaticMesh, const FString& AssetPath, const FString& MeshName, const FVector& RootBonePosition);

    /**
     * Создать Skeleton для SkeletalMesh
     */
    USkeleton* CreateSkeleton(const FString& AssetPath, const FString& SkeletonName, const FVector& RootBonePosition);

    /**
     * Вычислить позицию пивота
     */
    FVector CalculatePivotPosition(const FBox& MeshBounds, EManualSpritePivotPlacement PivotPlacement, const FVector& CustomOffset);
}