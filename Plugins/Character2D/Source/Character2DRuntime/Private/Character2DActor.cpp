#include "Character2DActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "PaperFlipbook.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacter2DActor, Log, All);

ACharacter2DActor::ACharacter2DActor()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Создаем компонент таймлайна
	TransitionTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("TransitionTimeline"));

	SetupComponents();
}

void ACharacter2DActor::SetupComponents()
{
	BodyComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyComponent"));
	ArmsComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ArmsComponent"));
	HeadComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HeadComponent"));
	BodyComponent->SetupAttachment(RootComponent);
	ArmsComponent->SetupAttachment(RootComponent);
	HeadComponent->SetupAttachment(RootComponent);

	SpriteBody = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteBody"));
	SpriteArms = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteArms"));
	SpriteHead = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteHead"));
	SpriteEyebrow = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteEyebrow"));
	SpriteEyes = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteEyes"));
	SpriteEyelids = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteEyelids"));
	SpriteMouth = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteMouth"));
	SpriteEffect1 = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteEffect1"));
	SpriteEffect2 = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteEffect2"));
	SpriteEffect3 = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteEffect3"));

	SpriteBody->SetupAttachment(RootComponent);
	SpriteArms->SetupAttachment(RootComponent);
	SpriteHead->SetupAttachment(RootComponent);
	SpriteEyebrow->SetupAttachment(SpriteHead);
	SpriteEyes->SetupAttachment(SpriteHead);
	SpriteEyelids->SetupAttachment(SpriteHead);
	SpriteMouth->SetupAttachment(SpriteHead);
	SpriteEffect1->SetupAttachment(SpriteHead);
	SpriteEffect2->SetupAttachment(SpriteHead);
	SpriteEffect3->SetupAttachment(SpriteHead);

	for (UPaperSpriteComponent* Component : GetAllSpriteComponents())
	{
		if (Component)
		{
			Component->SetCastShadow(false);
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ACharacter2DActor::BeginPlay()
{
    Super::BeginPlay();
	
    // Привязываем функции обратного вызова к таймлайну
    if (TransitionTimeline)
    {
        FOnTimelineEvent FinishedCallback;
        FinishedCallback.BindUFunction(this, FName("HandleTransitionFinished"));
        TransitionTimeline->SetTimelineFinishedFunc(FinishedCallback);
    }
	
    StoreOriginalValues();
	
    if (CharacterAsset)
    {
        if (SpriteEyelids)
        {
            OriginalEyelidsSprite = CharacterAsset->GetEyelidsSprite().Sprite;
        }
        if (SpriteMouth)
        {
            OriginalMouthSprite = CharacterAsset->GetMouthSprite().Sprite;
        }
		
        // ИСПРАВЛЕНО: Включаем Auto анимации только в игре, НЕ в редакторе
        if (GetWorld() && GetWorld()->IsGameWorld())
        {
            EnableBlinking(CharacterAsset->bAutoBlink);
            EnableTalking(CharacterAsset->bAutoTalk);
        }
        else
        {
            // В редакторе всегда отключены по умолчанию
            bBlinkingActive = false;
            bTalkingActive = false;
        }
    }
}

void ACharacter2DActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (!CharacterAsset) return;

    SetupSkeletalComponent(BodyComponent, CharacterAsset->Body);
    SetupSkeletalComponent(ArmsComponent, CharacterAsset->Arms);
    SetupSkeletalComponent(HeadComponent, CharacterAsset->Head);

    const auto& BodySpriteData = CharacterAsset->GetBodySprite();
    const auto& ArmsSpriteData = CharacterAsset->GetArmsSprite();
    
    // ОБНОВЛЕНО: Добавлены Color и Opacity параметры
    SetupSpriteComponent(SpriteBody, BodySpriteData.Sprite, BodySpriteData.Offset, BodySpriteData.Scale, BodySpriteData.bVisible, BodySpriteData.Color, BodySpriteData.Opacity);
    SetupSpriteComponent(SpriteArms, ArmsSpriteData.Sprite, ArmsSpriteData.Offset, ArmsSpriteData.Scale, ArmsSpriteData.bVisible, ArmsSpriteData.Color, ArmsSpriteData.Opacity);
    SetupHeadHierarchy();
    
    AttachSpriteToSocket(SpriteBody, BodySpriteData.AttachmentTarget, BodySpriteData.SocketName, BodySpriteData.bUseSocketTransform, BodySpriteData.Offset, BodySpriteData.Scale);
    AttachSpriteToSocket(SpriteArms, ArmsSpriteData.AttachmentTarget, ArmsSpriteData.SocketName, ArmsSpriteData.bUseSocketTransform, ArmsSpriteData.Offset, ArmsSpriteData.Scale);
    AttachHeadToSocket();

    SetupEffectLayers();
    SetSpritesVisible(HasValidSprites());
    SetSkeletalVisible(HasValidSkeletalMeshes());
	
    // ИСПРАВЛЕНО: НЕ устанавливаем флаги анимаций в OnConstruction
    // Это должно происходить только в BeginPlay для игровых акторов
    // В редакторе анимации контролируются вручную через Action Panel
}

// --- РЕАЛИЗАЦИЯ НОВЫХ ФУНКЦИЙ ---

void ACharacter2DActor::Appear(UCurveFloat* Curve, float Duration)
{
    StartTransition(ECharacter2DTransitionState::Appearing, Curve, Duration);
}

void ACharacter2DActor::Disappear(UCurveFloat* Curve, float Duration)
{
    StartTransition(ECharacter2DTransitionState::Disappearing, Curve, Duration);
}

void ACharacter2DActor::StartTransition(ECharacter2DTransitionState NewState, UCurveFloat* Curve, float Duration)
{
    if (IsInTransition())
    {
        // Если уже идет анимация, прерываем ее, чтобы начать новую
        StopCurrentTransition();
    }

    if (!Curve)
    {
        UE_LOG(LogCharacter2DActor, Warning, TEXT("StartTransition: Curve is null. Aborting."));
        return;
    }

    CurrentTransitionState = NewState;
    CurrentTransitionCurve = Curve;

    // Сохраняем оригинальные цвета всех спрайтов
    OriginalSpriteColorsForTransition.Empty();
    for (UPaperSpriteComponent* SpriteComp : GetAllSpriteComponents(true))
    {
        if (SpriteComp)
        {
            OriginalSpriteColorsForTransition.Add(SpriteComp, SpriteComp->GetSpriteColor());
        }
    }
    
    // Устанавливаем начальное состояние в зависимости от типа перехода
    if (CurrentTransitionState == ECharacter2DTransitionState::Appearing)
    {
        // Для появления начинаем с черных и полностью прозрачных спрайтов
        const FLinearColor StartColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
        for (auto const& [SpriteComp, OriginalColor] : OriginalSpriteColorsForTransition)
        {
            if (SpriteComp) SpriteComp->SetSpriteColor(StartColor);
        }
        // Скелетные меши тоже изначально невидимы
        SetAllSkeletalOpacity(0.f);
    }

    // Настраиваем и запускаем таймлайн
    if (TransitionTimeline)
    {
        // Останавливаем и очищаем предыдущую анимацию
        TransitionTimeline->Stop();
        
        // Создаем новый трек для кривой
        FOnTimelineFloat UpdateCallback;
        UpdateCallback.BindUFunction(this, FName("HandleTransitionUpdate"));
        TransitionTimeline->AddInterpFloat(CurrentTransitionCurve, UpdateCallback);
        TransitionTimeline->SetPlayRate(1.0f / FMath::Max(Duration, 0.001f));
        TransitionTimeline->PlayFromStart();
    }
}

void ACharacter2DActor::StopCurrentTransition()
{
    if (!IsInTransition()) return;

    TransitionTimeline->Stop();
    HandleTransitionFinished(); // Вызываем вручную для очистки
}

void ACharacter2DActor::HandleTransitionUpdate(float Value)
{
    // 'Value' - это значение с кривой, обычно от 0 до 1

    if (CurrentTransitionState == ECharacter2DTransitionState::Appearing)
    {
        // Фаза 1: Появление (увеличение прозрачности) - от 0.0 до 0.5
        // Фаза 2: Просветление (уход черного цвета) - от 0.5 до 1.0
        if (Value <= 0.5f)
        {
            const float OpacityProgress = Value / 0.5f; // Переводим [0, 0.5] в [0, 1]
            const FLinearColor CurrentColor(0.f, 0.f, 0.f, OpacityProgress);
            SetAllSpritesColor(CurrentColor);
            SetAllSkeletalOpacity(OpacityProgress);
        }
        else
        {
            const float ColorProgress = (Value - 0.5f) / 0.5f; // Переводим [0.5, 1] в [0, 1]
            for (auto const& [SpriteComp, OriginalColor] : OriginalSpriteColorsForTransition)
            {
                if (SpriteComp)
                {
                    // Интерполируем от черного к оригинальному цвету
                    FLinearColor NewColor = FMath::Lerp(FLinearColor::Black, OriginalColor, ColorProgress);
                    NewColor.A = OriginalColor.A; // Сохраняем оригинальную прозрачность
                    SpriteComp->SetSpriteColor(NewColor);
                }
            }
            SetAllSkeletalOpacity(1.f); // Скелетные меши уже должны быть полностью видимы
        }
    }
    else if (CurrentTransitionState == ECharacter2DTransitionState::Disappearing)
    {
        // Фаза 1: Затемнение (переход в черный цвет) - от 0.0 до 0.5
        // Фаза 2: Исчезновение (уменьшение прозрачности) - от 0.5 до 1.0
        if (Value <= 0.5f)
        {
            const float ColorProgress = Value / 0.5f;
            for (auto const& [SpriteComp, OriginalColor] : OriginalSpriteColorsForTransition)
            {
                if (SpriteComp)
                {
                    FLinearColor NewColor = FMath::Lerp(OriginalColor, FLinearColor::Black, ColorProgress);
                    NewColor.A = OriginalColor.A;
                    SpriteComp->SetSpriteColor(NewColor);
                }
            }
        }
        else
        {
            const float OpacityProgress = (Value - 0.5f) / 0.5f;
            const float FinalOpacity = 1.0f - OpacityProgress; // Инвертируем, чтобы идти от 1 к 0
            const FLinearColor CurrentColor(0.f, 0.f, 0.f, FinalOpacity);
            SetAllSpritesColor(CurrentColor);
            SetAllSkeletalOpacity(FinalOpacity);
        }
    }

    // Оповещаем Blueprint о текущем прогрессе
    OnTransitionUpdate.Broadcast(CurrentTransitionState, Value);
}

void ACharacter2DActor::HandleTransitionFinished()
{
    if (CurrentTransitionState == ECharacter2DTransitionState::Appearing)
    {
        // Гарантируем, что все цвета восстановлены в исходное состояние
        for (auto const& [SpriteComp, OriginalColor] : OriginalSpriteColorsForTransition)
        {
            if (SpriteComp) SpriteComp->SetSpriteColor(OriginalColor);
        }
        SetAllSkeletalOpacity(1.f);
    }
    else if (CurrentTransitionState == ECharacter2DTransitionState::Disappearing)
    {
        // Гарантируем, что все стало полностью невидимым
        const FLinearColor EndColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
        SetAllSpritesColor(EndColor);
        SetAllSkeletalOpacity(0.f);
    }

    // Оповещаем Blueprint о завершении
    OnTransitionFinished.Broadcast(CurrentTransitionState);

    // Сбрасываем состояние
    CurrentTransitionState = ECharacter2DTransitionState::None;
    CurrentTransitionCurve = nullptr;
    OriginalSpriteColorsForTransition.Empty();
}

// --- СУЩЕСТВУЮЩИЕ ФУНКЦИИ ---

void ACharacter2DActor::SetupHeadHierarchy()
{
    if (!CharacterAsset) return;

    const auto& HeadStructure = CharacterAsset->SpriteStructure.Head;
    
    // ОБНОВЛЕНО: Устанавливаем Head Root с Color/Opacity
    SetupSpriteComponent(SpriteHead, HeadStructure.Head.Sprite, HeadStructure.Head.Offset, HeadStructure.Head.Scale, HeadStructure.Head.bVisible, HeadStructure.Head.Color, HeadStructure.Head.Opacity);
    
    auto SetupChildSprite = [this, &HeadStructure](
        UPaperSpriteComponent* Component, 
        const FCharacter2DHeadChildSprite& ChildData)
    {
        if (!Component) return;
        Component->SetSprite(ChildData.Sprite);
        Component->SetRelativeLocation(ChildData.LocalOffset);
        Component->SetRelativeScale3D(FVector(ChildData.LocalScale));
        Component->SetVisibility(HeadStructure.GetFinalChildVisibility(ChildData) && bSpritesVisible);
        
        // НОВОЕ: Применяем Color и Opacity для дочерних спрайтов головы
        FLinearColor FinalColor = ChildData.Color;
        FinalColor.A = ChildData.Opacity;
        Component->SetSpriteColor(FinalColor);
    };

    SetupChildSprite(SpriteEyebrow, HeadStructure.Eyebrows);
    SetupChildSprite(SpriteEyes, HeadStructure.Eyes);
    SetupChildSprite(SpriteEyelids, HeadStructure.Eyelids);
    SetupChildSprite(SpriteMouth, HeadStructure.Mouth);
}

void ACharacter2DActor::SetupEffectLayers()
{
    if (!CharacterAsset) return;
    const auto& HeadStructure = CharacterAsset->SpriteStructure.Head;
    
    auto SetupLayer = [this](UPaperSpriteComponent* Component, const FCharacter2DEffectLayer& Layer)
    {
        if (!Component) return;
        Component->SetSprite(Layer.Sprite);
        Component->SetRelativeLocation(Layer.LocalOffset);
        Component->SetRelativeScale3D(FVector(Layer.LocalScale));
        Component->SetVisibility(Layer.bVisible && bSpritesVisible);
        FLinearColor Color = Layer.Color;
        Color.A = Layer.Opacity;
        Component->SetSpriteColor(Color);
    };
    
    SetupLayer(SpriteEffect1, HeadStructure.EffectLayer1);
    SetupLayer(SpriteEffect2, HeadStructure.EffectLayer2);
    SetupLayer(SpriteEffect3, HeadStructure.EffectLayer3);
}

void ACharacter2DActor::AttachHeadToSocket()
{
    if (!CharacterAsset) return;
    const auto& HeadRoot = CharacterAsset->SpriteStructure.Head.Head;
    AttachSpriteToSocket(SpriteHead, HeadRoot.AttachmentTarget, HeadRoot.SocketName, HeadRoot.bUseSocketTransform, HeadRoot.Offset, HeadRoot.Scale);
}

void ACharacter2DActor::SetEyebrowSprite(UPaperSprite* NewSprite) { if (SpriteEyebrow) SpriteEyebrow->SetSprite(NewSprite); }
void ACharacter2DActor::SetEyesSprite(UPaperSprite* NewSprite) { if (SpriteEyes) SpriteEyes->SetSprite(NewSprite); }
void ACharacter2DActor::SetEyelidsSprite(UPaperSprite* NewSprite) { if (SpriteEyelids) { SpriteEyelids->SetSprite(NewSprite); if (!bIsBlinking) OriginalEyelidsSprite = NewSprite; } }
void ACharacter2DActor::SetMouthSprite(UPaperSprite* NewSprite) { if (SpriteMouth) { SpriteMouth->SetSprite(NewSprite); if (!bIsTalking) OriginalMouthSprite = NewSprite; } }
void ACharacter2DActor::SetHeadSprite(UPaperSprite* NewSprite) { if (SpriteHead) SpriteHead->SetSprite(NewSprite); }
void ACharacter2DActor::SetBodySprite(UPaperSprite* NewSprite) { if (SpriteBody) SpriteBody->SetSprite(NewSprite); }
void ACharacter2DActor::SetArmsSprite(UPaperSprite* NewSprite) { if (SpriteArms) SpriteArms->SetSprite(NewSprite); }

void ACharacter2DActor::SetEffectLayer1(UPaperSprite* Sprite, bool bVisible, const FLinearColor& Color, float Opacity) { if(SpriteEffect1){ SpriteEffect1->SetSprite(Sprite); SpriteEffect1->SetVisibility(bVisible && bSpritesVisible); FLinearColor FinalColor = Color; FinalColor.A = Opacity; SpriteEffect1->SetSpriteColor(FinalColor); } }
void ACharacter2DActor::SetEffectLayer2(UPaperSprite* Sprite, bool bVisible, const FLinearColor& Color, float Opacity) { if(SpriteEffect2){ SpriteEffect2->SetSprite(Sprite); SpriteEffect2->SetVisibility(bVisible && bSpritesVisible); FLinearColor FinalColor = Color; FinalColor.A = Opacity; SpriteEffect2->SetSpriteColor(FinalColor); } }
void ACharacter2DActor::SetEffectLayer3(UPaperSprite* Sprite, bool bVisible, const FLinearColor& Color, float Opacity) { if(SpriteEffect3){ SpriteEffect3->SetSprite(Sprite); SpriteEffect3->SetVisibility(bVisible && bSpritesVisible); FLinearColor FinalColor = Color; FinalColor.A = Opacity; SpriteEffect3->SetSpriteColor(FinalColor); } }

void ACharacter2DActor::ShowEffectLayer(int32 LayerIndex, bool bShow)
{
    UPaperSpriteComponent* EffectComponent = nullptr;
    switch (LayerIndex) { case 1: EffectComponent = SpriteEffect1; break; case 2: EffectComponent = SpriteEffect2; break; case 3: EffectComponent = SpriteEffect3; break; default: return; }
    if (EffectComponent) EffectComponent->SetVisibility(bShow && bSpritesVisible);
}

void ACharacter2DActor::ClearAllEffects()
{
    for (UPaperSpriteComponent* Effect : {SpriteEffect1, SpriteEffect2, SpriteEffect3}) { if (Effect) { Effect->SetSprite(nullptr); Effect->SetVisibility(false); } }
}

void ACharacter2DActor::EnableBlinking(bool bEnable)
{
    bBlinkingActive = bEnable;
    if (bSpritesVisible) { if (bEnable && !bIsBlinking) StartBlinking(); else if (!bEnable && bIsBlinking) StopBlinking(); }
}

void ACharacter2DActor::EnableTalking(bool bEnable)
{
    bTalkingActive = bEnable;
    if (bSpritesVisible) { if (bEnable && !bIsTalking) StartTalking(); else if (!bEnable && bIsTalking) StopTalking(); }
}

void ACharacter2DActor::BlinkOnce()
{
    if (!IsValid(this) || !IsValid(SpriteEyelids) || !CharacterAsset) return;
    const auto& Settings = CharacterAsset->GetBlinkSettings();
    OnBlinkStarted.Broadcast();
    bBlinkScheduleNext = false;
    GetWorldTimerManager().ClearTimer(BlinkFrameTimerHandle);

    if (Settings.ClosedEyelidsFlipbook && Settings.ClosedEyelidsFlipbook->GetNumFrames() >= 2)
    {
        CurrentBlinkFlipbook = Settings.ClosedEyelidsFlipbook;
        BlinkTotalFrames = CurrentBlinkFlipbook->GetNumFrames();
        BlinkFrameDuration = Settings.BlinkDuration / BlinkTotalFrames;
        BlinkFrameIndex = 0;
        if (UPaperSprite* FrameSprite = CurrentBlinkFlipbook->GetSpriteAtFrame(0))
        {
            SpriteEyelids->SetSprite(FrameSprite);
        }
        GetWorldTimerManager().SetTimer(BlinkFrameTimerHandle, this, &ACharacter2DActor::HandleBlinkFrame, BlinkFrameDuration, false);
    }
    else
    {
        if (Settings.ClosedEyelidsFlipbook && Settings.ClosedEyelidsFlipbook->GetNumFrames() > 0)
        {
            UPaperSprite* FrameSprite = Settings.ClosedEyelidsFlipbook->GetSpriteAtFrame(Settings.ClosedEyelidsFlipbook->GetNumFrames() - 1);
            if (FrameSprite) SpriteEyelids->SetSprite(FrameSprite);
        }
        else
        {
            SpriteEyelids->SetVisibility(false);
        }
        FTimerDelegate RestoreDelegate = FTimerDelegate::CreateLambda([this]()
        {
            if (!IsValid(this) || !IsValid(SpriteEyelids)) return;
            SpriteEyelids->SetSprite(OriginalEyelidsSprite);
            if (CharacterAsset)
            {
                SpriteEyelids->SetVisibility(bSpritesVisible && CharacterAsset->SpriteStructure.Head.GetFinalChildVisibility(CharacterAsset->SpriteStructure.Head.Eyelids));
            }
            OnBlinkFinished.Broadcast();
        });
        GetWorldTimerManager().SetTimer(BlinkTimerHandle, RestoreDelegate, Settings.BlinkDuration, false);
    }
}

void ACharacter2DActor::StartBlinking()
{
    if (!IsValid(this) || !IsValid(SpriteEyelids) || !CharacterAsset) return;
    bIsBlinking = true;
    const auto& Settings = CharacterAsset->GetBlinkSettings();
    const float Delay = FMath::FRandRange(Settings.BlinkIntervalMin, Settings.BlinkIntervalMax);
    GetWorldTimerManager().SetTimer(BlinkTimerHandle, this, &ACharacter2DActor::HandleBlink, Delay, false);
}

void ACharacter2DActor::StopBlinking()
{
    bIsBlinking = false;
    GetWorldTimerManager().ClearTimer(BlinkTimerHandle);
    if (IsValid(SpriteEyelids) && CharacterAsset)
    {
        SpriteEyelids->SetSprite(OriginalEyelidsSprite);
        SpriteEyelids->SetVisibility(CharacterAsset->SpriteStructure.Head.GetFinalChildVisibility(CharacterAsset->SpriteStructure.Head.Eyelids) && bSpritesVisible);
    }
}

void ACharacter2DActor::HandleBlink()
{
    if (!bIsBlinking || !CharacterAsset || !IsValid(SpriteEyelids)) { StopBlinking(); return; }
    const auto& Settings = CharacterAsset->GetBlinkSettings();
    OnBlinkStarted.Broadcast();
    bBlinkScheduleNext = true;
    GetWorldTimerManager().ClearTimer(BlinkFrameTimerHandle);

    if (Settings.ClosedEyelidsFlipbook && Settings.ClosedEyelidsFlipbook->GetNumFrames() >= 2)
    {
        CurrentBlinkFlipbook = Settings.ClosedEyelidsFlipbook;
        BlinkTotalFrames = CurrentBlinkFlipbook->GetNumFrames();
        BlinkFrameDuration = Settings.BlinkDuration / BlinkTotalFrames;
        BlinkFrameIndex = 0;
        if (UPaperSprite* FrameSprite = CurrentBlinkFlipbook->GetSpriteAtFrame(0))
        {
            SpriteEyelids->SetSprite(FrameSprite);
        }
        GetWorldTimerManager().SetTimer(BlinkFrameTimerHandle, this, &ACharacter2DActor::HandleBlinkFrame, BlinkFrameDuration, false);
    }
    else
    {
        if (Settings.ClosedEyelidsFlipbook && Settings.ClosedEyelidsFlipbook->GetNumFrames() > 0)
        {
            UPaperSprite* FrameSprite = Settings.ClosedEyelidsFlipbook->GetSpriteAtFrame(Settings.ClosedEyelidsFlipbook->GetNumFrames() - 1);
            if (FrameSprite) SpriteEyelids->SetSprite(FrameSprite);
        }
        else
        {
            SpriteEyelids->SetVisibility(false);
        }
        FTimerDelegate RestoreDelegate = FTimerDelegate::CreateLambda([this]()
        {
            if (!IsValid(this) || !IsValid(SpriteEyelids) || !CharacterAsset) return;
            SpriteEyelids->SetSprite(OriginalEyelidsSprite);
            SpriteEyelids->SetVisibility(CharacterAsset->SpriteStructure.Head.GetFinalChildVisibility(CharacterAsset->SpriteStructure.Head.Eyelids) && bSpritesVisible);
            OnBlinkFinished.Broadcast();
            if (bIsBlinking)
            {
                const auto& S = CharacterAsset->GetBlinkSettings();
                const float NextDelay = FMath::FRandRange(S.BlinkIntervalMin, S.BlinkIntervalMax);
                GetWorldTimerManager().SetTimer(BlinkTimerHandle, this, &ACharacter2DActor::HandleBlink, NextDelay, false);
            }
        });
        GetWorldTimerManager().SetTimer(BlinkTimerHandle, RestoreDelegate, Settings.BlinkDuration, false);
    }
}

void ACharacter2DActor::HandleBlinkFrame()
{
    if (!IsValid(this) || !IsValid(SpriteEyelids) || !CurrentBlinkFlipbook) return;

    ++BlinkFrameIndex;
    if (BlinkFrameIndex < BlinkTotalFrames)
    {
        if (UPaperSprite* FrameSprite = CurrentBlinkFlipbook->GetSpriteAtFrame(BlinkFrameIndex))
        {
            SpriteEyelids->SetSprite(FrameSprite);
        }
        GetWorldTimerManager().SetTimer(BlinkFrameTimerHandle, this, &ACharacter2DActor::HandleBlinkFrame, BlinkFrameDuration, false);
    }
    else
    {
        GetWorldTimerManager().ClearTimer(BlinkFrameTimerHandle);
        SpriteEyelids->SetSprite(OriginalEyelidsSprite);
        if (CharacterAsset)
        {
            SpriteEyelids->SetVisibility(bSpritesVisible && CharacterAsset->SpriteStructure.Head.GetFinalChildVisibility(CharacterAsset->SpriteStructure.Head.Eyelids));
        }
        OnBlinkFinished.Broadcast();
        if (bBlinkScheduleNext && bIsBlinking && CharacterAsset)
        {
            const auto& Settings = CharacterAsset->GetBlinkSettings();
            const float NextDelay = FMath::FRandRange(Settings.BlinkIntervalMin, Settings.BlinkIntervalMax);
            GetWorldTimerManager().SetTimer(BlinkTimerHandle, this, &ACharacter2DActor::HandleBlink, NextDelay, false);
        }
    }
}

void ACharacter2DActor::StartTalking()
{
    if (!IsValid(SpriteMouth) || !CharacterAsset || !CharacterAsset->GetTalkSettings().TalkFlipbook) return;
    bIsTalking = true;
    OnTalkStarted.Broadcast();
    HandleTalkFrame();
}

void ACharacter2DActor::StopTalking()
{
    bIsTalking = false;
    GetWorldTimerManager().ClearTimer(TalkTimerHandle);
    if (IsValid(SpriteMouth) && CharacterAsset)
    {
        SpriteMouth->SetSprite(OriginalMouthSprite);
        SpriteMouth->SetVisibility(CharacterAsset->SpriteStructure.Head.GetFinalChildVisibility(CharacterAsset->SpriteStructure.Head.Mouth) && bSpritesVisible);
    }
    OnTalkStopped.Broadcast();
}

void ACharacter2DActor::HandleTalkFrame()
{
    if (!bIsTalking || !CharacterAsset || !IsValid(SpriteMouth) || !CharacterAsset->GetTalkSettings().TalkFlipbook) { StopTalking(); return; }
    const auto& Settings = CharacterAsset->GetTalkSettings();
    const int32 NumFrames = Settings.TalkFlipbook->GetNumFrames();
    if (NumFrames > 0)
    {
        UPaperSprite* RandomMouthSprite = Settings.TalkFlipbook->GetSpriteAtFrame(FMath::RandRange(0, NumFrames - 1));
        if (RandomMouthSprite) SpriteMouth->SetSprite(RandomMouthSprite);
    }
    GetWorldTimerManager().SetTimer(TalkTimerHandle, this, &ACharacter2DActor::HandleTalkFrame, Settings.MouthChangeInterval, false);
}

void ACharacter2DActor::SetSpritesVisible(bool bVisible)
{
    bSpritesVisible = bVisible;
    for (UPaperSpriteComponent* Component : GetAllSpriteComponents()) { if(Component) Component->SetVisibility(bVisible && Component->GetSprite() != nullptr); }
    if(CharacterAsset) SetupHeadHierarchy(); // Re-apply visibility rules
}

void ACharacter2DActor::SetSkeletalVisible(bool bVisible)
{
    bSkeletalVisible = bVisible;
    for (USkeletalMeshComponent* Component : GetAllSkeletalComponents()) { if(Component) Component->SetVisibility(bVisible); }
}

void ACharacter2DActor::SetBothVisible(bool bSprites, bool bSkeletal) { SetSpritesVisible(bSprites); SetSkeletalVisible(bSkeletal); }

void ACharacter2DActor::RefreshFromAsset()
{
    if (!CharacterAsset) { UE_LOG(LogCharacter2DActor, Warning, TEXT("RefreshFromAsset: CharacterAsset is null")); return; }
    bool bOldHidden = IsHidden();
    FTransform SavedTransform = GetActorTransform();
    bool bOldBlinkingActive = bBlinkingActive;
    bool bOldTalkingActive = bTalkingActive;
    
    StopAllAnimationsForRefresh();
    OnConstruction(SavedTransform);
    SetActorTransform(SavedTransform);
    SetActorHiddenInGame(bOldHidden);
    
    // ИСПРАВЛЕНО: Восстанавливаем анимации только если это игровой мир
    if (GetWorld() && GetWorld()->IsGameWorld())
    {
        EnableBlinking(bOldBlinkingActive);
        EnableTalking(bOldTalkingActive);
    }
    else
    {
        // В редакторе оставляем анимации выключенными
        // Они будут управляться через Action Panel
        bBlinkingActive = false;
        bTalkingActive = false;
    }
}

void ACharacter2DActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    StopAllAnimationsForRefresh();
}

void ACharacter2DActor::StopAllAnimationsForRefresh()
{
    GetWorldTimerManager().ClearTimer(BlinkTimerHandle);
    GetWorldTimerManager().ClearTimer(BlinkFrameTimerHandle);
    GetWorldTimerManager().ClearTimer(TalkTimerHandle);
    bIsBlinking = false;
    bIsTalking  = false;
}

void ACharacter2DActor::UpdateFromAssetPreserveState()
{
    if (!CharacterAsset) return;
    TMap<TObjectPtr<UPaperSpriteComponent>, TObjectPtr<UPaperSprite>> SavedSprites;
    for(auto* Comp : GetAllSpriteComponents()) { if(Comp) SavedSprites.Add(Comp, Comp->GetSprite()); }
    RefreshFromAsset();
    for (const auto& Pair : SavedSprites) { if (Pair.Key && Pair.Value) Pair.Key->SetSprite(Pair.Value); }
}

void ACharacter2DActor::StoreOriginalValues()
{
    OriginalActorLocation = GetActorLocation();
    OriginalActorScale = GetActorScale3D();
    OriginalSpriteColors.Empty();
    for (UPaperSpriteComponent* Component : GetAllSpriteComponents()) { if(Component) OriginalSpriteColors.Add(Component, Component->GetSpriteColor()); }
}

TArray<UPaperSpriteComponent*> ACharacter2DActor::GetAllSpriteComponents(bool bIncludeEffects) const
{
    TArray<UPaperSpriteComponent*> Components = {SpriteBody, SpriteArms, SpriteHead, SpriteEyebrow, SpriteEyes, SpriteEyelids, SpriteMouth};
    if(bIncludeEffects) Components.Append({SpriteEffect1, SpriteEffect2, SpriteEffect3});
    return Components;
}

TArray<USkeletalMeshComponent*> ACharacter2DActor::GetAllSkeletalComponents() const { return {BodyComponent, ArmsComponent, HeadComponent}; }

USkeletalMeshComponent* ACharacter2DActor::GetSkeletalComponentByTarget(ECharacter2DAttachmentTarget Target) const
{
    switch (Target) { case ECharacter2DAttachmentTarget::Body: return BodyComponent; case ECharacter2DAttachmentTarget::Arms: return ArmsComponent; case ECharacter2DAttachmentTarget::Head: return HeadComponent; default: return nullptr; }
}

bool ACharacter2DActor::HasValidSprites() const { return CharacterAsset && CharacterAsset->HasValidSpriteConfiguration(); }
bool ACharacter2DActor::HasValidSkeletalMeshes() const { return CharacterAsset && CharacterAsset->HasValidSkeletalConfiguration(); }

void ACharacter2DActor::SetupSkeletalComponent(USkeletalMeshComponent* Component, const FCharacter2DSkeletalPart& Part)
{
    if (!Component || !CharacterAsset) return;
    Component->SetSkeletalMesh(Part.Mesh);
    Component->SetAnimInstanceClass(Part.AnimInstance);
    for (const auto& Material : Part.Materials) { Component->SetMaterial(Material.SlotIndex, Material.Material); }
    Component->SetRelativeLocation(Part.Offset + CharacterAsset->SkeletalGlobalOffset);
    Component->SetRelativeScale3D(FVector(Part.Scale * CharacterAsset->GlobalScale));
    Component->SetVisibility(Part.Mesh != nullptr && bSkeletalVisible);
}

// ОБНОВЛЕНО: Добавлены параметры Color и Opacity
void ACharacter2DActor::SetupSpriteComponent(UPaperSpriteComponent* Component, TObjectPtr<UPaperSprite> Sprite, const FVector& Offset, float Scale, bool bIsVisible, const FLinearColor& Color, float Opacity)
{
    if (!Component || !CharacterAsset) return;
    const FVector FinalOffset = CharacterAsset->GetGlobalSpriteOffset() + Offset;
    const float FinalScale = CharacterAsset->GetGlobalSpriteScale() * Scale;
    Component->SetSprite(Sprite);
    Component->SetRelativeLocation(FinalOffset);
    Component->SetRelativeScale3D(FVector(FinalScale));
    Component->SetVisibility(bIsVisible && bSpritesVisible);
    
    // НОВОЕ: Применяем Color и Opacity
    FLinearColor FinalColor = Color;
    FinalColor.A = Opacity;
    Component->SetSpriteColor(FinalColor);
}

void ACharacter2DActor::AttachSpriteToSocket(UPaperSpriteComponent* SpriteComp, ECharacter2DAttachmentTarget TargetType, FName SocketName, bool bUseSocketTransform, const FVector& Offset, float Scale)
{
    if (!SpriteComp || !CharacterAsset || TargetType == ECharacter2DAttachmentTarget::None) return;
    USkeletalMeshComponent* TargetComponent = GetSkeletalComponentByTarget(TargetType);
    if (AttachComponentToSocket(SpriteComp, TargetComponent, SocketName, bUseSocketTransform))
    {
        if(!bUseSocketTransform)
        {
            SetupSpriteComponent(SpriteComp, SpriteComp->GetSprite(), Offset, Scale, SpriteComp->IsVisible());
        }
    }
}

bool ACharacter2DActor::AttachComponentToSocket(USceneComponent* Component, USkeletalMeshComponent* TargetMesh, FName SocketName, bool bUseSocketTransform)
{
    if (!Component || !TargetMesh || SocketName == NAME_None || !TargetMesh->DoesSocketExist(SocketName)) return false;
    Component->AttachToComponent(TargetMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
    if(bUseSocketTransform) { Component->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator); }
    return true;
}

void ACharacter2DActor::SetAllSpritesOpacity(float Opacity)
{
    for (UPaperSpriteComponent* Component : GetAllSpriteComponents()) { if (Component) { FLinearColor C = Component->GetSpriteColor(); C.A = FMath::Clamp(Opacity, 0.f, 1.f); Component->SetSpriteColor(C); } }
}

void ACharacter2DActor::SetAllSpritesColor(const FLinearColor& Color)
{
    for (UPaperSpriteComponent* Component : GetAllSpriteComponents()) { if (Component) { FLinearColor C = Color; C.A = Component->GetSpriteColor().A; Component->SetSpriteColor(C); } }
}

void ACharacter2DActor::SetAllSkeletalOpacity(float Opacity)
{
    for (USkeletalMeshComponent* Component : GetAllSkeletalComponents()) { if (Component) Component->SetVisibility((Opacity > 0.01f) && bSkeletalVisible); }
}