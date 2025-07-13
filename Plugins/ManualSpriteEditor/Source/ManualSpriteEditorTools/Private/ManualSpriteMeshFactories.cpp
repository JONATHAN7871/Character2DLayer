#include "ManualSpriteMeshFactories.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Animation/Skeleton.h"
#include "ReferenceSkeleton.h"
#include "StaticToSkeletalMeshConverter.h"

// ========== Material Factory ==========
UManualSpriteMaterialFactory::UManualSpriteMaterialFactory()
{
    SupportedClass = UMaterial::StaticClass();
    bCreateNew = false;
    bEditAfterNew = false;
}

UObject* UManualSpriteMaterialFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    UMaterial* NewMaterial = NewObject<UMaterial>(InParent, Class, Name, Flags);
    
    if (SourceTexture != nullptr)
    {
        // Создаем Texture Sample узел
        UMaterialExpressionTextureSample* TextureSampler = NewObject<UMaterialExpressionTextureSample>(NewMaterial);
        TextureSampler->Texture = SourceTexture;
        TextureSampler->AutoSetSampleType();
        
        // Настройка материала
        if (bUnlitMaterial)
        {
            NewMaterial->SetShadingModel(MSM_Unlit);
        }
        else
        {
            NewMaterial->SetShadingModel(MSM_DefaultLit);
        }
        
        NewMaterial->BlendMode = BLEND_Masked;
        NewMaterial->TwoSided = bTwoSided;
        
        NewMaterial->GetExpressionCollection().AddExpression(TextureSampler);

        // Подключение к материалу
        UMaterialEditorOnlyData* EditorOnly = NewMaterial->GetEditorOnlyData();
        if (bUnlitMaterial)
        {
            EditorOnly->EmissiveColor.Connect(0, TextureSampler); // RGB to Emissive
        }
        else
        {
            EditorOnly->BaseColor.Connect(0, TextureSampler); // RGB to Base Color
        }
        EditorOnly->OpacityMask.Connect(4, TextureSampler); // Alpha to Opacity Mask
        
        NewMaterial->PostEditChange();
    }
    
    return NewMaterial;
}

// ========== Skeleton Factory ==========
UManualSpriteSkeletonFactory::UManualSpriteSkeletonFactory()
{
    SupportedClass = USkeleton::StaticClass();
    bCreateNew = false;
    bEditAfterNew = false;
}

UObject* UManualSpriteSkeletonFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, FFeedbackContext* InWarn)
{
    USkeleton* Skeleton = NewObject<USkeleton>(InParent, InName, InFlags);
    
    // Создаем корневую кость
    const TCHAR* RootBoneName = TEXT("Root");
    FTransform RootTransform = FTransform::Identity;
    RootTransform.SetTranslation(RootBonePosition);
    
    FReferenceSkeletonModifier Modifier(Skeleton);
    Modifier.Add(FMeshBoneInfo(RootBoneName, RootBoneName, INDEX_NONE), RootTransform);
    
    return Skeleton;
}

// ========== SkeletalMesh Factory ==========
UManualSpriteSkeletalMeshFactory::UManualSpriteSkeletalMeshFactory()
{
    SupportedClass = USkeletalMesh::StaticClass();
    bCreateNew = false;
    bEditAfterNew = false;
}

UObject* UManualSpriteSkeletalMeshFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, FFeedbackContext* InWarn)
{
    if (!SourceStaticMesh || !TargetSkeleton)
    {
        return nullptr;
    }

    USkeletalMesh* SkeletalMesh = NewObject<USkeletalMesh>(InParent, InName, InFlags);
    
    // Конвертируем StaticMesh в SkeletalMesh
    FReferenceSkeleton ReferenceSkeleton = TargetSkeleton->GetReferenceSkeleton();
    
    if (!FStaticToSkeletalMeshConverter::InitializeSkeletalMeshFromStaticMesh(
        SkeletalMesh, 
        SourceStaticMesh, 
        ReferenceSkeleton, 
        RootBoneName))
    {
        return nullptr;
    }

    SkeletalMesh->SetSkeleton(TargetSkeleton);
    TargetSkeleton->MergeAllBonesToBoneTree(SkeletalMesh);
    
    if (!TargetSkeleton->GetPreviewMesh())
    {
        TargetSkeleton->SetPreviewMesh(SkeletalMesh);
    }
    
    return SkeletalMesh;
}       