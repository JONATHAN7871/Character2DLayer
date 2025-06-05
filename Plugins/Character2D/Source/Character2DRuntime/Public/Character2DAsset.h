#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PaperSprite.h"
#include "PaperFlipbook.h"
#include "AssetRegistry/FAssetRegistryTagsContext.h"
#include "Curves/CurveFloat.h"
#include "Character2DAsset.generated.h"

/* ───────────────────────────── Visual Novel Effects Enums ───────────────────────────── */
UENUM(BlueprintType)
enum class ECharacter2DTransitionType : uint8
{
    None        UMETA(DisplayName = "None"),
    FadeIn      UMETA(DisplayName = "Fade In"),
    FadeOut     UMETA(DisplayName = "Fade Out"),
    SlideInLeft UMETA(DisplayName = "Slide In From Left"),
    SlideInRight UMETA(DisplayName = "Slide In From Right"),
    SlideOutLeft UMETA(DisplayName = "Slide Out To Left"),
    SlideOutRight UMETA(DisplayName = "Slide Out To Right"),
    ScaleIn     UMETA(DisplayName = "Scale In"),
    ScaleOut    UMETA(DisplayName = "Scale Out")
};

UENUM(BlueprintType)
enum class ECharacter2DEmotionEffect : uint8
{
    None        UMETA(DisplayName = "None"),
    Shake       UMETA(DisplayName = "Shake"),
    Pulse       UMETA(DisplayName = "Pulse"),
    ColorShift  UMETA(DisplayName = "Color Shift"),
    Bounce      UMETA(DisplayName = "Bounce"),
    Flash       UMETA(DisplayName = "Flash"),
    Darken      UMETA(DisplayName = "Darken"),
    Brighten    UMETA(DisplayName = "Brighten")
};

/* ───────────────────────────── Effect Settings ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DTransitionSettings
{
    GENERATED_BODY()

    /** Duration of the transition in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition", meta=(ClampMin="0.1"))
    float Duration = 1.0f;

    /** Animation curve for the transition */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition")
    TObjectPtr<UCurveFloat> AnimationCurve = nullptr;

    /** Delay before starting the transition */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition", meta=(ClampMin="0.0"))
    float StartDelay = 0.0f;

    /** For slide transitions - distance to move */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition")
    float SlideDistance = 500.0f;
};

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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Blink")
    TObjectPtr<UPaperFlipbook> BlinkFlipbook = nullptr;

    /** Сдвиг от корня (X вправо, Y вверх) для Flipbook */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Blink")
    FVector Offset = FVector::ZeroVector;

    /** Локальный Scale */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character2D|Blink")
    float Scale = 1.0f;

    /** Мин/Макс интервал до моргания (сек) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Blink", meta=(ClampMin="0.1"))
    float BlinkIntervalMin = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Blink", meta=(ClampMin="0.1"))
    float BlinkIntervalMax = 5.f;

    /** Мин/Макс скорость воспроизведения */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Blink", meta=(ClampMin="0.1"))
    float BlinkPlayRateMin = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Blink", meta=(ClampMin="0.1"))
    float BlinkPlayRateMax = 2.f;
};

/* ───────────────────────────── Talk Settings ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DTalkSettings
{
    GENERATED_BODY()

    /** Flipbook с кадрами говорения (движение губ) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Talk")
    TObjectPtr<UPaperFlipbook> TalkFlipbook = nullptr;

    /** Сдвиг от корня (X вправо, Y вверх) для Flipbook */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Talk")
    FVector Offset = FVector::ZeroVector;

    /** Локальный Scale */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character2D|Talk")
    float Scale = 1.0f;

    /** Скорость зацикленного воспроизведения разговора */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Talk", meta=(ClampMin="0.1"))
    float TalkPlayRate = 1.f;
};

/* ───────────────────────────── Sprite Layer ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DSpriteLayer
{
    GENERATED_BODY()

    /** Имя слоя (Body, Arms, Head, и т.д.) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Sprite")
    FName Name;

    /** Статичный спрайт */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Sprite")
    TObjectPtr<UPaperSprite> Sprite = nullptr;

    /** Локальный оффсет (X вправо, Y вверх) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Sprite")
    FVector Offset = FVector::ZeroVector;

    /** Локальный Scale */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character2D|Sprite")
    float Scale = 1.0f;

    /** Видимость */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Sprite")
    bool bVisible = true;
};

/* ───────────────────────────── Sprite Structure ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DSpriteStructure
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Sprite") FCharacter2DSpriteLayer Body;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Sprite") FCharacter2DSpriteLayer Arms;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Sprite") FCharacter2DSpriteLayer Head;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Sprite") FCharacter2DSpriteLayer Eyebrow;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Sprite") FCharacter2DSpriteLayer Eyes;

    /** Статичный слой век */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Sprite") FCharacter2DSpriteLayer Eyelids;
    /** Настройки случайного моргания */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Sprite|Blink") FCharacter2DBlinkSettings EyelidsBlinkSettings;

    /** Статичный слой рта */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Sprite") FCharacter2DSpriteLayer Mouth;
    /** Настройки разговора */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Sprite|Talk")  FCharacter2DTalkSettings MouthTalkSettings;

    /** Глобальный оффсет для всех Sprite */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character2D|Sprite", meta=(DisplayName="Global Offset"))
    FVector GlobalOffset = FVector::ZeroVector;

    /** Глобальный Scale для всех Sprite */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character2D|Sprite", meta=(DisplayName="Global Scale"))
    float GlobalScale = 1.0f;
};

/* ───────────────────────────── Skeletal Material Entry ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DSkeletalMaterial
{
    GENERATED_BODY()

    /** Материал */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Skeletal")
    TObjectPtr<UMaterialInterface> Material = nullptr;

    /** Индекс слота (0,1,2…) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|Skeletal")
    int32 SlotIndex = 0;
};

/* ───────────────────────────── Skeletal Part ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DSkeletalPart
{
    GENERATED_BODY()

    /** SkeletalMesh */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character2D|Skeletal")
    TObjectPtr<USkeletalMesh> Mesh = nullptr;

    /** Список материалов + индекс */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character2D|Skeletal")
    TArray<FCharacter2DSkeletalMaterial> Materials;

    /** AnimBlueprint */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character2D|Skeletal")
    TSubclassOf<UAnimInstance> AnimInstance;

    /** Локальный оффсет */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character2D|Skeletal")
    FVector Offset = FVector::ZeroVector;

    /** Локальный Scale */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character2D|Skeletal")
    float Scale = 1.0f;
};

/* ───────────────────────────── Visual Novel Effects Settings ───────────────────────────── */
USTRUCT(BlueprintType)
struct FCharacter2DVisualNovelSettings
{
    GENERATED_BODY()

    /** Default transition settings for appearance */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual Novel|Transitions")
    FCharacter2DTransitionSettings DefaultAppearanceTransition;

    /** Default transition settings for disappearance */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual Novel|Transitions")
    FCharacter2DTransitionSettings DefaultDisappearanceTransition;

    /** Default emotion effect settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual Novel|Emotions")
    FCharacter2DEmotionSettings DefaultEmotionSettings;

    /** Whether to automatically play entrance transition when spawned */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual Novel|Auto")
    bool bAutoPlayEntranceTransition = false;

    /** Default entrance transition type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual Novel|Auto")
    ECharacter2DTransitionType DefaultEntranceTransition = ECharacter2DTransitionType::FadeIn;
};

/* ───────────────────────────── DataAsset ───────────────────────────── */
UCLASS(BlueprintType)
class CHARACTER2DRUNTIME_API UCharacter2DAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    /* ─── Skeletal Parts ───────────────────────────────────────── */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character2D|Skeletal")
    FCharacter2DSkeletalPart Body;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character2D|Skeletal")
    FCharacter2DSkeletalPart Arms;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character2D|Skeletal")
    FCharacter2DSkeletalPart Head;

    /** Глобальный оффсет для всех Skeletal */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character2D|Skeletal")
    FVector SkeletalGlobalOffset = FVector::ZeroVector;

    /** Локальный Scale для всех Skeletal */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character2D|Skeletal")
    float GlobalScale = 1.0f;

    /* ─── Sprite ─────────────────────────────────────────────────── */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character2D|Sprite")
    FCharacter2DSpriteStructure SpriteStructure;

    /* ─── Visual Novel Effects ────────────────────────────────────── */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character2D|Visual Novel")
    FCharacter2DVisualNovelSettings VisualNovelSettings;

    /** Автоматически моргать при запуске */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|General")
    bool bAutoBlink = false;

    /** Автоматически говорить при запуске */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|General")
    bool bAutoTalk  = false;

    /** Enable both sprite and skeletal mesh rendering simultaneously */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character2D|General")
    bool bEnableDualRendering = false;

#if WITH_EDITOR
    // Editor-only methods for validation and migration
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void GetAssetRegistryTags(FAssetRegistryTagsContext& Context) const override;
    virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
    
    /** Get a human-readable description of the current rendering mode */
    UFUNCTION(CallInEditor, Category = "Character2D|Debug")
    FString GetRenderingModeDescription() const;
    
    /** Get a list of configuration warnings */
    UFUNCTION(CallInEditor, Category = "Character2D|Debug")
    TArray<FString> GetConfigurationWarnings() const;
    
    /** Validate animation settings and log warnings */
    void ValidateAnimationSettings();
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
    /** Migrate data from legacy versions that used bUseSpriteInsteadOfMesh */
    void MigrateLegacyData();
};
