#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PaperSpriteComponent.h"
#include "PaperFlipbookComponent.h"
#include "TimerManager.h"
#include "Components/TimelineComponent.h"
#include "Character2DAsset.h"
#include "Character2DActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacter2DTransitionFinished, ECharacter2DTransitionType, TransitionType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacter2DEmotionFinished, ECharacter2DEmotionEffect, EmotionType);

UCLASS(BlueprintType, Blueprintable)
class CHARACTER2DRUNTIME_API ACharacter2DActor : public AActor
{
	GENERATED_BODY()

public:
	ACharacter2DActor();

	/* ---------------- Components ---------------- */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USkeletalMeshComponent> BodyComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USkeletalMeshComponent> ArmsComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USkeletalMeshComponent> HeadComponent;

	/* ---------------- Sprite components ------------------ */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Sprites")
	TObjectPtr<UPaperSpriteComponent> SpriteBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Sprites")
	TObjectPtr<UPaperSpriteComponent> SpriteArms;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Sprites")
	TObjectPtr<UPaperSpriteComponent> SpriteHead;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Sprites")
	TObjectPtr<UPaperSpriteComponent> SpriteEyebrow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Sprites")
	TObjectPtr<UPaperSpriteComponent> SpriteEyes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Sprites")
	TObjectPtr<UPaperSpriteComponent> SpriteEyelids;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Sprites")
	TObjectPtr<UPaperSpriteComponent> SpriteMouth;

	/* ---------------- Flipbook components ---------------- */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Animation")
	TObjectPtr<UPaperFlipbookComponent> EyelidComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Animation")
	TObjectPtr<UPaperFlipbookComponent> MouthComponent;

	/* ---------------- Timeline components for Visual Novel effects ---------------- */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Timeline")
	TObjectPtr<UTimelineComponent> TransitionTimeline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Timeline")
	TObjectPtr<UTimelineComponent> EmotionTimeline;

	/* ---------------- DataAsset reference ---------------- */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character")
	TObjectPtr<UCharacter2DAsset> CharacterAsset;

	/* ---------------- Runtime State ---------------------- */
	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime")
	bool bSpritesVisible = true;

	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime")
	bool bSkeletalVisible = true;

	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime")
	bool bBlinkingActive = false;

	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime")
	bool bTalkingActive = false;

	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime")
	bool bIsInTransition = false;

	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime")
	bool bIsPlayingEmotion = false;

	/* ---------------- Visual Novel API ------------------------- */
	
	/** Events */
	UPROPERTY(BlueprintAssignable, Category="Character|Events")
	FOnCharacter2DTransitionFinished OnTransitionFinished;

	UPROPERTY(BlueprintAssignable, Category="Character|Events")
	FOnCharacter2DEmotionFinished OnEmotionFinished;

	/** Transition Methods */
	UFUNCTION(BlueprintCallable, Category="Character|Visual Novel")
	void PlayTransition(ECharacter2DTransitionType TransitionType, 
	                   const FCharacter2DTransitionSettings& Settings);

	UFUNCTION(BlueprintCallable, Category="Character|Visual Novel")
	void PlayTransitionWithDefaults(ECharacter2DTransitionType TransitionType);

	/** Emotion Methods */
	UFUNCTION(BlueprintCallable, Category="Character|Visual Novel")
	void PlayEmotion(ECharacter2DEmotionEffect EmotionType, 
	                const FCharacter2DEmotionSettings& Settings);

	UFUNCTION(BlueprintCallable, Category="Character|Visual Novel")
	void PlayEmotionWithDefaults(ECharacter2DEmotionEffect EmotionType);

	UFUNCTION(BlueprintCallable, Category="Character|Visual Novel")
	void StopCurrentEmotion();

	/** Visibility Control */
	UFUNCTION(BlueprintCallable, Category="Character|Visibility")
	void SetSpritesVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category="Character|Visibility")
	void SetSkeletalVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category="Character|Visibility")
	void SetBothVisible(bool bSprites, bool bSkeletal);

	/* ---------------- Traditional Character API ------------------------- */
	UFUNCTION(BlueprintCallable, Category="Character|Animation")
	void EnableBlinking(bool bEnable);

	UFUNCTION(BlueprintCallable, Category="Character|Animation")
	void EnableTalking(bool bEnable);

	UFUNCTION(BlueprintCallable, Category="Character|Animation")
	bool IsBlinking() const { return bIsBlinking; }

	UFUNCTION(BlueprintCallable, Category="Character|Animation")
	bool IsTalking() const { return bIsTalking; }

	UFUNCTION(BlueprintCallable, Category="Character|Runtime")
	void RefreshFromAsset();

	/** Quick appearance/disappearance methods */
	UFUNCTION(BlueprintCallable, Category="Character|Visual Novel")
	void AppearInstantly();

	UFUNCTION(BlueprintCallable, Category="Character|Visual Novel")
	void DisappearInstantly();

	UFUNCTION(BlueprintCallable, Category="Character|Visual Novel")
	void AppearWithDefaultTransition();

	UFUNCTION(BlueprintCallable, Category="Character|Visual Novel")
	void DisappearWithDefaultTransition();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/* --- Original Animation State --- */
	FTimerHandle BlinkTimerHandle;
	FTimerHandle BlinkRestoreHandle;
	bool bIsBlinking = false;
	bool bIsTalking = false;

	/* --- Visual Novel Effect State --- */
	ECharacter2DTransitionType CurrentTransitionType = ECharacter2DTransitionType::None;
	ECharacter2DEmotionEffect CurrentEmotionType = ECharacter2DEmotionEffect::None;
	
	FVector OriginalActorLocation;
	FVector OriginalActorScale;
	FLinearColor OriginalSpriteColors[7]; // For all sprite components
	
	/* --- Additional transition state variables --- */
	FVector SlideStartLocation;
	FVector SlideTargetLocation;
	
	/* --- Current emotion settings for timeline callbacks --- */
	FCharacter2DEmotionSettings CurrentEmotionSettings;
	
	/* --- Timeline Callbacks --- */
	UFUNCTION()
	void OnTransitionTimelineUpdate(float Value);
	
	UFUNCTION()
	void OnTransitionTimelineFinished();
	
	UFUNCTION()
	void OnEmotionTimelineUpdate(float Value);
	
	UFUNCTION()
	void OnEmotionTimelineFinished();

	/* --- Helper Methods --- */
	void SetupComponents();
	void SetupSpriteComponent(UPaperSpriteComponent* Component, const FCharacter2DSpriteLayer& Layer);
	void SetupSkeletalComponent(USkeletalMeshComponent* Component, const FCharacter2DSkeletalPart& Part);

	bool HasValidSprites() const;
	bool HasValidSkeletalMeshes() const;
	
	void StoreOriginalValues();
	void RestoreOriginalValues();
	
	void SetAllSpritesOpacity(float Opacity);
	void SetAllSpritesColor(const FLinearColor& Color);
	void SetAllSkeletalOpacity(float Opacity);
	
	TArray<UPaperSpriteComponent*> GetAllSpriteComponents() const;
	TArray<USkeletalMeshComponent*> GetAllSkeletalComponents() const;

	/* --- Original Animation Methods --- */
	void StartBlinking();
	void StopBlinking();
	void HandleBlink();
	void StartTalking();
	void StopTalking();

	/* --- Transition Implementation --- */
	void ExecuteFadeTransition(bool bFadeIn, const FCharacter2DTransitionSettings& Settings);
	void ExecuteSlideTransition(bool bSlideIn, bool bFromLeft, const FCharacter2DTransitionSettings& Settings);
	void ExecuteScaleTransition(bool bScaleIn, const FCharacter2DTransitionSettings& Settings);

	/* --- Emotion Implementation --- */
	void ExecuteShakeEmotion(const FCharacter2DEmotionSettings& Settings);
	void ExecutePulseEmotion(const FCharacter2DEmotionSettings& Settings);
	void ExecuteColorShiftEmotion(const FCharacter2DEmotionSettings& Settings);
	void ExecuteBounceEmotion(const FCharacter2DEmotionSettings& Settings);
	void ExecuteFlashEmotion(const FCharacter2DEmotionSettings& Settings);
};