#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "Data/VNCharacterIdleAnimationStructs.h"
#include "Data/VNCharacterEnums.h"
#include "VNCharacterIdleAnimationManager.generated.h"

class AVNCharacter;
class UPaperSpriteComponent;

UCLASS(BlueprintType, Blueprintable, ClassGroup=(VNCharacter), meta=(BlueprintSpawnableComponent))
class VNCHARACTERSYSTEM_API UVNCharacterIdleAnimationManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UVNCharacterIdleAnimationManager();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // =====================================================
    // ПУБЛИЧНЫЕ МЕТОДЫ
    // =====================================================

    UFUNCTION(BlueprintCallable, Category = "VN Idle Animations")
    void SetBlinkEnabled(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "VN Idle Animations")
    void SetTalkEnabled(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "VN Idle Animations")
    void SetEyesRandomEnabled(bool bEnable);

    UFUNCTION(BlueprintCallable, Category = "VN Idle Animations")
    void SetIdleAnimationsConfig(const FVNIdleAnimationsConfig& NewConfig);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Idle Animations")
    const FVNIdleAnimationsConfig& GetIdleAnimationsConfig() const { return IdleAnimationsConfig; }

    UFUNCTION(BlueprintCallable, Category = "VN Idle Animations")
    void StopAllIdleAnimations();

    UFUNCTION(BlueprintCallable, Category = "VN Idle Animations")
    void StartAllIdleAnimations();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Idle Animations")
    bool IsBlinkActive() const { return IdleAnimationsConfig.BlinkConfig.bEnabled; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Idle Animations")
    bool IsTalkActive() const { return IdleAnimationsConfig.TalkConfig.bEnabled; }
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Idle Animations")
    bool IsEyesRandomActive() const { return IdleAnimationsConfig.EyesRandomConfig.bEnabled; }

    UFUNCTION(BlueprintCallable, Category = "VN Idle Animations")
    void UpdateSavedSprites();

    // Интеграция с VNCharacter
    void HandleExternalSpriteChange(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> NewSprite);
    void UpdateBlinkModeForNewEyelidsState(TSoftObjectPtr<UPaperSprite> NewEyelidsSprite);
    
public:
    // =====================================================
    // НАСТРОЙКИ
    // =====================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle Animations")
    FVNIdleAnimationsConfig IdleAnimationsConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bDisableIdleAnimations = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bVerboseLogging = false;

private:
    // =====================================================
    // ДАННЫЕ
    // =====================================================

    UPROPERTY(Transient)
    TWeakObjectPtr<AVNCharacter> OwnerCharacter;

    // Моргание
    FTimerHandle BlinkTimerHandle;
    bool bIsBlinkAnimationPlaying = false;

    // Разговор
    FTimerHandle TalkTimerHandle;

    // Глаза
    FTimerHandle EyesRandomTimerHandle;
    bool bIsEyesRandomAnimationPlaying = false;

    // =====================================================
    // МЕТОДЫ
    // =====================================================

    AVNCharacter* GetVNCharacterOwner() const;
    void LogIdleAnimation(const FString& Message, bool bForceLog = false) const;
    void CheckForSpriteChanges();
    bool IsAnimationSprite(UPaperSpriteComponent* Component, UPaperSprite* Sprite) const;
    bool IsFlipbookSprite(UPaperFlipbook* Flipbook, UPaperSprite* Sprite) const;

    // Моргание
    void StartBlinkAnimation();
    void StopBlinkAnimation();
    void ExecuteBlink();
    void BlinkPhase2_FullyClosed();
    void BlinkPhase3_HalfOpen();
    void BlinkPhase4_SecondClosed();
    void BlinkPhase5_FinalHalfOpen();
    void ApplyBlinkColorToEyelids(AVNCharacter* Character);
    void FinishBlinkAnimation();
    void ScheduleNextBlink();
    bool IsCurrentSpritePartOfBlinkAnimation(UPaperSprite* Sprite) const;
    void ShowEyelidsAndSetSprite(UPaperSprite* NewSprite);

    // Разговор
    void StartTalkAnimation();
    void StopTalkAnimation();
    void UpdateTalkFrame();
    bool IsCurrentSpritePartOfTalkAnimation(UPaperSprite* Sprite) const;

    // Глаза
    void StartEyesRandomAnimation();
    void StopEyesRandomAnimation();
    void ExecuteRandomEyesMovement();
    void ReturnEyesToOriginal();
    void ScheduleNextEyesMovement();

    // Utility
    UPaperSprite* GetFlipbookSpriteImproved(UPaperFlipbook* Flipbook, int32 FrameIndex) const;
    UPaperSprite* GetRandomFlipbookSpriteImproved(UPaperFlipbook* Flipbook, bool bExcludeFirstFrame = false) const;
    int32 GetFlipbookFrameCountImproved(UPaperFlipbook* Flipbook) const;

    UFUNCTION(BlueprintCallable, Category = "VN Idle Animations")
    void DebugFlipbook(UPaperFlipbook* Flipbook) const;
};