#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PaperSprite.h"
#include "PaperFlipbook.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Materials/MaterialInterface.h"
#include "Curves/CurveFloat.h"
#include "Character2DAsset.generated.h"

class FAssetRegistryTagsContext;

UENUM(BlueprintType)
enum class ECharacter2DAttachmentTarget : uint8
{
    None    UMETA(DisplayName = "None"),
    Body    UMETA(DisplayName = "Body Mesh"),
    Arms    UMETA(DisplayName = "Arms Mesh"),
    Head    UMETA(DisplayName = "Head Mesh")
};

USTRUCT(BlueprintType)
struct FCharacter2DMovementSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement", meta=(ClampMin="0.0"))
    float Duration = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    TObjectPtr<UCurveFloat> AnimationCurve = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    bool bTeleport = false;
};

USTRUCT(BlueprintType)
struct FCharacter2DEmotionSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Emotion", meta=(ClampMin="0.1"))
    float Duration = 2.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Emotion", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Intensity = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Emotion", meta=(ClampMin="1.0"))
    float ShakeFrequency = 10.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Emotion")
    FLinearColor TargetColor = FLinearColor::Red;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Emotion")
    bool bLoop = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Emotion")
    TObjectPtr<UCurveFloat> AnimationCurve = nullptr;
};

USTRUCT(BlueprintType)
struct FCharacter2DEffectLayer
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Effect")
    FName Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effect")
    TObjectPtr<UPaperSprite> Sprite = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effect|Transform")
    FVector LocalOffset = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effect|Transform")
    float LocalScale = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effect|Visibility")
    bool bVisible = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effect|Appearance")
    FLinearColor Color = FLinearColor::White;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effect|Appearance", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Opacity = 1.0f;
    FCharacter2DEffectLayer() { Name = TEXT("Effect"); }
};

USTRUCT(BlueprintType)
struct FCharacter2DHeadChildSprite
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sprite")
    FName Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
    TObjectPtr<UPaperSprite> Sprite = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Transform", meta=(DisplayName="Local Offset"))
    FVector LocalOffset = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Transform", meta=(DisplayName="Local Scale"))
    float LocalScale = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Visibility")
    bool bVisible = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Visibility", meta=(DisplayName="Override Head Visibility"))
    bool bOverrideHeadVisibility = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Appearance")
    FLinearColor Color = FLinearColor::White;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Appearance", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Opacity = 1.0f;
    
    FCharacter2DHeadChildSprite() { Name = TEXT("HeadChild"); }
    bool GetFinalVisibility(bool bHeadVisible) const { return bOverrideHeadVisibility ? bVisible : (bHeadVisible && bVisible); }
};

USTRUCT(BlueprintType)
struct FCharacter2DBlinkSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink Animation")
    TObjectPtr<UPaperFlipbook> ClosedEyelidsFlipbook = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink Timing", meta=(ClampMin="0.1"))
    float BlinkIntervalMin = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink Timing", meta=(ClampMin="0.1"))
    float BlinkIntervalMax = 5.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink Timing", meta=(ClampMin="0.05"))
    float BlinkDuration = 0.15f;
};

USTRUCT(BlueprintType)
struct FCharacter2DTalkSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Talk Animation")
    TObjectPtr<UPaperFlipbook> TalkFlipbook = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Talk Animation|Timing", meta=(ClampMin="0.05"))
    float MouthChangeInterval = 0.1f;
};

USTRUCT(BlueprintType)
struct FCharacter2DHeadRootSprite
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Head Root")
    FName Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root|Sprite")
    TObjectPtr<UPaperSprite> Sprite = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root|Attachment")
    ECharacter2DAttachmentTarget AttachmentTarget = ECharacter2DAttachmentTarget::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides))
    FName SocketName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides))
    bool bUseSocketTransform = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root|Transform")
    FVector Offset = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root|Transform")
    float Scale = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root|Visibility")
    bool bVisible = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root|Appearance")
    FLinearColor Color = FLinearColor::White;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root|Appearance", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Opacity = 1.0f;
    
    FCharacter2DHeadRootSprite() { Name = TEXT("Head"); }
};

USTRUCT(BlueprintType)
struct FCharacter2DHeadStructure
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root", meta=(DisplayName="Head (Root)"))
    FCharacter2DHeadRootSprite Head;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Elements", meta=(DisplayName="Eyebrows"))
    FCharacter2DHeadChildSprite Eyebrows;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Elements", meta=(DisplayName="Eyes"))
    FCharacter2DHeadChildSprite Eyes;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Elements", meta=(DisplayName="Eyelids (Static)"))
    FCharacter2DHeadChildSprite Eyelids;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Elements", meta=(DisplayName="Mouth (Static)"))
    FCharacter2DHeadChildSprite Mouth;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Effects", meta=(DisplayName="Effect Layer 1"))
    FCharacter2DEffectLayer EffectLayer1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Effects", meta=(DisplayName="Effect Layer 2"))
    FCharacter2DEffectLayer EffectLayer2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Effects", meta=(DisplayName="Effect Layer 3"))
    FCharacter2DEffectLayer EffectLayer3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Animations", meta=(DisplayName="Blink Settings"))
    FCharacter2DBlinkSettings BlinkSettings;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Animations", meta=(DisplayName="Talk Settings"))
    FCharacter2DTalkSettings TalkSettings;
    FCharacter2DHeadStructure() { Head.Name = TEXT("Head"); Eyebrows.Name = TEXT("Eyebrows"); Eyes.Name = TEXT("Eyes"); Eyelids.Name = TEXT("Eyelids"); Mouth.Name = TEXT("Mouth"); EffectLayer1.Name = TEXT("EffectLayer1"); EffectLayer2.Name = TEXT("EffectLayer2"); EffectLayer3.Name = TEXT("EffectLayer3"); }
    bool GetFinalChildVisibility(const FCharacter2DHeadChildSprite& ChildSprite) const { return ChildSprite.GetFinalVisibility(Head.bVisible); }
};

USTRUCT(BlueprintType)
struct FCharacter2DSpriteBodyStructure
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sprite") FName Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite") TObjectPtr<UPaperSprite> Sprite = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Attachment") ECharacter2DAttachmentTarget AttachmentTarget = ECharacter2DAttachmentTarget::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides)) FName SocketName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides)) bool bUseSocketTransform = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite") FVector Offset = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite") float Scale = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite") bool bVisible = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Appearance")
    FLinearColor Color = FLinearColor::White;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Appearance", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Opacity = 1.0f;
    
    FCharacter2DSpriteBodyStructure() { Name = TEXT("Body"); }
};

USTRUCT(BlueprintType)
struct FCharacter2DSpriteArmsStructure
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sprite") FName Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite") TObjectPtr<UPaperSprite> Sprite = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Attachment") ECharacter2DAttachmentTarget AttachmentTarget = ECharacter2DAttachmentTarget::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides)) FName SocketName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides)) bool bUseSocketTransform = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite") FVector Offset = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite") float Scale = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite") bool bVisible = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Appearance")
    FLinearColor Color = FLinearColor::White;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Appearance", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Opacity = 1.0f;
    
    FCharacter2DSpriteArmsStructure() { Name = TEXT("Arms"); }
};

USTRUCT(BlueprintType)
struct FCharacter2DSpriteTransformStructure
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Transform", meta=(DisplayName="Global Offset"))
    FVector GlobalOffset = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Transform", meta=(DisplayName="Global Scale"))
    float GlobalScale = 1.0f;
};

USTRUCT(BlueprintType)
struct FCharacter2DSpriteStructure
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Body") FCharacter2DSpriteBodyStructure Body;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Arms") FCharacter2DSpriteArmsStructure Arms;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Head") FCharacter2DHeadStructure Head;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Transform") FCharacter2DSpriteTransformStructure Transform;
};

USTRUCT(BlueprintType)
struct FCharacter2DSkeletalMaterial
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skeletal") TObjectPtr<UMaterialInterface> Material = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skeletal") int32 SlotIndex = 0;
};

USTRUCT(BlueprintType)
struct FCharacter2DSkeletalPart
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal") TObjectPtr<USkeletalMesh> Mesh = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal") TArray<FCharacter2DSkeletalMaterial> Materials;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal") TSubclassOf<UAnimInstance> AnimInstance;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal") FVector Offset = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal") float Scale = 1.0f;
};

UCLASS(BlueprintType)
class CHARACTER2DRUNTIME_API UCharacter2DAsset : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal", meta=(DisplayName="Body")) FCharacter2DSkeletalPart Body;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal", meta=(DisplayName="Arms")) FCharacter2DSkeletalPart Arms;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal", meta=(DisplayName="Head")) FCharacter2DSkeletalPart Head;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal", meta=(DisplayName="Global Offset")) FVector SkeletalGlobalOffset = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal", meta=(DisplayName="Global Scale")) float GlobalScale = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite") FCharacter2DSpriteStructure SpriteStructure;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="General") bool bAutoBlink = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="General") bool bAutoTalk = false;

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites") const FCharacter2DSpriteBodyStructure& GetBodySprite() const { return SpriteStructure.Body; }
    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites") const FCharacter2DSpriteArmsStructure& GetArmsSprite() const { return SpriteStructure.Arms; }
    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites") const FCharacter2DHeadRootSprite& GetHeadSprite() const { return SpriteStructure.Head.Head; }
    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites") const FCharacter2DHeadChildSprite& GetEyebrowSprite() const { return SpriteStructure.Head.Eyebrows; }
    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites") const FCharacter2DHeadChildSprite& GetEyesSprite() const { return SpriteStructure.Head.Eyes; }
    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites") const FCharacter2DHeadChildSprite& GetEyelidsSprite() const { return SpriteStructure.Head.Eyelids; }
    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites") const FCharacter2DHeadChildSprite& GetMouthSprite() const { return SpriteStructure.Head.Mouth; }
    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites") const FCharacter2DBlinkSettings& GetBlinkSettings() const { return SpriteStructure.Head.BlinkSettings; }
    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites") const FCharacter2DTalkSettings& GetTalkSettings() const { return SpriteStructure.Head.TalkSettings; }
    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites") const FCharacter2DEffectLayer& GetEffectLayer1() const { return SpriteStructure.Head.EffectLayer1; }
    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites") const FCharacter2DEffectLayer& GetEffectLayer2() const { return SpriteStructure.Head.EffectLayer2; }
    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites") const FCharacter2DEffectLayer& GetEffectLayer3() const { return SpriteStructure.Head.EffectLayer3; }
    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites") FVector GetGlobalSpriteOffset() const { return SpriteStructure.Transform.GlobalOffset; }
    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites") float GetGlobalSpriteScale() const { return SpriteStructure.Transform.GlobalScale; }

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
    void ValidateHeadHierarchy();
#endif

    UFUNCTION(BlueprintCallable, Category = "Character2D|Validation") bool IsValidForRuntime() const;
    UFUNCTION(BlueprintCallable, Category = "Character2D|Validation") bool HasValidSpriteConfiguration() const;
    UFUNCTION(BlueprintCallable, Category = "Character2D|Validation") bool HasValidSkeletalConfiguration() const;
protected:
    virtual void PostLoad() override;
};