#include "AnimInstance/VNCharacterAnimInstance.h"
#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"

UVNCharacterAnimInstance::UVNCharacterAnimInstance()
{
    // Инициализация переменных по умолчанию
    OwnerVNCharacter = nullptr;
    bIsInFocus = true;
    bIsVisible = true;
    bIsAnimating = false;
    MovementSpeed = 0.0f;
    AnimationAlpha = 1.0f;

    // Инициализация предыдущих значений
    bPreviousInFocus = true;
    bPreviousVisible = true;
    bPreviousAnimating = false;
}

void UVNCharacterAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    // Получаем ссылку на владельца VNCharacter
    OwnerVNCharacter = Cast<AVNCharacter>(GetOwningActor());
    
    if (OwnerVNCharacter)
    {
        VN_LOG_DEBUG(TEXT("VNCharacterAnimInstance: Successfully initialized with owner %s"), 
            *OwnerVNCharacter->GetName());
        
        // Инициализируем значения от владельца
        UpdateFromOwner();
        
        // Устанавливаем предыдущие значения равными текущим
        bPreviousInFocus = bIsInFocus;
        bPreviousVisible = bIsVisible;
        bPreviousAnimating = bIsAnimating;
    }
    else
    {
        VN_LOG_WARNING(TEXT("VNCharacterAnimInstance: Owner is not a VNCharacter! Animation features will be limited."));
    }
}

void UVNCharacterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
    Super::NativeUpdateAnimation(DeltaTime);

    if (OwnerVNCharacter)
    {
        // Обновляем данные от владельца
        UpdateFromOwner();
        
        // Проверяем изменения и вызываем события
        CheckForChanges();
    }
}

void UVNCharacterAnimInstance::UpdateFromOwner()
{
    if (!OwnerVNCharacter)
        return;

    // Обновляем основные состояния
    bIsInFocus = OwnerVNCharacter->IsInFocus();
    bIsVisible = OwnerVNCharacter->IsVisible();
    bIsAnimating = OwnerVNCharacter->IsAnimating();

    // Обновляем альфу анимации (базовая логика)
    if (bIsVisible)
    {
        AnimationAlpha = 1.0f;
    }
    else
    {
        AnimationAlpha = 0.0f;
    }

    // Для будущего использования - скорость движения
    MovementSpeed = 0.0f; // Пока что статичные персонажи
}

void UVNCharacterAnimInstance::CheckForChanges()
{
    // Проверяем изменение фокуса
    if (bIsInFocus != bPreviousInFocus)
    {
        OnCharacterFocusChanged(bIsInFocus);
        bPreviousInFocus = bIsInFocus;
        
        VN_LOG_DEBUG(TEXT("VNCharacterAnimInstance: Focus changed to %s"), 
            bIsInFocus ? TEXT("true") : TEXT("false"));
    }

    // Проверяем изменение видимости
    if (bIsVisible != bPreviousVisible)
    {
        OnCharacterVisibilityChanged(bIsVisible);
        bPreviousVisible = bIsVisible;
        
        VN_LOG_DEBUG(TEXT("VNCharacterAnimInstance: Visibility changed to %s"), 
            bIsVisible ? TEXT("true") : TEXT("false"));
    }

    // Проверяем изменение состояния анимации
    if (bIsAnimating != bPreviousAnimating)
    {
        if (bIsAnimating)
        {
            OnAnimationStarted();
        }
        else
        {
            OnAnimationFinished();
        }
        bPreviousAnimating = bIsAnimating;
        
        VN_LOG_DEBUG(TEXT("VNCharacterAnimInstance: Animation state changed to %s"), 
            bIsAnimating ? TEXT("true") : TEXT("false"));
    }
}

USkeletalMeshComponent* UVNCharacterAnimInstance::GetOwnerSkeletalComponent(E_VN_ComponentID_Skeletal ComponentID) const
{
    if (!OwnerVNCharacter)
        return nullptr;

    return OwnerVNCharacter->GetSkeletalComponent(ComponentID);
}

UPaperSpriteComponent* UVNCharacterAnimInstance::GetOwnerSpriteComponent(E_VN_ComponentID_Sprite ComponentID) const
{
    if (!OwnerVNCharacter)
        return nullptr;

    return OwnerVNCharacter->GetSpriteComponent(ComponentID);
}

bool UVNCharacterAnimInstance::IsAnimationTypeActive(EVNAnimationType AnimationType) const
{
    if (!OwnerVNCharacter || !OwnerVNCharacter->GetAnimationManager())
        return false;

    return OwnerVNCharacter->GetAnimationManager()->GetCurrentAnimationType() == AnimationType;
}