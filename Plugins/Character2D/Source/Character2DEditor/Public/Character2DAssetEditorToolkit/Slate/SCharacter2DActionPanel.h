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
        /** Указываем сам DataAsset (чтобы при желании можно было читать/писать настройки) */
        SLATE_ARGUMENT(UCharacter2DAsset*, CharacterAsset)

        /** Слабая ссылка на ACharacter2DActor, которого спавнит вьюпорт */
        SLATE_ARGUMENT(TWeakObjectPtr<ACharacter2DActor>, PreviewActor)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    // Ссылка на DataAsset (вдруг понадобится читать настройки)
    UCharacter2DAsset* CharacterAsset = nullptr;

    // Слабая ссылка на ACharacter2DActor, который спавнится в PreviewScene
    TWeakObjectPtr<ACharacter2DActor> PreviewActor;

    // Состояние флажков (чтобы UI сразу отображал, включён ли blink/talk/visible)
    bool bBlinkingEnabled = false;
    bool bTalkingEnabled  = false;
    bool bSpritesVisible  = true;
    bool bSkeletalVisible = true;

    // Для Transition Section
    TArray<TSharedPtr<ECharacter2DTransitionType>> TransitionOptions;
    TSharedPtr<ECharacter2DTransitionType> CurrentTransition;
    float TransitionDuration = 1.0f;

    // Для Emotion Section
    TArray<TSharedPtr<ECharacter2DEmotionEffect>> EmotionOptions;
    TSharedPtr<ECharacter2DEmotionEffect> CurrentEmotion;
    float EmotionDuration  = 2.0f;
    float EmotionIntensity = 0.5f;

private:
    // === Синхронизация состояния ===
    void SyncStateFromActor();

    // === Разбиение UI на блоки ===
    TSharedRef<SWidget> BuildQuickActionsSection();
    TSharedRef<SWidget> BuildAnimationTestingSection();
    TSharedRef<SWidget> BuildTransitionTestingSection();
    TSharedRef<SWidget> BuildEmotionTestSection();
    TSharedRef<SWidget> BuildVisibilityTestSection();

    // === Обработчики кликов / изменения состояния ===

    // Quick Actions
    FReply OnAppearInstantly();
    FReply OnDisappearInstantly();
    FReply OnAppearDefault();
    FReply OnDisappearDefault();
    FReply OnResetCharacter();

    // Blink / Talk
    void OnBlinkChanged(ECheckBoxState NewState);
    void OnTalkChanged(ECheckBoxState NewState);
    FReply OnTestBlink();
    FReply OnTestTalk();

    // Helper methods to keep preview visible
    void StopAllPreviewAnimations();
    void EnsurePreviewVisible();

    // Timer handles for temporary tests
    FTimerHandle BlinkTestHandle;
    FTimerHandle TalkTestHandle;
    FTimerHandle TransitionTestHandle;

    // Visibility (CheckBox изменил состояние)
    void OnToggleSprites(ECheckBoxState NewState);
    void OnToggleSkeletal(ECheckBoxState NewState);
    ECheckBoxState GetSpritesVisibleState() const;
    ECheckBoxState GetSkeletalVisibleState() const;

    // Transition UI callbacks
    void OnTransitionChanged(TSharedPtr<ECharacter2DTransitionType> NewSelection, ESelectInfo::Type SelectInfo);
    void OnTransitionDurationChanged(float NewValue);
    FText GetTransitionTypeText() const;
    FCharacter2DTransitionSettings GetCurrentTransitionSettings() const;
    FReply OnTestTransition();

    // Emotion UI callbacks
    void OnEmotionChanged(TSharedPtr<ECharacter2DEmotionEffect> NewSelection, ESelectInfo::Type SelectInfo);
    void OnEmotionDurationChanged(float NewValue);
    void OnEmotionIntensityChanged(float NewValue);
    FText GetEmotionTypeText() const;
    FCharacter2DEmotionSettings GetCurrentEmotionSettings() const;
    FReply OnTestEmotion();
    FReply OnStopEmotion();


    // Помощники
    bool IsPreviewActorValid() const
    {
        return PreviewActor.IsValid() && IsValid(PreviewActor.Get());
    }

    // Конвертеры enum -> текст
    static FText TransitionTypeToText(ECharacter2DTransitionType Type);
    static FText EmotionTypeToText(ECharacter2DEmotionEffect Type);
};
