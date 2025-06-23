#include "Character2DActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "PaperFlipbook.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacter2DActor, Log, All);

ACharacter2DActor::ACharacter2DActor(const FObjectInitializer& ObjInit)
    : Super(ObjInit)
{
    PrimaryActorTick.bCanEverTick = true;

    // Корень и таймлайн-компоненты
    RootComponent        = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    TransitionTimeline   = CreateDefaultSubobject<UTimelineComponent>(TEXT("TransitionTimeline"));
    BlinkTimeline        = CreateDefaultSubobject<UTimelineComponent>(TEXT("BlinkTimeline"));
    TalkTimeline         = CreateDefaultSubobject<UTimelineComponent>(TEXT("TalkTimeline"));

    // Кривые создаём как default-subobjects, им даётся имя
    DefaultLinearCurve   = CreateDefaultSubobject<UCurveFloat>(TEXT("DefaultLinearCurve"));
    DefaultSmoothCurve   = CreateDefaultSubobject<UCurveFloat>(TEXT("DefaultSmoothCurve"));
    DefaultEaseInCurve   = CreateDefaultSubobject<UCurveFloat>(TEXT("DefaultEaseInCurve"));
    DefaultEaseOutCurve  = CreateDefaultSubobject<UCurveFloat>(TEXT("DefaultEaseOutCurve"));
    BlinkCurve           = CreateDefaultSubobject<UCurveFloat>(TEXT("BlinkCurve"));

    // Спрайты и остальное
    SetupComponents();

    // Только здесь заполняем FloatCurve у уже созданных саб-объектов
    InitializeDefaultCurves();
}

void ACharacter2DActor::RestoreEyelidsAfterBlink()
{
    if (IsValid(SpriteEyelids))
    {
        if (OriginalEyelidsSprite)
        {
            SpriteEyelids->SetSprite(OriginalEyelidsSprite);
        }
        else if(CharacterAsset)
        {
            SpriteEyelids->SetSprite(CharacterAsset->GetEyelidsSprite().Sprite);
        }
        
        if (CharacterAsset)
        {
            SpriteEyelids->SetVisibility(CharacterAsset->SpriteStructure.Head.GetFinalChildVisibility(CharacterAsset->SpriteStructure.Head.Eyelids) && bSpritesVisible);
        }
    }
}

void ACharacter2DActor::StartTalking()
{
    if (!IsValid(SpriteMouth) || !CharacterAsset || !CharacterAsset->GetTalkSettings().TalkFlipbook) return;
    
    UE_LOG(LogCharacter2DActor, Log, TEXT("StartTalking called"));
    
    bIsTalking = true;
    OnTalkStarted.Broadcast();

    // Настраиваем длительность цикла таймлайна
    const auto& Settings = CharacterAsset->GetTalkSettings();
    TalkTimeline->SetTimelineLength(Settings.MouthChangeInterval);

    // Сразу меняем спрайт, чтобы не ждать первого цикла
    HandleTalkTimelineEvent(); 
    TalkTimeline->Play();
}

void ACharacter2DActor::StopTalking()
{
    UE_LOG(LogCharacter2DActor, Log, TEXT("StopTalking called"));
    
    bIsTalking = false;
    if (TalkTimeline->IsPlaying())
    {
        TalkTimeline->Stop();
    }
    
    RestoreMouthAfterTalk();
    OnTalkStopped.Broadcast();
}

void ACharacter2DActor::HandleTalkTimelineEvent()
{
    // Эта функция вызывается в начале каждого цикла TalkTimeline
    if (!bIsTalking || !CharacterAsset || !IsValid(SpriteMouth)) 
    {
        StopTalking(); 
        return; 
    }

    const auto& Settings = CharacterAsset->GetTalkSettings();
    if (!Settings.TalkFlipbook)
    {
        StopTalking();
        return;
    }
    
    const int32 NumFrames = Settings.TalkFlipbook->GetNumFrames();
    if (NumFrames > 0)
    {
        const int32 RandomIndex = FMath::RandRange(0, NumFrames - 1);
        if (UPaperSprite* RandomMouthSprite = Settings.TalkFlipbook->GetSpriteAtFrame(RandomIndex))
        {
            SpriteMouth->SetSprite(RandomMouthSprite);
        }
    }
}

void ACharacter2DActor::RestoreMouthAfterTalk()
{
    if (IsValid(SpriteMouth))
    {
        if (OriginalMouthSprite)
        {
            SpriteMouth->SetSprite(OriginalMouthSprite);
        }
        else if (CharacterAsset)
        {
            SpriteMouth->SetSprite(CharacterAsset->GetMouthSprite().Sprite);
        }
        
        if (CharacterAsset)
        {
            SpriteMouth->SetVisibility(CharacterAsset->SpriteStructure.Head.GetFinalChildVisibility(CharacterAsset->SpriteStructure.Head.Mouth) && bSpritesVisible);
        }
    }
}

void ACharacter2DActor::SetSpritesVisible(bool bVisible)
{
    bSpritesVisible = bVisible;
    for (UPaperSpriteComponent* Component : GetAllSpriteComponents()) { if(Component) Component->SetVisibility(bVisible && Component->GetSprite() != nullptr); }
    if(CharacterAsset) SetupHeadHierarchy();
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
        // В редакторе восстанавливаем состояние анимаций (управляется через Action Panel)
        // НЕ запускаем автоматически, но сохраняем флаги активности
        bBlinkingActive = bOldBlinkingActive;
        bTalkingActive = bOldTalkingActive;
        
        // Если анимации были активны, перезапускаем их
        if (bBlinkingActive && !bIsBlinking) StartBlinking();
        if (bTalkingActive && !bIsTalking) StartTalking();
    }
}

void ACharacter2DActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    StopAllAnimationsForRefresh();
}

void ACharacter2DActor::StopAllAnimationsForRefresh()
{
    GetWorldTimerManager().ClearAllTimersForObject(this); // Очищаем все таймеры

    if (BlinkTimeline && BlinkTimeline->IsPlaying()) BlinkTimeline->Stop();
    if (TalkTimeline && TalkTimeline->IsPlaying()) TalkTimeline->Stop();
    if (TransitionTimeline && TransitionTimeline->IsPlaying()) TransitionTimeline->Stop();
    
    bIsBlinking = false;
    bIsTalking  = false;
    bIsInTransition = false;
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

void ACharacter2DActor::SetupSpriteComponent(UPaperSpriteComponent* Component, TObjectPtr<UPaperSprite> Sprite, const FVector& Offset, float Scale, bool bIsVisible, const FLinearColor& Color, float Opacity)
{
    if (!Component || !CharacterAsset) return;
    const FVector FinalOffset = CharacterAsset->GetGlobalSpriteOffset() + Offset;
    const float FinalScale = CharacterAsset->GetGlobalSpriteScale() * Scale;
    Component->SetSprite(Sprite);
    Component->SetRelativeLocation(FinalOffset);
    Component->SetRelativeScale3D(FVector(FinalScale));
    Component->SetVisibility(bIsVisible && bSpritesVisible);
    
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

UCurveFloat* ACharacter2DActor::GetDefaultCurve(ECharacter2DTransitionCurve CurveType) const
{
	switch(CurveType)
	{
		case ECharacter2DTransitionCurve::Linear:
			return DefaultLinearCurve;
		case ECharacter2DTransitionCurve::Smooth:
			return DefaultSmoothCurve;
		case ECharacter2DTransitionCurve::EaseIn:
			return DefaultEaseInCurve;
		case ECharacter2DTransitionCurve::EaseOut:
			return DefaultEaseOutCurve;
		default:
			return DefaultSmoothCurve; // По умолчанию используем smooth
	}
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

    // --- Привязка функций к таймлайну переходов ---
    if (TransitionTimeline)
    {
        FOnTimelineFloat TransitionUpdateCallback;
        TransitionUpdateCallback.BindUFunction(this, FName("HandleTransitionUpdate"));
        TransitionTimeline->SetLooping(false);
        // Добавление кривой будет происходить в StartTransition
        
        FOnTimelineEvent TransitionFinishedCallback;
        TransitionFinishedCallback.BindUFunction(this, FName("HandleTransitionFinished"));
        TransitionTimeline->SetTimelineFinishedFunc(TransitionFinishedCallback);
    }

    // --- НОВОЕ: Привязка функций к таймлайну моргания ---
    if (BlinkTimeline)
    {
        FOnTimelineFloat BlinkUpdateCallback;
        BlinkUpdateCallback.BindUFunction(this, FName("HandleBlinkTimelineUpdate"));
        BlinkTimeline->AddInterpFloat(BlinkCurve, BlinkUpdateCallback);
        BlinkTimeline->SetLooping(false);

        FOnTimelineEvent BlinkFinishedCallback;
        BlinkFinishedCallback.BindUFunction(this, FName("HandleBlinkTimelineFinished"));
        BlinkTimeline->SetTimelineFinishedFunc(BlinkFinishedCallback);
    }

    // --- НОВОЕ: Привязка функций к таймлайну разговора ---
    if (TalkTimeline)
    {
        FOnTimelineEvent TalkEventCallback;
        TalkEventCallback.BindUFunction(this, FName("HandleTalkTimelineEvent"));
        // Событие срабатывает в самом начале каждого цикла
        TalkTimeline->AddEvent(0.0f, TalkEventCallback);
        TalkTimeline->SetLooping(true);
    }
	
    StoreOriginalValues();
	
    if (CharacterAsset)
    {
        UpdateOriginalSprites();
		
        if (GetWorld() && GetWorld()->IsGameWorld())
        {
            EnableBlinking(CharacterAsset->bAutoBlink);
            EnableTalking(CharacterAsset->bAutoTalk);
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
    
    SetupSpriteComponent(SpriteBody, BodySpriteData.Sprite, BodySpriteData.Offset, BodySpriteData.Scale, BodySpriteData.bVisible, BodySpriteData.Color, BodySpriteData.Opacity);
    SetupSpriteComponent(SpriteArms, ArmsSpriteData.Sprite, ArmsSpriteData.Offset, ArmsSpriteData.Scale, ArmsSpriteData.bVisible, ArmsSpriteData.Color, ArmsSpriteData.Opacity);
    SetupHeadHierarchy();
    
    AttachSpriteToSocket(SpriteBody, BodySpriteData.AttachmentTarget, BodySpriteData.SocketName, BodySpriteData.bUseSocketTransform, BodySpriteData.Offset, BodySpriteData.Scale);
    AttachSpriteToSocket(SpriteArms, ArmsSpriteData.AttachmentTarget, ArmsSpriteData.SocketName, ArmsSpriteData.bUseSocketTransform, ArmsSpriteData.Offset, ArmsSpriteData.Scale);
    AttachHeadToSocket();

    SetupEffectLayers();
    SetSpritesVisible(HasValidSprites());
    SetSkeletalVisible(HasValidSkeletalMeshes());
	
    // ИСПРАВЛЕНО: Обновляем оригинальные спрайты после конструкции
    UpdateOriginalSprites();
}

// НОВАЯ ФУНКЦИЯ: Обновляет оригинальные спрайты из текущего состояния ассета
void ACharacter2DActor::UpdateOriginalSprites()
{
    if (!CharacterAsset) return;
    
    // Сохраняем текущие спрайты как "оригинальные" для анимаций
    if (SpriteEyelids && CharacterAsset->GetEyelidsSprite().Sprite)
    {
        OriginalEyelidsSprite = CharacterAsset->GetEyelidsSprite().Sprite;
    }
    
    if (SpriteMouth && CharacterAsset->GetMouthSprite().Sprite)
    {
        OriginalMouthSprite = CharacterAsset->GetMouthSprite().Sprite;
    }
    
    UE_LOG(LogCharacter2DActor, Log, TEXT("Updated original sprites: Eyelids=%s, Mouth=%s"), 
           OriginalEyelidsSprite ? *OriginalEyelidsSprite->GetName() : TEXT("None"),
           OriginalMouthSprite ? *OriginalMouthSprite->GetName() : TEXT("None"));
}

// --- ОБНОВЛЕННЫЕ ФУНКЦИИ ПОЯВЛЕНИЯ/ИСЧЕЗНОВЕНИЯ ---

void ACharacter2DActor::Appear(float Duration, ECharacter2DTransitionCurve CurveType)
{
    UCurveFloat* CurveToUse = GetDefaultCurve(CurveType);
    StartTransition(ECharacter2DTransitionState::Appearing, CurveToUse, Duration);
}

void ACharacter2DActor::AppearWithCustomCurve(UCurveFloat* Curve, float Duration)
{
    if (!Curve)
    {
        UE_LOG(LogCharacter2DActor, Warning, TEXT("AppearWithCustomCurve: Custom curve is null, using default smooth curve"));
        Curve = GetDefaultCurve(ECharacter2DTransitionCurve::Smooth);
    }
    StartTransition(ECharacter2DTransitionState::Appearing, Curve, Duration);
}

void ACharacter2DActor::Disappear(float Duration, ECharacter2DTransitionCurve CurveType)
{
    UCurveFloat* CurveToUse = GetDefaultCurve(CurveType);
    StartTransition(ECharacter2DTransitionState::Disappearing, CurveToUse, Duration);
}

void ACharacter2DActor::DisappearWithCustomCurve(UCurveFloat* Curve, float Duration)
{
    if (!Curve)
    {
        UE_LOG(LogCharacter2DActor, Warning, TEXT("DisappearWithCustomCurve: Custom curve is null, using default smooth curve"));
        Curve = GetDefaultCurve(ECharacter2DTransitionCurve::Smooth);
    }
    StartTransition(ECharacter2DTransitionState::Disappearing, Curve, Duration);
}

void ACharacter2DActor::StartTransition(ECharacter2DTransitionState NewState, UCurveFloat* Curve, float Duration)
{
    if (IsInTransition())
    {
        StopCurrentTransition();
    }

    if (!Curve)
    {
        UE_LOG(LogCharacter2DActor, Warning, TEXT("StartTransition: Curve is null, using default smooth curve"));
        Curve = GetDefaultCurve(ECharacter2DTransitionCurve::Smooth);
    }

    CurrentTransitionState = NewState;
    CurrentTransitionCurve = Curve;

    OriginalSpriteColorsForTransition.Empty();
    for (UPaperSpriteComponent* SpriteComp : GetAllSpriteComponents(true))
    {
        if (SpriteComp)
        {
            OriginalSpriteColorsForTransition.Add(SpriteComp, SpriteComp->GetSpriteColor());
        }
    }
    
    if (CurrentTransitionState == ECharacter2DTransitionState::Appearing)
    {
        const FLinearColor StartColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
        for (auto const& [SpriteComp, OriginalColor] : OriginalSpriteColorsForTransition)
        {
            if (SpriteComp) SpriteComp->SetSpriteColor(StartColor);
        }
        SetAllSkeletalOpacity(0.f);
    }

    if (TransitionTimeline)
    {
        TransitionTimeline->Stop();
        
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
    HandleTransitionFinished();
}

void ACharacter2DActor::HandleTransitionUpdate(float Value)
{
    if (CurrentTransitionState == ECharacter2DTransitionState::Appearing)
    {
        if (Value <= 0.5f)
        {
            const float OpacityProgress = Value / 0.5f;
            const FLinearColor CurrentColor(0.f, 0.f, 0.f, OpacityProgress);
            SetAllSpritesColor(CurrentColor);
            SetAllSkeletalOpacity(OpacityProgress);
        }
        else
        {
            const float ColorProgress = (Value - 0.5f) / 0.5f;
            for (auto const& [SpriteComp, OriginalColor] : OriginalSpriteColorsForTransition)
            {
                if (SpriteComp)
                {
                    FLinearColor NewColor = FMath::Lerp(FLinearColor::Black, OriginalColor, ColorProgress);
                    NewColor.A = OriginalColor.A;
                    SpriteComp->SetSpriteColor(NewColor);
                }
            }
            SetAllSkeletalOpacity(1.f);
        }
    }
    else if (CurrentTransitionState == ECharacter2DTransitionState::Disappearing)
    {
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
            const float FinalOpacity = 1.0f - OpacityProgress;
            const FLinearColor CurrentColor(0.f, 0.f, 0.f, FinalOpacity);
            SetAllSpritesColor(CurrentColor);
            SetAllSkeletalOpacity(FinalOpacity);
        }
    }

    OnTransitionUpdate.Broadcast(CurrentTransitionState, Value);
}

void ACharacter2DActor::HandleTransitionFinished()
{
    if (CurrentTransitionState == ECharacter2DTransitionState::Appearing)
    {
        for (auto const& [SpriteComp, OriginalColor] : OriginalSpriteColorsForTransition)
        {
            if (SpriteComp) SpriteComp->SetSpriteColor(OriginalColor);
        }
        SetAllSkeletalOpacity(1.f);
    }
    else if (CurrentTransitionState == ECharacter2DTransitionState::Disappearing)
    {
        const FLinearColor EndColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
        SetAllSpritesColor(EndColor);
        SetAllSkeletalOpacity(0.f);
    }

    OnTransitionFinished.Broadcast(CurrentTransitionState);

    CurrentTransitionState = ECharacter2DTransitionState::None;
    CurrentTransitionCurve = nullptr;
    OriginalSpriteColorsForTransition.Empty();
}

// --- ОСТАЛЬНЫЕ ФУНКЦИИ ОСТАЮТСЯ БЕЗ ИЗМЕНЕНИЙ ---

void ACharacter2DActor::SetupHeadHierarchy()
{
    if (!CharacterAsset) return;

    const auto& HeadStructure = CharacterAsset->SpriteStructure.Head;
    
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
        
        FLinearColor FinalColor = ChildData.Color;
        FinalColor.A = ChildData.Opacity;
        Component->SetSpriteColor(FinalColor);
    };

    SetupChildSprite(SpriteEyebrow, HeadStructure.Eyebrows);
    SetupChildSprite(SpriteEyes, HeadStructure.Eyes);
    SetupChildSprite(SpriteEyelids, HeadStructure.Eyelids);
    SetupChildSprite(SpriteMouth, HeadStructure.Mouth);
}

// [Включаю все остальные функции без изменений для краткости - они остаются теми же]

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

void ACharacter2DActor::SetEyelidsSprite(UPaperSprite* NewSprite) 
{ 
    if (SpriteEyelids) 
    { 
        SpriteEyelids->SetSprite(NewSprite); 
        if (!bIsBlinking) 
        {
            OriginalEyelidsSprite = NewSprite;
            UE_LOG(LogCharacter2DActor, Log, TEXT("Updated OriginalEyelidsSprite to: %s"), 
                   NewSprite ? *NewSprite->GetName() : TEXT("None"));
        }
    }
}

void ACharacter2DActor::SetMouthSprite(UPaperSprite* NewSprite) 
{ 
    if (SpriteMouth) 
    { 
        SpriteMouth->SetSprite(NewSprite); 
        if (!bIsTalking) 
        {
            OriginalMouthSprite = NewSprite;
            UE_LOG(LogCharacter2DActor, Log, TEXT("Updated OriginalMouthSprite to: %s"), 
                   NewSprite ? *NewSprite->GetName() : TEXT("None"));
        }
    }
}

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
    UE_LOG(LogCharacter2DActor, Log, TEXT("EnableBlinking called: %s"), bEnable ? TEXT("true") : TEXT("false"));
    
    bBlinkingActive = bEnable;
    if (bSpritesVisible) 
    { 
        if (bEnable && !bIsBlinking) 
        {
            UpdateOriginalSprites();
            StartBlinking();
        }
        else if (!bEnable && bIsBlinking) 
        {
            StopBlinking();
        }
    }
}

void ACharacter2DActor::EnableTalking(bool bEnable)
{
    UE_LOG(LogCharacter2DActor, Log, TEXT("EnableTalking called: %s"), bEnable ? TEXT("true") : TEXT("false"));
    
    bTalkingActive = bEnable;
    if (bSpritesVisible) 
    { 
        if (bEnable && !bIsTalking) 
        {
            UpdateOriginalSprites();
            StartTalking();
        }
        else if (!bEnable && bIsTalking) 
        {
            StopTalking();
        }
    }
}

void ACharacter2DActor::BlinkOnce()
{
    if (!IsValid(this) || !IsValid(SpriteEyelids) || !CharacterAsset || BlinkTimeline->IsPlaying()) return;

    bBlinkScheduleNext = false; // Это одиночное моргание, не планируем следующее
    UpdateOriginalSprites();
    TriggerBlink();
}

void ACharacter2DActor::TriggerBlink()
{
    if (!bIsBlinking && !bBlinkScheduleNext) // Дополнительная проверка для BlinkOnce
    {
        if (!CharacterAsset || !SpriteEyelids || !IsValid(this)) return;
    }
    else if (!bIsBlinking) return;
    
    const auto& Settings = CharacterAsset->GetBlinkSettings();
    if (!Settings.ClosedEyelidsFlipbook || Settings.ClosedEyelidsFlipbook->GetNumFrames() == 0) return;

    OnBlinkStarted.Broadcast();
    
    CurrentBlinkFlipbook = Settings.ClosedEyelidsFlipbook;
    const int32 NumFrames = CurrentBlinkFlipbook->GetNumFrames();
    const float BlinkDuration = Settings.BlinkDuration;
    
    // Настраиваем кривую, чтобы она выдавала значения от 0 до (NumFrames - 1)
    BlinkCurve->FloatCurve.Reset();
    BlinkCurve->FloatCurve.AddKey(0.0f, 0.0f); // Начальный кадр
    BlinkCurve->FloatCurve.AddKey(BlinkDuration, NumFrames); // Конечный кадр (timeline сам дойдет до него)

    BlinkTimeline->SetPlayRate(1.0f);
    BlinkTimeline->SetTimelineLength(BlinkDuration);
    BlinkTimeline->PlayFromStart();
}

void ACharacter2DActor::StartBlinking()
{
    if (!IsValid(this) || !IsValid(SpriteEyelids) || !CharacterAsset) return;
    
    UE_LOG(LogCharacter2DActor, Log, TEXT("StartBlinking called"));
    
    bIsBlinking = true;
    bBlinkScheduleNext = true;

    // Запускаем первое моргание после случайной задержки
    const auto& Settings = CharacterAsset->GetBlinkSettings();
    const float Delay = FMath::FRandRange(Settings.BlinkIntervalMin, Settings.BlinkIntervalMax);
    GetWorldTimerManager().SetTimer(BlinkTimerHandle, this, &ACharacter2DActor::TriggerBlink, Delay, false);
}

void ACharacter2DActor::StopBlinking()
{
    UE_LOG(LogCharacter2DActor, Log, TEXT("StopBlinking called"));
    
    bIsBlinking = false;
    bBlinkScheduleNext = false;
    
    GetWorldTimerManager().ClearTimer(BlinkTimerHandle); // Отменяем запланированное моргание
    if (BlinkTimeline && BlinkTimeline->IsPlaying())
    {
        BlinkTimeline->Stop();
    }
    
    RestoreEyelidsAfterBlink();
}

void ACharacter2DActor::HandleBlinkTimelineUpdate(float Value)
{
    if (!CurrentBlinkFlipbook) return;

    // Value - это текущий номер кадра (может быть дробным)
    int32 FrameIndex = FMath::Clamp(FMath::FloorToInt(Value), 0, CurrentBlinkFlipbook->GetNumFrames() - 1);
    
    if (UPaperSprite* FrameSprite = CurrentBlinkFlipbook->GetSpriteAtFrame(FrameIndex))
    {
        SpriteEyelids->SetSprite(FrameSprite);
    }
}

void ACharacter2DActor::HandleBlinkTimelineFinished()
{
    RestoreEyelidsAfterBlink();
    OnBlinkFinished.Broadcast();

    CurrentBlinkFlipbook = nullptr;

    // Если моргание должно продолжаться, планируем следующее
    if (bBlinkScheduleNext && bIsBlinking && CharacterAsset)
    {
        const auto& Settings = CharacterAsset->GetBlinkSettings();
        const float NextDelay = FMath::FRandRange(Settings.BlinkIntervalMin, Settings.BlinkIntervalMax);
        GetWorldTimerManager().SetTimer(BlinkTimerHandle, this, &ACharacter2DActor::TriggerBlink, NextDelay, false);
    }
}

void ACharacter2DActor::InitializeDefaultCurves()
{
    // 1) Linear
    {
        FRichCurve& C = DefaultLinearCurve->FloatCurve;
        C.Reset();
        C.AddKey(0.f, 0.f);
        C.AddKey(1.f, 1.f);
    }

    // 2) Smooth (Ease In-Out)
    {
        FRichCurve& C = DefaultSmoothCurve->FloatCurve;
        auto AddCubic = [&](float T, float V)
        {
            FKeyHandle H = C.AddKey(T, V);
            FRichCurveKey& K = C.GetKey(H);
            K.InterpMode    = ERichCurveInterpMode::RCIM_Cubic;
            K.ArriveTangent = 0.f;
            K.LeaveTangent  = 0.f;
        };
        AddCubic(0.f, 0.f);
        AddCubic(1.f, 1.f);
    }

    // 3) Ease-In
    {
        FRichCurve& C = DefaultEaseInCurve->FloatCurve;
        auto AddEaseIn = [&](float T, float V, float Arr)
        {
            FKeyHandle H = C.AddKey(T, V);
            FRichCurveKey& K = C.GetKey(H);
            K.InterpMode    = ERichCurveInterpMode::RCIM_Cubic;
            K.ArriveTangent = Arr;
            K.LeaveTangent  = 0.f;
        };
        AddEaseIn(0.f, 0.f, 0.f);
        AddEaseIn(1.f, 1.f, 3.f);
    }

    // 4) Ease-Out
    {
        FRichCurve& C = DefaultEaseOutCurve->FloatCurve;
        auto AddEaseOut = [&](float T, float V, float Leave)
        {
            FKeyHandle H = C.AddKey(T, V);
            FRichCurveKey& K = C.GetKey(H);
            K.InterpMode    = ERichCurveInterpMode::RCIM_Cubic;
            K.ArriveTangent = 0.f;
            K.LeaveTangent  = Leave;
        };
        AddEaseOut(0.f, 0.f, 3.f);
        AddEaseOut(1.f, 1.f, 0.f);
    }

    UE_LOG(LogCharacter2DActor, Log, TEXT("Initialized default transition curves"));
}