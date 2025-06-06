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

    /** Скорость зацикленного воспроизведения разговора */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Talk", meta=(ClampMin="0.1"))
    float TalkPlayRate = 1.f;
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

/* ───────────────────────────── Sprite Structure ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DSpriteStructure
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Layers", meta=(DisplayName="Body"))
    FCharacter2DSpriteLayer Body;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Layers", meta=(DisplayName="Arms"))
    FCharacter2DSpriteLayer Arms;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Layers", meta=(DisplayName="Head"))
    FCharacter2DSpriteLayer Head;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Layers", meta=(DisplayName="Eyebrow"))
    FCharacter2DSpriteLayer Eyebrow;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Layers", meta=(DisplayName="Eyes"))
    FCharacter2DSpriteLayer Eyes;

    /** Статичный слой век */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Layers", meta=(DisplayName="Eyelids"))
    FCharacter2DSpriteLayer Eyelids;

    /** Настройки случайного моргания */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Animation")
    FCharacter2DBlinkSettings EyelidsBlinkSettings;

    /** Статичный слой рта */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Layers", meta=(DisplayName="Mouth"))
    FCharacter2DSpriteLayer Mouth;

    /** Настройки разговора */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite Animation")
    FCharacter2DTalkSettings MouthTalkSettings;

    /** Глобальный оффсет для всех Sprite */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite Transform", meta=(DisplayName="Global Offset"))
    FVector GlobalOffset = FVector::ZeroVector;

    /** Глобальный Scale для всех Sprite */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite Transform", meta=(DisplayName="Global Scale"))
    float GlobalScale = 1.0f;

    FCharacter2DSpriteStructure()
    {
        // Auto-fill names
        Body.Name = TEXT("Body");
        Arms.Name = TEXT("Arms");
        Head.Name = TEXT("Head");
        Eyebrow.Name = TEXT("Eyebrow");
        Eyes.Name = TEXT("Eyes");
        Eyelids.Name = TEXT("Eyelids");
        Mouth.Name = TEXT("Mouth");
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