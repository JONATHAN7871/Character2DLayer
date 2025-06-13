#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PaperSpriteComponent.h"
#include "PaperFlipbookComponent.h"
#include "TimerManager.h"
#include "Components/TimelineComponent.h"
#include "Character2DAsset.h"
#include "Character2DActor.generated.h"

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

    /* ---------------- Timeline components ---------------- */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Timeline")
    TObjectPtr<UTimelineComponent> MovementTimeline;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Timeline")
    TObjectPtr<UTimelineComponent> EmotionTimeline;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Timeline")
    TObjectPtr<UTimelineComponent> FadeTimeline;

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
    bool bIsMoving = false;

    UPROPERTY(BlueprintReadOnly, Category="Character|Runtime")
    bool bIsPlayingEmotion = false;

    UPROPERTY(BlueprintReadOnly, Category="Character|Runtime")
    bool bIsFading = false;

    /* ---------------- Visual Novel API ------------------------- */
    
    /** Events */
    UPROPERTY(BlueprintAssignable, Category="Character|Events")
    FOnCharacter2DEmotionFinished OnEmotionFinished;

    /** Movement Methods */
    UFUNCTION(BlueprintCallable, Category="Character|Movement")
    void MoveToLocation(const FVector& TargetLocation, float Duration = 1.0f);

    UFUNCTION(BlueprintCallable, Category="Character|Movement")
    void MoveToLocationWithSettings(const FVector& TargetLocation, const FCharacter2DMovementSettings& Settings);

    /** Fade Methods */
    UFUNCTION(BlueprintCallable, Category="Character|Appearance")
    void PlayFadeIn(float Duration = 1.0f);

    UFUNCTION(BlueprintCallable, Category="Character|Appearance")
    void PlayFadeOut(float Duration = 1.0f);

    /** Emotion Methods */
    UFUNCTION(BlueprintCallable, Category="Character|Emotions")
    void PlayEmotion(ECharacter2DEmotionEffect EmotionType, const FCharacter2DEmotionSettings& Settings);

    UFUNCTION(BlueprintCallable, Category="Character|Emotions")
    void PlayEmotionWithDefaults(ECharacter2DEmotionEffect EmotionType);

    UFUNCTION(BlueprintCallable, Category="Character|Emotions")
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

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    /* --- Animation State --- */
    FTimerHandle BlinkTimerHandle;
    FTimerHandle BlinkRestoreHandle;
    bool bIsBlinking = false;
    bool bIsTalking = false;

    /* --- Visual Effect State --- */
    ECharacter2DEmotionEffect CurrentEmotionType = ECharacter2DEmotionEffect::None;
    
    FVector OriginalActorLocation;
    FVector OriginalActorScale;
    FLinearColor OriginalSpriteColors[7];
    
    /* --- Movement state --- */
    FVector MovementStartLocation;
    FVector MovementTargetLocation;
    
    /* --- Current settings for timeline callbacks --- */
    FCharacter2DEmotionSettings CurrentEmotionSettings;
    FCharacter2DMovementSettings CurrentMovementSettings;
    
    /* --- Timeline Callbacks --- */
    UFUNCTION()
    void OnMovementTimelineUpdate(float Value);
    
    UFUNCTION()
    void OnMovementTimelineFinished();
    
    UFUNCTION()
    void OnEmotionTimelineUpdate(float Value);
    
    UFUNCTION()
    void OnEmotionTimelineFinished();
    
    UFUNCTION()
    void OnFadeTimelineUpdate(float Value);
    
    UFUNCTION()
    void OnFadeTimelineFinished();

    /* --- Helper Methods --- */
    void SetupComponents();
    void SetupSpriteComponent(UPaperSpriteComponent* Component, const FCharacter2DSpriteLayer& Layer);
    void SetupSkeletalComponent(USkeletalMeshComponent* Component, const FCharacter2DSkeletalPart& Part);
    void AttachSpriteToSocket(UPaperSpriteComponent* SpriteComp, const FCharacter2DSpriteLayer& Layer);
    void AttachFlipbookToSocket(UPaperFlipbookComponent* FlipbookComp,
        ECharacter2DAttachmentTarget Target, FName Socket, bool bUseSocketTransform,
        const FVector& Offset, float Scale);

    // New methods for Body/Arms structures
    void SetupSpriteComponentFromStruct(UPaperSpriteComponent* Component, const FCharacter2DSpriteBodyStructure& BodyStruct);
    void SetupSpriteComponentFromStruct(UPaperSpriteComponent* Component, const FCharacter2DSpriteArmsStructure& ArmsStruct);
    void AttachSpriteToSocketFromStruct(UPaperSpriteComponent* SpriteComp, const FCharacter2DSpriteBodyStructure& BodyStruct);
    void AttachSpriteToSocketFromStruct(UPaperSpriteComponent* SpriteComp, const FCharacter2DSpriteArmsStructure& ArmsStruct);

    bool HasValidSprites() const;
    bool HasValidSkeletalMeshes() const;
    
    void StoreOriginalValues();
    void RestoreOriginalValues();
    
    void SetAllSpritesOpacity(float Opacity);
    void SetAllSpritesColor(const FLinearColor& Color);
    void SetAllSkeletalOpacity(float Opacity);
    
    TArray<UPaperSpriteComponent*> GetAllSpriteComponents() const;
    TArray<USkeletalMeshComponent*> GetAllSkeletalComponents() const;
    USkeletalMeshComponent* GetSkeletalComponentByTarget(ECharacter2DAttachmentTarget Target) const;

    /* --- Animation Methods --- */
    void StartBlinking();
    void StopBlinking();
    void HandleBlink();
    void StartTalking();
    void StopTalking();

    /* --- Emotion Implementation --- */
    void ExecuteShakeEmotion(const FCharacter2DEmotionSettings& Settings);
    void ExecutePulseEmotion(const FCharacter2DEmotionSettings& Settings);
    void ExecuteColorShiftEmotion(const FCharacter2DEmotionSettings& Settings);
    void ExecuteBounceEmotion(const FCharacter2DEmotionSettings& Settings);
    void ExecuteFlashEmotion(const FCharacter2DEmotionSettings& Settings);
};