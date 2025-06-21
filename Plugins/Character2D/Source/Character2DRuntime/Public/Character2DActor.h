#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PaperSpriteComponent.h"
#include "TimerManager.h"
#include "Components/TimelineComponent.h"
#include "Character2DAsset.h"
#include "Character2DActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacter2DEmotionFinished, ECharacter2DEmotionEffect, EmotionType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacter2DBlinkStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacter2DBlinkFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacter2DTalkStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacter2DTalkStopped);

UCLASS(BlueprintType, Blueprintable)
class CHARACTER2DRUNTIME_API ACharacter2DActor : public AActor
{
	GENERATED_BODY()

public:
	ACharacter2DActor();

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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Timeline") TObjectPtr<UTimelineComponent> MovementTimeline;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Timeline") TObjectPtr<UTimelineComponent> EmotionTimeline;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Timeline") TObjectPtr<UTimelineComponent> FadeTimeline;

    // Data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character") TObjectPtr<UCharacter2DAsset> CharacterAsset;

    // State
	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime") bool bSpritesVisible = true;
	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime") bool bSkeletalVisible = true;
	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime") bool bBlinkingActive = false;
	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime") bool bTalkingActive = false;
	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime") bool bIsMoving = false;
	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime") bool bIsPlayingEmotion = false;
	UPROPERTY(BlueprintReadOnly, Category="Character|Runtime") bool bIsFading = false;

    // Events
	UPROPERTY(BlueprintAssignable, Category="Character|Events") FOnCharacter2DEmotionFinished OnEmotionFinished;
	UPROPERTY(BlueprintAssignable, Category="Character|Events") FOnCharacter2DBlinkStarted OnBlinkStarted;
	UPROPERTY(BlueprintAssignable, Category="Character|Events") FOnCharacter2DBlinkFinished OnBlinkFinished;
	UPROPERTY(BlueprintAssignable, Category="Character|Events") FOnCharacter2DTalkStarted OnTalkStarted;
	UPROPERTY(BlueprintAssignable, Category="Character|Events") FOnCharacter2DTalkStopped OnTalkStopped;

    // API
	UFUNCTION(BlueprintCallable, Category="Character|Movement") void MoveToLocation(const FVector& TargetLocation, float Duration = 1.0f);
	UFUNCTION(BlueprintCallable, Category="Character|Movement") void MoveToLocationWithSettings(const FVector& TargetLocation, const FCharacter2DMovementSettings& Settings);
	UFUNCTION(BlueprintCallable, Category="Character|Appearance") void PlayFadeIn(float Duration = 1.0f);
	UFUNCTION(BlueprintCallable, Category="Character|Appearance") void PlayFadeOut(float Duration = 1.0f);
	UFUNCTION(BlueprintCallable, Category="Character|Emotions") void PlayEmotion(ECharacter2DEmotionEffect EmotionType, const FCharacter2DEmotionSettings& Settings);
	UFUNCTION(BlueprintCallable, Category="Character|Emotions") void PlayEmotionWithDefaults(ECharacter2DEmotionEffect EmotionType);
	UFUNCTION(BlueprintCallable, Category="Character|Emotions") void StopCurrentEmotion();
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
	void SetupSpriteComponent(UPaperSpriteComponent* Component, TObjectPtr<UPaperSprite> Sprite, const FVector& Offset, float Scale, bool bIsVisible);
	void AttachSpriteToSocket(UPaperSpriteComponent* SpriteComp, ECharacter2DAttachmentTarget TargetType, FName SocketName, bool bUseSocketTransform, const FVector& Offset, float Scale);
	bool HasValidSprites() const;
	bool HasValidSkeletalMeshes() const;
	void StoreOriginalValues();
	void RestoreOriginalValues();
	void SetAllSpritesOpacity(float Opacity);
	void SetAllSpritesColor(const FLinearColor& Color);
	void SetAllSkeletalOpacity(float Opacity);
	TArray<UPaperSpriteComponent*> GetAllSpriteComponents(bool bIncludeEffects = true) const;
	TArray<USkeletalMeshComponent*> GetAllSkeletalComponents() const;
	USkeletalMeshComponent* GetSkeletalComponentByTarget(ECharacter2DAttachmentTarget Target) const;
	void StartBlinking();
	void StopBlinking();
	void HandleBlink();
	void StartTalking();
	void StopTalking();
	void HandleTalkFrame();
	
	FTimerHandle BlinkTimerHandle;
	FTimerHandle TalkTimerHandle;
	bool bIsBlinking = false;
	bool bIsTalking = false;
	TObjectPtr<UPaperSprite> OriginalEyelidsSprite = nullptr;
	TObjectPtr<UPaperSprite> OriginalMouthSprite = nullptr;
	ECharacter2DEmotionEffect CurrentEmotionType = ECharacter2DEmotionEffect::None;
	FVector OriginalActorLocation;
	FVector OriginalActorScale;
	TMap<TObjectPtr<UPaperSpriteComponent>, FLinearColor> OriginalSpriteColors;
	FVector MovementStartLocation;
	FVector MovementTargetLocation;
	FCharacter2DEmotionSettings CurrentEmotionSettings;
	FCharacter2DMovementSettings CurrentMovementSettings;
	
	UFUNCTION() void OnMovementTimelineUpdate(float Value);
	UFUNCTION() void OnMovementTimelineFinished();
	UFUNCTION() void OnEmotionTimelineUpdate(float Value);
	UFUNCTION() void OnEmotionTimelineFinished();
	UFUNCTION() void OnFadeTimelineUpdate(float Value);
	UFUNCTION() void OnFadeTimelineFinished();
};