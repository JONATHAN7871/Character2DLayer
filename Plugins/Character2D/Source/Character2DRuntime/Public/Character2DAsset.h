#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PaperSprite.h"
#include "PaperFlipbook.h"
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

/* ───────────────────────────── Blink Settings ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DBlinkSettings
{
    GENERATED_BODY()

    /** Flipbook с кадрами моргания (открыто → полу-закрыто → закрыто) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink")
    TObjectPtr<UPaperFlipbook> BlinkFlipbook = nullptr;

    /** Сдвиг от корня (X вправо, Y вверх) для Flipbook */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink")
    FVector Offset = FVector::ZeroVector;

    /** Локальный Scale */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blink")
    float Scale = 1.0f;

    /** Target skeletal mesh for attachment */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink|Attachment")
    ECharacter2DAttachmentTarget AttachmentTarget = ECharacter2DAttachmentTarget::None;

    /** Socket name on target skeletal mesh */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides))
    FName SocketName;

    /** Use socket transform or apply custom offset/scale */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides))
    bool bUseSocketTransform = true;

    /** Мин/Макс интервал до моргания (сек) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink", meta=(ClampMin="0.1"))
    float BlinkIntervalMin = 2.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink", meta=(ClampMin="0.1"))
    float BlinkIntervalMax = 5.f;

    /** Мин/Макс скорость воспроизведения */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink", meta=(ClampMin="0.1"))
    float BlinkPlayRateMin = 1.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink", meta=(ClampMin="0.1"))
    float BlinkPlayRateMax = 2.f;
};

/* ───────────────────────────── Talk Settings ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DTalkSettings
{
    GENERATED_BODY()

    /** Flipbook с кадрами говорения (движение губ) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Talk")
    TObjectPtr<UPaperFlipbook> TalkFlipbook = nullptr;

    /** Сдвиг от корня (X вправо, Y вверх) для Flipbook */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Talk")
    FVector Offset = FVector::ZeroVector;

    /** Локальный Scale */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Talk")
    float Scale = 1.0f;

    /** Target skeletal mesh for attachment */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Talk|Attachment")
    ECharacter2DAttachmentTarget AttachmentTarget = ECharacter2DAttachmentTarget::None;

    /** Socket name on target skeletal mesh */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Talk|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides))
    FName SocketName;

    /** Use socket transform or apply custom offset/scale */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Talk|Attachment", meta=(EditCondition="AttachmentTarget != ECharacter2DAttachmentTarget::None", EditConditionHides))
    bool bUseSocketTransform = true;

    /** Скорость зацикленного воспроизведения разговора */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Talk", meta=(ClampMin="0.1"))
    float TalkPlayRate = 1.f;
};

/* ───────────────────────────── Sprite Layer ───────────────────────────── */
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

/* ───────────────────────────── Sprite Body Structure ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DSpriteBodyStructure
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

    FCharacter2DSpriteBodyStructure()
    {
        Name = TEXT("Body");
    }
};

/* ───────────────────────────── Sprite Arms Structure ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DSpriteArmsStructure
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

    FCharacter2DSpriteArmsStructure()
    {
        Name = TEXT("Arms");
    }
};

/* ───────────────────────────── Sprite Head Structure ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DSpriteHeadStructure
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite", meta=(DisplayName="Head"))
    FCharacter2DSpriteLayer Head;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite", meta=(DisplayName="Eyebrow"))
    FCharacter2DSpriteLayer Eyebrow;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite", meta=(DisplayName="Eyes"))
    FCharacter2DSpriteLayer Eyes;

    /** Статичный слой век */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite", meta=(DisplayName="Eyelids"))
    FCharacter2DSpriteLayer Eyelids;

    /** Настройки случайного моргания */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blink", meta=(DisplayName="Eyelids Blink Settings"))
    FCharacter2DBlinkSettings EyelidsBlinkSettings;

    /** Статичный слой рта */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite", meta=(DisplayName="Mouth"))
    FCharacter2DSpriteLayer Mouth;

    /** Настройки разговора */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Talk", meta=(DisplayName="Mouth Talk Settings"))
    FCharacter2DTalkSettings MouthTalkSettings;

    FCharacter2DSpriteHeadStructure()
    {
        Head.Name = TEXT("Head");
        Eyebrow.Name = TEXT("Eyebrow");
        Eyes.Name = TEXT("Eyes");
        Eyelids.Name = TEXT("Eyelids");
        Mouth.Name = TEXT("Mouth");
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

/* ───────────────────────────── Sprite Structure ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DSpriteStructure
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Body")
    FCharacter2DSpriteBodyStructure Body;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Arms")
    FCharacter2DSpriteArmsStructure Arms;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Head")
    FCharacter2DSpriteHeadStructure Head;

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
        Head = FCharacter2DSpriteHeadStructure();
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

        // Migrate head structure
        if (LegacyHead.Sprite != nullptr || !LegacyHead.Offset.IsZero() || LegacyHead.Scale != 1.0f)
        {
            Head.Head = LegacyHead;
        }
        if (LegacyEyebrow.Sprite != nullptr || !LegacyEyebrow.Offset.IsZero() || LegacyEyebrow.Scale != 1.0f)
        {
            Head.Eyebrow = LegacyEyebrow;
        }
        if (LegacyEyes.Sprite != nullptr || !LegacyEyes.Offset.IsZero() || LegacyEyes.Scale != 1.0f)
        {
            Head.Eyes = LegacyEyes;
        }
        if (LegacyEyelids.Sprite != nullptr || !LegacyEyelids.Offset.IsZero() || LegacyEyelids.Scale != 1.0f)
        {
            Head.Eyelids = LegacyEyelids;
        }
        if (LegacyMouth.Sprite != nullptr || !LegacyMouth.Offset.IsZero() || LegacyMouth.Scale != 1.0f)
        {
            Head.Mouth = LegacyMouth;
        }

        // Migrate animation settings
        if (LegacyEyelidsBlinkSettings.BlinkFlipbook != nullptr)
        {
            Head.EyelidsBlinkSettings = LegacyEyelidsBlinkSettings;
        }
        if (LegacyMouthTalkSettings.TalkFlipbook != nullptr)
        {
            Head.MouthTalkSettings = LegacyMouthTalkSettings;
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

    // Helper functions to access sprite data with backward compatibility
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
    const FCharacter2DSpriteLayer& GetHeadSprite() const
    {
        return SpriteStructure.Head.Head;
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites")
    const FCharacter2DSpriteLayer& GetEyebrowSprite() const
    {
        return SpriteStructure.Head.Eyebrow;
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites")
    const FCharacter2DSpriteLayer& GetEyesSprite() const
    {
        return SpriteStructure.Head.Eyes;
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites")
    const FCharacter2DSpriteLayer& GetEyelidsSprite() const
    {
        return SpriteStructure.Head.Eyelids;
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites")
    const FCharacter2DSpriteLayer& GetMouthSprite() const
    {
        return SpriteStructure.Head.Mouth;
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites")
    const FCharacter2DBlinkSettings& GetBlinkSettings() const
    {
        return SpriteStructure.Head.EyelidsBlinkSettings;
    }

    UFUNCTION(BlueprintCallable, Category = "Character2D|Sprites")
    const FCharacter2DTalkSettings& GetTalkSettings() const
    {
        return SpriteStructure.Head.MouthTalkSettings;
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

#if WITH_EDITOR
    // Editor-only hooks
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
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