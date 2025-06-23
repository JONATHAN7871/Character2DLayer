#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "TimerManager.h"

// Прямые объявления для уменьшения зависимостей в заголовке
class UCharacter2DAsset;
class ACharacter2DActor;

/**
 * Панель действий для тестирования возможностей Character2D в редакторе.
 * Эта панель напрямую взаимодействует с PreviewActor для управления анимациями в реальном времени.
 */
class SCharacter2DActionPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCharacter2DActionPanel) {}
        SLATE_ARGUMENT(UCharacter2DAsset*, CharacterAsset)
        SLATE_ARGUMENT(TWeakObjectPtr<ACharacter2DActor>, PreviewActor)
    SLATE_END_ARGS()

    /** Конструирует виджет */
    void Construct(const FArguments& InArgs);

private:
    // === ССЫЛКИ ===
    /** Редактируемый ассет, используется для изменения настроек Auto-Blink/Talk. */
    TWeakObjectPtr<UCharacter2DAsset> CharacterAsset;
    /** Актор в окне предпросмотра, которым мы управляем. */
    TWeakObjectPtr<ACharacter2DActor> PreviewActor;
    
    // === ВИДЖЕТЫ ===
    /** Чекбокс для управления свойством bAutoBlink в ассете. */
    TSharedPtr<SCheckBox> AutoBlinkCheckBox;
    /** Чекбокс для управления свойством bAutoTalk в ассете. */
    TSharedPtr<SCheckBox> AutoTalkCheckBox;

    // === СОСТОЯНИЕ UI ===
    /** Хранит состояние видимости спрайтов, установленное через эту панель. */
    bool bSpritesVisible = true;
    /** Хранит состояние видимости скелетных мешей, установленное через эту панель. */
    bool bSkeletalVisible = true;
    
private:
    // === ПОСТРОЕНИЕ UI ===
    TSharedRef<SWidget> BuildAutoAnimationsSection();      // Секция для настроек ассета (In-Game Behavior)
    TSharedRef<SWidget> BuildAnimationTestingSection();    // Секция для управления предпросмотром (Editor-Only)
    TSharedRef<SWidget> BuildVisibilityTestSection();      // Секция для управления видимостью

    // === ОБРАБОТЧИКИ ДЛЯ НАСТРОЕК АССЕТА ===
    void OnAutoBlinkChanged(ECheckBoxState NewState);        
    void OnAutoTalkChanged(ECheckBoxState NewState);
    ECheckBoxState GetAutoBlinkState() const;
    ECheckBoxState GetAutoTalkState() const;

    // === ОБРАБОТЧИКИ ДЛЯ ПРЕДПРОСМОТРА В РЕДАКТОРЕ ===
    /** Включает/выключает моргание для актора в предпросмотре. */
    void OnBlinkChanged(ECheckBoxState NewState);
    /** Включает/выключает разговор для актора в предпросмотре. */
    void OnTalkChanged(ECheckBoxState NewState);
    /** Возвращает текущее состояние моргания актора в предпросмотре. */
    ECheckBoxState GetBlinkState() const;
    /** Возвращает текущее состояние разговора актора в предпросмотре. */
    ECheckBoxState GetTalkState() const;
    /** Запускает однократное моргание. */
    FReply OnBlinkOnce();

    // === ОБРАБОТЧИКИ ВИДИМОСТИ ===
    void OnToggleSprites(ECheckBoxState NewState);
    void OnToggleSkeletal(ECheckBoxState NewState);
    ECheckBoxState GetSpritesVisibleState() const;
    ECheckBoxState GetSkeletalVisibleState() const;
    
    // === ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ===
    /** Проверяет, действителен ли актор для предпросмотра. */
    bool IsPreviewActorValid() const
    {
        return PreviewActor.IsValid();
    }
};