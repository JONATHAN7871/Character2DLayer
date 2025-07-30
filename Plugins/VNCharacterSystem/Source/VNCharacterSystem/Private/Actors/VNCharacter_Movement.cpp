#include "Actors/VNCharacter.h"
#include "AnimInstance/VNCharacterAnimInstance.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

// =====================================================
// УПРАВЛЕНИЕ ANIMATION BLUEPRINT
// =====================================================

void AVNCharacter::SetAnimClass(E_VN_ComponentID_Skeletal ComponentID, TSubclassOf<UAnimInstance> AnimClass)
{
    USkeletalMeshComponent* SkeletalComp = GetSkeletalComponent(ComponentID);
    if (!SkeletalComp)
    {
        VN_LOG_WARNING(TEXT("SetAnimClass: Skeletal component not found for ID %d"), (int32)ComponentID);
        return;
    }

    if (AnimClass)
    {
        SkeletalComp->SetAnimInstanceClass(AnimClass);
        VN_LOG_DEBUG(TEXT("SetAnimClass: Set AnimBP %s for component %s"), 
            *AnimClass->GetName(), *SkeletalComp->GetName());
    }
    else
    {
        SkeletalComp->SetAnimInstanceClass(nullptr);
        VN_LOG_DEBUG(TEXT("SetAnimClass: Cleared AnimBP for component %s"), *SkeletalComp->GetName());
    }
}

TSubclassOf<UAnimInstance> AVNCharacter::GetAnimClass(E_VN_ComponentID_Skeletal ComponentID) const
{
    USkeletalMeshComponent* SkeletalComp = GetSkeletalComponent(ComponentID);
    if (!SkeletalComp)
    {
        return nullptr;
    }

    return SkeletalComp->GetAnimClass();
}

void AVNCharacter::SetVNAnimInstanceForAllComponents(TSubclassOf<UVNCharacterAnimInstance> CustomVNCharacterClass)
{
    // Определяем класс для использования
    TSubclassOf<UVNCharacterAnimInstance> ClassToUse = CustomVNCharacterClass;
    if (!ClassToUse)
    {
        ClassToUse = UVNCharacterAnimInstance::StaticClass();
    }

    VN_LOG_DEBUG(TEXT("SetVNAnimInstanceForAllComponents: Setting %s for all skeletal components"), 
        *ClassToUse->GetName());

    // Устанавливаем для всех Skeletal компонентов
    TArray<E_VN_ComponentID_Skeletal> ComponentIDs = {
        E_VN_ComponentID_Skeletal::Body,
        E_VN_ComponentID_Skeletal::Arms,
        E_VN_ComponentID_Skeletal::Head,
        E_VN_ComponentID_Skeletal::Custom01,
        E_VN_ComponentID_Skeletal::Custom02,
        E_VN_ComponentID_Skeletal::Custom03
    };

    int32 SetCount = 0;
    for (E_VN_ComponentID_Skeletal ComponentID : ComponentIDs)
    {
        USkeletalMeshComponent* Component = GetSkeletalComponent(ComponentID);
        if (Component && Component->GetSkeletalMeshAsset())
        {
            Component->SetAnimInstanceClass(ClassToUse);
            SetCount++;
        }
    }

    VN_LOG_DEBUG(TEXT("SetVNAnimInstanceForAllComponents: Set AnimBP for %d components"), SetCount);
}

// =====================================================
// СИСТЕМА ПЕРЕМЕЩЕНИЯ И МАСШТАБИРОВАНИЯ
// =====================================================

void AVNCharacter::MoveTo(bool bTeleport, FVector NewLocation, bool bApplyScale, FVector NewScale, float Duration)
{
    // Останавливаем текущее движение если есть
    if (bIsMoving)
    {
        StopMovement();
    }

    if (bTeleport)
    {
        // Мгновенное перемещение
        SetActorLocation(NewLocation);
        
        if (bApplyScale)
        {
            SetActorScale3D(NewScale);
        }
        
        VN_LOG_DEBUG(TEXT("MoveTo: Teleported to %s%s"), 
            *NewLocation.ToString(), 
            bApplyScale ? *FString::Printf(TEXT(" with scale %s"), *NewScale.ToString()) : TEXT(""));
    }
    else
    {
        // Интерполированное перемещение
        StartLocation = GetActorLocation();
        TargetLocation = NewLocation;
        
        if (bApplyScale)
        {
            StartScale = GetActorScale3D();
            TargetScale = NewScale;
            bShouldInterpolateScale = true;
        }
        else
        {
            bShouldInterpolateScale = false;
        }
        
        MovementStartTime = GetWorld()->GetTimeSeconds();
        MovementDuration = FMath::Max(Duration, 0.01f); // Минимум 0.01 секунды
        bIsMoving = true;
        
        // Включаем Tick для обновления движения
        PrimaryActorTick.bCanEverTick = true;
        PrimaryActorTick.bStartWithTickEnabled = true;
        
        OnMovementStarted.Broadcast();
        
        VN_LOG_DEBUG(TEXT("MoveTo: Started interpolated movement to %s%s over %.2f seconds"), 
            *NewLocation.ToString(), 
            bApplyScale ? *FString::Printf(TEXT(" with scale %s"), *NewScale.ToString()) : TEXT(""),
            Duration);
    }
}

void AVNCharacter::MoveToActor(AActor* TargetActor, bool bTeleport, FVector LocationOffset, bool bApplyScale, FVector NewScale, float Duration)
{
    if (!TargetActor)
    {
        VN_LOG_WARNING(TEXT("MoveToActor: TargetActor is null"));
        return;
    }

    FVector FinalTargetLocation = TargetActor->GetActorLocation() + LocationOffset;
    MoveTo(bTeleport, FinalTargetLocation, bApplyScale, NewScale, Duration);
    
    VN_LOG_DEBUG(TEXT("MoveToActor: Moving to actor %s at location %s"), 
        *TargetActor->GetName(), *TargetLocation.ToString());
}

void AVNCharacter::StopMovement()
{
    if (!bIsMoving)
    {
        return;
    }

    bIsMoving = false;
    bShouldInterpolateScale = false;
    
    // Можем отключить Tick если не нужны другие обновления
    if (!IsAnimating())
    {
        PrimaryActorTick.bCanEverTick = false;
        PrimaryActorTick.bStartWithTickEnabled = false;
    }
    
    OnMovementFinished.Broadcast();
    
    VN_LOG_DEBUG(TEXT("StopMovement: Movement stopped"));
}

bool AVNCharacter::IsMoving() const
{
    return bIsMoving;
}

float AVNCharacter::GetMovementProgress() const
{
    if (!bIsMoving)
    {
        return -1.0f;
    }

    if (MovementDuration <= 0.0f)
    {
        return 1.0f;
    }

    float ElapsedTime = GetWorld()->GetTimeSeconds() - MovementStartTime;
    return FMath::Clamp(ElapsedTime / MovementDuration, 0.0f, 1.0f);
}

void AVNCharacter::UpdateMovement(float DeltaTime)
{
    if (!bIsMoving)
    {
        return;
    }

    float Progress = GetMovementProgress();
    
    // Применяем интерполяцию
    ApplyMovementInterpolation(Progress);
    
    // Уведомляем о прогрессе
    OnMovementProgress.Broadcast(Progress);
    
    // Проверяем завершение
    if (Progress >= 1.0f)
    {
        FinishMovement();
    }
}

void AVNCharacter::FinishMovement()
{
    if (!bIsMoving)
    {
        return;
    }

    // Устанавливаем финальные значения точно
    SetActorLocation(TargetLocation);
    
    if (bShouldInterpolateScale)
    {
        SetActorScale3D(TargetScale);
    }
    
    // Завершаем движение
    StopMovement();
    
    VN_LOG_DEBUG(TEXT("FinishMovement: Movement completed at %s%s"), 
        *TargetLocation.ToString(),
        bShouldInterpolateScale ? *FString::Printf(TEXT(" with scale %s"), *TargetScale.ToString()) : TEXT(""));
}

void AVNCharacter::ApplyMovementInterpolation(float Alpha)
{
    // Применяем сглаживание (ease in-out)
    float SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);
    
    // Интерполируем позицию
    FVector CurrentLocation = FMath::Lerp(StartLocation, TargetLocation, SmoothAlpha);
    SetActorLocation(CurrentLocation);
    
    // Интерполируем масштаб если нужно
    if (bShouldInterpolateScale)
    {
        FVector CurrentScale = FMath::Lerp(StartScale, TargetScale, SmoothAlpha);
        SetActorScale3D(CurrentScale);
    }
}

// =====================================================
// ОБНОВЛЕНИЕ TICK ДЛЯ ПОДДЕРЖКИ ДВИЖЕНИЯ
// =====================================================

// Добавить в существующий метод Tick или создать новый если его нет:
void AVNCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Обновляем движение
    if (bIsMoving)
    {
        UpdateMovement(DeltaTime);
    }
    
    // Если нет активных процессов, отключаем Tick
    if (!bIsMoving && !IsAnimating())
    {
        PrimaryActorTick.bCanEverTick = false;
        PrimaryActorTick.bStartWithTickEnabled = false;
    }
}