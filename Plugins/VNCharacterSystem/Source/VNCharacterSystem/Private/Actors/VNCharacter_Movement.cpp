// VNCharacter_Movement.cpp - ИСПРАВЛЕННАЯ версия

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
// СИСТЕМА ПЕРЕМЕЩЕНИЯ И МАСШТАБИРОВАНИЯ - ИСПРАВЛЕННАЯ
// =====================================================

void AVNCharacter::MoveTo(bool bTeleport, FVector NewLocation, bool bApplyScale, FVector NewScale, float Duration)
{
    // ИСПОЛЬЗУЕМ Warning/Error вместо Fatal!
    UE_LOG(LogTemp, Error, TEXT("=== MOVETO FUNCTION CALLED ==="));
    UE_LOG(LogTemp, Warning, TEXT("MoveTo Parameters:"));
    UE_LOG(LogTemp, Warning, TEXT("  bTeleport: %s"), bTeleport ? TEXT("true") : TEXT("false"));
    UE_LOG(LogTemp, Warning, TEXT("  NewLocation: %s"), *NewLocation.ToString());
    UE_LOG(LogTemp, Warning, TEXT("  bApplyScale: %s"), bApplyScale ? TEXT("true") : TEXT("false"));
    UE_LOG(LogTemp, Warning, TEXT("  NewScale: %s"), *NewScale.ToString());
    UE_LOG(LogTemp, Warning, TEXT("  Duration: %.3f"), Duration);
    
    // Проверяем состояние актора
    UE_LOG(LogTemp, Warning, TEXT("Actor state:"));
    UE_LOG(LogTemp, Warning, TEXT("  Actor Name: %s"), *GetName());
    UE_LOG(LogTemp, Warning, TEXT("  Current Location: %s"), *GetActorLocation().ToString());
    UE_LOG(LogTemp, Warning, TEXT("  IsValid: %s"), IsValid(this) ? TEXT("true") : TEXT("false"));
    UE_LOG(LogTemp, Warning, TEXT("  IsBeingDestroyed: %s"), IsActorBeingDestroyed() ? TEXT("true") : TEXT("false"));
    UE_LOG(LogTemp, Warning, TEXT("  World: %s"), GetWorld() ? TEXT("Valid") : TEXT("NULL"));
    
    // Проверяем текущее состояние движения
    UE_LOG(LogTemp, Warning, TEXT("Movement state:"));
    UE_LOG(LogTemp, Warning, TEXT("  bIsMoving: %s"), bIsMoving ? TEXT("true") : TEXT("false"));
    UE_LOG(LogTemp, Warning, TEXT("  CanEverTick: %s"), PrimaryActorTick.bCanEverTick ? TEXT("true") : TEXT("false"));
    UE_LOG(LogTemp, Warning, TEXT("  IsTickEnabled: %s"), IsActorTickEnabled() ? TEXT("true") : TEXT("false"));
    
    // Если это телепорт - делаем его сразу
    if (bTeleport)
    {
        UE_LOG(LogTemp, Warning, TEXT("EXECUTING TELEPORT"));
        FVector OldLocation = GetActorLocation();
        SetActorLocation(NewLocation);
        FVector ActualNewLocation = GetActorLocation();
        
        UE_LOG(LogTemp, Warning, TEXT("Teleport: %s -> %s (actual: %s)"), 
            *OldLocation.ToString(), *NewLocation.ToString(), *ActualNewLocation.ToString());
        
        if (bApplyScale)
        {
            SetActorScale3D(NewScale);
            UE_LOG(LogTemp, Warning, TEXT("Scale applied: %s"), *NewScale.ToString());
        }
        
        OnMovementStarted.Broadcast();
        OnMovementFinished.Broadcast();
        UE_LOG(LogTemp, Warning, TEXT("TELEPORT COMPLETED"));
        return;
    }
    
    // Для плавного движения
    UE_LOG(LogTemp, Warning, TEXT("SETTING UP SMOOTH MOVEMENT"));
    
    // Останавливаем текущее движение
    if (bIsMoving)
    {
        UE_LOG(LogTemp, Warning, TEXT("Stopping current movement"));
        bIsMoving = false;
        bShouldInterpolateScale = false;
    }
    
    // Проверяем длительность
    if (Duration <= 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid Duration %.3f, setting to 1.0f"), Duration);
        Duration = 1.0f;
    }
    
    // Проверяем мир
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("CRITICAL ERROR: GetWorld() returned NULL!"));
        return;
    }
    
    // Устанавливаем параметры движения
    StartLocation = GetActorLocation();
    TargetLocation = NewLocation;
    MovementStartTime = World->GetTimeSeconds();
    MovementDuration = Duration;
    
    UE_LOG(LogTemp, Warning, TEXT("Movement parameters set:"));
    UE_LOG(LogTemp, Warning, TEXT("  StartLocation: %s"), *StartLocation.ToString());
    UE_LOG(LogTemp, Warning, TEXT("  TargetLocation: %s"), *TargetLocation.ToString());
    UE_LOG(LogTemp, Warning, TEXT("  StartTime: %.3f"), MovementStartTime);
    UE_LOG(LogTemp, Warning, TEXT("  Duration: %.3f"), MovementDuration);
    
    if (bApplyScale)
    {
        StartScale = GetActorScale3D();
        TargetScale = NewScale;
        bShouldInterpolateScale = true;
        UE_LOG(LogTemp, Warning, TEXT("Scale interpolation: %s -> %s"), *StartScale.ToString(), *NewScale.ToString());
    }
    else
    {
        bShouldInterpolateScale = false;
        UE_LOG(LogTemp, Warning, TEXT("No scale interpolation"));
    }
    
    // КРИТИЧЕСКИ ВАЖНО: включаем движение и тик
    bIsMoving = true;
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    SetActorTickEnabled(true);
    
    UE_LOG(LogTemp, Warning, TEXT("Movement flags set:"));
    UE_LOG(LogTemp, Warning, TEXT("  bIsMoving: %s"), bIsMoving ? TEXT("true") : TEXT("false"));
    UE_LOG(LogTemp, Warning, TEXT("  CanEverTick: %s"), PrimaryActorTick.bCanEverTick ? TEXT("true") : TEXT("false"));
    UE_LOG(LogTemp, Warning, TEXT("  IsTickEnabled: %s"), IsActorTickEnabled() ? TEXT("true") : TEXT("false"));
    
    // Отправляем событие
    OnMovementStarted.Broadcast();
    UE_LOG(LogTemp, Warning, TEXT("OnMovementStarted broadcasted"));
    
    UE_LOG(LogTemp, Error, TEXT("=== MOVETO SETUP COMPLETE ==="));
}

void AVNCharacter::MoveToActor(AActor* TargetActor, bool bTeleport, FVector LocationOffset, bool bApplyScale, FVector NewScale, float Duration)
{
    if (!TargetActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveToActor: TargetActor is null"));
        return;
    }

    FVector FinalTargetLocation = TargetActor->GetActorLocation() + LocationOffset;
    UE_LOG(LogTemp, Warning, TEXT("MoveToActor: Moving to actor %s at location %s"), 
        *TargetActor->GetName(), *FinalTargetLocation.ToString());
    
    MoveTo(bTeleport, FinalTargetLocation, bApplyScale, NewScale, Duration);
}

void AVNCharacter::StopMovement()
{
    if (!bIsMoving)
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("StopMovement: Stopping movement"));
    
    bIsMoving = false;
    bShouldInterpolateScale = false;
    
    // Проверяем, нужно ли отключить Tick
    if (!IsAnimating()) // Если нет других активных процессов
    {
        PrimaryActorTick.bCanEverTick = false;
        PrimaryActorTick.bStartWithTickEnabled = false;
        SetActorTickEnabled(false);
        UE_LOG(LogTemp, Log, TEXT("StopMovement: Tick disabled"));
    }
    
    OnMovementFinished.Broadcast();
    UE_LOG(LogTemp, Warning, TEXT("StopMovement: Movement stopped"));
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

    UWorld* World = GetWorld();
    if (!World)
    {
        return -1.0f;
    }

    float ElapsedTime = World->GetTimeSeconds() - MovementStartTime;
    return FMath::Clamp(ElapsedTime / MovementDuration, 0.0f, 1.0f);
}


void AVNCharacter::UpdateMovement(float DeltaTime)
{
    UE_LOG(LogTemp, Error, TEXT("=== UpdateMovement CALLED ==="));
    UE_LOG(LogTemp, Error, TEXT("UpdateMovement: bIsMoving=%s"), bIsMoving ? TEXT("true") : TEXT("false"));
    
    if (!bIsMoving)
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateMovement: bIsMoving is false, returning"));
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateMovement: World is NULL"));
        StopMovement();
        return;
    }

    float CurrentTime = World->GetTimeSeconds();
    float ElapsedTime = CurrentTime - MovementStartTime;
    float Progress = FMath::Clamp(ElapsedTime / MovementDuration, 0.0f, 1.0f);
    
    UE_LOG(LogTemp, Error, TEXT("UpdateMovement: CurrentTime=%.3f, StartTime=%.3f, ElapsedTime=%.3f"), 
        CurrentTime, MovementStartTime, ElapsedTime);
    UE_LOG(LogTemp, Error, TEXT("UpdateMovement: Duration=%.3f, Progress=%.3f"), MovementDuration, Progress);
    
    // Применяем интерполяцию
    float SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, Progress);
    FVector CurrentLocation = FMath::Lerp(StartLocation, TargetLocation, SmoothAlpha);
    
    UE_LOG(LogTemp, Warning, TEXT("UpdateMovement: SmoothAlpha=%.3f"), SmoothAlpha);
    UE_LOG(LogTemp, Warning, TEXT("UpdateMovement: StartLocation=%s"), *StartLocation.ToString());
    UE_LOG(LogTemp, Warning, TEXT("UpdateMovement: TargetLocation=%s"), *TargetLocation.ToString());
    UE_LOG(LogTemp, Warning, TEXT("UpdateMovement: CurrentLocation=%s"), *CurrentLocation.ToString());
    
    // ВАЖНО: Проверяем, что локация действительно устанавливается
    FVector BeforeLocation = GetActorLocation();
    SetActorLocation(CurrentLocation);
    FVector AfterLocation = GetActorLocation();
    
    UE_LOG(LogTemp, Warning, TEXT("UpdateMovement: Location BEFORE SetActorLocation: %s"), *BeforeLocation.ToString());
    UE_LOG(LogTemp, Warning, TEXT("UpdateMovement: Location AFTER SetActorLocation: %s"), *AfterLocation.ToString());
    
    // Интерполируем масштаб если нужно
    if (bShouldInterpolateScale)
    {
        FVector CurrentScale = FMath::Lerp(StartScale, TargetScale, SmoothAlpha);
        SetActorScale3D(CurrentScale);
        UE_LOG(LogTemp, Log, TEXT("UpdateMovement: Scale applied: %s"), *CurrentScale.ToString());
    }
    
    // Уведомляем о прогрессе
    OnMovementProgress.Broadcast(Progress);
    
    // Проверяем завершение
    if (Progress >= 1.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateMovement: Movement completed (Progress=%.3f), calling FinishMovement"), Progress);
        FinishMovement();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("UpdateMovement: Movement continuing (Progress=%.3f)"), Progress);
    }
}

void AVNCharacter::FinishMovement()
{
    if (!bIsMoving)
    {
        return; // Уже завершено
    }

    UE_LOG(LogTemp, Warning, TEXT("FinishMovement: Setting final location to %s"), *TargetLocation.ToString());
    
    // Устанавливаем финальные значения точно
    SetActorLocation(TargetLocation);
    
    if (bShouldInterpolateScale)
    {
        SetActorScale3D(TargetScale);
        UE_LOG(LogTemp, Warning, TEXT("FinishMovement: Set final scale to %s"), *TargetScale.ToString());
    }
    
    // Завершаем движение
    bIsMoving = false;
    bShouldInterpolateScale = false;
    
    // Проверяем, нужно ли отключить Tick
    if (!IsAnimating()) // Если нет других активных процессов
    {
        PrimaryActorTick.bCanEverTick = false;
        PrimaryActorTick.bStartWithTickEnabled = false;
        SetActorTickEnabled(false);
        UE_LOG(LogTemp, Log, TEXT("FinishMovement: Tick disabled"));
    }
    
    OnMovementFinished.Broadcast();
    UE_LOG(LogTemp, Warning, TEXT("FinishMovement: Movement finished, OnMovementFinished broadcasted"));
}

void AVNCharacter::ApplyMovementInterpolation(float Alpha)
{
    // Эта функция больше не используется, логика перенесена в UpdateMovement
    // Оставляем для совместимости
    float SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);
    FVector CurrentLocation = FMath::Lerp(StartLocation, TargetLocation, SmoothAlpha);
    SetActorLocation(CurrentLocation);
    
    if (bShouldInterpolateScale)
    {
        FVector CurrentScale = FMath::Lerp(StartScale, TargetScale, SmoothAlpha);
        SetActorScale3D(CurrentScale);
    }
}

// =====================================================
// ОБНОВЛЕНИЕ TICK ДЛЯ ПОДДЕРЖКИ ДВИЖЕНИЯ - ИСПРАВЛЕННАЯ
// =====================================================

void AVNCharacter::Tick(float DeltaTime)
{
    // ВСЕГДА логируем первые несколько вызовов, чтобы убедиться что Tick работает
    static int32 TickCallCount = 0;
    TickCallCount++;
    
    if (TickCallCount <= 5) // Первые 5 вызовов
    {
        UE_LOG(LogTemp, Error, TEXT("=== TICK CALLED #%d ==="), TickCallCount);
        UE_LOG(LogTemp, Error, TEXT("Tick: DeltaTime=%.4f"), DeltaTime);
        UE_LOG(LogTemp, Error, TEXT("Tick: bIsMoving=%s"), bIsMoving ? TEXT("true") : TEXT("false"));
        UE_LOG(LogTemp, Error, TEXT("Tick: IsAnimating=%s"), IsAnimating() ? TEXT("true") : TEXT("false"));
    }
    
    // ОБЯЗАТЕЛЬНО вызываем Super::Tick
    Super::Tick(DeltaTime);
    
    // Логируем каждый раз когда движемся
    if (bIsMoving)
    {
        UE_LOG(LogTemp, Error, TEXT("TICK: Movement active, calling UpdateMovement"));
        UpdateMovement(DeltaTime);
        
        // Дополнительная информация о прогрессе
        float Progress = GetMovementProgress();
        UE_LOG(LogTemp, Warning, TEXT("TICK: Movement Progress=%.3f"), Progress);
    }
    else if (TickCallCount <= 10) // Логируем первые 10 тиков даже если не движемся
    {
        UE_LOG(LogTemp, Log, TEXT("Tick #%d: Not moving"), TickCallCount);
    }
}