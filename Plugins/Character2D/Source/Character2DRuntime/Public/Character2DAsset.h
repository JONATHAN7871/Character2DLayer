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

/* ───────────────────────────── Visual Novel Emotions ───────────────────────────── */
UENUM(BlueprintType)
enum class ECharacter2DEmotionEffect : uint8
{
    None        UMETA(DisplayName = "None"),
    Shake       UMETA(DisplayName = "Shake"),
    Pulse       UMETA(DisplayName = "Pulse"),
    ColorShift  UMETA(DisplayName = "Color Shift"),
    Bounce      UMETA(DisplayName = "Bounce"),
    Flash       UMETA(DisplayName = "Flash")
};

/* ───────────────────────────── Sprite Attachment ───────────────────────────── */
UENUM(BlueprintType)
enum class ECharacter2DAttachmentTarget : uint8
{
    None    UMETA(DisplayName = "None"),
    Body    UMETA(DisplayName = "Body Mesh"),
    Arms    UMETA(DisplayName = "Arms Mesh"),
    Head    UMETA(DisplayName = "Head Mesh")
};

/* ───────────────────────────── Movement Settings ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DMovementSettings
{
    GENERATED_BODY()

    /** Duration of the movement in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement", meta=(ClampMin="0.0"))
    float Duration = 1.0f;

    /** Animation curve for the movement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    TObjectPtr<UCurveFloat> AnimationCurve = nullptr;

    /** Whether to use teleport (instant movement) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    bool bTeleport = false;
};

/* ───────────────────────────── Emotion Settings ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DEmotionSettings
{
    GENERATED_BODY()

    /** Duration of the emotion effect */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Emotion", meta=(ClampMin="0.1"))
    float Duration = 2.0f;

    /** Intensity of the effect (0.0 to 1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Emotion", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Intensity = 0.5f;

    /** For shake effect - frequency */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Emotion", meta=(ClampMin="1.0"))
    float ShakeFrequency = 10.0f;

    /** For color effects - target color */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Emotion")
    FLinearColor TargetColor = FLinearColor::Red;

    /** Whether to loop the effect */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Emotion")
    bool bLoop = false;

    /** Animation curve for the effect */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Emotion")
    TObjectPtr<UCurveFloat> AnimationCurve = nullptr;
};

/* ───────────────────────────── Head Child Sprite Layer ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DHeadChildSprite
{
    GENERATED_BODY()

    /** Имя элемента */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sprite")
    FName Name;

    /** Статичный спрайт */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
    TObjectPtr<UPaperSprite> Sprite = nullptr;

    /** Локальный оффсет относительно головы (X вправо, Y вверх) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Transform")
    FVector LocalOffset = FVector::ZeroVector;

    /** Локальный Scale относительно головы */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Transform")
    float LocalScale = 1.0f;

    /** Видимость (наследуется от головы если голова невидима) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Visibility")
    bool bVisible = true;

    /** Переопределить наследование видимости головы */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Visibility", meta=(DisplayName="Override Head Visibility"))
    bool bOverrideHeadVisibility = false;

    FCharacter2DHeadChildSprite()
    {
        Name = TEXT("HeadChild");
    }

    // Utility function to get final visibility considering head inheritance
    bool GetFinalVisibility(bool bHeadVisible) const
    {
        if (bOverrideHeadVisibility)
        {
            return bVisible;
        }
        return bHeadVisible && bVisible;
    }
};

/* ───────────────────────────── Blink Settings ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DBlinkSettings
{
    GENERATED_BODY()

    /** Flipbook с кадрами моргания (открыто → полу-закрыто → закрыто) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink Animation")
    TObjectPtr<UPaperFlipbook> BlinkFlipbook = nullptr;

    /** Локальный сдвиг относительно головы для Flipbook */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink Animation|Transform")
    FVector LocalOffset = FVector::ZeroVector;

    /** Локальный Scale относительно головы */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink Animation|Transform")
    float LocalScale = 1.0f;

    /** Мин/Макс интервал до моргания (сек) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink Timing", meta=(ClampMin="0.1"))
    float BlinkIntervalMin = 2.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink Timing", meta=(ClampMin="0.1"))
    float BlinkIntervalMax = 5.f;

    /** Мин/Макс скорость воспроизведения */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink Timing", meta=(ClampMin="0.1"))
    float BlinkPlayRateMin = 1.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink Timing", meta=(ClampMin="0.1"))
    float BlinkPlayRateMax = 2.f;
};

/* ───────────────────────────── Talk Settings ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DTalkSettings
{
    GENERATED_BODY()

    /** Flipbook с кадрами говорения (движение губ) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Talk Animation")
    TObjectPtr<UPaperFlipbook> TalkFlipbook = nullptr;

    /** Локальный сдвиг относительно головы для Flipbook */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Talk Animation|Transform")
    FVector LocalOffset = FVector::ZeroVector;

    /** Локальный Scale относительно головы */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Talk Animation|Transform")
    float LocalScale = 1.0f;

    /** Скорость зацикленного воспроизведения разговора */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Talk Animation|Timing", meta=(ClampMin="0.1"))
    float TalkPlayRate = 1.f;
};

/* ───────────────────────────── Head Root Sprite Layer ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DHeadRootSprite
{
    GENERATED_BODY()

    /** Имя головы */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Head Root")
    FName Name;

    /** Статичный спрайт головы */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root|Sprite")
    TObjectPtr<UPaperSprite> Sprite = nullptr;

    /** Attachment target - только для головы как root элемента */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root|Attachment")
    ECharacter2DAttachmentTarget AttachmentTarget = ECharacter2DAttachmentTarget::None;

    /** Socket name for attachment (if AttachmentTarget is set) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides))
    FName SocketName;

    /** Whether to use socket transform (true) or apply custom offset/scale (false) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides))
    bool bUseSocketTransform = true;

    /** Локальный оффсет головы (влияет на все дочерние элементы) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root|Transform")
    FVector Offset = FVector::ZeroVector;

    /** Локальный Scale головы (влияет на все дочерние элементы) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root|Transform")
    float Scale = 1.0f;

    /** Видимость головы (влияет на все дочерние элементы) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root|Visibility")
    bool bVisible = true;

    FCharacter2DHeadRootSprite()
    {
        Name = TEXT("Head");
    }
};

/* ───────────────────────────── New Head Structure (Hierarchical) ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DHeadStructure
{
    GENERATED_BODY()

    /** Корневой элемент головы - управляет общими настройками */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Root", meta=(DisplayName="Head (Root)"))
    FCharacter2DHeadRootSprite Head;

    /** === Лицевые элементы (наследуют от Head) === */
    
    /** Брови */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Elements", meta=(DisplayName="Eyebrows"))
    FCharacter2DHeadChildSprite Eyebrows;

    /** Глаза */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Elements", meta=(DisplayName="Eyes"))
    FCharacter2DHeadChildSprite Eyes;

    /** Статичные веки */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Elements", meta=(DisplayName="Eyelids (Static)"))
    FCharacter2DHeadChildSprite Eyelids;

    /** Статичный рот */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Elements", meta=(DisplayName="Mouth (Static)"))
    FCharacter2DHeadChildSprite Mouth;

    /** === Анимации === */
    
    /** Настройки моргания (наследует трансформации от Head) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Animations", meta=(DisplayName="Blink Animation"))
    FCharacter2DBlinkSettings BlinkSettings;

    /** Настройки разговора (наследует трансформации от Head) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Head Animations", meta=(DisplayName="Talk Animation"))
    FCharacter2DTalkSettings TalkSettings;

    FCharacter2DHeadStructure()
    {
        Head.Name = TEXT("Head");
        Eyebrows.Name = TEXT("Eyebrows");
        Eyes.Name = TEXT("Eyes");
        Eyelids.Name = TEXT("Eyelids");
        Mouth.Name = TEXT("Mouth");
    }

    /** Helper functions для вычисления финальных трансформаций */
    
    /** Получить финальный offset для дочернего элемента (Head.Offset + LocalOffset) */
    FVector GetFinalChildOffset(const FCharacter2DHeadChildSprite& ChildSprite, const FVector& GlobalSpriteOffset) const
    {
        return GlobalSpriteOffset + Head.Offset + ChildSprite.LocalOffset;
    }

    /** Получить финальный scale для дочернего элемента (Head.Scale * LocalScale) */
    float GetFinalChildScale(const FCharacter2DHeadChildSprite& ChildSprite, float GlobalSpriteScale) const
    {
        return GlobalSpriteScale * Head.Scale * ChildSprite.LocalScale;
    }

    /** Получить финальную видимость для дочернего элемента */
    bool GetFinalChildVisibility(const FCharacter2DHeadChildSprite& ChildSprite) const
    {
        return ChildSprite.GetFinalVisibility(Head.bVisible);
    }

    /** Получить финальный offset для анимации (Head.Offset + Animation.LocalOffset) */
    FVector GetFinalAnimationOffset(const FVector& AnimationLocalOffset, const FVector& GlobalSpriteOffset) const
    {
        return GlobalSpriteOffset + Head.Offset + AnimationLocalOffset;
    }

    /** Получить финальный scale для анимации (Head.Scale * Animation.LocalScale) */
    float GetFinalAnimationScale(float AnimationLocalScale, float GlobalSpriteScale) const
    {
        return GlobalSpriteScale * Head.Scale * AnimationLocalScale;
    }
};

/* ───────────────────────────── Basic Sprite Layer (для Body/Arms) ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DSpriteLayer
{
    GENERATED_BODY()

    /** Имя слоя (автозаполняется из категории) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sprite")
    FName Name;

    /** Статичный спрайт */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
    TObjectPtr<UPaperSprite> Sprite = nullptr;

    /** Attachment target (which skeletal mesh to attach to) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Attachment")
    ECharacter2DAttachmentTarget AttachmentTarget = ECharacter2DAttachmentTarget::None;

    /** Socket name for attachment (if AttachmentTarget is set) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides))
    FName SocketName;

    /** Whether to use socket transform (true) or apply custom offset/scale (false) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides))
    bool bUseSocketTransform = true;

    /** Локальный оффсет (X вправо, Y вверх) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
    FVector Offset = FVector::ZeroVector;

    /** Локальный Scale */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite")
    float Scale = 1.0f;

    /** Видимость */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
    bool bVisible = true;
};

/* ───────────────────────────── Body/Arms Sprite Structures ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DSpriteBodyStructure
{
    GENERATED_BODY()

    /** Имя слоя */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sprite")
    FName Name;

    /** Статичный спрайт */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
    TObjectPtr<UPaperSprite> Sprite = nullptr;

    /** Attachment target */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Attachment")
    ECharacter2DAttachmentTarget AttachmentTarget = ECharacter2DAttachmentTarget::None;

    /** Socket name for attachment */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides))
    FName SocketName;

    /** Whether to use socket transform */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides))
    bool bUseSocketTransform = true;

    /** Локальный оффсет */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
    FVector Offset = FVector::ZeroVector;

    /** Локальный Scale */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite")
    float Scale = 1.0f;

    /** Видимость */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
    bool bVisible = true;

    FCharacter2DSpriteBodyStructure()
    {
        Name = TEXT("Body");
    }
};

USTRUCT(BlueprintType)
struct FCharacter2DSpriteArmsStructure
{
    GENERATED_BODY()

    /** Имя слоя */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sprite")
    FName Name;

    /** Статичный спрайт */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
    TObjectPtr<UPaperSprite> Sprite = nullptr;

    /** Attachment target */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Attachment")
    ECharacter2DAttachmentTarget AttachmentTarget = ECharacter2DAttachmentTarget::None;

    /** Socket name for attachment */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides))
    FName SocketName;

    /** Whether to use socket transform */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides))
    bool bUseSocketTransform = true;

    /** Локальный оффсет */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
    FVector Offset = FVector::ZeroVector;

    /** Локальный Scale */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite")
    float Scale = 1.0f;

    /** Видимость */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
    bool bVisible = true;

    FCharacter2DSpriteArmsStructure()
    {
        Name = TEXT("Arms");
    }
};

/* ───────────────────────────── Sprite Transform Structure ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DSpriteTransformStructure
{
    GENERATED_BODY()

    /** Глобальный оффсет для всех Sprite */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Transform", meta=(DisplayName="Global Offset"))
    FVector GlobalOffset = FVector::ZeroVector;

    /** Глобальный Scale для всех Sprite */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Transform", meta=(DisplayName="Global Scale"))
    float GlobalScale = 1.0f;
};

/* ───────────────────────────── Updated Sprite Structure ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DSpriteStructure
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Body")
    FCharacter2DSpriteBodyStructure Body;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Arms")
    FCharacter2DSpriteArmsStructure Arms;

    /** Новая иерархическая структура головы */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Head")
    FCharacter2DHeadStructure Head;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Transform")
    FCharacter2DSpriteTransformStructure Transform;

    // Legacy properties for backward compatibility - marked as deprecated
    UPROPERTY(meta=(DeprecatedProperty))
    FCharacter2DSpriteLayer LegacyBody;

    UPROPERTY(meta=(DeprecatedProperty))
    FCharacter2DSpriteLayer LegacyArms;

    UPROPERTY(meta=(DeprecatedProperty))
    FCharacter2DSpriteLayer LegacyHead;

    UPROPERTY(meta=(DeprecatedProperty))
    FCharacter2DSpriteLayer LegacyEyebrow;

    UPROPERTY(meta=(DeprecatedProperty))
    FCharacter2DSpriteLayer LegacyEyes;

    UPROPERTY(meta=(DeprecatedProperty))
    FCharacter2DSpriteLayer LegacyEyelids;

    UPROPERTY(meta=(DeprecatedProperty))
    FCharacter2DBlinkSettings LegacyEyelidsBlinkSettings;

    UPROPERTY(meta=(DeprecatedProperty))
    FCharacter2DSpriteLayer LegacyMouth;

    UPROPERTY(meta=(DeprecatedProperty))
    FCharacter2DTalkSettings LegacyMouthTalkSettings;

    UPROPERTY(meta=(DeprecatedProperty))
    FVector LegacyGlobalOffset = FVector::ZeroVector;

    UPROPERTY(meta=(DeprecatedProperty))
    float LegacyGlobalScale = 1.0f;

    FCharacter2DSpriteStructure()
    {
        // Initialize new structure
        Body = FCharacter2DSpriteBodyStructure();
        Arms = FCharacter2DSpriteArmsStructure();
        Head = FCharacter2DHeadStructure();
        Transform = FCharacter2DSpriteTransformStructure();
    }

    // Migration helper - call this in PostLoad to migrate old data
    void MigrateFromLegacyStructure()
    {
        // Migrate body
        if (LegacyBody.Sprite != nullptr || !LegacyBody.Offset.IsZero() || LegacyBody.Scale != 1.0f)
        {
            Body.Sprite = LegacyBody.Sprite;
            Body.AttachmentTarget = LegacyBody.AttachmentTarget;
            Body.SocketName = LegacyBody.SocketName;
            Body.bUseSocketTransform = LegacyBody.bUseSocketTransform;
            Body.Offset = LegacyBody.Offset;
            Body.Scale = LegacyBody.Scale;
            Body.bVisible = LegacyBody.bVisible;
        }

        // Migrate arms
        if (LegacyArms.Sprite != nullptr || !LegacyArms.Offset.IsZero() || LegacyArms.Scale != 1.0f)
        {
            Arms.Sprite = LegacyArms.Sprite;
            Arms.AttachmentTarget = LegacyArms.AttachmentTarget;
            Arms.SocketName = LegacyArms.SocketName;
            Arms.bUseSocketTransform = LegacyArms.bUseSocketTransform;
            Arms.Offset = LegacyArms.Offset;
            Arms.Scale = LegacyArms.Scale;
            Arms.bVisible = LegacyArms.bVisible;
        }

        // Migrate head structure to new hierarchical format
        if (LegacyHead.Sprite != nullptr || !LegacyHead.Offset.IsZero() || LegacyHead.Scale != 1.0f)
        {
            // Migrate head root
            Head.Head.Sprite = LegacyHead.Sprite;
            Head.Head.AttachmentTarget = LegacyHead.AttachmentTarget;
            Head.Head.SocketName = LegacyHead.SocketName;
            Head.Head.bUseSocketTransform = LegacyHead.bUseSocketTransform;
            Head.Head.Offset = LegacyHead.Offset;
            Head.Head.Scale = LegacyHead.Scale;
            Head.Head.bVisible = LegacyHead.bVisible;
        }

        // Migrate facial elements as children
        if (LegacyEyebrow.Sprite != nullptr || !LegacyEyebrow.Offset.IsZero() || LegacyEyebrow.Scale != 1.0f)
        {
            Head.Eyebrows.Sprite = LegacyEyebrow.Sprite;
            Head.Eyebrows.LocalOffset = LegacyEyebrow.Offset;
            Head.Eyebrows.LocalScale = LegacyEyebrow.Scale;
            Head.Eyebrows.bVisible = LegacyEyebrow.bVisible;
        }

        if (LegacyEyes.Sprite != nullptr || !LegacyEyes.Offset.IsZero() || LegacyEyes.Scale != 1.0f)
        {
            Head.Eyes.Sprite = LegacyEyes.Sprite;
            Head.Eyes.LocalOffset = LegacyEyes.Offset;
            Head.Eyes.LocalScale = LegacyEyes.Scale;
            Head.Eyes.bVisible = LegacyEyes.bVisible;
        }

        if (LegacyEyelids.Sprite != nullptr || !LegacyEyelids.Offset.IsZero() || LegacyEyelids.Scale != 1.0f)
        {
            Head.Eyelids.Sprite = LegacyEyelids.Sprite;
            Head.Eyelids.LocalOffset = LegacyEyelids.Offset;
            Head.Eyelids.LocalScale = LegacyEyelids.Scale;
            Head.Eyelids.bVisible = LegacyEyelids.bVisible;
        }

        if (LegacyMouth.Sprite != nullptr || !LegacyMouth.Offset.IsZero() || LegacyMouth.Scale != 1.0f)
        {
            Head.Mouth.Sprite = LegacyMouth.Sprite;
            Head.Mouth.LocalOffset = LegacyMouth.Offset;
            Head.Mouth.LocalScale = LegacyMouth.Scale;
            Head.Mouth.bVisible = LegacyMouth.bVisible;
        }

        // Migrate animation settings
        if (LegacyEyelidsBlinkSettings.BlinkFlipbook != nullptr)
        {
            Head.BlinkSettings = LegacyEyelidsBlinkSettings;
        }
        if (LegacyMouthTalkSettings.TalkFlipbook != nullptr)
        {
            Head.TalkSettings = LegacyMouthTalkSettings;
        }

        // Migrate transform
        if (!LegacyGlobalOffset.IsZero() || LegacyGlobalScale != 1.0f)
        {
            Transform.GlobalOffset = LegacyGlobalOffset;
            Transform.GlobalScale = LegacyGlobalScale;
        }
    }
};

/* ───────────────────────────── Skeletal Material Entry ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DSkeletalMaterial
{
    GENERATED_BODY()

    /** Материал */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skeletal")
    TObjectPtr<UMaterialInterface> Material = nullptr;

    /** Индекс слота (0,1,2…) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skeletal")
    int32 SlotIndex = 0;
};

/* ───────────────────────────── Skeletal Part ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DSkeletalPart
{
    GENERATED_BODY()

    /** SkeletalMesh */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal")
    TObjectPtr<USkeletalMesh> Mesh = nullptr;

    /** Список материалов + индекс */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal")
    TArray<FCharacter2DSkeletalMaterial> Materials;

    /** AnimBlueprint */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal")
    TSubclassOf<UAnimInstance> AnimInstance;

    /** Локальный оффсет */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal")
    FVector Offset = FVector::ZeroVector;

    /** Локальный Scale */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal")
    float Scale = 1.0f;
};

/* ───────────────────────────── Visual Novel Effects Settings ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DVisualNovelSettings
{
    GENERATED_BODY()

    /** Default movement settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual Novel|Movement")
    FCharacter2DMovementSettings DefaultMovementSettings;

    /** Default emotion effect settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual Novel|Emotions")
    FCharacter2DEmotionSettings DefaultEmotionSettings;

    /** Default appearance settings (fade, scale, etc) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual Novel|Appearance")
    float DefaultFadeDuration = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual Novel|Appearance")
    TObjectPtr<UCurveFloat> DefaultFadeCurve = nullptr;
};

/* ───────────────────────────── DataAsset ───────────────────────────── */
UCLASS(BlueprintType)
class CHARACTER2DRUNTIME_API UCharacter2DAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    /* ─── Skeletal Parts ───────────────────────────────────────── */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal", meta=(DisplayName="Body"))
    FCharacter2DSkeletalPart Body;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal", meta=(DisplayName="Arms"))
    FCharacter2DSkeletalPart Arms;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal", meta=(DisplayName="Head"))
    FCharacter2DSkeletalPart Head;

    /** Глобальный оффсет для всех Skeletal */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal", meta=(DisplayName="Global Offset"))
    FVector SkeletalGlobalOffset = FVector::ZeroVector;

    /** Глобальный Scale для всех Skeletal */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skeletal", meta=(DisplayName="Global Scale"))
    float GlobalScale = 1.0f;

    /* ─── Sprite ─────────────────────────────────────────────────── */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite")
    FCharacter2DSpriteStructure SpriteStructure;

    /* ─── Visual Novel Effects ────────────────────────────────────── */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual Novel")
    FCharacter2DVisualNovelSettings VisualNovelSettings;

    /** Автоматически моргать при запуске */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="General")
    bool bAutoBlink = false;

    /** Автоматически говорить при запуске */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="General")
    bool bAutoTalk = false;

    /** Enable both sprite and skeletal mesh rendering simultaneously */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="General")
    bool bEnableDualRendering = false;

    // Helper functions to access sprite data with new hierarchical structure
    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites")
    const FCharacter2DSpriteBodyStructure& GetBodySprite() const
    {
        return SpriteStructure.Body;
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites")
    const FCharacter2DSpriteArmsStructure& GetArmsSprite() const
    {
        return SpriteStructure.Arms;
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites")
    const FCharacter2DHeadRootSprite& GetHeadSprite() const
    {
        return SpriteStructure.Head.Head;
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites")
    const FCharacter2DHeadChildSprite& GetEyebrowSprite() const
    {
        return SpriteStructure.Head.Eyebrows;
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites")
    const FCharacter2DHeadChildSprite& GetEyesSprite() const
    {
        return SpriteStructure.Head.Eyes;
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites")
    const FCharacter2DHeadChildSprite& GetEyelidsSprite() const
    {
        return SpriteStructure.Head.Eyelids;
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites")
    const FCharacter2DHeadChildSprite& GetMouthSprite() const
    {
        return SpriteStructure.Head.Mouth;
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites")
    const FCharacter2DBlinkSettings& GetBlinkSettings() const
    {
        return SpriteStructure.Head.BlinkSettings;
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites")
    const FCharacter2DTalkSettings& GetTalkSettings() const
    {
        return SpriteStructure.Head.TalkSettings;
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites")
    FVector GetGlobalSpriteOffset() const
    {
        return SpriteStructure.Transform.GlobalOffset;
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites")
    float GetGlobalSpriteScale() const
    {
        return SpriteStructure.Transform.GlobalScale;
    }

    /** New helper functions for hierarchical head structure */
    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites|Head")
    FVector GetFinalEyebrowOffset() const
    {
        return SpriteStructure.Head.GetFinalChildOffset(SpriteStructure.Head.Eyebrows, GetGlobalSpriteOffset());
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites|Head")
    FVector GetFinalEyesOffset() const
    {
        return SpriteStructure.Head.GetFinalChildOffset(SpriteStructure.Head.Eyes, GetGlobalSpriteOffset());
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites|Head")
    FVector GetFinalEyelidsOffset() const
    {
        return SpriteStructure.Head.GetFinalChildOffset(SpriteStructure.Head.Eyelids, GetGlobalSpriteOffset());
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites|Head")
    FVector GetFinalMouthOffset() const
    {
        return SpriteStructure.Head.GetFinalChildOffset(SpriteStructure.Head.Mouth, GetGlobalSpriteOffset());
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites|Head")
    float GetFinalEyebrowScale() const
    {
        return SpriteStructure.Head.GetFinalChildScale(SpriteStructure.Head.Eyebrows, GetGlobalSpriteScale());
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites|Head")
    float GetFinalEyesScale() const
    {
        return SpriteStructure.Head.GetFinalChildScale(SpriteStructure.Head.Eyes, GetGlobalSpriteScale());
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites|Head")
    float GetFinalEyelidsScale() const
    {
        return SpriteStructure.Head.GetFinalChildScale(SpriteStructure.Head.Eyelids, GetGlobalSpriteScale());
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites|Head")
    float GetFinalMouthScale() const
    {
        return SpriteStructure.Head.GetFinalChildScale(SpriteStructure.Head.Mouth, GetGlobalSpriteScale());
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites|Head")
    bool GetFinalEyebrowVisibility() const
    {
        return SpriteStructure.Head.GetFinalChildVisibility(SpriteStructure.Head.Eyebrows);
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites|Head")
    bool GetFinalEyesVisibility() const
    {
        return SpriteStructure.Head.GetFinalChildVisibility(SpriteStructure.Head.Eyes);
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites|Head")
    bool GetFinalEyelidsVisibility() const
    {
        return SpriteStructure.Head.GetFinalChildVisibility(SpriteStructure.Head.Eyelids);
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites|Head")
    bool GetFinalMouthVisibility() const
    {
        return SpriteStructure.Head.GetFinalChildVisibility(SpriteStructure.Head.Mouth);
    }

    /** Animation offset/scale helpers */
    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites|Head")
    FVector GetFinalBlinkOffset() const
    {
        return SpriteStructure.Head.GetFinalAnimationOffset(SpriteStructure.Head.BlinkSettings.LocalOffset, GetGlobalSpriteOffset());
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites|Head")
    float GetFinalBlinkScale() const
    {
        return SpriteStructure.Head.GetFinalAnimationScale(SpriteStructure.Head.BlinkSettings.LocalScale, GetGlobalSpriteScale());
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites|Head")
    FVector GetFinalTalkOffset() const
    {
        return SpriteStructure.Head.GetFinalAnimationOffset(SpriteStructure.Head.TalkSettings.LocalOffset, GetGlobalSpriteOffset());
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites|Head")
    float GetFinalTalkScale() const
    {
        return SpriteStructure.Head.GetFinalAnimationScale(SpriteStructure.Head.TalkSettings.LocalScale, GetGlobalSpriteScale());
    }

#if WITH_EDITOR
    // Editor-only hooks
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
    
    /** Validate head hierarchy settings */
    void ValidateHeadHierarchy();
#endif

    /** Runtime validation methods */
    UFUNCTION(BlueprintCallable, Category = "Character2D|Validation")
    bool IsValidForRuntime() const;
    
    UFUNCTION(BlueprintCallable, Category = "Character2D|Validation")
    bool HasValidSpriteConfiguration() const;
    
    UFUNCTION(BlueprintCallable, Category = "Character2D|Validation")
    bool HasValidSkeletalConfiguration() const;

protected:
    virtual void PostLoad() override;
    
private:
    /** Migrate data from legacy versions */
    void MigrateLegacyData();
};