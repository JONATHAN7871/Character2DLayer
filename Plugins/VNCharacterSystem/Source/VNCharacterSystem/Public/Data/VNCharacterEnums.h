#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "VNCharacterEnums.generated.h"

/**
 * Перечисления для системы VN персонажей
 * 
 * Содержит все Enum'ы, используемые в системе:
 * - Цели прикрепления для Skeletal Mesh компонентов
 * - Цели прикрепления для Sprite компонентов  
 * - Идентификаторы компонентов
 * - Типы анимаций
 */

/**
 * Цель прикрепления для SkeletalMeshComponent
 * Определяет, к какому компоненту будет прикреплен Skeletal Mesh
 */
UENUM(BlueprintType)
enum class E_SkeletalAttachmentTarget : uint8
{
    None        UMETA(DisplayName = "None"),
    Body        UMETA(DisplayName = "Body")
};

/**
 * Цель прикрепления для PaperSpriteComponent
 * Определяет, к какому Skeletal Mesh компоненту будет прикреплен спрайт
 */
UENUM(BlueprintType)
enum class E_SpriteAttachmentTarget : uint8
{
    None                UMETA(DisplayName = "None"),
    Body_Skeletal       UMETA(DisplayName = "Body Skeletal"),
    Arms_Skeletal       UMETA(DisplayName = "Arms Skeletal"),
    Head_Skeletal       UMETA(DisplayName = "Head Skeletal"),
    Custom01_Skeletal   UMETA(DisplayName = "Custom01 Skeletal"),
    Custom02_Skeletal   UMETA(DisplayName = "Custom02 Skeletal"),
    Custom03_Skeletal   UMETA(DisplayName = "Custom03 Skeletal")
};

/**
 * Идентификаторы для SkeletalMeshComponent
 * Используется для быстрого доступа к конкретным компонентам
 */
UENUM(BlueprintType)
enum class E_VN_ComponentID_Skeletal : uint8
{
    Body        UMETA(DisplayName = "Body"),
    Arms        UMETA(DisplayName = "Arms"),
    Head        UMETA(DisplayName = "Head"),
    Custom01    UMETA(DisplayName = "Custom01"),
    Custom02    UMETA(DisplayName = "Custom02"),
    Custom03    UMETA(DisplayName = "Custom03")
};

/**
 * Идентификаторы для PaperSpriteComponent
 * Используется для быстрого доступа к конкретным спрайт компонентам
 */
UENUM(BlueprintType)
enum class E_VN_ComponentID_Sprite : uint8
{
    Body                UMETA(DisplayName = "Body"),
    Arms                UMETA(DisplayName = "Arms"),
    Head                UMETA(DisplayName = "Head"),
    Eyebrow             UMETA(DisplayName = "Eyebrow"),
    Eyes                UMETA(DisplayName = "Eyes"),
    Eyelids             UMETA(DisplayName = "Eyelids"),
    Wink                UMETA(DisplayName = "Wink"),
    Mouth               UMETA(DisplayName = "Mouth"),
    BodyShadow          UMETA(DisplayName = "Body Shadow"),
    EmotionHead_01      UMETA(DisplayName = "Emotion Head 01"),
    EmotionHead_02      UMETA(DisplayName = "Emotion Head 02"),
    EmotionHead_03      UMETA(DisplayName = "Emotion Head 03"),
    EmotionBody_01      UMETA(DisplayName = "Emotion Body 01"),
    EmotionBody_02      UMETA(DisplayName = "Emotion Body 02"),
    EmotionBody_03      UMETA(DisplayName = "Emotion Body 03")
};

/**
 * Типы анимаций в системе VN персонажей
 * Используется менеджером анимаций для определения типа текущей анимации
 */
UENUM(BlueprintType)
enum class EVNAnimationType : uint8
{
    None            UMETA(DisplayName = "None"),
    Transition      UMETA(DisplayName = "State Transition"),
    SpawnDespawn    UMETA(DisplayName = "Spawn/Despawn"),
    Focus           UMETA(DisplayName = "Focus Change")
};

/**
 * Состояния фокуса персонажа
 * Определяет, находится ли персонаж в фокусе или нет
 */
UENUM(BlueprintType)
enum class EVNFocusState : uint8
{
    InFocus         UMETA(DisplayName = "In Focus"),
    OutOfFocus      UMETA(DisplayName = "Out of Focus")
};

/**
 * Состояния видимости персонажа
 * Определяет текущее состояние видимости персонажа
 */
UENUM(BlueprintType)
enum class EVNVisibilityState : uint8
{
    Hidden          UMETA(DisplayName = "Hidden"),
    Visible         UMETA(DisplayName = "Visible"),
    Appearing       UMETA(DisplayName = "Appearing"),
    Disappearing    UMETA(DisplayName = "Disappearing")
};

/**
 * Эмоциональные состояния для idle анимаций
 * Влияют на частоту и характер анимаций
 */
UENUM(BlueprintType)
enum class EIdleEmotionalState : uint8
{
    None        UMETA(DisplayName = "None (Use DataAsset Settings)"),  // Использовать настройки из DataAsset
    Calm        UMETA(DisplayName = "Calm"),           // Спокойное состояние
    Nervous     UMETA(DisplayName = "Nervous"),        // Нервное состояние (быстрое моргание)
    Sleepy      UMETA(DisplayName = "Sleepy"),         // Сонное состояние (медленное моргание)
    Excited     UMETA(DisplayName = "Excited"),        // Возбужденное состояние (частое моргание)
    Focused     UMETA(DisplayName = "Focused"),        // Сосредоточенное состояние (редкое моргание)
    Tired       UMETA(DisplayName = "Tired")           // Усталое состояние (неравномерное моргание)
};