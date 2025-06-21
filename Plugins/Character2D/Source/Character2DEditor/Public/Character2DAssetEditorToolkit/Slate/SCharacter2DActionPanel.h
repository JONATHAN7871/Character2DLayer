#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Character2DEnums.h" // Для ECharacter2DEmotionEffect
#include "Character2DAsset.h" // Для FCharacter2DEmotionSettings
#include "TimerManager.h"

// Прямые объявления для уменьшения зависимостей в заголовке
class UCharacter2DAsset;
class ACharacter2DActor;

/**
 * Панель действий для тестирования возможностей Character2D в редакторе
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
    // === Ссылки ===
    TWeakObjectPtr<UCharacter2DAsset> CharacterAsset;
    TWeakObjectPtr<ACharacter2DActor> PreviewActor;

    // === Флаги состояния ===
    bool bBlinkingEnabled = false;
    bool bTalkingEnabled = false;
    bool bSpritesVisible = true;
    bool bSkeletalVisible = true;
    bool bEffect1Enabled = false;
    bool bEffect2Enabled = false;
    bool bEffect3Enabled = false;

    // === Настройки перемещения ===
    FVector TargetLocation = FVector::ZeroVector;
    FVector CurrentLocation = FVector::ZeroVector;
    float MovementDuration = 1.0f;
    bool bTeleportInstant = false;

    // === Настройки эмоций ===
    TArray<TSharedPtr<ECharacter2DEmotionEffect>> EmotionOptions;
    TSharedPtr<ECharacter2DEmotionEffect> CurrentEmotion;
    float EmotionDuration = 2.0f;
    float EmotionIntensity = 0.5f;

    // === Хэндлы таймеров ===
    FTimerHandle BlinkTestHandle;
    FTimerHandle TalkTestHandle;
    FTimerHandle TransitionTestHandle;
    FTimerHandle LocationSyncHandle;

private:
    // === Управление состоянием ===
    void SyncStateFromActor();
    void UpdateLocationFromActor();
    void StopAllPreviewAnimations();
    void EnsurePreviewVisible();

    // === Построение UI ===
    TSharedRef<SWidget> BuildQuickActionsSection();
    TSharedRef<SWidget> BuildSpriteManipulationSection();
    TSharedRef<SWidget> BuildEffectLayersSection();
    TSharedRef<SWidget> BuildAnimationTestingSection();
    TSharedRef<SWidget> BuildMovementTestingSection();
    TSharedRef<SWidget> BuildEmotionTestSection();
    TSharedRef<SWidget> BuildVisibilityTestSection();

    // === Быстрые действия ===
    FReply OnShowCharacter();
    FReply OnHideCharacter();
    FReply OnFadeIn();
    FReply OnFadeOut();
    FReply OnResetCharacter();

    // === Манипуляции со спрайтами ===
    FReply OnTestChangeEyebrows();
    FReply OnTestChangeMouth();
    FReply OnRestoreFromAsset();

    // === Слои эффектов ===
    void OnEffect1Changed(ECheckBoxState NewState);
    void OnEffect2Changed(ECheckBoxState NewState);
    void OnEffect3Changed(ECheckBoxState NewState);
    FReply OnClearAllEffects();

    // === Перемещение ===
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

    // === Анимация ===
    void OnBlinkChanged(ECheckBoxState NewState);
    void OnTalkChanged(ECheckBoxState NewState);
    FReply OnTestBlink();
    FReply OnTestTalk();
    FReply OnBlinkOnce();

    // === Видимость ===
    void OnToggleSprites(ECheckBoxState NewState);
    void OnToggleSkeletal(ECheckBoxState NewState);
    ECheckBoxState GetSpritesVisibleState() const;
    ECheckBoxState GetSkeletalVisibleState() const;

    // === Эмоции ===
    void OnEmotionChanged(TSharedPtr<ECharacter2DEmotionEffect> NewSelection, ESelectInfo::Type SelectInfo);
    void OnEmotionDurationChanged(float NewValue);
    void OnEmotionIntensityChanged(float NewValue);
    FText GetEmotionTypeText() const;
    FCharacter2DEmotionSettings GetCurrentEmotionSettings() const;
    FReply OnTestEmotion();
    FReply OnStopEmotion();

    // === Вспомогательные функции ===
    bool IsPreviewActorValid() const
    {
        return PreviewActor.IsValid();
    }

    static FText EmotionTypeToText(ECharacter2DEmotionEffect Type);
};