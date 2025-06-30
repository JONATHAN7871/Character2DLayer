#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PaperSpriteComponent.h"
#include "TimerManager.h"
#include "Components/TimelineComponent.h"
#include "Character2DAsset.h"
#include "Character2DActor.generated.h"

UENUM(BlueprintType)
enum class ECharacter2DEffectLayerIndex : uint8
{
	Effect1    UMETA(DisplayName = "Effect Layer 1"),
	Effect2    UMETA(DisplayName = "Effect Layer 2"),
	Effect3    UMETA(DisplayName = "Effect Layer 3"),
};

// Существующие делегаты
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacter2DBlinkStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacter2DBlinkFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacter2DTalkStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacter2DTalkStopped);

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

	/**
 * @brief Вызывается, когда нужно показать или обновить слой эффекта. Реализуйте анимацию появления в Blueprint.
 * @param LayerIndex Индекс слоя, который нужно показать.
 * @param EffectSprite Спрайт для установки.
 */
	UFUNCTION(BlueprintImplementableEvent, Category="Character|Effects|Events", meta=(DisplayName="On Show Effect Animate"))
	void OnShowEffect(ECharacter2DEffectLayerIndex LayerIndex, UPaperSprite* EffectSprite);
    
	/**
	 * @brief Вызывается, когда нужно скрыть слой эффекта. Реализуйте анимацию исчезновения в Blueprint.
	 * @param LayerIndex Индекс слоя, который нужно скрыть.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Character|Effects|Events", meta=(DisplayName="On Hide Effect Animate"))
	void OnHideEffect(ECharacter2DEffectLayerIndex LayerIndex);
	
	/** Компонент тени как отдельный слой */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Shadow") 
	TObjectPtr<UPaperSpriteComponent> SpriteShadow;

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
	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime") bool bIsFocused = false;

    // Events
	UPROPERTY(BlueprintAssignable, Category="Character|Events") FOnCharacter2DBlinkStarted OnBlinkStarted;
	UPROPERTY(BlueprintAssignable, Category="Character|Events") FOnCharacter2DBlinkFinished OnBlinkFinished;
	UPROPERTY(BlueprintAssignable, Category="Character|Events") FOnCharacter2DTalkStarted OnTalkStarted;
	UPROPERTY(BlueprintAssignable, Category="Character|Events") FOnCharacter2DTalkStopped OnTalkStopped;

    // API
	UFUNCTION(BlueprintCallable, Category="Character|Visibility") void SetSpritesVisible(bool bVisible);
	UFUNCTION(BlueprintCallable, Category="Character|Visibility") void SetSkeletalVisible(bool bVisible);
	UFUNCTION(BlueprintCallable, Category="Character|Visibility") void SetAllSpritesVisible(bool bVisible);
	UFUNCTION(BlueprintCallable, Category="Character|Visibility") void SetAllSkeletalVisible(bool bVisible);
	UFUNCTION(BlueprintCallable, Category="Character|Visibility") void SetBothVisible(bool bSprites, bool bSkeletal);
	UFUNCTION(BlueprintCallable, Category="Character|Sprites") void SetEyebrowSprite(UPaperSprite* NewSprite);
	UFUNCTION(BlueprintCallable, Category="Character|Sprites") void SetEyesSprite(UPaperSprite* NewSprite);
	UFUNCTION(BlueprintCallable, Category="Character|Sprites") void SetEyelidsSprite(UPaperSprite* NewSprite);
	UFUNCTION(BlueprintCallable, Category="Character|Sprites") void SetMouthSprite(UPaperSprite* NewSprite);
	UFUNCTION(BlueprintCallable, Category="Character|Sprites") void SetHeadSprite(UPaperSprite* NewSprite);
	UFUNCTION(BlueprintCallable, Category="Character|Sprites") void SetBodySprite(UPaperSprite* NewSprite);
	UFUNCTION(BlueprintCallable, Category="Character|Sprites") void SetArmsSprite(UPaperSprite* NewSprite);

	/**
	 * @brief Устанавливает новый цвет для всех основных спрайтов персонажа. Не влияет на тень.
	 * @param NewColor Новый цвет. Прозрачность (Alpha) будет проигнорирована, используется текущая прозрачность каждого спрайта.
	 * @param bIncludeEffects Включать ли в изменение цвета спрайты эффектов.
	 */
	UFUNCTION(BlueprintCallable, Category="Character|Appearance")
	void SetAllCharacterSpritesColor(const FLinearColor& NewColor, bool bIncludeEffects = true);
	
    // Effects API
	UFUNCTION(BlueprintCallable, Category="Character|Effects") 
	void SetEffectLayer(ECharacter2DEffectLayerIndex LayerIndex, UPaperSprite* Sprite, bool bVisible = true, const FLinearColor& Color = FLinearColor::White, float Opacity = 1.0f);
    
	UFUNCTION(BlueprintCallable, Category="Character|Effects") 
	void SetEffectLayerOpacity(ECharacter2DEffectLayerIndex LayerIndex, float Opacity);
    
	UFUNCTION(BlueprintCallable, Category="Character|Effects") 
	void SetEffectLayerColor(ECharacter2DEffectLayerIndex LayerIndex, const FLinearColor& Color);
    
	/** Запускает логику показа/скрытия эффекта через события в Blueprint. */
	UFUNCTION(BlueprintCallable, Category="Character|Effects") 
	void ShowEffectLayer(ECharacter2DEffectLayerIndex LayerIndex, bool bShow);
    
	/** Запускает логику скрытия для всех активных эффектов через события в Blueprint. */
	UFUNCTION(BlueprintCallable, Category="Character|Effects") 
	void ClearAllEffects();
    
	/** Мгновенно скрывает слой эффекта, устанавливая его видимость в false. Вызывайте в конце анимации исчезновения. */
	UFUNCTION(BlueprintCallable, Category="Character|Effects")
	void ForceHideEffect(ECharacter2DEffectLayerIndex LayerIndex);
    
	/** Мгновенно скрывает все слои эффектов. */
	UFUNCTION(BlueprintCallable, Category="Character|Effects")
	void ForceHideAllEffects();

	
	/** Устанавливает спрайт тени и её параметры */
	UFUNCTION(BlueprintCallable, Category="Character|Shadow")
	void SetShadowLayer(UPaperSprite* Sprite, bool bVisible = true, const FLinearColor& Color = FLinearColor::Black, float Opacity = 0.5f);
	
	/** Устанавливает только прозрачность тени */
	UFUNCTION(BlueprintCallable, Category="Character|Shadow")
	void SetShadowOpacity(float Opacity);
	
	/** Устанавливает цвет тени */
	UFUNCTION(BlueprintCallable, Category="Character|Shadow")
	void SetShadowColor(const FLinearColor& Color);
	
	/** Показывает/скрывает тень */
	UFUNCTION(BlueprintCallable, Category="Character|Shadow")
	void SetShadowVisible(bool bVisible);
	
	UFUNCTION(BlueprintCallable, Category="Character|Animation") void EnableBlinking(bool bEnable);
	UFUNCTION(BlueprintCallable, Category="Character|Animation") void EnableTalking(bool bEnable);
	UFUNCTION(BlueprintCallable, Category="Character|Animation") bool IsBlinkingEnabled() const { return bBlinkingActive; }
	UFUNCTION(BlueprintCallable, Category="Character|Animation") bool IsTalkingEnabled() const { return bTalkingActive; }
	UFUNCTION(BlueprintCallable, Category="Character|Animation") void BlinkOnce();
	UFUNCTION(BlueprintCallable, Category="Character|Runtime") void RefreshFromAsset();
	UFUNCTION(BlueprintCallable, Category="Character|Runtime") void UpdateFromAssetPreserveState();

	/**
	 * @brief Устанавливает новый CharacterAsset и полностью обновляет актора.
	 * @param NewAsset Новый ассет для применения.
	 * @param bPreserveAnimationState Если true, состояние анимаций (моргание, разговор) будет сохранено и перезапущено. Если false, анимации будут остановлены.
	 */
	UFUNCTION(BlueprintCallable, Category="Character|Runtime", meta=(DisplayName="Set Character Asset"))
	void SetCharacterAsset(UCharacter2DAsset* NewAsset, bool bPreserveAnimationState = true);

    // Transition API for Blueprint
    UFUNCTION(BlueprintCallable, Category="Character|Animation|Transition")
    void Appear();
    
    UFUNCTION(BlueprintCallable, Category="Character|Animation|Transition")
    void Disappear();
    
    UFUNCTION(BlueprintImplementableEvent, Category="Character|Animation|Transition", meta=(DisplayName="On Appear"))
    void OnAppear();
    
    UFUNCTION(BlueprintImplementableEvent, Category="Character|Animation|Transition", meta=(DisplayName="On Disappear"))
    void OnDisappear();
    
    // Focus API
    UFUNCTION(BlueprintCallable, Category="Character|Focus")
    void GainFocus();
    
    UFUNCTION(BlueprintCallable, Category="Character|Focus")
    void LoseFocus();
    
    UFUNCTION(BlueprintImplementableEvent, Category="Character|Focus", meta=(DisplayName="On Focus Gained"))
    void OnFocusGained();
    
    UFUNCTION(BlueprintImplementableEvent, Category="Character|Focus", meta=(DisplayName="On Focus Lost"))
    void OnFocusLost();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void SetupComponents();
	void StopAllAnimationsForRefresh();
	bool AttachComponentToSocket(USceneComponent* Component, USkeletalMeshComponent* TargetMesh, FName SocketName, bool bUseSocketTransform);
	USkeletalMeshComponent* GetSkeletalComponentByAttachmentTarget(ECharacter2DSkeletalAttachmentTarget Target) const;
	void SetupHeadHierarchy();
	void SetupEffectLayers();
	void SetupShadowLayer();
	void AttachHeadToSocket();
	void AttachShadowToSocket();
	void AttachSkeletalComponentToSocket(USkeletalMeshComponent* Component, const FCharacter2DSkeletalPart& Part);
	void SetupSkeletalComponent(USkeletalMeshComponent* Component, const FCharacter2DSkeletalPart& Part);
	void SetupSpriteComponent(UPaperSpriteComponent* Component, TObjectPtr<UPaperSprite> Sprite, const FVector& Offset, float Scale, bool bIsVisible, const FLinearColor& Color = FLinearColor::White, float Opacity = 1.0f);
	void AttachSpriteToSocket(UPaperSpriteComponent* SpriteComp, ECharacter2DAttachmentTarget TargetType, FName SocketName, bool bUseSocketTransform, const FVector& Offset, float Scale);
	bool HasValidSprites() const;
	bool HasValidSkeletalMeshes() const;
	void StoreOriginalValues();
	TArray<UPaperSpriteComponent*> GetAllSpriteComponents(bool bIncludeEffects = true, bool bIncludeShadow = false) const;
	TArray<USkeletalMeshComponent*> GetAllSkeletalComponents() const;
	USkeletalMeshComponent* GetSkeletalComponentByTarget(ECharacter2DAttachmentTarget Target) const;
	UPaperSpriteComponent* GetEffectComponentByIndex(ECharacter2DEffectLayerIndex LayerIndex) const;
	
	// Blinking
    void StartBlinking();
    void StopBlinking();
    void TriggerBlink();
	
	// Talking
    void StartTalking();
    void StopTalking();

    void UpdateOriginalSprites();
    void RestoreEyelidsAfterBlink();
    void RestoreEyelidsPartiallyAfterBlink(float OpenAmount); // New for double blinks
    void RestoreMouthAfterTalk();

	// Timelines Callbacks
    UFUNCTION() void HandleBlinkTimelineUpdate(float Value);
    UFUNCTION() void HandleBlinkTimelineFinished();
    UFUNCTION() void HandleTalkTimelineUpdate(float Value);
    UFUNCTION() void HandleTalkTimelineFinished();

    FTimerHandle BlinkTimerHandle;
    FTimerHandle TalkTimerHandle;
    FTimerHandle InterBlinkTimerHandle; // For delay between double blinks
    
    bool bIsBlinking = false;
    bool bIsTalking = false;
    bool bBlinkScheduleNext = false;
    int32 BlinksLeftInSequence = 0; // For double blinks

    UPROPERTY() TObjectPtr<UCurveFloat> BlinkCurve;
    UPROPERTY() TObjectPtr<UCurveFloat> TalkCurve;

    TObjectPtr<UPaperFlipbook> CurrentBlinkFlipbook = nullptr;
    TObjectPtr<UPaperFlipbook> CurrentTalkFlipbook = nullptr;
    TObjectPtr<UPaperSprite> OriginalEyelidsSprite = nullptr;
    TObjectPtr<UPaperSprite> OriginalMouthSprite = nullptr;
    
	FVector OriginalActorLocation;
	FVector OriginalActorScale;
	TMap<TObjectPtr<UPaperSpriteComponent>, FLinearColor> OriginalSpriteColors;
};