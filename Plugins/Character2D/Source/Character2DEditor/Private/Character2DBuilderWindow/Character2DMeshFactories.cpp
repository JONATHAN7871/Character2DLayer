#include "Character2DBuilderWindow/Character2DMeshFactories.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Animation/Skeleton.h"
#include "ReferenceSkeleton.h"
#include "StaticToSkeletalMeshConverter.h"

// ========== Material Factory ==========
UCharacter2D_MaterialFactory::UCharacter2D_MaterialFactory()
{
    SupportedClass = UMaterial::StaticClass();
    bCreateNew = false;
    bEditAfterNew = true;
}

UObject* UCharacter2D_MaterialFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    UMaterial* NewMaterial = NewObject<UMaterial>(InParent, Class, Name, Flags);
    if (InitialTexture != nullptr)
    {
        UMaterialExpressionTextureSample* TextureSampler = NewObject<UMaterialExpressionTextureSample>(NewMaterial);
        TextureSampler->Texture = InitialTexture;
        TextureSampler->AutoSetSampleType();
        NewMaterial->BlendMode = BLEND_Masked;
        NewMaterial->SetShadingModel(MSM_Unlit);
        NewMaterial->TwoSided = true;
        NewMaterial->GetExpressionCollection().AddExpression(TextureSampler);

        UMaterialEditorOnlyData* EditorOnly = NewMaterial->GetEditorOnlyData();
        EditorOnly->EmissiveColor.Connect(0, TextureSampler);
        EditorOnly->OpacityMask.Connect(4, TextureSampler);
        NewMaterial->PostEditChange();
    }
    return NewMaterial;
}

// ========== Skeleton Factory ==========
UCharacter2D_SkeletonFactory::UCharacter2D_SkeletonFactory()
{
    SupportedClass = USkeleton::StaticClass();
    bCreateNew = false;
    bEditAfterNew = false;
}

UObject* UCharacter2D_SkeletonFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, FFeedbackContext* InWarn)
{
    USkeleton* Skeleton = NewObject<USkeleton>(InParent, InName, InFlags);
    FVector WantedRootPosition = RootPosition;
    if (PositionReference == ECharacter2D_RootBoneReference::Relative)
    {
        const FBox Bounds = StaticMesh->GetBoundingBox();
        WantedRootPosition = Bounds.Min + (Bounds.Max - Bounds.Min) * RootPosition;
    }
    const TCHAR* RootBoneName = TEXT("Root");
    FTransform RootTransform(FTransform::Identity);
    RootTransform.SetTranslation(WantedRootPosition);
    FReferenceSkeletonModifier Modifier(Skeleton);
    Modifier.Add(FMeshBoneInfo(RootBoneName, RootBoneName, INDEX_NONE), RootTransform);
    return Skeleton;
}

// ========== SkeletalMesh Factory ==========
UCharacter2D_SkeletalMeshFactory::UCharacter2D_SkeletalMeshFactory()
{
    SupportedClass = USkeletalMesh::StaticClass();
    bCreateNew = false;
    bEditAfterNew = true;
}

UObject* UCharacter2D_SkeletalMeshFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, FFeedbackContext* InWarn)
{
    USkeletalMesh* SkeletalMesh = NewObject<USkeletalMesh>(InParent, InName, InFlags);
    if (!FStaticToSkeletalMeshConverter::InitializeSkeletalMeshFromStaticMesh(SkeletalMesh, StaticMesh, ReferenceSkeleton, BindBoneName))
        return nullptr;
    SkeletalMesh->SetSkeleton(Skeleton);
    Skeleton->MergeAllBonesToBoneTree(SkeletalMesh);
    if (!Skeleton->GetPreviewMesh())
        Skeleton->SetPreviewMesh(SkeletalMesh);
    return SkeletalMesh;
}
