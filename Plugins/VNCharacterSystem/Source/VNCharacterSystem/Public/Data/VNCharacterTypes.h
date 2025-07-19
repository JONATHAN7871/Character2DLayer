#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "VNCharacterEnums.h"
#include "VNCharacterStructs.h"
#include "VNCharacterTypes.generated.h"

/**
 * Главная структура состояния персонажа
 * 
 * Содержит полную конфигурацию внешнего вида персонажа,
 * включая все Skeletal Mesh и Sprite компоненты.
 * Используется для быстрого переключения между состояниями.
 */
USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API F_VN_CharacterState
{
    GENERATED_BODY()

    /** Уникальный идентификатор состояния */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Info")
    FName StateID = NAME_None;

    /** Описание состояния (для удобства в редакторе) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Info")
    FString Description = TEXT("");

    // =====================================================
    // SKELETAL MESH CONFIGURATIONS
    // =====================================================

    /** Конфигурация основного тела */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes|Body")
    F_VN_SkeletalConfig_Body BodyConfig;

    /** Конфигурация рук */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes|Attachments")
    F_VN_SkeletalConfig_Attachment ArmsConfig;

    /** Конфигурация головы */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes|Attachments")
    F_VN_SkeletalConfig_Attachment HeadConfig;

    /** Дополнительный элемент 1 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes|Custom")
    F_VN_SkeletalConfig_Attachment Custom01Config;

    /** Дополнительный элемент 2 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes|Custom")
    F_VN_SkeletalConfig_Attachment Custom02Config;

    /** Дополнительный элемент 3 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes|Custom")
    F_VN_SkeletalConfig_Attachment Custom03Config;

    // =====================================================
    // SPRITE CONFIGURATIONS - ATTACHED TO SKELETAL
    // =====================================================

    /** Спрайт тела, прикрепленный к скелету */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites|Body Attached")
    F_VN_SpriteConfig_Attachment BodySpriteConfig;

    /** Спрайт рук, прикрепленный к скелету */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites|Body Attached")
    F_VN_SpriteConfig_Attachment ArmsSpriteConfig;

    /** Тень тела */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites|Body Attached")
    F_VN_SpriteConfig_Attachment BodyShadowSpriteConfig;

    /** Эмоциональный эффект тела 1 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites|Body Effects")
    F_VN_SpriteConfig_Attachment EmotionBodyEffect01SpriteConfig;

    /** Эмоциональный эффект тела 2 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites|Body Effects")
    F_VN_SpriteConfig_Attachment EmotionBodyEffect02SpriteConfig;

    /** Эмоциональный эффект тела 3 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites|Body Effects")
    F_VN_SpriteConfig_Attachment EmotionBodyEffect03SpriteConfig;

    // =====================================================
    // SPRITE CONFIGURATIONS - HEAD (ATTACHMENT + FACE ELEMENTS)
    // =====================================================

    /** Базовый спрайт головы - ТЕПЕРЬ может прикрепляться к скелету */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites|Head Base")
    F_VN_SpriteConfig_Attachment HeadSpriteConfig;  // ИЗМЕНЕНО: было Simple, стало Attachment

    /** Брови (всегда прикреплены к Head_Sprite) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites|Head Facial")
    F_VN_SpriteConfig_Simple EyebrowSpriteConfig;

    /** Глаза (всегда прикреплены к Head_Sprite) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites|Head Facial")
    F_VN_SpriteConfig_Simple EyesSpriteConfig;

    /** Веки (всегда прикреплены к Head_Sprite) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites|Head Facial")
    F_VN_SpriteConfig_Simple EyelidsSpriteConfig;

    /** Подмигивание (всегда прикреплено к Head_Sprite) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites|Head Facial")
    F_VN_SpriteConfig_Simple WinkSpriteConfig;

    /** Рот (всегда прикреплен к Head_Sprite) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites|Head Facial")
    F_VN_SpriteConfig_Simple MouthSpriteConfig;

    /** Эмоциональные эффекты головы (всегда прикреплены к Head_Sprite) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites|Head Effects")
    F_VN_SpriteConfig_Simple EmotionHeadEffect01SpriteConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites|Head Effects")
    F_VN_SpriteConfig_Simple EmotionHeadEffect02SpriteConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites|Head Effects")
    F_VN_SpriteConfig_Simple EmotionHeadEffect03SpriteConfig;

    // =====================================================
    // CONSTRUCTORS AND UTILITY FUNCTIONS
    // =====================================================

    F_VN_CharacterState()
    {
        StateID = NAME_None;
        Description = TEXT("New Character State");
        
        // Все конфигурации уже инициализированы своими конструкторами
    }

    /**
     * Проверка валидности состояния
     * @return true если состояние содержит валидные данные
     */
    bool IsValid() const
    {
        // Как минимум должно быть задано имя состояния
        return !StateID.IsNone();
    }

    /**
     * Создание пустого состояния с заданным ID
     * @param InStateID Идентификатор нового состояния
     * @return Новое пустое состояние
     */
    static F_VN_CharacterState CreateEmpty(const FName& InStateID)
    {
        F_VN_CharacterState NewState;
        NewState.StateID = InStateID;
        NewState.Description = FString::Printf(TEXT("State: %s"), *InStateID.ToString());
        return NewState;
    }

    /**
     * Копирование состояния с новым ID
     * @param NewStateID Новый идентификатор
     * @return Копию текущего состояния с новым ID
     */
    F_VN_CharacterState CopyWithNewID(const FName& NewStateID) const
    {
        F_VN_CharacterState NewState = *this;
        NewState.StateID = NewStateID;
        NewState.Description = FString::Printf(TEXT("Copy of %s"), *StateID.ToString());
        return NewState;
    }

    /**
     * Проверка, отличается ли это состояние от другого
     * @param Other Другое состояние для сравнения
     * @return true если состояния различаются
     */
    bool IsDifferentFrom(const F_VN_CharacterState& Other) const
    {
        // Сравниваем по StateID - это самый быстрый способ
        // В реальной реализации можно добавить более детальное сравнение
        return StateID != Other.StateID;
    }

    /** Операторы сравнения */
    bool operator==(const F_VN_CharacterState& Other) const;
    bool operator!=(const F_VN_CharacterState& Other) const;

    /** Проверка наличия видимых компонентов */
    bool HasVisibleComponents() const;

    /** Валидация всех конфигураций */
    bool ValidateAllConfigs() const;

    /** Получение детальных ошибок валидации */
    TArray<FString> GetDetailedValidationErrors() const;

    /** Получение краткой сводки о состоянии */
    FString GetSummary() const;
};

/**
 * Предустановки для быстрого создания типовых состояний
 */
USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API FVNCharacterStatePreset
{
    GENERATED_BODY()

    /** Название пресета */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
    FString PresetName = TEXT("New Preset");

    /** Базовое состояние */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
    F_VN_CharacterState BaseState;

    /** Список вариаций этого пресета */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
    TArray<F_VN_CharacterState> Variations;

    FVNCharacterStatePreset()
    {
        PresetName = TEXT("New Preset");
    }
};

/**
 * Настройки рендеринга для персонажа
 * Используется для оптимизации производительности
 */
USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API FVNCharacterRenderSettings
{
    GENERATED_BODY()

    /** Включить LOD систему */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    bool bEnableLOD = true;

    /** Расстояние для переключения LOD */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance", 
        meta = (ClampMin = "100.0", ClampMax = "5000.0", EditCondition = "bEnableLOD"))
    float LODDistance = 1000.0f;

    /** Отключать тени на мобильных устройствах */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mobile")
    bool bDisableShadowsOnMobile = true;

    /** Упрощенные материалы для мобильных */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mobile")
    bool bUseSimplifiedMaterialsOnMobile = true;

    FVNCharacterRenderSettings()
    {
        bEnableLOD = true;
        LODDistance = 1000.0f;
        bDisableShadowsOnMobile = true;
        bUseSimplifiedMaterialsOnMobile = true;
    }
};