#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/VNCharacterIdleAnimationStructs.h"
#include "Data/VNCharacterEnums.h"
#include "VNCharacterIdleAnimationDataAsset.generated.h"

/**
 * Упрощенный DataAsset для настроек idle анимаций
 * Содержит только базовые настройки, эмоциональные состояния применяются отдельно
 */
UCLASS(BlueprintType, Blueprintable)
class VNCHARACTERSYSTEM_API UVNCharacterIdleAnimationDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UVNCharacterIdleAnimationDataAsset();

    // =====================================================
    // ОСНОВНЫЕ НАСТРОЙКИ
    // =====================================================

    /** Эмоциональное состояние по умолчанию (None = использовать настройки из DataAsset) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
    EIdleEmotionalState DefaultEmotionalState = EIdleEmotionalState::None;

    /** Автоматически применять эти настройки при загрузке */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
    bool bAutoApplyOnLoad = true;

    // =====================================================
    // КОНФИГУРАЦИИ АНИМАЦИЙ
    // =====================================================

    /** Конфигурация анимации моргания */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blink Animation")
    FVNBlinkAnimationConfig BlinkConfig;

    /** Конфигурация анимации разговора */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talk Animation")
    FVNTalkAnimationConfig TalkConfig;

    /** Конфигурация анимации случайных движений глаз */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyes Animation")
    FVNEyesRandomAnimationConfig EyesRandomConfig;

    // =====================================================
    // БЫСТРЫЕ ПРЕСЕТЫ
    // =====================================================

    /** Пресет "Спокойный персонаж" */
    UFUNCTION(BlueprintCallable, Category = "Idle Animation Presets")
    void SetCalmPreset();

    /** Пресет "Нервный персонаж" */
    UFUNCTION(BlueprintCallable, Category = "Idle Animation Presets")
    void SetNervousPreset();

    /** Пресет "Сонный персонаж" */
    UFUNCTION(BlueprintCallable, Category = "Idle Animation Presets")
    void SetSleepyPreset();

    /** Пресет "Возбужденный персонаж" */
    UFUNCTION(BlueprintCallable, Category = "Idle Animation Presets")
    void SetExcitedPreset();

    /** Отключить все анимации */
    UFUNCTION(BlueprintCallable, Category = "Idle Animation Presets")
    void DisableAllAnimations();

    // =====================================================
    // УТИЛИТЫ
    // =====================================================

    /** Проверить валидность всех настроек */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Validation")
    bool IsValid() const;

    /** Получить список отсутствующих ресурсов */
    UFUNCTION(BlueprintCallable, Category = "Validation")
    TArray<FString> GetMissingAssets() const;

    /** Получить итоговую конфигурацию для применения */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Configuration")
    FVNIdleAnimationsConfig GetIdleAnimationsConfig() const;

    /** Получить краткое описание настроек */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Info")
    FString GetConfigSummary() const;
};