#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "VNCharacterAnimInstance.generated.h"

class AVNCharacter;

/**
 * Специальный AnimInstance для VN персонажей
 * Содержит ссылку на владельца VNCharacter для доступа к данным персонажа
 */
UCLASS(BlueprintType, Blueprintable)
class VNCHARACTERSYSTEM_API UVNCharacterAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    UVNCharacterAnimInstance();

protected:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaTime) override;

public:
    // =====================================================
    // ОСНОВНЫЕ ПЕРЕМЕННЫЕ ДЛЯ ИСПОЛЬЗОВАНИЯ В BLUEPRINT
    // =====================================================

    /** Ссылка на владельца VNCharacter */
    UPROPERTY(BlueprintReadOnly, Category = "VN Character", meta = (BlueprintProtected = "true"))
    AVNCharacter* OwnerVNCharacter;

    /** Находится ли персонаж в фокусе */
    UPROPERTY(BlueprintReadOnly, Category = "VN Character", meta = (BlueprintProtected = "true"))
    bool bIsInFocus;

    /** Видим ли персонаж */
    UPROPERTY(BlueprintReadOnly, Category = "VN Character", meta = (BlueprintProtected = "true"))
    bool bIsVisible;

    /** Выполняется ли анимация */
    UPROPERTY(BlueprintReadOnly, Category = "VN Character", meta = (BlueprintProtected = "true"))
    bool bIsAnimating;

    /** Скорость движения персонажа (для будущего использования) */
    UPROPERTY(BlueprintReadOnly, Category = "VN Character", meta = (BlueprintProtected = "true"))
    float MovementSpeed;

    /** Альфа для анимаций (0.0 - полностью прозрачный, 1.0 - полностью непрозрачный) */
    UPROPERTY(BlueprintReadOnly, Category = "VN Character", meta = (BlueprintProtected = "true"))
    float AnimationAlpha;

    // =====================================================
    // КАСТОМНЫЕ СОБЫТИЯ ДЛЯ BLUEPRINT
    // =====================================================

    /** Событие смены фокуса персонажа */
    UFUNCTION(BlueprintImplementableEvent, Category = "VN Character Events")
    void OnCharacterFocusChanged(bool bNewInFocus);

    /** Событие смены видимости персонажа */
    UFUNCTION(BlueprintImplementableEvent, Category = "VN Character Events")
    void OnCharacterVisibilityChanged(bool bNewVisible);

    /** Событие начала анимации */
    UFUNCTION(BlueprintImplementableEvent, Category = "VN Character Events")
    void OnAnimationStarted();

    /** Событие завершения анимации */
    UFUNCTION(BlueprintImplementableEvent, Category = "VN Character Events")
    void OnAnimationFinished();

    // =====================================================
    // УТИЛИТЫ ДЛЯ BLUEPRINT
    // =====================================================

    /** Получить компонент по ID (для использования в Blueprint) */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character Helpers")
    class USkeletalMeshComponent* GetOwnerSkeletalComponent(E_VN_ComponentID_Skeletal ComponentID) const;

    /** Получить спрайт компонент по ID (для использования в Blueprint) */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character Helpers")
    class UPaperSpriteComponent* GetOwnerSpriteComponent(E_VN_ComponentID_Sprite ComponentID) const;

    /** Проверить, выполняется ли конкретный тип анимации */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VN Character Helpers")
    bool IsAnimationTypeActive(EVNAnimationType AnimationType) const;

protected:
    /** Предыдущие значения для отслеживания изменений */
    bool bPreviousInFocus;
    bool bPreviousVisible;
    bool bPreviousAnimating;

    /** Обновить данные от владельца */
    void UpdateFromOwner();

    /** Проверить изменения и вызвать события */
    void CheckForChanges();
};