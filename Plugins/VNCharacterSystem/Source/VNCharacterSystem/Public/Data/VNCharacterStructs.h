#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Animation/AnimInstance.h"
#include "Materials/MaterialInterface.h"
#include "VNCharacterEnums.h"
#include "VNCharacterStructs.generated.h"

/**
 * Структуры данных для системы VN персонажей
 * 
 * Содержит все структуры конфигурации:
 * - Конфигурации для Skeletal Mesh компонентов
 * - Конфигурации для Sprite компонентов
 * - Главная структура состояния персонажа
 * - Вспомогательные структуры для анимаций
 */

/**
 * Конфигурация для основного тела персонажа (Skeletal Mesh)
 * Содержит все настройки для главного Skeletal Mesh компонента
 */
USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API F_VN_SkeletalConfig_Body
{
    GENERATED_BODY()

    /** Скелетная сетка для тела персонажа */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Mesh", 
        meta = (AllowedClasses = "/Script/Engine.SkeletalMesh"))
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    /** Класс AnimInstance для анимации */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TSubclassOf<UAnimInstance> AnimInstanceClass;

    /** Переопределения материалов по индексам */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    TMap<int32, TSoftObjectPtr<UMaterialInterface>> MaterialOverrides;

    /** Цвет компонента (тинт) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor Color = FLinearColor::White;

    /** Смещение относительно родительского компонента */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
    FVector Offset = FVector::ZeroVector;

    /** Масштаб компонента */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform", 
        meta = (ClampMin = "0.01", ClampMax = "10.0"))
    float Scale = 1.0f;

    /** Видимость компонента */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility")
    bool bVisible = true;

    F_VN_SkeletalConfig_Body()
    {
        SkeletalMesh = nullptr;
        AnimInstanceClass = nullptr;
        Color = FLinearColor::White;
        Offset = FVector::ZeroVector;
        Scale = 1.0f;
        bVisible = true;
    }

    /** Операторы сравнения */
    bool operator==(const F_VN_SkeletalConfig_Body& Other) const;
    bool operator!=(const F_VN_SkeletalConfig_Body& Other) const;

    /** Проверка валидности конфигурации */
    bool IsValid() const;

private:
    /** Вспомогательный метод для сравнения материалов */
    bool CompareMaterialMaps(
        const TMap<int32, TSoftObjectPtr<UMaterialInterface>>& Map1,
        const TMap<int32, TSoftObjectPtr<UMaterialInterface>>& Map2) const;
};

/**
 * Конфигурация для прикрепляемых Skeletal Mesh компонентов
 * Используется для рук, головы и дополнительных элементов
 */
USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API F_VN_SkeletalConfig_Attachment
{
    GENERATED_BODY()

    /** Скелетная сетка для прикрепляемого элемента */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skeletal Mesh", 
        meta = (AllowedClasses = "/Script/Engine.SkeletalMesh"))
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    /** Класс AnimInstance для анимации */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TSubclassOf<UAnimInstance> AnimInstanceClass;

    /** Переопределения материалов по индексам */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    TMap<int32, TSoftObjectPtr<UMaterialInterface>> MaterialOverrides;

    /** К какому компоненту прикрепить */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
    E_SkeletalAttachmentTarget AttachTo = E_SkeletalAttachmentTarget::Body;

    /** Имя сокета для прикрепления */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
    FName SocketName = NAME_None;

    /** Использовать трансформ сокета */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
    bool bUseSocketTransform = true;

    /** Цвет компонента (тинт) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor Color = FLinearColor::White;

    /** Смещение относительно точки прикрепления */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
    FVector Offset = FVector::ZeroVector;

    /** Масштаб компонента */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform", 
        meta = (ClampMin = "0.01", ClampMax = "10.0"))
    float Scale = 1.0f;

    /** Видимость компонента */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility")
    bool bVisible = true;

    F_VN_SkeletalConfig_Attachment()
    {
        SkeletalMesh = nullptr;
        AnimInstanceClass = nullptr;
        AttachTo = E_SkeletalAttachmentTarget::Body;
        SocketName = NAME_None;
        bUseSocketTransform = true;
        Color = FLinearColor::White;
        Offset = FVector::ZeroVector;
        Scale = 1.0f;
        bVisible = true;
    }

    /** Операторы сравнения */
    bool operator==(const F_VN_SkeletalConfig_Attachment& Other) const;
    bool operator!=(const F_VN_SkeletalConfig_Attachment& Other) const;

    /** Проверка валидности конфигурации */
    bool IsValid() const;

private:
    /** Вспомогательный метод для сравнения материалов */
    bool CompareMaterialMaps(
        const TMap<int32, TSoftObjectPtr<UMaterialInterface>>& Map1,
        const TMap<int32, TSoftObjectPtr<UMaterialInterface>>& Map2) const;
};

/**
 * Конфигурация для спрайтов, прикрепляемых к Skeletal Mesh
 * Используется для элементов, которые должны следовать за костями
 */
USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API F_VN_SpriteConfig_Attachment
{
    GENERATED_BODY()

    /** Спрайт для отображения */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite", 
        meta = (AllowedClasses = "/Script/Paper2D.PaperSprite"))
    TSoftObjectPtr<UPaperSprite> Sprite;

    /** К какому Skeletal Mesh компоненту прикрепить */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
    E_SpriteAttachmentTarget AttachTo = E_SpriteAttachmentTarget::Body_Skeletal;

    /** Имя сокета для прикрепления */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
    FName SocketName = NAME_None;

    /** Использовать трансформ сокета */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
    bool bUseSocketTransform = true;

    /** Смещение относительно точки прикрепления */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
    FVector Offset = FVector::ZeroVector;

    /** Масштаб спрайта */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform", 
        meta = (ClampMin = "0.01", ClampMax = "10.0"))
    float Scale = 1.0f;

    /** Цвет спрайта (тинт) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor Color = FLinearColor::White;

    /** Видимость спрайта */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility")
    bool bVisible = true;

    F_VN_SpriteConfig_Attachment()
    {
        Sprite = nullptr;
        AttachTo = E_SpriteAttachmentTarget::Body_Skeletal;
        SocketName = NAME_None;
        bUseSocketTransform = true;
        Offset = FVector::ZeroVector;
        Scale = 1.0f;
        Color = FLinearColor::White;
        bVisible = true;
    }

    /** Операторы сравнения */
    bool operator==(const F_VN_SpriteConfig_Attachment& Other) const;
    bool operator!=(const F_VN_SpriteConfig_Attachment& Other) const;

    /** Проверка валидности конфигурации */
    bool IsValid() const;
};

/**
 * Конфигурация для независимых спрайтов
 * Используется для элементов лица и эффектов головы
 */
USTRUCT(BlueprintType)
struct VNCHARACTERSYSTEM_API F_VN_SpriteConfig_Simple
{
    GENERATED_BODY()

    /** Спрайт для отображения */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprite", 
        meta = (AllowedClasses = "/Script/Paper2D.PaperSprite"))
    TSoftObjectPtr<UPaperSprite> Sprite;

    /** Смещение относительно родительского компонента */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
    FVector Offset = FVector::ZeroVector;

    /** Масштаб спрайта */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform", 
        meta = (ClampMin = "0.01", ClampMax = "10.0"))
    float Scale = 1.0f;

    /** Цвет спрайта (тинт) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor Color = FLinearColor::White;

    /** Видимость спрайта */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility")
    bool bVisible = true;

    F_VN_SpriteConfig_Simple()
    {
        Sprite = nullptr;
        Offset = FVector::ZeroVector;
        Scale = 1.0f;
        Color = FLinearColor::White;
        bVisible = true;
    }

    /** Операторы сравнения */
    bool operator==(const F_VN_SpriteConfig_Simple& Other) const;
    bool operator!=(const F_VN_SpriteConfig_Simple& Other) const;

    /** Проверка валидности конфигурации */
    bool IsValid() const;
};

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
    bool operator==(const FVNAnimationRequest& Other) const;
    bool operator!=(const FVNAnimationRequest& Other) const;

    /** Преобразование в строку для отладки */
    FString ToString() const;
};