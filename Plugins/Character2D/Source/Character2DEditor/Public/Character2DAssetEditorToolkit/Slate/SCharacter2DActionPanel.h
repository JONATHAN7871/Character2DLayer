#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Character2DAsset.h"
#include "Character2DActor.h"
#include "Character2DEnums.h"
#include "TimerManager.h"

class ACharacter2DActor;

/**
 * Action panel for testing Character2D features in the editor
 */
class SCharacter2DActionPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCharacter2DActionPanel) {}
        SLATE_ARGUMENT(UCharacter2DAsset*, CharacterAsset)
        SLATE_ARGUMENT(TWeakObjectPtr<ACharacter2DActor>, PreviewActor)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    // References
    UCharacter2DAsset* CharacterAsset = nullptr;
    TWeakObjectPtr<ACharacter2DActor> PreviewActor;

    // State flags
    bool bBlinkingEnabled = false;
    bool bTalkingEnabled = false;
    bool bSpritesVisible = true;
    bool bSkeletalVisible = true;

    // Movement settings
    FVector TargetLocation = FVector::ZeroVector;
    FVector CurrentLocation = FVector::ZeroVector;
    float MovementDuration = 1.0f;
    bool bTeleportInstant = false;

    // Emotion settings
    TArray<TSharedPtr<ECharacter2DEmotionEffect>> EmotionOptions;
    TSharedPtr<ECharacter2DEmotionEffect> CurrentEmotion;
    float EmotionDuration = 2.0f;
    float EmotionIntensity = 0.5f;

    // Timer handles
    FTimerHandle BlinkTestHandle;
    FTimerHandle TalkTestHandle;
    FTimerHandle TransitionTestHandle;
    FTimerHandle LocationSyncHandle;

private:
    // === State Management ===
    void SyncStateFromActor();
    void UpdateLocationFromActor();
    void StopAllPreviewAnimations();
    void EnsurePreviewVisible();

    // === UI Building ===
    TSharedRef<SWidget> BuildQuickActionsSection();
    TSharedRef<SWidget> BuildAnimationTestingSection();
    TSharedRef<SWidget> BuildMovementTestingSection();
    TSharedRef<SWidget> BuildEmotionTestSection();
    TSharedRef<SWidget> BuildVisibilityTestSection();

    // === Quick Actions ===
    FReply OnShowCharacter();
    FReply OnHideCharacter();
    FReply OnFadeIn();
    FReply OnFadeOut();
    FReply OnResetCharacter();

    // === Movement ===
    FText GetCurrentPositionText() const;
    TOptional<float> GetTargetX() const;
    TOptional<float> GetTargetY() const;
    TOptional<float> GetTargetZ() const;
    void OnTargetXChanged(float NewValue, ETextCommit::Type);
    void OnTargetYChanged(float NewValue, ETextCommit::Type);
    void OnTargetZChanged(float NewValue, ETextCommit::Type);
    void OnMovementDurationChanged(float NewValue);
    void OnTeleportInstantChanged(ECheckBoxState NewState);
    FReply OnMoveToTarget();
    FReply OnSetTargetFromCurrent();

    // === Animation ===
    void OnBlinkChanged(ECheckBoxState NewState);
    void OnTalkChanged(ECheckBoxState NewState);
    FReply OnTestBlink();
    FReply OnTestTalk();

    // === Visibility ===
    void OnToggleSprites(ECheckBoxState NewState);
    void OnToggleSkeletal(ECheckBoxState NewState);
    ECheckBoxState GetSpritesVisibleState() const;
    ECheckBoxState GetSkeletalVisibleState() const;

    // === Emotion ===
    void OnEmotionChanged(TSharedPtr<ECharacter2DEmotionEffect> NewSelection, ESelectInfo::Type SelectInfo);
    void OnEmotionDurationChanged(float NewValue);
    void OnEmotionIntensityChanged(float NewValue);
    FText GetEmotionTypeText() const;
    FCharacter2DEmotionSettings GetCurrentEmotionSettings() const;
    FReply OnTestEmotion();
    FReply OnStopEmotion();

    // === Helpers ===
    bool IsPreviewActorValid() const
    {
        return PreviewActor.IsValid() && IsValid(PreviewActor.Get());
    }

    static FText EmotionTypeToText(ECharacter2DEmotionEffect Type);
};