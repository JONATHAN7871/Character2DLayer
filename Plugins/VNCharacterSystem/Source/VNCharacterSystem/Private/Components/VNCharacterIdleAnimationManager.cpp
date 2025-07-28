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
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;

    bDisableIdleAnimations = false;
    bVerboseLogging = false;
    
    bIsBlinkAnimationPlaying = false;
    bIsEyesRandomAnimationPlaying = false;
    CurrentBlinkState = EBlinkState::WaitingForBlink;
    bPendingDoubleBlink = false;
}

void UVNCharacterIdleAnimationManager::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<AVNCharacter>(GetOwner());
    
    if (!OwnerCharacter.IsValid())
    {
        VN_LOG_ERROR(TEXT("VNCharacterIdleAnimationManager: Owner is not a VNCharacter!"));
        return;
    }

    LogIdleAnimation(TEXT("Idle Animation Manager initialized successfully"));
    StartAllIdleAnimations();
}

void UVNCharacterIdleAnimationManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // НОВАЯ ФУНКЦИЯ: Отслеживаем изменения спрайтов во время анимаций
    CheckForSpriteChanges();

    if (bVerboseLogging)
    {
        static float LogTimer = 0.0f;
        LogTimer += DeltaTime;
        if (LogTimer >= 2.0f)
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
// НОВАЯ СИСТЕМА ОТСЛЕЖИВАНИЯ ИЗМЕНЕНИЙ СПРАЙТОВ
// =====================================================

void UVNCharacterIdleAnimationManager::CheckForSpriteChanges()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character) return;

    // Проверяем изменения во время анимации моргания
    if (bIsBlinkAnimationPlaying && Character->Eyelids_Sprite)
    {
        UPaperSprite* CurrentSprite = Character->Eyelids_Sprite->GetSprite();
        if (CurrentSprite && CurrentSprite != GetCurrentAnimationSprite(Character->Eyelids_Sprite))
        {
            // Спрайт был изменен извне во время анимации
            HandleExternalSpriteChange(Character->Eyelids_Sprite, CurrentSprite, TEXT("Eyelids"));
        }
    }

    // Проверяем изменения во время анимации разговора
    if (IdleAnimationsConfig.TalkConfig.bEnabled && Character->Mouth_Sprite)
    {
        UPaperSprite* CurrentSprite = Character->Mouth_Sprite->GetSprite();
        if (CurrentSprite && !IsAnimationSprite(Character->Mouth_Sprite, CurrentSprite))
        {
            // Спрайт был изменен извне во время анимации
            HandleExternalSpriteChange(Character->Mouth_Sprite, CurrentSprite, TEXT("Mouth"));
        }
    }

    // Проверяем изменения во время анимации глаз
    if (bIsEyesRandomAnimationPlaying && Character->Eyes_Sprite)
    {
        UPaperSprite* CurrentSprite = Character->Eyes_Sprite->GetSprite();
        if (CurrentSprite && !IsAnimationSprite(Character->Eyes_Sprite, CurrentSprite))
        {
            // Спрайт был изменен извне во время анимации
            HandleExternalSpriteChange(Character->Eyes_Sprite, CurrentSprite, TEXT("Eyes"));
        }
    }
}

void UVNCharacterIdleAnimationManager::HandleExternalSpriteChange(UPaperSpriteComponent* Component, UPaperSprite* NewSprite, const FString& ComponentName)
{
    VN_LOG_WARNING(TEXT("HandleExternalSpriteChange: %s sprite changed during idle animation"), *ComponentName);
    
    // Обновляем наш сохраненный оригинальный спрайт
    if (Component == GetVNCharacterOwner()->Eyelids_Sprite)
    {
        OriginalEyelidsSprite = NewSprite;
        LogIdleAnimation(FString::Printf(TEXT("Updated original eyelids sprite during animation")));
    }
    else if (Component == GetVNCharacterOwner()->Mouth_Sprite)
    {
        OriginalMouthSprite = NewSprite;
        LogIdleAnimation(FString::Printf(TEXT("Updated original mouth sprite during animation")));
    }
    else if (Component == GetVNCharacterOwner()->Eyes_Sprite)
    {
        OriginalEyesSprite = NewSprite;
        LogIdleAnimation(FString::Printf(TEXT("Updated original eyes sprite during animation")));
    }
}

bool UVNCharacterIdleAnimationManager::IsAnimationSprite(UPaperSpriteComponent* Component, UPaperSprite* Sprite) const
{
    if (!Component || !Sprite) return false;

    // Проверяем, является ли спрайт частью текущей анимации
    if (Component == GetVNCharacterOwner()->Eyelids_Sprite && bIsBlinkAnimationPlaying)
    {
        UPaperFlipbook* BlinkFlipbook = IdleAnimationsConfig.BlinkConfig.BlinkFlipbook.LoadSynchronous();
        if (BlinkFlipbook)
        {
            return IsFlipbookSprite(BlinkFlipbook, Sprite);
        }
    }
    else if (Component == GetVNCharacterOwner()->Mouth_Sprite && IdleAnimationsConfig.TalkConfig.bEnabled)
    {
        UPaperFlipbook* TalkFlipbook = IdleAnimationsConfig.TalkConfig.TalkFlipbook.LoadSynchronous();
        if (TalkFlipbook)
        {
            return IsFlipbookSprite(TalkFlipbook, Sprite);
        }
    }
    else if (Component == GetVNCharacterOwner()->Eyes_Sprite && bIsEyesRandomAnimationPlaying)
    {
        UPaperFlipbook* EyesFlipbook = IdleAnimationsConfig.EyesRandomConfig.EyesDirectionsFlipbook.LoadSynchronous();
        if (EyesFlipbook)
        {
            return IsFlipbookSprite(EyesFlipbook, Sprite);
        }
    }

    return false;
}

UPaperSprite* UVNCharacterIdleAnimationManager::GetCurrentAnimationSprite(UPaperSpriteComponent* Component) const
{
    if (!Component) return nullptr;
    return Component->GetSprite();
}

bool UVNCharacterIdleAnimationManager::IsFlipbookSprite(UPaperFlipbook* Flipbook, UPaperSprite* Sprite) const
{
    if (!Flipbook || !Sprite) return false;

    // Проверяем все кадры flipbook
    float TotalDuration = Flipbook->GetTotalDuration();
    if (TotalDuration <= 0.0f) return false;

    // Проверяем несколько точек времени
    const int32 CheckPoints = 10;
    for (int32 i = 0; i < CheckPoints; ++i)
    {
        float TimePoint = (TotalDuration / CheckPoints) * i;
        UPaperSprite* FlipbookSprite = Flipbook->GetSpriteAtTime(TimePoint);
        if (FlipbookSprite == Sprite)
        {
            return true;
        }
    }

    return false;
}

// =====================================================
// ПУБЛИЧНЫЕ МЕТОДЫ
// =====================================================

void UVNCharacterIdleAnimationManager::SetBlinkEnabled(bool bEnable)
{
    if (IdleAnimationsConfig.BlinkConfig.bEnabled == bEnable)
    {
        return;
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
        return;
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
        return;
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
    StopAllIdleAnimations();
    IdleAnimationsConfig = NewConfig;
    LogIdleAnimation(TEXT("Idle animations config updated"));
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
// МЕТОДЫ ДЛЯ АНИМАЦИИ МОРГАНИЯ (ИСПРАВЛЕННЫЕ)
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

    // ИСПРАВЛЕНИЕ: Сохраняем текущий спрайт век, а не оригинальный
    SaveCurrentSprite(Character->Eyelids_Sprite, OriginalEyelidsSprite);
    
    LogIdleAnimation(TEXT("Blink animation started"));

    CurrentBlinkState = EBlinkState::WaitingForBlink;
    ScheduleNextBlink();
}

void UVNCharacterIdleAnimationManager::StopBlinkAnimation()
{
    if (!bIsBlinkAnimationPlaying && !GetWorld()->GetTimerManager().IsTimerActive(BlinkTimerHandle))
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(BlinkTimerHandle);
    
    if (bIsBlinkAnimationPlaying)
    {
        FinishBlinkAnimation();
    }
    
    CurrentBlinkState = EBlinkState::WaitingForBlink;
    bPendingDoubleBlink = false;
    
    LogIdleAnimation(TEXT("Blink animation stopped"));
}

void UVNCharacterIdleAnimationManager::ExecuteBlink()
{
    VN_LOG_WARNING(TEXT("=== ExecuteBlink CALLED ==="));
    
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyelids_Sprite)
    {
        VN_LOG_ERROR(TEXT("ExecuteBlink: Invalid character or eyelids component"));
        return;
    }

    UPaperFlipbook* BlinkFlipbook = IdleAnimationsConfig.BlinkConfig.BlinkFlipbook.LoadSynchronous();
    if (!BlinkFlipbook)
    {
        VN_LOG_ERROR(TEXT("ExecuteBlink: Invalid flipbook"));
        return;
    }

    VN_LOG_WARNING(TEXT("ExecuteBlink: Flipbook loaded successfully: %s"), *BlinkFlipbook->GetName());

    UPaperSprite* HalfClosedSprite = GetFlipbookSpriteImproved(BlinkFlipbook, 0);
    UPaperSprite* ClosedSprite = GetFlipbookSpriteImproved(BlinkFlipbook, 1);
    
    VN_LOG_WARNING(TEXT("ExecuteBlink: HalfClosed: %s"), HalfClosedSprite ? *HalfClosedSprite->GetName() : TEXT("NULL"));
    VN_LOG_WARNING(TEXT("ExecuteBlink: Closed: %s"), ClosedSprite ? *ClosedSprite->GetName() : TEXT("NULL"));
    
    if (!HalfClosedSprite || !ClosedSprite)
    {
        VN_LOG_ERROR(TEXT("ExecuteBlink: Cannot get sprites from flipbook!"));
        return;
    }

    // УЛУЧШЕНИЕ: Динамическая вероятность двойного моргания
    static int32 ConsecutiveSingleBlinks = 0;
    
    float DoubleBlinkChance = IdleAnimationsConfig.BlinkConfig.DoubleBlinkChance;
    
    // Увеличиваем шанс двойного моргания после серии одиночных
    if (ConsecutiveSingleBlinks >= 3)
    {
        DoubleBlinkChance *= 2.0f; // Удваиваем шанс
        ConsecutiveSingleBlinks = 0;
    }
    
    bPendingDoubleBlink = FMath::RandRange(0.0f, 1.0f) <= DoubleBlinkChance;
    
    if (bPendingDoubleBlink)
    {
        ConsecutiveSingleBlinks = 0;
        VN_LOG_WARNING(TEXT("ExecuteBlink: Double blink: YES (enhanced chance)"));
    }
    else
    {
        ConsecutiveSingleBlinks++;
        VN_LOG_WARNING(TEXT("ExecuteBlink: Single blink: %d consecutive"), ConsecutiveSingleBlinks);
    }

    // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Сохраняем ТЕКУЩИЙ спрайт перед анимацией
    SaveCurrentSprite(Character->Eyelids_Sprite, OriginalEyelidsSprite);

    bIsBlinkAnimationPlaying = true;
    CurrentBlinkState = EBlinkState::FirstBlinkHalf;
    
    VN_LOG_WARNING(TEXT("ExecuteBlink: Starting blink animation"));
    UpdateBlinkState();
}

void UVNCharacterIdleAnimationManager::UpdateBlinkState()
{
    VN_LOG_WARNING(TEXT("=== UpdateBlinkState: State = %d ==="), (int32)CurrentBlinkState);
    
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character || !Character->Eyelids_Sprite)
    {
        VN_LOG_ERROR(TEXT("UpdateBlinkState: Invalid character or eyelids"));
        FinishBlinkAnimation();
        return;
    }

    UPaperFlipbook* BlinkFlipbook = IdleAnimationsConfig.BlinkConfig.BlinkFlipbook.LoadSynchronous();
    if (!BlinkFlipbook)
    {
        VN_LOG_ERROR(TEXT("UpdateBlinkState: Invalid flipbook"));
        FinishBlinkAnimation();
        return;
    }

    UPaperSprite* HalfClosedSprite = GetFlipbookSpriteImproved(BlinkFlipbook, 0);
    UPaperSprite* ClosedSprite = GetFlipbookSpriteImproved(BlinkFlipbook, 1);
    
    if (!HalfClosedSprite || !ClosedSprite)
    {
        VN_LOG_ERROR(TEXT("UpdateBlinkState: Cannot get sprites from flipbook"));
        FinishBlinkAnimation();
        return;
    }

    // УЛУЧШЕНИЕ: Вариативная длительность моргания для живости
    float BaseDuration = IdleAnimationsConfig.BlinkConfig.BlinkDuration;
    float VariableDuration = BaseDuration * FMath::RandRange(0.8f, 1.2f); // ±20% вариации
    float DoubleBlinkPause = IdleAnimationsConfig.BlinkConfig.DoubleBlinkPause;
    
    VN_LOG_WARNING(TEXT("UpdateBlinkState: BaseDuration=%.3f, VariableDuration=%.3f, DoubleBlinkPause=%.3f"), 
        BaseDuration, VariableDuration, DoubleBlinkPause);

    switch (CurrentBlinkState)
    {
        case EBlinkState::FirstBlinkHalf:
        {
            VN_LOG_WARNING(TEXT("UpdateBlinkState: Setting half-closed sprite"));
            Character->Eyelids_Sprite->SetSprite(HalfClosedSprite);
            CurrentBlinkState = EBlinkState::FirstBlinkFull;
            
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle,
                this,
                &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                VariableDuration * 0.4f, // Быстрее до полного закрытия
                false
            );
            break;
        }
        
        case EBlinkState::FirstBlinkFull:
        {
            VN_LOG_WARNING(TEXT("UpdateBlinkState: Setting closed sprite"));
            Character->Eyelids_Sprite->SetSprite(ClosedSprite);
            
            if (bPendingDoubleBlink)
            {
                CurrentBlinkState = EBlinkState::BetweenBlinks;
                VN_LOG_WARNING(TEXT("UpdateBlinkState: Preparing for double blink"));
                
                GetWorld()->GetTimerManager().SetTimer(
                    BlinkTimerHandle,
                    this,
                    &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                    VariableDuration * 0.6f, // Дольше в закрытом состоянии
                    false
                );
            }
            else
            {
                VN_LOG_WARNING(TEXT("UpdateBlinkState: Single blink, finishing"));
                GetWorld()->GetTimerManager().SetTimer(
                    BlinkTimerHandle,
                    this,
                    &UVNCharacterIdleAnimationManager::FinishBlinkAnimation,
                    VariableDuration * 0.6f, // Дольше в закрытом состоянии
                    false
                );
            }
            break;
        }
        
        case EBlinkState::BetweenBlinks:
        {
            VN_LOG_WARNING(TEXT("UpdateBlinkState: Returning to original between blinks"));
            RestoreCurrentSprite(Character->Eyelids_Sprite, OriginalEyelidsSprite);
            CurrentBlinkState = EBlinkState::SecondBlinkHalf;
            
            // УЛУЧШЕНИЕ: Вариативная пауза между двойными морганиями
            float VariablePause = DoubleBlinkPause * FMath::RandRange(0.5f, 1.5f);
            
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle,
                this,
                &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                VariablePause,
                false
            );
            break;
        }
        
        case EBlinkState::SecondBlinkHalf:
        {
            VN_LOG_WARNING(TEXT("UpdateBlinkState: Second blink - half closed"));
            Character->Eyelids_Sprite->SetSprite(HalfClosedSprite);
            CurrentBlinkState = EBlinkState::SecondBlinkFull;
            
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle,
                this,
                &UVNCharacterIdleAnimationManager::UpdateBlinkState,
                VariableDuration * 0.3f, // Быстрее второе моргание
                false
            );
            break;
        }
        
        case EBlinkState::SecondBlinkFull:
        {
            VN_LOG_WARNING(TEXT("UpdateBlinkState: Second blink - fully closed"));
            Character->Eyelids_Sprite->SetSprite(ClosedSprite);
            
            GetWorld()->GetTimerManager().SetTimer(
                BlinkTimerHandle,
                this,
                &UVNCharacterIdleAnimationManager::FinishBlinkAnimation,
                VariableDuration * 0.4f, // Короче второе закрытие
                false
            );
            break;
        }
        
        default:
            VN_LOG_ERROR(TEXT("UpdateBlinkState: Unknown state"));
            FinishBlinkAnimation();
            break;
    }
}

void UVNCharacterIdleAnimationManager::FinishBlinkAnimation()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Eyelids_Sprite)
    {
        // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Проверяем, изменился ли спрайт во время анимации
        UPaperSprite* CurrentEyelidsSprite = Character->Eyelids_Sprite->GetSprite();
        UPaperSprite* SavedSprite = OriginalEyelidsSprite.LoadSynchronous();
        
        // Логируем состояние для отладки
        VN_LOG_WARNING(TEXT("FinishBlinkAnimation: Current: %s, Saved: %s"), 
            CurrentEyelidsSprite ? *CurrentEyelidsSprite->GetName() : TEXT("NULL"),
            SavedSprite ? *SavedSprite->GetName() : TEXT("NULL"));
        
        // Восстанавливаем ТОЛЬКО если текущий спрайт является частью анимации моргания
        if (IsCurrentSpritePartOfBlinkAnimation(CurrentEyelidsSprite))
        {
            RestoreCurrentSprite(Character->Eyelids_Sprite, OriginalEyelidsSprite);
            VN_LOG_DEBUG(TEXT("FinishBlinkAnimation: Restored original eyelids sprite"));
        }
        else
        {
            // Спрайт был изменен извне - сохраняем как новый оригинальный
            OriginalEyelidsSprite = CurrentEyelidsSprite;
            VN_LOG_DEBUG(TEXT("FinishBlinkAnimation: Keeping externally changed sprite"));
        }
    }

    bIsBlinkAnimationPlaying = false;
    CurrentBlinkState = EBlinkState::WaitingForBlink;
    bPendingDoubleBlink = false;
    
    LogIdleAnimation(TEXT("Blink animation finished"));
    
    if (IdleAnimationsConfig.BlinkConfig.bEnabled)
    {
        ScheduleNextBlink();
    }
}

bool UVNCharacterIdleAnimationManager::IsCurrentSpritePartOfBlinkAnimation(UPaperSprite* Sprite) const
{
    if (!Sprite) return false;
    
    UPaperFlipbook* BlinkFlipbook = IdleAnimationsConfig.BlinkConfig.BlinkFlipbook.LoadSynchronous();
    if (!BlinkFlipbook) return false;
    
    // Проверяем, является ли это одним из спрайтов моргания
    UPaperSprite* HalfClosed = GetFlipbookSpriteImproved(BlinkFlipbook, 0);
    UPaperSprite* Closed = GetFlipbookSpriteImproved(BlinkFlipbook, 1);
    
    return (Sprite == HalfClosed || Sprite == Closed);
}

void UVNCharacterIdleAnimationManager::ScheduleNextBlink()
{
    if (!IdleAnimationsConfig.BlinkConfig.bEnabled)
    {
        VN_LOG_WARNING(TEXT("ScheduleNextBlink: Blink disabled, not scheduling"));
        return;
    }

    // УЛУЧШЕНИЕ: Более сложная логика интервалов для живости
    float BaseInterval = IdleAnimationsConfig.BlinkConfig.GetRandomBlinkInterval();
    
    // Добавляем вариативность на основе "эмоционального состояния"
    static int32 BlinkCounter = 0;
    BlinkCounter++;
    
    float EmotionalMultiplier = 1.0f;
    
    // Каждое 4-5 моргание делаем с разной частотой для живости
    if (BlinkCounter % 4 == 0)
    {
        EmotionalMultiplier = 0.3f; // Быстрое моргание (нервность)
    }
    else if (BlinkCounter % 7 == 0)
    {
        EmotionalMultiplier = 2.0f; // Долгая пауза (расслабленность)
    }
    else if (BlinkCounter % 11 == 0)
    {
        EmotionalMultiplier = 0.5f; // Средне-быстрое моргание
    }
    
    float FinalInterval = BaseInterval * EmotionalMultiplier;
    FinalInterval = FMath::Clamp(FinalInterval, 0.5f, 8.0f); // Разумные пределы
    
    VN_LOG_WARNING(TEXT("ScheduleNextBlink: Next blink in %.2f seconds (base: %.2f, multiplier: %.2f, counter: %d)"), 
        FinalInterval, BaseInterval, EmotionalMultiplier, BlinkCounter);
    
    GetWorld()->GetTimerManager().SetTimer(
        BlinkTimerHandle,
        this,
        &UVNCharacterIdleAnimationManager::ExecuteBlink,
        FinalInterval,
        false
    );
}

// =====================================================
// МЕТОДЫ ДЛЯ АНИМАЦИИ РАЗГОВОРА (ИСПРАВЛЕННЫЕ)
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

    // ИСПРАВЛЕНИЕ: Сохраняем текущий спрайт рта
    SaveCurrentSprite(Character->Mouth_Sprite, OriginalMouthSprite);
    
    LogIdleAnimation(TEXT("Talk animation started"));

    float FrameInterval = IdleAnimationsConfig.TalkConfig.GetFrameInterval();
    GetWorld()->GetTimerManager().SetTimer(
        TalkTimerHandle,
        this,
        &UVNCharacterIdleAnimationManager::UpdateTalkFrame,
        FrameInterval,
        true
    );
}

void UVNCharacterIdleAnimationManager::StopTalkAnimation()
{
    GetWorld()->GetTimerManager().ClearTimer(TalkTimerHandle);
    
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Mouth_Sprite)
    {
        // ИСПРАВЛЕНИЕ: Проверяем, является ли текущий спрайт частью Talk анимации
        UPaperSprite* CurrentMouthSprite = Character->Mouth_Sprite->GetSprite();
        
        if (IsCurrentSpritePartOfTalkAnimation(CurrentMouthSprite))
        {
            // Восстанавливаем сохраненный спрайт
            RestoreCurrentSprite(Character->Mouth_Sprite, OriginalMouthSprite);
            VN_LOG_DEBUG(TEXT("StopTalkAnimation: Restored original mouth sprite"));
        }
        else
        {
            // Спрайт был изменен извне - оставляем как есть
            VN_LOG_DEBUG(TEXT("StopTalkAnimation: Keeping externally changed mouth sprite"));
        }
    }
    
    LogIdleAnimation(TEXT("Talk animation stopped"));
}

// Проверка, является ли спрайт частью Talk анимации
bool UVNCharacterIdleAnimationManager::IsCurrentSpritePartOfTalkAnimation(UPaperSprite* Sprite) const
{
    if (!Sprite) return false;
    
    UPaperFlipbook* TalkFlipbook = IdleAnimationsConfig.TalkConfig.TalkFlipbook.LoadSynchronous();
    if (!TalkFlipbook) return false;
    
    return IsFlipbookSprite(TalkFlipbook, Sprite);
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

    // ИСПРАВЛЕНИЕ: Используем улучшенный метод получения спрайтов
    UPaperSprite* RandomSprite = GetRandomFlipbookSpriteImproved(TalkFlipbook, false);
    if (RandomSprite)
    {
        Character->Mouth_Sprite->SetSprite(RandomSprite);
        LogIdleAnimation(TEXT("Talk frame updated to random sprite"), false);
    }
}

// =====================================================
// МЕТОДЫ ДЛЯ АНИМАЦИИ СЛУЧАЙНЫХ ДВИЖЕНИЙ ГЛАЗ (ИСПРАВЛЕННЫЕ)
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

    // ИСПРАВЛЕНИЕ: Сохраняем текущий спрайт глаз
    SaveCurrentSprite(Character->Eyes_Sprite, OriginalEyesSprite);
    
    LogIdleAnimation(TEXT("Eyes random animation started"));
    ScheduleNextEyesMovement();
}

void UVNCharacterIdleAnimationManager::StopEyesRandomAnimation()
{
    if (!bIsEyesRandomAnimationPlaying && !GetWorld()->GetTimerManager().IsTimerActive(EyesRandomTimerHandle))
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(EyesRandomTimerHandle);
    
    AVNCharacter* Character = GetVNCharacterOwner();
    if (Character && Character->Eyes_Sprite)
    {
        // ИСПРАВЛЕНИЕ: Восстанавливаем актуальный спрайт глаз
        RestoreCurrentSprite(Character->Eyes_Sprite, OriginalEyesSprite);
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

    // ИСПРАВЛЕНИЕ: Используем улучшенный метод
    UPaperSprite* RandomDirection = GetRandomFlipbookSpriteImproved(EyesFlipbook, false);
    if (RandomDirection)
    {
        Character->Eyes_Sprite->SetSprite(RandomDirection);
        bIsEyesRandomAnimationPlaying = true;
        
        LogIdleAnimation(TEXT("Eyes direction changed to random sprite"));
        
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
        // ИСПРАВЛЕНИЕ: Восстанавливаем актуальный спрайт
        RestoreCurrentSprite(Character->Eyes_Sprite, OriginalEyesSprite);
        LogIdleAnimation(TEXT("Eyes returned to original position"));
    }

    bIsEyesRandomAnimationPlaying = false;
    
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
// УЛУЧШЕННЫЕ УТИЛИТЫ ДЛЯ РАБОТЫ С FLIPBOOK
// =====================================================

UPaperSprite* UVNCharacterIdleAnimationManager::GetFlipbookSpriteImproved(UPaperFlipbook* Flipbook, int32 FrameIndex) const
{
    if (!Flipbook)
    {
        VN_LOG_ERROR(TEXT("GetFlipbookSpriteImproved: Flipbook is null"));
        return nullptr;
    }

    // ИСПРАВЛЕНИЕ ДЛЯ UE 5.5: Упрощенный подход без сложной рефлексии
    // Используем только временные точки с улучшенной логикой
    
    float TotalDuration = Flipbook->GetTotalDuration();
    if (TotalDuration <= 0.0f)
    {
        VN_LOG_WARNING(TEXT("GetFlipbookSpriteImproved: Flipbook has zero duration"));
        return nullptr;
    }
    
    UPaperSprite* ResultSprite = nullptr;
    
    if (FrameIndex == 0)
    {
        // Первый кадр - в самом начале
        ResultSprite = Flipbook->GetSpriteAtTime(0.0f);
        VN_LOG_DEBUG(TEXT("GetFlipbookSpriteImproved: Frame 0 at time 0.0"));
    }
    else if (FrameIndex == 1)
    {
        // Второй кадр - пробуем несколько позиций
        float TestTimes[] = { 
            TotalDuration * 0.3f,   // 30% от общего времени
            TotalDuration * 0.5f,   // 50% от общего времени  
            TotalDuration * 0.7f,   // 70% от общего времени
            TotalDuration - 0.001f  // Почти в конце
        };
        
        for (float TestTime : TestTimes)
        {
            UPaperSprite* TestSprite = Flipbook->GetSpriteAtTime(TestTime);
            if (TestSprite)
            {
                // Проверяем, отличается ли этот спрайт от первого кадра
                UPaperSprite* FirstSprite = Flipbook->GetSpriteAtTime(0.0f);
                if (TestSprite != FirstSprite)
                {
                    ResultSprite = TestSprite;
                    VN_LOG_DEBUG(TEXT("GetFlipbookSpriteImproved: Frame 1 found at time %.3f"), TestTime);
                    break;
                }
            }
        }
        
        // Если не нашли отличающийся спрайт, берем средний по времени
        if (!ResultSprite)
        {
            ResultSprite = Flipbook->GetSpriteAtTime(TotalDuration * 0.5f);
            VN_LOG_DEBUG(TEXT("GetFlipbookSpriteImproved: Frame 1 fallback at mid time"));
        }
    }
    else
    {
        // Остальные кадры - равномерно распределяем по времени
        float FrameTime = (TotalDuration / FMath::Max(4.0f, (float)(FrameIndex + 1))) * FrameIndex;
        if (FrameTime >= TotalDuration)
        {
            FrameTime = TotalDuration - 0.001f;
        }
        ResultSprite = Flipbook->GetSpriteAtTime(FrameTime);
        VN_LOG_DEBUG(TEXT("GetFlipbookSpriteImproved: Frame %d at time %.3f"), FrameIndex, FrameTime);
    }
    
    if (!ResultSprite)
    {
        VN_LOG_WARNING(TEXT("GetFlipbookSpriteImproved: Could not get sprite for frame %d"), FrameIndex);
        // Fallback - возвращаем первый кадр
        ResultSprite = Flipbook->GetSpriteAtTime(0.0f);
    }
    
    return ResultSprite;
}

UPaperSprite* UVNCharacterIdleAnimationManager::GetRandomFlipbookSpriteImproved(UPaperFlipbook* Flipbook, bool bExcludeFirstFrame) const
{
    if (!Flipbook)
    {
        return nullptr;
    }

    // Попробуем получить точное количество кадров
    int32 TotalFrames = GetFlipbookFrameCountImproved(Flipbook);
    if (TotalFrames <= 0)
    {
        return nullptr;
    }

    int32 StartIndex = bExcludeFirstFrame ? 1 : 0;
    int32 MaxIndex = TotalFrames - 1;
    
    if (StartIndex > MaxIndex)
    {
        return nullptr;
    }

    int32 RandomIndex = FMath::RandRange(StartIndex, MaxIndex);
    return GetFlipbookSpriteImproved(Flipbook, RandomIndex);
}

int32 UVNCharacterIdleAnimationManager::GetFlipbookFrameCountImproved(UPaperFlipbook* Flipbook) const
{
    if (!Flipbook)
    {
        return 0;
    }

    // УПРОЩЕННЫЙ ПОДХОД: Оценка на основе длительности и тестирования
    float TotalDuration = Flipbook->GetTotalDuration();
    if (TotalDuration <= 0.0f)
    {
        return 0;
    }
    
    // Тестируем несколько временных точек, чтобы найти уникальные спрайты
    TSet<UPaperSprite*> UniqueSprites;
    
    const int32 TestPoints = 20; // Проверяем 20 точек по времени
    for (int32 i = 0; i < TestPoints; ++i)
    {
        float TestTime = (TotalDuration / TestPoints) * i;
        if (UPaperSprite* TestSprite = Flipbook->GetSpriteAtTime(TestTime))
        {
            UniqueSprites.Add(TestSprite);
        }
    }
    
    // Добавляем последний кадр
    if (UPaperSprite* LastSprite = Flipbook->GetSpriteAtTime(TotalDuration - 0.001f))
    {
        UniqueSprites.Add(LastSprite);
    }
    
    int32 FrameCount = FMath::Max(2, UniqueSprites.Num()); // Минимум 2 кадра
    VN_LOG_DEBUG(TEXT("GetFlipbookFrameCountImproved: Found %d unique sprites"), FrameCount);
    return FrameCount;
}

// =====================================================
// УЛУЧШЕННЫЕ МЕТОДЫ СОХРАНЕНИЯ И ВОССТАНОВЛЕНИЯ СПРАЙТОВ
// =====================================================

void UVNCharacterIdleAnimationManager::SaveCurrentSprite(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite>& SavedSprite)
{
    if (!Component)
    {
        VN_LOG_WARNING(TEXT("SaveCurrentSprite: Component is null"));
        return;
    }

    UPaperSprite* CurrentSprite = Component->GetSprite();
    if (CurrentSprite)
    {
        SavedSprite = CurrentSprite;
        LogIdleAnimation(FString::Printf(TEXT("Saved current sprite for %s: %s"), 
            *Component->GetName(), *CurrentSprite->GetName()), false);
    }
    else
    {
        SavedSprite = nullptr;
        LogIdleAnimation(FString::Printf(TEXT("No current sprite to save for %s"), *Component->GetName()), false);
    }
}

void UVNCharacterIdleAnimationManager::RestoreCurrentSprite(UPaperSpriteComponent* Component, const TSoftObjectPtr<UPaperSprite>& SavedSprite)
{
    if (!Component)
    {
        VN_LOG_WARNING(TEXT("RestoreCurrentSprite: Component is null"));
        return;
    }

    if (!SavedSprite.IsNull())
    {
        UPaperSprite* LoadedSprite = SavedSprite.LoadSynchronous();
        if (LoadedSprite)
        {
            Component->SetSprite(LoadedSprite);
            LogIdleAnimation(FString::Printf(TEXT("Restored sprite for %s: %s"), 
                *Component->GetName(), *LoadedSprite->GetName()), false);
        }
        else
        {
            VN_LOG_WARNING(TEXT("RestoreCurrentSprite: Failed to load saved sprite for %s"), *Component->GetName());
        }
    }
    else
    {
        // Сохраненного спрайта не было, очищаем компонент
        Component->SetSprite(nullptr);
        LogIdleAnimation(FString::Printf(TEXT("Cleared sprite for %s (no saved sprite)"), *Component->GetName()), false);
    }
}

// =====================================================
// УСТАРЕВШИЕ МЕТОДЫ (СОВМЕСТИМОСТЬ)
// =====================================================

UPaperSprite* UVNCharacterIdleAnimationManager::GetSpriteFromFlipbook(UPaperFlipbook* Flipbook, int32 FrameIndex) const
{
    // Перенаправляем на улучшенную версию
    return GetFlipbookSpriteImproved(Flipbook, FrameIndex);
}

int32 UVNCharacterIdleAnimationManager::GetFlipbookFrameCount(UPaperFlipbook* Flipbook) const
{
    // Перенаправляем на улучшенную версию
    return GetFlipbookFrameCountImproved(Flipbook);
}

UPaperSprite* UVNCharacterIdleAnimationManager::GetRandomSpriteFromFlipbook(UPaperFlipbook* Flipbook, bool bExcludeFirstFrame) const
{
    // Перенаправляем на улучшенную версию
    return GetRandomFlipbookSpriteImproved(Flipbook, bExcludeFirstFrame);
}

void UVNCharacterIdleAnimationManager::SaveOriginalSprite(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite>& OriginalSprite)
{
    // Перенаправляем на улучшенную версию
    SaveCurrentSprite(Component, OriginalSprite);
}

void UVNCharacterIdleAnimationManager::RestoreOriginalSprite(UPaperSpriteComponent* Component, const TSoftObjectPtr<UPaperSprite>& OriginalSprite)
{
    // Перенаправляем на улучшенную версию
    RestoreCurrentSprite(Component, OriginalSprite);
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

void UVNCharacterIdleAnimationManager::UpdateSavedSprites()
{
    AVNCharacter* Character = GetVNCharacterOwner();
    if (!Character)
    {
        return;
    }

    VN_LOG_DEBUG(TEXT("UpdateSavedSprites: Updating all saved sprites to current state"));

    // Обновляем сохраненные спрайты для всех компонентов
    if (Character->Eyelids_Sprite)
    {
        SaveCurrentSprite(Character->Eyelids_Sprite, OriginalEyelidsSprite);
        VN_LOG_DEBUG(TEXT("UpdateSavedSprites: Updated eyelids sprite"));
    }

    if (Character->Mouth_Sprite)
    {
        SaveCurrentSprite(Character->Mouth_Sprite, OriginalMouthSprite);
        VN_LOG_DEBUG(TEXT("UpdateSavedSprites: Updated mouth sprite"));
    }

    if (Character->Eyes_Sprite)
    {
        SaveCurrentSprite(Character->Eyes_Sprite, OriginalEyesSprite);
        VN_LOG_DEBUG(TEXT("UpdateSavedSprites: Updated eyes sprite"));
    }
}