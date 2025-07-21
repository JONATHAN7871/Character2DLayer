#pragma once

#include "CoreMinimal.h"
#include "Engine/SkeletalMesh.h"
#include "PaperSprite.h"
#include "Materials/MaterialInterface.h"
#include "Animation/AnimInstance.h"
#include "Data/VNCharacterEnums.h"
#include "VNCharacterDataAssetStructs.generated.h"

/**
 * Структуры конфигурации для DataAsset
 */

USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API F_VN_SkeletalConfig_Body
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowedClasses = "SkeletalMesh"))
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UAnimInstance> AnimInstanceClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<int32, TSoftObjectPtr<UMaterialInterface>> MaterialOverrides;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Color = FLinearColor::White;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Offset = FVector::ZeroVector;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Scale = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bVisible = true;
    
    F_VN_SkeletalConfig_Body()
    {
        SkeletalMesh = nullptr;
        AnimInstanceClass = nullptr;
        Color = FLinearColor::White;
        Offset = FVector::ZeroVector;
        Scale = 1.0f;
        bVisible = true;
    }
};

USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API F_VN_SkeletalConfig_Attachment
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowedClasses = "SkeletalMesh"))
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UAnimInstance> AnimInstanceClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<int32, TSoftObjectPtr<UMaterialInterface>> MaterialOverrides;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    E_SkeletalAttachmentTarget AttachTo = E_SkeletalAttachmentTarget::Body;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SocketName = NAME_None;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUseSocketTransform = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Color = FLinearColor::White;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Offset = FVector::ZeroVector;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Scale = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bVisible = true;
    
    F_VN_SkeletalConfig_Attachment()
    {
        SkeletalMesh = nullptr;
        AnimInstanceClass = nullptr;
        AttachTo = E_SkeletalAttachmentTarget::Body;
        SocketName = NAME_None;
        bUseSocketTransform = true;
        Color = FLinearColor::White;
        Offset = FVector::ZeroVector;
        Scale = 1.0f;
        bVisible = true;
    }
};

USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API F_VN_SpriteConfig_Attachment
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowedClasses = "PaperSprite"))
    TSoftObjectPtr<UPaperSprite> Sprite;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    E_SpriteAttachmentTarget AttachTo = E_SpriteAttachmentTarget::Body_Skeletal;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SocketName = NAME_None;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUseSocketTransform = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Offset = FVector::ZeroVector;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Scale = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Color = FLinearColor::White;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bVisible = true;
    
    F_VN_SpriteConfig_Attachment()
    {
        Sprite = nullptr;
        AttachTo = E_SpriteAttachmentTarget::Body_Skeletal;
        SocketName = NAME_None;
        bUseSocketTransform = true;
        Offset = FVector::ZeroVector;
        Scale = 1.0f;
        Color = FLinearColor::White;
        bVisible = true;
    }
};

USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API F_VN_SpriteConfig_Simple
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowedClasses = "PaperSprite"))
    TSoftObjectPtr<UPaperSprite> Sprite;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Offset = FVector::ZeroVector;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Scale = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Color = FLinearColor::White;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bVisible = true;
    
    F_VN_SpriteConfig_Simple()
    {
        Sprite = nullptr;
        Offset = FVector::ZeroVector;
        Scale = 1.0f;
        Color = FLinearColor::White;
        bVisible = true;
    }
};