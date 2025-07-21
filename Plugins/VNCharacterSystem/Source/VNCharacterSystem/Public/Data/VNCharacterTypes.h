#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"  // ← ДОБАВЛЕНО: Для USkeletalMesh
#include "PaperSprite.h"          // ← ДОБАВЛЕНО: Для UPaperSprite
#include "VNCharacterEnums.h"
#include "VNCharacterTypes.generated.h"

/**
 * Упрощенные структуры данных для переработанной системы VN персонажей
 * 
 * ОСНОВНЫЕ ИЗМЕНЕНИЯ:
 * - Убрана структура F_VN_CharacterState (система состояний удалена)
 * - Убраны сложные конфигурации (F_VN_SkeletalConfig_Body, F_VN_SpriteConfig_Simple и т.д.)
 * - Оставлены только структуры для анимаций и простых настроек
 * - Валидация теперь работает на уровне отдельных компонентов
 */

/**
 * Структура запроса анимации для менеджера анимаций
 * Содержит всю информацию, необходимую для выполнения анимации
 */
USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API FVNAnimationRequest
{
    GENERATED_BODY()

    /** Тип анимации */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    EVNAnimationType AnimationType = EVNAnimationType::None;

    /** Длительность анимации в секундах */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", 
        meta = (ClampMin = "0.0", ClampMax = "60.0"))
    float Duration = 0.0f;

    /** Для SpawnDespawn: true = Appear, false = Disappear */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", 
        meta = (EditCondition = "AnimationType == EVNAnimationType::SpawnDespawn"))
    bool bIsSpawnAnimation = true;

    /** Для Focus: true = в фокусе, false = вне фокуса */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", 
        meta = (EditCondition = "AnimationType == EVNAnimationType::Focus"))
    bool bIsInFocus = true;

    FVNAnimationRequest()
    {
        AnimationType = EVNAnimationType::None;
        Duration = 0.0f;
        bIsSpawnAnimation = true;
        bIsInFocus = true;
    }

    /** Проверка валидности запроса */
    bool IsValid() const
    {
        return AnimationType != EVNAnimationType::None && Duration >= 0.0f;
    }

    /** Операторы сравнения */
    bool operator==(const FVNAnimationRequest& Other) const
    {
        return AnimationType == Other.AnimationType &&
               FMath::IsNearlyEqual(Duration, Other.Duration, 0.001f) &&
               bIsSpawnAnimation == Other.bIsSpawnAnimation &&
               bIsInFocus == Other.bIsInFocus;
    }

    bool operator!=(const FVNAnimationRequest& Other) const
    {
        return !(*this == Other);
    }

    /** Преобразование в строку для отладки */
    FString ToString() const
    {
        FString AnimTypeStr;
        switch (AnimationType)
        {
            case EVNAnimationType::None:
                AnimTypeStr = TEXT("None");
                break;
            case EVNAnimationType::Transition:
                AnimTypeStr = TEXT("Transition");
                break;
            case EVNAnimationType::SpawnDespawn:
                AnimTypeStr = FString::Printf(TEXT("SpawnDespawn (%s)"), 
                    bIsSpawnAnimation ? TEXT("Appear") : TEXT("Disappear"));
                break;
            case EVNAnimationType::Focus:
                AnimTypeStr = FString::Printf(TEXT("Focus (%s)"), 
                    bIsInFocus ? TEXT("In Focus") : TEXT("Out of Focus"));
                break;
        }
        
        return FString::Printf(TEXT("AnimRequest: %s, Duration: %.2f"), *AnimTypeStr, Duration);
    }
};

/**
 * Упрощенные настройки рендеринга для персонажа
 * Мобильные оптимизации без сложной LOD системы
 */
USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API FVNCharacterRenderSettings
{
    GENERATED_BODY()

    /** Отключать тени на мобильных устройствах */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mobile")
    bool bDisableShadowsOnMobile = true;

    /** Упрощенные материалы для мобильных */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mobile")
    bool bUseSimplifiedMaterialsOnMobile = true;

    /** Максимальное количество одновременно видимых компонентов */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance", 
        meta = (ClampMin = "1", ClampMax = "50"))
    int32 MaxVisibleComponents = 20;

    FVNCharacterRenderSettings()
    {
        bDisableShadowsOnMobile = true;
        bUseSimplifiedMaterialsOnMobile = true;
        MaxVisibleComponents = 20;
    }
};

/**
 * Простая информация о компоненте для отладки
 */
USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API FVNComponentInfo
{
    GENERATED_BODY()

    /** Имя компонента */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component Info")
    FString ComponentName;

    /** Тип компонента (Skeletal или Sprite) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component Info")
    FString ComponentType;

    /** Видим ли компонент */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component Info")
    bool bIsVisible = false;

    /** Есть ли контент (mesh/sprite) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component Info")
    bool bHasContent = false;

    /** Текущая альфа */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component Info")
    float CurrentAlpha = 1.0f;

    FVNComponentInfo()
    {
        ComponentName = TEXT("Unknown");
        ComponentType = TEXT("Unknown");
        bIsVisible = false;
        bHasContent = false;
        CurrentAlpha = 1.0f;
    }

    FVNComponentInfo(const FString& InName, const FString& InType, bool InVisible, bool InHasContent, float InAlpha = 1.0f)
    {
        ComponentName = InName;
        ComponentType = InType;
        bIsVisible = InVisible;
        bHasContent = InHasContent;
        CurrentAlpha = InAlpha;
    }
};

/**
 * Набор предустановленных компонентов для быстрой настройки персонажа
 * Заменяет сложную систему состояний
 */
USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API FVNCharacterPreset
{
    GENERATED_BODY()

    /** Название пресета */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
    FString PresetName = TEXT("New Preset");

    /** Описание пресета */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
    FString Description = TEXT("");

    // =====================================================
    // ОСНОВНЫЕ SKELETAL MESH АССЕТЫ
    // =====================================================

    /** Основное тело */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes", 
        meta = (AllowedClasses = "/Script/Engine.SkeletalMesh"))
    TSoftObjectPtr<USkeletalMesh> BodyMesh;

    /** Руки */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes", 
        meta = (AllowedClasses = "/Script/Engine.SkeletalMesh"))
    TSoftObjectPtr<USkeletalMesh> ArmsMesh;

    /** Голова */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Meshes", 
        meta = (AllowedClasses = "/Script/Engine.SkeletalMesh"))
    TSoftObjectPtr<USkeletalMesh> HeadMesh;

    // =====================================================
    // ОСНОВНЫЕ SPRITE АССЕТЫ
    // =====================================================

    /** Спрайт глаз */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites", 
        meta = (AllowedClasses = "/Script/Paper2D.PaperSprite"))
    TSoftObjectPtr<UPaperSprite> EyesSprite;

    /** Спрайт рта */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites", 
        meta = (AllowedClasses = "/Script/Paper2D.PaperSprite"))
    TSoftObjectPtr<UPaperSprite> MouthSprite;

    /** Спрайт бровей */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites", 
        meta = (AllowedClasses = "/Script/Paper2D.PaperSprite"))
    TSoftObjectPtr<UPaperSprite> EyebrowSprite;

    /** Спрайт тела */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites", 
        meta = (AllowedClasses = "/Script/Paper2D.PaperSprite"))
    TSoftObjectPtr<UPaperSprite> BodySprite;

    /** Спрайт рук */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprites", 
        meta = (AllowedClasses = "/Script/Paper2D.PaperSprite"))
    TSoftObjectPtr<UPaperSprite> ArmsSprite;

    FVNCharacterPreset()
    {
        PresetName = TEXT("New Preset");
        Description = TEXT("Character preset");
    }

    /**
     * Проверить, пустой ли пресет
     */
    bool IsEmpty() const
    {
        return BodyMesh.IsNull() && ArmsMesh.IsNull() && HeadMesh.IsNull() &&
               EyesSprite.IsNull() && MouthSprite.IsNull() && EyebrowSprite.IsNull() &&
               BodySprite.IsNull() && ArmsSprite.IsNull();
    }

    /**
     * Получить количество заполненных элементов
     */
    int32 GetFilledElementsCount() const
    {
        int32 Count = 0;
        
        if (!BodyMesh.IsNull()) Count++;
        if (!ArmsMesh.IsNull()) Count++;
        if (!HeadMesh.IsNull()) Count++;
        if (!EyesSprite.IsNull()) Count++;
        if (!MouthSprite.IsNull()) Count++;
        if (!EyebrowSprite.IsNull()) Count++;
        if (!BodySprite.IsNull()) Count++;
        if (!ArmsSprite.IsNull()) Count++;
        
        return Count;
    }

    /**
     * Получить краткое описание пресета
     */
    FString GetSummary() const
    {
        int32 FilledCount = GetFilledElementsCount();
        return FString::Printf(TEXT("Preset '%s': %d elements"), *PresetName, FilledCount);
    }
};

/**
 * Коллекция пресетов для персонажа
 */
USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API FVNCharacterPresetCollection
{
    GENERATED_BODY()

    /** Название коллекции */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collection")
    FString CollectionName = TEXT("Character Presets");

    /** Базовый пресет (основная поза) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collection")
    FVNCharacterPreset BasePreset;

    /** Дополнительные пресеты */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collection")
    TArray<FVNCharacterPreset> AdditionalPresets;

    FVNCharacterPresetCollection()
    {
        CollectionName = TEXT("Character Presets");
        BasePreset.PresetName = TEXT("Base Pose");
    }

    /**
     * Найти пресет по имени
     */
    FVNCharacterPreset* FindPresetByName(const FString& PresetName)
    {
        if (BasePreset.PresetName == PresetName)
        {
            return &BasePreset;
        }
        
        for (FVNCharacterPreset& Preset : AdditionalPresets)
        {
            if (Preset.PresetName == PresetName)
            {
                return &Preset;
            }
        }
        
        return nullptr;
    }

    /**
     * Получить общее количество пресетов
     */
    int32 GetTotalPresetsCount() const
    {
        return 1 + AdditionalPresets.Num(); // Base + Additional
    }
};