#include "Components/VNCharacterIdleAnimationManager.h"
#include "Actors/VNCharacter.h"
#include "VNCharacterSystemModule.h"
#include "PaperSpriteComponent.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "Engine/World.h"
#include "TimerManager.h"

UVNCharacterIdleAnimationManager::UVNCharacterIdleAnimationManager()
{
    // Включаем тик для обновления анимаций
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;

    // Инициализация значений по умолчанию
    bDisableIdleAnimations = false;
    bVerboseLogging = false;
    
    // Инициализация состояния анимаций
    bIsBlinkAnimationPlaying = false;
    bIsEyesRandomAnimationPlaying = false;
    CurrentBlinkState = EBlinkState::WaitingForBlink;
    bPendingDoubleBlink = false;
}

void UVNCharacterIdleAnimationManager::BeginPlay()
{
    Super::BeginPlay();

    // Кэшируем ссылку на владельца
    OwnerCharacter = Cast<AVNCharacter>(GetOwner());
    
    if (!OwnerCharacter.IsValid())
    {
        VN_LOG_ERROR(TEXT("VNCharacterIdleAnimationManager: Owner is not a VNCharacter! Component will not function properly."));
        return;
    }

    LogIdleAnimation(TEXT("Idle Animation Manager initialized successfully"));

    // Запускаем активные idle анимации
    StartAllIdleAnimations();
}

void UVNCharacterIdleAnimationManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // В отличие от основных анимаций, idle анимации работают через таймеры
    // Тик используется только для проверки состояния и отладки
    if (bVerboseLogging)
    {
        static float LogTimer = 0.0f;
        LogTimer += DeltaTime;
        if (LogTimer >= 2.0f) // Логируем каждые 2 секунды
        {
            LogIdleAnimation(FString::Printf(TEXT("Active idle animations: Blink=%s, Talk=%s, EyesRandom=%s"), 
                IsBlinkActive() ? TEXT("ON") : TEXT("OFF"),
                IsTalkActive() ? TEXT("ON") : TEXT("OFF"),
                IsEyesRandomActive() ? TEXT("ON") : TEXT("OFF")));
            LogTimer = 0.0f;
        }
    }
}

// =====================================================
// ПУБЛИЧНЫЕ МЕТОДЫ
// =====================================================

void UVNCharacterIdleAnimationManager::SetBlinkEnabled(bool bEnable)
{
    if (IdleAnimationsConfig.BlinkConfig.bEnabled == bEnable)
    {
        return; // Состояние не изменилось
    }

    IdleAnimationsConfig.BlinkConfig.bEnabled = bEnable;
    
    if (bEnable)
    {
        LogIdleAnimation(TEXT("Blink animation enabled"));
        StartBlinkAnimation();
    }
    else
    {
        LogIdleAnimation(TEXT("Blink animation disabled"));
        StopBlinkAnimation();
    }
}

void UVNCharacterIdleAnimationManager::SetTalkEnabled(bool bEnable)
{
    if (IdleAnimationsConfig.TalkConfig.bEnabled == bEnable)
    {
        return; // Состояние не изменилось
    }

    IdleAnimationsConfig.TalkConfig.bEnabled = bEnable;
    
    if (bEnable)
    {
        LogIdleAnimation(TEXT("Talk animation enabled"));
        StartTalkAnimation();
    }
    else
    {
        LogIdleAnimation(TEXT("Talk animation disabled"));
        StopTalkAnimation();
    }
}

void UVNCharacterIdleAnimationManager::SetEyesRandomEnabled(bool bEnable)
{
    if (IdleAnimationsConfig.EyesRandomConfig.bEnabled == bEnable)
    {
        return; // Состояние не изменилось
    }

    IdleAnimationsConfig.EyesRandomConfig.bEnabled = bEnable;
    
    if (bEnable)
    {
        LogIdleAnimation(TEXT("Eyes random animation enabled"));
        StartEyesRandomAnimation();
    }
    else
    {
        LogIdleAnimation(TEXT("Eyes random animation disabled"));
        StopEyesRandomAnimation();
    }
}

void UVNCharacterIdleAnimationManager::SetIdleAnimationsConfig(const FVNIdleAnimationsConfig& NewConfig)
{
    // Останавливаем все текущие анимации
    StopAllIdleAnimations();
    
    // Устанавливаем новую конфигурацию
    IdleAnimationsConfig = NewConfig;
    
    LogIdleAnimation(TEXT("Idle animations config updated"));
    
    // Запускаем анимации согласно новой конфигурации
    StartAllIdleAnimations();
}

void UVNCharacterIdleAnimationManager::StopAllIdleAnimations()
{
    StopBlinkAnimation();
    StopTalkAnimation();
    StopEyesRandomAnimation();
    LogIdleAnimation(TEXT("All idle animations stopped"));
}

void UVNCharacterIdleAnimationManager::StartAllIdleAnimations()
{
    if (bDisableIdleAnimations)
    {
        LogIdleAnimation(TEXT("Idle animations disabled by debug flag"));
        return;
    }

    if (IdleAnimationsConfig.BlinkConfig.bEnabled)
    {
        StartBlinkAnimation();
    }
    
    if (IdleAnimationsConfig.TalkConfig.bEnabled)
    {
        StartTalkAnimation();
    }
    
    if (IdleAnimationsConfig.EyesRandomConfig.bEnabled)
    {
        StartEyesRandomAnimation();
    }
    
    LogIdleAnimation(FString::Printf(TEXT("Started %d idle animations"), 
        IdleAnimationsConfig.GetActiveAnimationsCount()));
}

// =====================================================
// МЕТОДЫ ДЛЯ АНИМАЦИИ МОРГАНИЯ
// =====================================================

void UVNCharacterIdleAnimationManager::StartBlinkAnimation()
{
    if (!IdleAnimationsConfig.BlinkConfig.IsValid())
    {
        LogIdleAnimation(TEXT("Cannot start blink animation: invalid config"));
        return;
    }

    if (bIsBlinkAnimationPlaying)
    {
        LogIdleAnimation(TEXT("Blink animation already playing"));
        return;
    }

    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyelids_Sprite)
    {
        LogIdleAnimation(TEXT("Cannot start blink animation: invalid character or eyelids component"));
        return;
    }

    // Сохраняем исходный спрайт век
    SaveOriginalSprite(Character->Eyelids_Sprite, OriginalEyelidsSprite);
    
    LogIdleAnimation(TEXT("Blink animation started"));

    // Сбрасываем состояние и запланируем первое моргание
    CurrentBlinkState = EBlinkState::WaitingForBlink;
    ScheduleNextBlink();
}

void UVNCharacterIdleAnimationManager::StopBlinkAnimation()
{
    if (!bIsBlinkAnimationPlaying && !GetWorld()->GetTimerManager().IsTimerActive(BlinkTimerHandle))
    {
        return; // Анимация уже остановлена
    }

    // Очищаем таймер
    GetWorld()->GetTimerManager().ClearTimer(BlinkTimerHandle);
    
    // Завершаем текущую анимацию моргания если она выполняется
    if (bIsBlinkAnimationPlaying)
    {
        FinishBlinkAnimation();
    }
    
    // Сбрасываем состояние
    CurrentBlinkState = EBlinkState::WaitingForBlink;
    bPendingDoubleBlink = false;
    
    LogIdleAnimation(TEXT("Blink animation stopped"));
}

void UVNCharacterIdleAnimationManager::ExecuteBlink()
{
    UE_LOG(LogTemp, Warning, TEXT("=== ExecuteBlink CALLED ==="));
    
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyelids_Sprite)
    {
        UE_LOG(LogTemp, Error, TEXT("ExecuteBlink: Invalid character or eyelids component"));
        return;
    }

    UPaperFlipbook* BlinkFlipbook = IdleAnimationsConfig.BlinkConfig.BlinkFlipbook.LoadSynchronous();
    if (!BlinkFlipbook)
    {
        UE_LOG(LogTemp, Error, TEXT("ExecuteBlink: Invalid flipbook"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("ExecuteBlink: Flipbook loaded successfully: %s"), *BlinkFlipbook->GetName());
    UE_LOG(LogTemp, Warning, TEXT("ExecuteBlink: Flipbook duration: %.3f"), BlinkFlipbook->GetTotalDuration());

    // Проверяем наличие спрайтов
    UPaperSprite* Sprite0 = GetSpriteFromFlipbook(BlinkFlipbook, 0);
    UPaperSprite* Sprite1 = GetSpriteFromFlipbook(BlinkFlipbook, 1);
    
    UE_LOG(LogTemp, Warning, TEXT("ExecuteBlink: Sprite 0: %s"), Sprite0 ? *Sprite0->GetName() : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("ExecuteBlink: Sprite 1: %s"), Sprite1 ? *Sprite1->GetName() : TEXT("NULL"));
    
    if (!Sprite0 || !Sprite1)
    {
        UE_LOG(LogTemp, Error, TEXT("ExecuteBlink: Cannot get sprites from flipbook!"));
        return;
    }

    // Определяем, будет ли двойное моргание
    bPendingDoubleBlink = IdleAnimationsConfig.BlinkConfig.ShouldDoubleBlink();
    
    UE_LOG(LogTemp, Warning, TEXT("ExecuteBlink: Double blink: %s"), bPendingDoubleBlink ? TEXT("YES") : TEXT("NO"));

    bIsBlinkAnimationPlaying = true;
    CurrentBlinkState = EBlinkState::FirstBlinkHalf;
    
    UE_LOG(LogTemp, Warning, TEXT("ExecuteBlink: Starting blink animation"));
    
    // Запускаем первую фазу моргания
    UpdateBlinkState();
}

void UVNCharacterIdleAnimationManager::UpdateBlinkState()
{
    UE_LOG(LogTemp, Warning, TEXT("=== UpdateBlinkState: State = %d ==="), (int32)CurrentBlinkState);
    
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyelids_Sprite)
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateBlinkState: Invalid character or eyelids"));
        FinishBlinkAnimation();
        return;
    }

    UPaperFlipbook* BlinkFlipbook = IdleAnimationsConfig.BlinkConfig.BlinkFlipbook.LoadSynchronous();
    if (!BlinkFlipbook)
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateBlinkState: Invalid flipbook"));
        FinishBlinkAnimation();
        return;
    }

    // Получаем спрайты из flipbook
    UPaperSprite* HalfClosedSprite = GetSpriteFromFlipbook(BlinkFlipbook, 0); // Полузакрытые глаза
    UPaperSprite* ClosedSprite = GetSpriteFromFlipbook(BlinkFlipbook, 1);     // Закрытые глаза
    
    if (!HalfClosedSprite || !ClosedSprite)
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateBlinkState: Cannot get sprites from flipbook"));
        FinishBlinkAnimation();
        return;
    }

    float BlinkDuration = IdleAnimationsConfig.BlinkConfig.BlinkDuration;
    float DoubleBlinkPause = IdleAnimationsConfig.BlinkConfig.DoubleBlinkPause;
    
    UE_LOG(LogTemp, Warning, TEXT("UpdateBlinkState: BlinkDuration=%.3f, DoubleBlinkPause=%.3f"), 
        BlinkDuration, DoubleBlinkPause);

    switch (CurrentBlinkState)
    {
        case EBlinkState::FirstBlinkHalf:
        {
            UE_LOG(LogTemp, Warning, TEXT("UpdateBlinkState: Setting half-closed sprite"));
            Character->Eyelids_Sprite->SetSprite(HalfClosedSprite);
            CurrentBlinkState = EBlinkState::FirstBlinkFull;
            
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle,
                this,
                &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                BlinkDuration * 0.5f,
                false
            );
            break;
        }
        
        case EBlinkState::FirstBlinkFull:
        {
            UE_LOG(LogTemp, Warning, TEXT("UpdateBlinkState: Setting closed sprite"));
            Character->Eyelids_Sprite->SetSprite(ClosedSprite);
            
            if (bPendingDoubleBlink)
            {
                CurrentBlinkState = EBlinkState::BetweenBlinks;
                UE_LOG(LogTemp, Warning, TEXT("UpdateBlinkState: Preparing for double blink"));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("UpdateBlinkState: Single blink, finishing"));
                GetWorld()->GetTimerManager().SetTimer(
                    BlinkTimerHandle,
                    this,
                    &UVNCharacterIdleAnimationManager::FinishBlinkAnimation,
                    BlinkDuration * 0.5f,
                    false
                );
                return;
            }
            
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle,
                this,
                &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                BlinkDuration * 0.5f,
                false
            );
            break;
        }
        
        case EBlinkState::BetweenBlinks:
        {
            UE_LOG(LogTemp, Warning, TEXT("UpdateBlinkState: Returning to original between blinks"));
            RestoreOriginalSprite(Character->Eyelids_Sprite, OriginalEyelidsSprite);
            CurrentBlinkState = EBlinkState::SecondBlinkHalf;
            
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle,
                this,
                &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                DoubleBlinkPause,
                false
            );
            break;
        }
        
        case EBlinkState::SecondBlinkHalf:
        {
            UE_LOG(LogTemp, Warning, TEXT("UpdateBlinkState: Second blink - half closed"));
            Character->Eyelids_Sprite->SetSprite(HalfClosedSprite);
            CurrentBlinkState = EBlinkState::SecondBlinkFull;
            
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle,
                this,
                &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                BlinkDuration * 0.5f,
                false
            );
            break;
        }
        
        case EBlinkState::SecondBlinkFull:
        {
            UE_LOG(LogTemp, Warning, TEXT("UpdateBlinkState: Second blink - fully closed"));
            Character->Eyelids_Sprite->SetSprite(ClosedSprite);
            
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle,
                this,
                &UVNCharacterIdleAnimationManager::FinishBlinkAnimation,
                BlinkDuration * 0.5f,
                false
            );
            break;
        }
        
        default:
            UE_LOG(LogTemp, Error, TEXT("UpdateBlinkState: Unknown state"));
            FinishBlinkAnimation();
            break;
    }
}

void UVNCharacterIdleAnimationManager::FinishBlinkAnimation()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Eyelids_Sprite)
    {
        // Возвращаем исходный спрайт
        RestoreOriginalSprite(Character->Eyelids_Sprite, OriginalEyelidsSprite);
    }

    bIsBlinkAnimationPlaying = false;
    CurrentBlinkState = EBlinkState::WaitingForBlink;
    bPendingDoubleBlink = false;
    
    LogIdleAnimation(TEXT("Blink animation finished"));
    
    // Планируем следующее моргание если анимация все еще включена
    if (IdleAnimationsConfig.BlinkConfig.bEnabled)
    {
        ScheduleNextBlink();
    }
}

void UVNCharacterIdleAnimationManager::ScheduleNextBlink()
{
    if (!IdleAnimationsConfig.BlinkConfig.bEnabled)
    {
        UE_LOG(LogTemp, Warning, TEXT("ScheduleNextBlink: Blink disabled, not scheduling"));
        return;
    }

    float NextBlinkDelay = IdleAnimationsConfig.BlinkConfig.GetRandomBlinkInterval();
    
    UE_LOG(LogTemp, Warning, TEXT("ScheduleNextBlink: Next blink in %.2f seconds"), NextBlinkDelay);
    
    GetWorld()->GetTimerManager().SetTimer(
        BlinkTimerHandle,
        this,
        &UVNCharacterIdleAnimationManager::ExecuteBlink,
        NextBlinkDelay,
        false
    );
}

// =====================================================
// МЕТОДЫ ДЛЯ АНИМАЦИИ РАЗГОВОРА
// =====================================================

void UVNCharacterIdleAnimationManager::StartTalkAnimation()
{
    if (!IdleAnimationsConfig.TalkConfig.IsValid())
    {
        LogIdleAnimation(TEXT("Cannot start talk animation: invalid config"));
        return;
    }

    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Mouth_Sprite)
    {
        LogIdleAnimation(TEXT("Cannot start talk animation: invalid character or mouth component"));
        return;
    }

    // Сохраняем исходный спрайт рта
    SaveOriginalSprite(Character->Mouth_Sprite, OriginalMouthSprite);
    
    LogIdleAnimation(TEXT("Talk animation started"));

    // Запускаем периодическое обновление
    float FrameInterval = IdleAnimationsConfig.TalkConfig.GetFrameInterval();
    GetWorld()->GetTimerManager().SetTimer(
        TalkTimerHandle,
        this,
        &UVNCharacterIdleAnimationManager::UpdateTalkFrame,
        FrameInterval,
        true // Повторять
    );
}

void UVNCharacterIdleAnimationManager::StopTalkAnimation()
{
    // Очищаем таймер
    GetWorld()->GetTimerManager().ClearTimer(TalkTimerHandle);
    
    // Восстанавливаем исходный спрайт рта
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Mouth_Sprite)
    {
        RestoreOriginalSprite(Character->Mouth_Sprite, OriginalMouthSprite);
    }
    
    LogIdleAnimation(TEXT("Talk animation stopped"));
}

void UVNCharacterIdleAnimationManager::UpdateTalkFrame()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Mouth_Sprite)
    {
        StopTalkAnimation();
        return;
    }

    UPaperFlipbook* TalkFlipbook = IdleAnimationsConfig.TalkConfig.TalkFlipbook.LoadSynchronous();
    if (!TalkFlipbook)
    {
        StopTalkAnimation();
        return;
    }

    // Получаем случайный кадр из flipbook
    UPaperSprite* RandomSprite = GetRandomSpriteFromFlipbook(TalkFlipbook, false);
    if (RandomSprite)
    {
        Character->Mouth_Sprite->SetSprite(RandomSprite);
        LogIdleAnimation(TEXT("Talk frame updated to random sprite"), false);
    }
}

// =====================================================
// МЕТОДЫ ДЛЯ АНИМАЦИИ СЛУЧАЙНЫХ ДВИЖЕНИЙ ГЛАЗ
// =====================================================

void UVNCharacterIdleAnimationManager::StartEyesRandomAnimation()
{
    if (!IdleAnimationsConfig.EyesRandomConfig.IsValid())
    {
        LogIdleAnimation(TEXT("Cannot start eyes random animation: invalid config"));
        return;
    }

    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyes_Sprite)
    {
        LogIdleAnimation(TEXT("Cannot start eyes random animation: invalid character or eyes component"));
        return;
    }

    // Сохраняем исходный спрайт глаз
    SaveOriginalSprite(Character->Eyes_Sprite, OriginalEyesSprite);
    
    LogIdleAnimation(TEXT("Eyes random animation started"));

    // Запланируем первое движение глаз
    ScheduleNextEyesMovement();
}

void UVNCharacterIdleAnimationManager::StopEyesRandomAnimation()
{
    if (!bIsEyesRandomAnimationPlaying && !GetWorld()->GetTimerManager().IsTimerActive(EyesRandomTimerHandle))
    {
        return; // Анимация уже остановлена
    }

    // Очищаем таймер
    GetWorld()->GetTimerManager().ClearTimer(EyesRandomTimerHandle);
    
    // Восстанавливаем исходный спрайт глаз
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Eyes_Sprite)
    {
        RestoreOriginalSprite(Character->Eyes_Sprite, OriginalEyesSprite);
    }

    bIsEyesRandomAnimationPlaying = false;
    LogIdleAnimation(TEXT("Eyes random animation stopped"));
}

void UVNCharacterIdleAnimationManager::ExecuteRandomEyesMovement()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyes_Sprite)
    {
        return;
    }

    UPaperFlipbook* EyesFlipbook = IdleAnimationsConfig.EyesRandomConfig.EyesDirectionsFlipbook.LoadSynchronous();
    if (!EyesFlipbook)
    {
        LogIdleAnimation(TEXT("Cannot execute eyes movement: invalid flipbook"));
        return;
    }

    // Получаем любой случайный кадр из flipbook (включая кадр 0)
    UPaperSprite* RandomDirection = GetRandomSpriteFromFlipbook(EyesFlipbook, false);
    if (RandomDirection)
    {
        Character->Eyes_Sprite->SetSprite(RandomDirection);
        bIsEyesRandomAnimationPlaying = true;
        
        LogIdleAnimation(TEXT("Eyes direction changed to random sprite"));
        
        // Планируем возврат к исходному взгляду
        float LookDuration = IdleAnimationsConfig.EyesRandomConfig.GetRandomLookDuration();
        GetWorld()->GetTimerManager().SetTimer(
            EyesRandomTimerHandle,
            this,
            &UVNCharacterIdleAnimationManager::ReturnEyesToOriginal,
            LookDuration,
            false
        );
    }
}

void UVNCharacterIdleAnimationManager::ReturnEyesToOriginal()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Eyes_Sprite)
    {
        // Возвращаем исходный взгляд
        RestoreOriginalSprite(Character->Eyes_Sprite, OriginalEyesSprite);
        LogIdleAnimation(TEXT("Eyes returned to original position"));
    }

    bIsEyesRandomAnimationPlaying = false;
    
    // Планируем следующее движение глаз если анимация все еще включена
    if (IdleAnimationsConfig.EyesRandomConfig.bEnabled)
    {
        ScheduleNextEyesMovement();
    }
}

void UVNCharacterIdleAnimationManager::ScheduleNextEyesMovement()
{
    if (!IdleAnimationsConfig.EyesRandomConfig.bEnabled)
    {
        return;
    }

    float NextMovementDelay = IdleAnimationsConfig.EyesRandomConfig.GetRandomWaitDuration();
    
    GetWorld()->GetTimerManager().SetTimer(
        EyesRandomTimerHandle,
        this,
        &UVNCharacterIdleAnimationManager::ExecuteRandomEyesMovement,
        NextMovementDelay,
        false
    );
    
    LogIdleAnimation(FString::Printf(TEXT("Next eyes movement scheduled in %.2f seconds"), NextMovementDelay));
}

// =====================================================
// УТИЛИТЫ ДЛЯ РАБОТЫ С FLIPBOOK
// =====================================================

UPaperSprite* UVNCharacterIdleAnimationManager::GetSpriteFromFlipbook(UPaperFlipbook* Flipbook, int32 FrameIndex) const
{
    if (!Flipbook)
    {
        return nullptr;
    }

    // ИСПРАВЛЕНИЕ: Используем другой подход для получения спрайтов
    // Проверяем, есть ли у flipbook метод GetSpriteKeyFrames (может отличаться в разных версиях UE)
    
    // Попробуем получить спрайт по времени (для UE 5.5)
    float TotalDuration = Flipbook->GetTotalDuration();
    if (TotalDuration <= 0.0f)
    {
        LogIdleAnimation(TEXT("GetSpriteFromFlipbook: Flipbook has zero duration"), true);
        return nullptr;
    }
    
    // Для моргания нам нужно только 2 кадра, поэтому упростим логику
    if (FrameIndex == 0)
    {
        // Первый кадр - в начале flipbook
        return Flipbook->GetSpriteAtTime(0.0f);
    }
    else if (FrameIndex == 1)
    {
        // Второй кадр - в середине или в конце flipbook
        float MidTime = TotalDuration * 0.5f;
        UPaperSprite* MidSprite = Flipbook->GetSpriteAtTime(MidTime);
        if (!MidSprite)
        {
            // Если нет спрайта в середине, попробуем в конце
            return Flipbook->GetSpriteAtTime(TotalDuration - 0.01f);
        }
        return MidSprite;
    }
    
    // Для остальных кадров
    float FrameTime = (TotalDuration / 4.0f) * FrameIndex; // Предполагаем 4 кадра максимум
    return Flipbook->GetSpriteAtTime(FrameTime);
}

int32 UVNCharacterIdleAnimationManager::GetFlipbookFrameCount(UPaperFlipbook* Flipbook) const
{
    if (!Flipbook)
    {
        return 0;
    }

    // Оцениваем количество кадров на основе длительности
    float TotalDuration = Flipbook->GetTotalDuration();
    if (TotalDuration <= 0.0f)
    {
        return 0;
    }
    
    // Предполагаем стандартную частоту кадров
    float FrameRate = 12.0f;
    return FMath::CeilToInt(TotalDuration * FrameRate);
}

UPaperSprite* UVNCharacterIdleAnimationManager::GetRandomSpriteFromFlipbook(UPaperFlipbook* Flipbook, bool bExcludeFirstFrame) const
{
    if (!Flipbook)
    {
        return nullptr;
    }

    int32 TotalFrames = GetFlipbookFrameCount(Flipbook);
    if (TotalFrames <= 0)
    {
        return nullptr;
    }

    int32 StartIndex = bExcludeFirstFrame ? 1 : 0;
    int32 MaxIndex = TotalFrames - 1;
    
    if (StartIndex > MaxIndex)
    {
        return nullptr; // Нет доступных кадров
    }

    int32 RandomIndex = FMath::RandRange(StartIndex, MaxIndex);
    return GetSpriteFromFlipbook(Flipbook, RandomIndex);
}

// =====================================================
// МЕТОДЫ ДЛЯ СОХРАНЕНИЯ И ВОССТАНОВЛЕНИЯ ИСХОДНЫХ СПРАЙТОВ
// =====================================================

void UVNCharacterIdleAnimationManager::SaveOriginalSprite(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite>& OriginalSprite)
{
    if (!Component)
    {
        return;
    }

    UPaperSprite* CurrentSprite = Component->GetSprite();
    if (CurrentSprite)
    {
        OriginalSprite = CurrentSprite;
        LogIdleAnimation(FString::Printf(TEXT("Saved original sprite for %s"), *Component->GetName()), false);
    }
    else
    {
        OriginalSprite = nullptr;
        LogIdleAnimation(FString::Printf(TEXT("No original sprite to save for %s"), *Component->GetName()), false);
    }
}

void UVNCharacterIdleAnimationManager::RestoreOriginalSprite(UPaperSpriteComponent* Component, const TSoftObjectPtr<UPaperSprite>& OriginalSprite)
{
    if (!Component)
    {
        return;
    }

    if (!OriginalSprite.IsNull())
    {
        UPaperSprite* LoadedSprite = OriginalSprite.LoadSynchronous();
        if (LoadedSprite)
        {
            Component->SetSprite(LoadedSprite);
            LogIdleAnimation(FString::Printf(TEXT("Restored original sprite for %s"), *Component->GetName()), false);
        }
    }
    else
    {
        // Исходного спрайта не было, очищаем компонент
        Component->SetSprite(nullptr);
        LogIdleAnimation(FString::Printf(TEXT("Cleared sprite for %s (no original)"), *Component->GetName()), false);
    }
}

// =====================================================
// УТИЛИТЫ
// =====================================================

AVNCharacter* UVNCharacterIdleAnimationManager::GetVNCharacterOwner() const
{
    return OwnerCharacter.Get();
}

void UVNCharacterIdleAnimationManager::LogIdleAnimation(const FString& Message, bool bForceLog) const
{
    if (bVerboseLogging || bForceLog)
    {
        VN_LOG_DEBUG(TEXT("IdleAnimManager [%s]: %s"), 
            OwnerCharacter.IsValid() ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
            *Message);
    }
}