#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PaperSpriteComponent.h"
#include "TimerManager.h"
#include "Components/TimelineComponent.h"
#include "Character2DAsset.h"
#include "Character2DActor.generated.h"

// Перечисление для отслеживания состояния перехода
UENUM(BlueprintType)
enum class ECharacter2DTransitionState : uint8
{
    None,
    Appearing,
    Disappearing
};

// Перечисление для типов дефолтных кривых
UENUM(BlueprintType)
enum class ECharacter2DTransitionCurve : uint8
{
    Linear      UMETA(DisplayName = "Linear"),
    Smooth      UMETA(DisplayName = "Smooth (Ease In-Out)"),
    EaseIn      UMETA(DisplayName = "Ease In"),
    EaseOut     UMETA(DisplayName = "Ease Out")
};

// Существующие делегаты
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacter2DBlinkStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacter2DBlinkFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacter2DTalkStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacter2DTalkStopped);

/** Вызывается на каждом кадре анимации появления/исчезновения */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacter2DTransitionUpdate, ECharacter2DTransitionState, State, float, Progress);

/** Вызывается по завершении анимации появления/исчезновения */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacter2DTransitionFinished, ECharacter2DTransitionState, State);

UCLASS(BlueprintType, Blueprintable)
class CHARACTER2DRUNTIME_API ACharacter2DActor : public AActor
{
	GENERATED_BODY()

public:
	ACharacter2DActor(const FObjectInitializer& ObjectInitializer);

    // Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<USkeletalMeshComponent> BodyComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<USkeletalMeshComponent> ArmsComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<USkeletalMeshComponent> HeadComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Sprites") TObjectPtr<UPaperSpriteComponent> SpriteBody;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Sprites") TObjectPtr<UPaperSpriteComponent> SpriteArms;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Sprites") TObjectPtr<UPaperSpriteComponent> SpriteHead;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Sprites") TObjectPtr<UPaperSpriteComponent> SpriteEyebrow;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Sprites") TObjectPtr<UPaperSpriteComponent> SpriteEyes;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Sprites") TObjectPtr<UPaperSpriteComponent> SpriteEyelids;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Sprites") TObjectPtr<UPaperSpriteComponent> SpriteMouth;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Effects") TObjectPtr<UPaperSpriteComponent> SpriteEffect1;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Effects") TObjectPtr<UPaperSpriteComponent> SpriteEffect2;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Effects") TObjectPtr<UPaperSpriteComponent> SpriteEffect3;

    /** Таймлайн для управления анимациями появления и исчезновения */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UTimelineComponent> TransitionTimeline;

    // НОВЫЕ КОМПОНЕНТЫ TIMELINE
    /** Таймлайн для управления анимацией моргания */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UTimelineComponent> BlinkTimeline;

    /** Таймлайн для управления анимацией разговора */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UTimelineComponent> TalkTimeline;
    
    // Data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character") TObjectPtr<UCharacter2DAsset> CharacterAsset;

    // State
	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime") bool bSpritesVisible = true;
	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime") bool bSkeletalVisible = true;
	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime") bool bBlinkingActive = false;
	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime") bool bTalkingActive = false;
	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime") bool bIsInTransition = false;

    // Events
	UPROPERTY(BlueprintAssignable, Category="Character|Events") FOnCharacter2DBlinkStarted OnBlinkStarted;
	UPROPERTY(BlueprintAssignable, Category="Character|Events") FOnCharacter2DBlinkFinished OnBlinkFinished;
	UPROPERTY(BlueprintAssignable, Category="Character|Events") FOnCharacter2DTalkStarted OnTalkStarted;
	UPROPERTY(BlueprintAssignable, Category="Character|Events") FOnCharacter2DTalkStopped OnTalkStopped;

    /** Событие, которое транслируется во время анимации появления/исчезновения. Прогресс изменяется от 0.0 до 1.0. */
    UPROPERTY(BlueprintAssignable, Category="Character|Events|Transition")
    FOnCharacter2DTransitionUpdate OnTransitionUpdate;

    /** Событие, которое транслируется по завершении анимации появления/исчезновения. */
    UPROPERTY(BlueprintAssignable, Category="Character|Events|Transition")
    FOnCharacter2DTransitionFinished OnTransitionFinished;

    // API
	UFUNCTION(BlueprintCallable, Category="Character|Visibility") void SetSpritesVisible(bool bVisible);
	UFUNCTION(BlueprintCallable, Category="Character|Visibility") void SetSkeletalVisible(bool bVisible);
	UFUNCTION(BlueprintCallable, Category="Character|Visibility") void SetBothVisible(bool bSprites, bool bSkeletal);
	UFUNCTION(BlueprintCallable, Category="Character|Sprites") void SetEyebrowSprite(UPaperSprite* NewSprite);
	UFUNCTION(BlueprintCallable, Category="Character|Sprites") void SetEyesSprite(UPaperSprite* NewSprite);
	UFUNCTION(BlueprintCallable, Category="Character|Sprites") void SetEyelidsSprite(UPaperSprite* NewSprite);
	UFUNCTION(BlueprintCallable, Category="Character|Sprites") void SetMouthSprite(UPaperSprite* NewSprite);
	UFUNCTION(BlueprintCallable, Category="Character|Sprites") void SetHeadSprite(UPaperSprite* NewSprite);
	UFUNCTION(BlueprintCallable, Category="Character|Sprites") void SetBodySprite(UPaperSprite* NewSprite);
	UFUNCTION(BlueprintCallable, Category="Character|Sprites") void SetArmsSprite(UPaperSprite* NewSprite);
	UFUNCTION(BlueprintCallable, Category="Character|Effects") void SetEffectLayer1(UPaperSprite* Sprite, bool bVisible = true, const FLinearColor& Color = FLinearColor::White, float Opacity = 1.0f);
	UFUNCTION(BlueprintCallable, Category="Character|Effects") void SetEffectLayer2(UPaperSprite* Sprite, bool bVisible = true, const FLinearColor& Color = FLinearColor::White, float Opacity = 1.0f);
	UFUNCTION(BlueprintCallable, Category="Character|Effects") void SetEffectLayer3(UPaperSprite* Sprite, bool bVisible = true, const FLinearColor& Color = FLinearColor::White, float Opacity = 1.0f);
	UFUNCTION(BlueprintCallable, Category="Character|Effects") void ShowEffectLayer(int32 LayerIndex, bool bShow);
	UFUNCTION(BlueprintCallable, Category="Character|Effects") void ClearAllEffects();
	UFUNCTION(BlueprintCallable, Category="Character|Animation") void EnableBlinking(bool bEnable);
	UFUNCTION(BlueprintCallable, Category="Character|Animation") void EnableTalking(bool bEnable);
	UFUNCTION(BlueprintCallable, Category="Character|Animation") bool IsBlinkingEnabled() const { return bBlinkingActive; }
	UFUNCTION(BlueprintCallable, Category="Character|Animation") bool IsTalkingEnabled() const { return bTalkingActive; }
	UFUNCTION(BlueprintCallable, Category="Character|Animation") void BlinkOnce();
	UFUNCTION(BlueprintCallable, Category="Character|Runtime") void RefreshFromAsset();
	UFUNCTION(BlueprintCallable, Category="Character|Runtime") void UpdateFromAssetPreserveState();

    // Transition API
    UFUNCTION(BlueprintCallable, Category="Character|Animation|Transition")
    void Appear(float Duration = 1.0f, ECharacter2DTransitionCurve CurveType = ECharacter2DTransitionCurve::Smooth);
    UFUNCTION(BlueprintCallable, Category="Character|Animation|Transition")
    void AppearWithCustomCurve(UCurveFloat* Curve, float Duration = 1.0f);
    UFUNCTION(BlueprintCallable, Category="Character|Animation|Transition")
    void Disappear(float Duration = 1.0f, ECharacter2DTransitionCurve CurveType = ECharacter2DTransitionCurve::Smooth);
    UFUNCTION(BlueprintCallable, Category="Character|Animation|Transition")
    void DisappearWithCustomCurve(UCurveFloat* Curve, float Duration = 1.0f);
    UFUNCTION(BlueprintPure, Category="Character|Animation|Transition")
    bool IsInTransition() const { return bIsInTransition; }

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void SetupComponents();
	void StopAllAnimationsForRefresh();
	bool AttachComponentToSocket(USceneComponent* Component, USkeletalMeshComponent* TargetMesh, FName SocketName, bool bUseSocketTransform);
	void SetupHeadHierarchy();
	void SetupEffectLayers();
	void AttachHeadToSocket();
	void SetupSkeletalComponent(USkeletalMeshComponent* Component, const FCharacter2DSkeletalPart& Part);
	void SetupSpriteComponent(UPaperSpriteComponent* Component, TObjectPtr<UPaperSprite> Sprite, const FVector& Offset, float Scale, bool bIsVisible, const FLinearColor& Color = FLinearColor::White, float Opacity = 1.0f);
	void AttachSpriteToSocket(UPaperSpriteComponent* SpriteComp, ECharacter2DAttachmentTarget TargetType, FName SocketName, bool bUseSocketTransform, const FVector& Offset, float Scale);
	bool HasValidSprites() const;
	bool HasValidSkeletalMeshes() const;
	void StoreOriginalValues();
	void SetAllSpritesOpacity(float Opacity);
	void SetAllSpritesColor(const FLinearColor& Color);
	void SetAllSkeletalOpacity(float Opacity);
	TArray<UPaperSpriteComponent*> GetAllSpriteComponents(bool bIncludeEffects = true) const;
	TArray<USkeletalMeshComponent*> GetAllSkeletalComponents() const;
	USkeletalMeshComponent* GetSkeletalComponentByTarget(ECharacter2DAttachmentTarget Target) const;
	
	// Blinking
    void StartBlinking();
    void StopBlinking();
    void TriggerBlink(); // Запускает одно моргание (для таймера и BlinkOnce)
	
	// Talking
    void StartTalking();
    void StopTalking();

    void UpdateOriginalSprites();
    void RestoreEyelidsAfterBlink();
    void RestoreMouthAfterTalk();

	// Timelines Callbacks
    UFUNCTION() void HandleBlinkTimelineUpdate(float Value);
    UFUNCTION() void HandleBlinkTimelineFinished();
    UFUNCTION() void HandleTalkTimelineEvent();
	
    // Transition
    UFUNCTION() void HandleTransitionUpdate(float Value);
    UFUNCTION() void HandleTransitionFinished();
    void StartTransition(ECharacter2DTransitionState NewState, UCurveFloat* Curve, float Duration);
    void StopCurrentTransition();

	// Curves
	void InitializeDefaultCurves();
    UCurveFloat* GetDefaultCurve(ECharacter2DTransitionCurve CurveType) const;
	
    FTimerHandle BlinkTimerHandle; // Остается для интервала между морганиями
    bool bIsBlinking = false;
    bool bIsTalking = false;
    bool bBlinkScheduleNext = false; // Определяет, нужно ли планировать следующее моргание

    UPROPERTY() TObjectPtr<UCurveFloat> BlinkCurve; // Кривая для анимации моргания

    TObjectPtr<UPaperFlipbook> CurrentBlinkFlipbook = nullptr;
    TObjectPtr<UPaperSprite> OriginalEyelidsSprite = nullptr;
    TObjectPtr<UPaperSprite> OriginalMouthSprite = nullptr;
	FVector OriginalActorLocation;
	FVector OriginalActorScale;
	TMap<TObjectPtr<UPaperSpriteComponent>, FLinearColor> OriginalSpriteColors;
	FVector MovementStartLocation;
	FVector MovementTargetLocation;
	FCharacter2DEmotionSettings CurrentEmotionSettings;
	FCharacter2DMovementSettings CurrentMovementSettings;

    ECharacter2DTransitionState CurrentTransitionState = ECharacter2DTransitionState::None;
    TMap<TObjectPtr<UPaperSpriteComponent>, FLinearColor> OriginalSpriteColorsForTransition;

	UPROPERTY() TObjectPtr<UCurveFloat> CurrentTransitionCurve;
	UPROPERTY() TObjectPtr<UCurveFloat> DefaultLinearCurve;
	UPROPERTY() TObjectPtr<UCurveFloat> DefaultSmoothCurve;
	UPROPERTY() TObjectPtr<UCurveFloat> DefaultEaseInCurve;
	UPROPERTY() TObjectPtr<UCurveFloat> DefaultEaseOutCurve;
};