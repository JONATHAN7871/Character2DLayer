#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
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
    
    // === Виджеты для Auto Animations ===
    TSharedPtr<SCheckBox> AutoBlinkCheckBox;
    TSharedPtr<SCheckBox> AutoTalkCheckBox;
    TSharedPtr<STextBlock> AutoBlinkLabel;
    TSharedPtr<STextBlock> AutoTalkLabel;

    // === Флаги состояния ===
    bool bBlinkingEnabled = false;
    bool bTalkingEnabled = false;
    bool bSpritesVisible = true;
    bool bSkeletalVisible = true;
    
    // НОВОЕ: Editor-only флаги для preview (не влияют на игровую логику)
    bool bPreviewBlinkingEnabled = false;
    bool bPreviewTalkingEnabled = false;

    // === Хэндлы таймеров ===
    FTimerHandle BlinkTestHandle;
    FTimerHandle TalkTestHandle;
    FTimerHandle TransitionTestHandle;
    FTimerHandle LocationSyncHandle;

private:
    // === Управление состоянием ===
    void SyncStateFromActor();
    void StopAllPreviewAnimations();
    void EnsurePreviewVisible();

    // === Построение UI ===
    TSharedRef<SWidget> BuildAutoAnimationsSection();        // Секция Auto Animations (настройки ассета)
    TSharedRef<SWidget> BuildAnimationTestingSection();      // Секция тестирования (только preview)
    TSharedRef<SWidget> BuildVisibilityTestSection();

    // === Быстрые действия ===
    FReply OnResetCharacter();

    // === Auto Animations (влияют на настройки ассета) ===
    void OnAutoBlinkChanged(ECheckBoxState NewState);        
    void OnAutoTalkChanged(ECheckBoxState NewState);
    ECheckBoxState GetAutoBlinkState() const;
    ECheckBoxState GetAutoTalkState() const;
    FSlateColor GetAutoBlinkColor() const;
    FSlateColor GetAutoTalkColor() const;

    // === Preview-only анимации (НЕ влияют на игровую логику) ===
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
    
    // === НОВОЕ: Preview-specific методы ===
    void SetPreviewBlinking(bool bEnable);
    void SetPreviewTalking(bool bEnable);

    // === Вспомогательные функции ===
    bool IsPreviewActorValid() const
    {
        return PreviewActor.IsValid();
    }
    
};