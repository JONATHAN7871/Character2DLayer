#include "Character2DActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TimelineComponent.h"
#include "Engine/World.h"
#include "Curves/CurveFloat.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacter2DActor, Log, All);

ACharacter2DActor::ACharacter2DActor()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    SetupComponents();
}

void ACharacter2DActor::SetupComponents()
{
    /* ---------- Skeletal Components ---------- */
    BodyComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyComponent"));
    ArmsComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ArmsComponent"));
    HeadComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HeadComponent"));
    
    BodyComponent->SetupAttachment(RootComponent);
    ArmsComponent->SetupAttachment(RootComponent);
    HeadComponent->SetupAttachment(RootComponent);

    /* ---------- Sprite Components ----------- */
    SpriteBody = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteBody"));
    SpriteArms = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteArms"));
    SpriteHead = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteHead"));
    SpriteEyebrow = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteEyebrow"));
    SpriteEyes = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteEyes"));
    SpriteEyelids = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteEyelids"));
    SpriteMouth = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteMouth"));

    // NEW HIERARCHY: Head is parent, facial sprites are children
    SpriteBody->SetupAttachment(RootComponent);
    SpriteArms->SetupAttachment(RootComponent);
    SpriteHead->SetupAttachment(RootComponent);  // Head attaches to root or skeletal mesh

    // Facial elements are children of Head (inherit transforms)
    SpriteEyebrow->SetupAttachment(SpriteHead);
    SpriteEyes->SetupAttachment(SpriteHead);
    SpriteEyelids->SetupAttachment(SpriteHead);
    SpriteMouth->SetupAttachment(SpriteHead);

    // Common sprite component settings
    TArray<UPaperSpriteComponent*> SpriteComponents = GetAllSpriteComponents();
    for (UPaperSpriteComponent* Component : SpriteComponents)
    {
        Component->SetCastShadow(false);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    /* ---------- Flipbook Components --------- */
    EyelidComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("EyelidFlipbook"));
    MouthComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("MouthFlipbook"));

    // Flipbook animations also follow the head hierarchy
    EyelidComponent->SetupAttachment(SpriteHead);
    MouthComponent->SetupAttachment(SpriteHead);

    /* ---------- Timeline Components --------- */
    MovementTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("MovementTimeline"));
    EmotionTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("EmotionTimeline"));
    FadeTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("FadeTimeline"));
}

void ACharacter2DActor::BeginPlay()
{
    Super::BeginPlay();
    
    StoreOriginalValues();
    
    if (CharacterAsset)
    {
        EnableBlinking(CharacterAsset->bAutoBlink);
        EnableTalking(CharacterAsset->bAutoTalk);
    }
}

void ACharacter2DActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (!CharacterAsset) return;

    // ИСПРАВЛЕНИЕ: Сброс всех трансформаций перед применением новых
    ResetAllComponentTransforms();

    // Setup skeletal parts
    SetupSkeletalComponent(BodyComponent, CharacterAsset->Body);
    SetupSkeletalComponent(ArmsComponent, CharacterAsset->Arms);
    SetupSkeletalComponent(HeadComponent, CharacterAsset->Head);

    // Setup sprite parts using new hierarchical structure
    SetupSpriteComponentFromStruct(SpriteBody, CharacterAsset->SpriteStructure.Body);
    SetupSpriteComponentFromStruct(SpriteArms, CharacterAsset->SpriteStructure.Arms);
    
    // ═══ НОВАЯ ПОСЛЕДОВАТЕЛЬНОСТЬ НАСТРОЙКИ ГОЛОВЫ ═══
    SetupHeadHierarchy();

    // Attach sprites to sockets if specified (порядок важен!)
    AttachSpriteToSocketFromStruct(SpriteBody, CharacterAsset->SpriteStructure.Body);
    AttachSpriteToSocketFromStruct(SpriteArms, CharacterAsset->SpriteStructure.Arms);
    
    // ИСПРАВЛЕНИЕ: Head attachment выполняется ПОСЛЕ настройки иерархии
    AttachHeadToSocket();

    // Setup flipbook components with new hierarchy
    SetupHeadAnimations();

    // Set initial visibility based on dual rendering setting
    SetSpritesVisible(CharacterAsset->bEnableDualRendering || !HasValidSkeletalMeshes());
    SetSkeletalVisible(CharacterAsset->bEnableDualRendering || !HasValidSprites());
}

// НОВЫЙ МЕТОД: Сброс всех трансформаций компонентов
void ACharacter2DActor::ResetAllComponentTransforms()
{
    // Отсоединяем все спрайты от их родителей
    TArray<UPaperSpriteComponent*> AllSprites = GetAllSpriteComponents();
    for (UPaperSpriteComponent* Sprite : AllSprites)
    {
        if (Sprite && Sprite->GetAttachParent() != RootComponent)
        {
            Sprite->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            Sprite->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
        }
        
        // Сбрасываем трансформации
        if (Sprite)
        {
            Sprite->SetRelativeLocation(FVector::ZeroVector);
            Sprite->SetRelativeScale3D(FVector::OneVector);
        }
    }

    // Сбрасываем анимационные компоненты
    if (EyelidComponent)
    {
        EyelidComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        EyelidComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
        EyelidComponent->SetRelativeLocation(FVector::ZeroVector);
        EyelidComponent->SetRelativeScale3D(FVector::OneVector);
    }
    
    if (MouthComponent)
    {
        MouthComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        MouthComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
        MouthComponent->SetRelativeLocation(FVector::ZeroVector);
        MouthComponent->SetRelativeScale3D(FVector::OneVector);
    }
}

void ACharacter2DActor::SetupHeadHierarchy()
{
    if (!CharacterAsset) return;

    const auto& HeadStructure = CharacterAsset->SpriteStructure.Head;
    const FVector GlobalOffset = CharacterAsset->GetGlobalSpriteOffset();
    const float GlobalScale = CharacterAsset->GetGlobalSpriteScale();

    // ═══ Setup Head Root ═══
    const auto& HeadRoot = HeadStructure.Head;
    
    // ИСПРАВЛЕНИЕ: Прикрепляем голову к RootComponent с базовыми настройками
    SpriteHead->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
    SpriteHead->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
    
    // Head root использует Global настройки (будут скорректированы в AttachHeadToSocket)
    SpriteHead->SetSprite(HeadRoot.Sprite);
    SpriteHead->SetRelativeLocation(GlobalOffset + HeadRoot.Offset);
    SpriteHead->SetRelativeScale3D(FVector(GlobalScale * HeadRoot.Scale));
    SpriteHead->SetVisibility(HeadRoot.bVisible && bSpritesVisible);

    // ═══ Setup Head Children (всегда прикреплены к Head) ═══
    
    // Eyebrows
    const auto& Eyebrows = HeadStructure.Eyebrows;
    SpriteEyebrow->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
    SpriteEyebrow->AttachToComponent(SpriteHead, FAttachmentTransformRules::KeepRelativeTransform);
    SpriteEyebrow->SetSprite(Eyebrows.Sprite);
    SpriteEyebrow->SetRelativeLocation(Eyebrows.LocalOffset);
    SpriteEyebrow->SetRelativeScale3D(FVector(Eyebrows.LocalScale));
    SpriteEyebrow->SetVisibility(HeadStructure.GetFinalChildVisibility(Eyebrows) && bSpritesVisible);

    // Eyes
    const auto& Eyes = HeadStructure.Eyes;
    SpriteEyes->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
    SpriteEyes->AttachToComponent(SpriteHead, FAttachmentTransformRules::KeepRelativeTransform);
    SpriteEyes->SetSprite(Eyes.Sprite);
    SpriteEyes->SetRelativeLocation(Eyes.LocalOffset);
    SpriteEyes->SetRelativeScale3D(FVector(Eyes.LocalScale));
    SpriteEyes->SetVisibility(HeadStructure.GetFinalChildVisibility(Eyes) && bSpritesVisible);

    // Eyelids
    const auto& Eyelids = HeadStructure.Eyelids;
    SpriteEyelids->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
    SpriteEyelids->AttachToComponent(SpriteHead, FAttachmentTransformRules::KeepRelativeTransform);
    SpriteEyelids->SetSprite(Eyelids.Sprite);
    SpriteEyelids->SetRelativeLocation(Eyelids.LocalOffset);
    SpriteEyelids->SetRelativeScale3D(FVector(Eyelids.LocalScale));
    SpriteEyelids->SetVisibility(HeadStructure.GetFinalChildVisibility(Eyelids) && bSpritesVisible);

    // Mouth
    const auto& Mouth = HeadStructure.Mouth;
    SpriteMouth->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
    SpriteMouth->AttachToComponent(SpriteHead, FAttachmentTransformRules::KeepRelativeTransform);
    SpriteMouth->SetSprite(Mouth.Sprite);
    SpriteMouth->SetRelativeLocation(Mouth.LocalOffset);
    SpriteMouth->SetRelativeScale3D(FVector(Mouth.LocalScale));
    SpriteMouth->SetVisibility(HeadStructure.GetFinalChildVisibility(Mouth) && bSpritesVisible);
}

void ACharacter2DActor::AttachHeadToSocket()
{
    if (!CharacterAsset) return;

    const auto& HeadRoot = CharacterAsset->SpriteStructure.Head.Head;

    // Если attachment не нужен, оставляем голову прикрепленной к RootComponent
    if (HeadRoot.AttachmentTarget == ECharacter2DAttachmentTarget::None)
    {
        UE_LOG(LogCharacter2DActor, Verbose, TEXT("AttachHeadToSocket: No attachment target, keeping head on RootComponent"));
        return;
    }

    USkeletalMeshComponent* TargetComponent = GetSkeletalComponentByTarget(HeadRoot.AttachmentTarget);
    if (!TargetComponent)
    {
        UE_LOG(LogCharacter2DActor, Warning, TEXT("AttachHeadToSocket: Target skeletal component not found for %s"), 
               *UEnum::GetValueAsString(HeadRoot.AttachmentTarget));
        return;
    }

    // ИСПРАВЛЕНИЕ: Используем новый универсальный метод для прикрепления
    const FVector GlobalOffset = CharacterAsset->GetGlobalSpriteOffset();
    const float GlobalScale = CharacterAsset->GetGlobalSpriteScale();
    
    bool bSuccess = AttachComponentToSocket(
        SpriteHead,
        TargetComponent,
        HeadRoot.SocketName,
        HeadRoot.bUseSocketTransform,
        HeadRoot.Offset,
        HeadRoot.Scale,
        GlobalOffset,
        GlobalScale
    );
    
    if (bSuccess)
    {
        UE_LOG(LogCharacter2DActor, Log, TEXT("AttachHeadToSocket: Successfully attached head to socket '%s' on %s"),
               *HeadRoot.SocketName.ToString(), *UEnum::GetValueAsString(HeadRoot.AttachmentTarget));
        
        // Логируем финальную трансформацию для отладки
        LogComponentTransform(SpriteHead, TEXT("SpriteHead"));
    }
    else
    {
        UE_LOG(LogCharacter2DActor, Error, TEXT("AttachHeadToSocket: Failed to attach head to socket"));
    }
}

void ACharacter2DActor::SetupHeadAnimations()
{
    if (!CharacterAsset) return;

    const auto& HeadStructure = CharacterAsset->SpriteStructure.Head;

    // ═══ Setup Blink Animation ═══
    const auto& BlinkSettings = HeadStructure.BlinkSettings;
    
    // ИСПРАВЛЕНИЕ: Отсоединяем и прикрепляем к голове заново
    EyelidComponent->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
    EyelidComponent->AttachToComponent(SpriteHead, FAttachmentTransformRules::KeepRelativeTransform);
    
    EyelidComponent->SetFlipbook(BlinkSettings.BlinkFlipbook);
    EyelidComponent->SetVisibility(false);
    
    // Используем локальные координаты относительно головы
    EyelidComponent->SetRelativeLocation(BlinkSettings.LocalOffset);
    EyelidComponent->SetRelativeScale3D(FVector(BlinkSettings.LocalScale));

    // ═══ Setup Talk Animation ═══
    const auto& TalkSettings = HeadStructure.TalkSettings;
    
    // ИСПРАВЛЕНИЕ: Отсоединяем и прикрепляем к голове заново
    MouthComponent->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
    MouthComponent->AttachToComponent(SpriteHead, FAttachmentTransformRules::KeepRelativeTransform);
    
    MouthComponent->SetFlipbook(TalkSettings.TalkFlipbook);
    MouthComponent->SetVisibility(false);
    
    MouthComponent->SetRelativeLocation(TalkSettings.LocalOffset);
    MouthComponent->SetRelativeScale3D(FVector(TalkSettings.LocalScale));
}

void ACharacter2DActor::RefreshFromAsset()
{
    if (!CharacterAsset)
    {
        UE_LOG(LogCharacter2DActor, Warning, TEXT("RefreshFromAsset: CharacterAsset is null"));
        return;
    }

    // Сохраняем runtime состояние перед обновлением
    bool bOldSpritesVisible = bSpritesVisible;
    bool bOldSkeletalVisible = bSkeletalVisible;
    bool bOldBlinkingActive = bBlinkingActive;
    bool bOldTalkingActive = bTalkingActive;
    bool bOldActorHidden = IsHidden();
    
    // Сохраняем позицию, поворот и масштаб актера
    FVector SavedLocation = GetActorLocation();
    FRotator SavedRotation = GetActorRotation();
    FVector SavedScale = GetActorScale3D();
    
    // ИСПРАВЛЕНИЕ: Останавливаем все анимации перед обновлением
    StopAllAnimationsForRefresh();
    
    // Обновляем конфигурацию (пересоздаем все компоненты)
    OnConstruction(GetActorTransform());
    
    // ИСПРАВЛЕНИЕ: Восстанавливаем трансформацию актера
    SetActorLocation(SavedLocation);
    SetActorRotation(SavedRotation);
    SetActorScale3D(SavedScale);
    
    // Восстанавливаем runtime состояние
    SetActorHiddenInGame(bOldActorHidden);
    SetBothVisible(bOldSpritesVisible, bOldSkeletalVisible);
    
    // Восстанавливаем анимации в конце
    if (bOldBlinkingActive)
    {
        EnableBlinking(true);
    }
    if (bOldTalkingActive)
    {
        EnableTalking(true);
    }
    
    UE_LOG(LogCharacter2DActor, Log, TEXT("RefreshFromAsset: Asset refreshed, runtime state restored"));
}

void ACharacter2DActor::StopAllAnimationsForRefresh()
{
    // Останавливаем таймеры анимаций
    GetWorldTimerManager().ClearTimer(BlinkTimerHandle);
    GetWorldTimerManager().ClearTimer(BlinkRestoreHandle);
    
    // Останавливаем Timeline компоненты
    if (MovementTimeline)
    {
        MovementTimeline->Stop();
    }
    if (EmotionTimeline)
    {
        EmotionTimeline->Stop();
    }
    if (FadeTimeline)
    {
        FadeTimeline->Stop();
    }
    
    // Сбрасываем флаги анимаций
    bIsBlinking = false;
    bIsTalking = false;
    bIsMoving = false;
    bIsPlayingEmotion = false;
    bIsFading = false;
}

// ========== НОВЫЕ ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ ДЛЯ Character2DActor.cpp ==========

void ACharacter2DActor::ApplyComponentTransform(
    USceneComponent* Component,
    const FVector& LocalOffset,
    float LocalScale,
    const FVector& GlobalOffset,
    float GlobalScale
)
{
    if (!Component)
    {
        return;
    }
    
    // Вычисляем финальные трансформации
    const FVector FinalOffset = GlobalOffset + LocalOffset;
    const float FinalScale = GlobalScale * LocalScale;
    
    // Применяем трансформации
    Component->SetRelativeLocation(FinalOffset);
    Component->SetRelativeScale3D(FVector(FinalScale));
    
    UE_LOG(LogCharacter2DActor, Verbose, TEXT("ApplyComponentTransform: %s - Offset: %s, Scale: %f"),
           *Component->GetName(), *FinalOffset.ToString(), FinalScale);
}

bool ACharacter2DActor::AttachComponentToSocket(
    USceneComponent* Component,
    USkeletalMeshComponent* TargetMesh,
    FName SocketName,
    bool bUseSocketTransform,
    const FVector& LocalOffset,
    float LocalScale,
    const FVector& GlobalOffset,
    float GlobalScale
)
{
    if (!Component || !TargetMesh)
    {
        UE_LOG(LogCharacter2DActor, Warning, TEXT("AttachComponentToSocket: Invalid component or target mesh"));
        return false;
    }
    
    if (SocketName == NAME_None)
    {
        UE_LOG(LogCharacter2DActor, Warning, TEXT("AttachComponentToSocket: Socket name is None"));
        return false;
    }
    
    if (!IsSocketValid(TargetMesh, SocketName))
    {
        UE_LOG(LogCharacter2DActor, Warning, TEXT("AttachComponentToSocket: Socket '%s' not found on mesh"), *SocketName.ToString());
        return false;
    }
    
    // Отсоединяем от текущего родителя
    Component->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
    
    // Прикрепляем к сокету
    Component->AttachToComponent(TargetMesh, FAttachmentTransformRules::KeepRelativeTransform, SocketName);
    
    // Применяем трансформации в зависимости от UseSocketTransform
    if (bUseSocketTransform)
    {
        // Используем только локальное смещение от сокета
        Component->SetRelativeLocation(LocalOffset);
        Component->SetRelativeScale3D(FVector(LocalScale));
    }
    else
    {
        // Применяем полные глобальные + локальные трансформации
        ApplyComponentTransform(Component, LocalOffset, LocalScale, GlobalOffset, GlobalScale);
    }
    
    UE_LOG(LogCharacter2DActor, Log, TEXT("AttachComponentToSocket: %s attached to socket '%s', UseSocketTransform=%s"),
           *Component->GetName(), *SocketName.ToString(), bUseSocketTransform ? TEXT("true") : TEXT("false"));
    
    return true;
}

bool ACharacter2DActor::IsSocketValid(USkeletalMeshComponent* Component, FName SocketName) const
{
    if (!Component || SocketName == NAME_None)
    {
        return false;
    }
    
    return Component->DoesSocketExist(SocketName);
}

void ACharacter2DActor::LogComponentTransform(USceneComponent* Component, const FString& ComponentName) const
{
    if (!Component)
    {
        UE_LOG(LogCharacter2DActor, Warning, TEXT("LogComponentTransform: %s is null"), *ComponentName);
        return;
    }
    
    const FVector Location = Component->GetRelativeLocation();
    const FVector Scale = Component->GetRelativeScale3D();
    const FRotator Rotation = Component->GetRelativeRotation();
    
    USceneComponent* Parent = Component->GetAttachParent();
    FString ParentName = Parent ? Parent->GetName() : TEXT("None");
    
    UE_LOG(LogCharacter2DActor, Log, TEXT("Transform %s: Loc=%s, Rot=%s, Scale=%s, Parent=%s"),
           *ComponentName, *Location.ToString(), *Rotation.ToString(), *Scale.ToString(), *ParentName);
}

/* ====================================================================== */
/*                            Movement System                             */
/* ====================================================================== */

void ACharacter2DActor::MoveToLocation(const FVector& TargetLocation, float Duration)
{
    FCharacter2DMovementSettings Settings;
    Settings.Duration = Duration;
    Settings.bTeleport = (Duration <= 0.0f);
    MoveToLocationWithSettings(TargetLocation, Settings);
}

void ACharacter2DActor::MoveToLocationWithSettings(const FVector& TargetLocation, const FCharacter2DMovementSettings& Settings)
{
    if (Settings.bTeleport || Settings.Duration <= 0.0f)
    {
        SetActorLocation(TargetLocation);
        return;
    }

    if (bIsMoving)
    {
        MovementTimeline->Stop();
    }

    bIsMoving = true;
    MovementStartLocation = GetActorLocation();
    MovementTargetLocation = TargetLocation;
    CurrentMovementSettings = Settings;

    // Setup timeline
    MovementTimeline->Stop();
    MovementTimeline->SetPlaybackPosition(0.0f, false);

    // Create curve if none provided
    UCurveFloat* CurveToUse = Settings.AnimationCurve;
    if (!CurveToUse)
    {
        CurveToUse = NewObject<UCurveFloat>(this);
        CurveToUse->FloatCurve.AddKey(0.0f, 0.0f);
        CurveToUse->FloatCurve.AddKey(1.0f, 1.0f);
    }

    FOnTimelineFloat TimelineProgress;
    TimelineProgress.BindUFunction(this, FName("OnMovementTimelineUpdate"));
    MovementTimeline->AddInterpFloat(CurveToUse, TimelineProgress);

    FOnTimelineEvent TimelineFinish;
    TimelineFinish.BindUFunction(this, FName("OnMovementTimelineFinished"));
    MovementTimeline->SetTimelineFinishedFunc(TimelineFinish);

    MovementTimeline->SetTimelineLength(Settings.Duration);
    MovementTimeline->Play();
}

void ACharacter2DActor::OnMovementTimelineUpdate(float Value)
{
    FVector NewLocation = FMath::Lerp(MovementStartLocation, MovementTargetLocation, Value);
    SetActorLocation(NewLocation);
}

void ACharacter2DActor::OnMovementTimelineFinished()
{
    bIsMoving = false;
    SetActorLocation(MovementTargetLocation);
}

/* ====================================================================== */
/*                              Fade System                               */
/* ====================================================================== */

void ACharacter2DActor::PlayFadeIn(float Duration)
{
    if (bIsFading)
    {
        FadeTimeline->Stop();
    }

    bIsFading = true;
    
    // Start invisible
    SetAllSpritesOpacity(0.0f);
    SetAllSkeletalOpacity(0.0f);
    SetActorHiddenInGame(false);

    // Setup timeline
    FadeTimeline->Stop();
    FadeTimeline->SetPlaybackPosition(0.0f, false);

    UCurveFloat* FadeCurve = nullptr;
    if (CharacterAsset)
    {
        FadeCurve = CharacterAsset->VisualNovelSettings.DefaultFadeCurve;
    }
    
    if (!FadeCurve)
    {
        FadeCurve = NewObject<UCurveFloat>(this);
        FadeCurve->FloatCurve.AddKey(0.0f, 0.0f);
        FadeCurve->FloatCurve.AddKey(1.0f, 1.0f);
    }

    FOnTimelineFloat TimelineProgress;
    TimelineProgress.BindUFunction(this, FName("OnFadeTimelineUpdate"));
    FadeTimeline->AddInterpFloat(FadeCurve, TimelineProgress);

    FOnTimelineEvent TimelineFinish;
    TimelineFinish.BindUFunction(this, FName("OnFadeTimelineFinished"));
    FadeTimeline->SetTimelineFinishedFunc(TimelineFinish);

    FadeTimeline->SetTimelineLength(Duration);
    FadeTimeline->Play();
}

void ACharacter2DActor::PlayFadeOut(float Duration)
{
    if (bIsFading)
    {
        FadeTimeline->Stop();
    }

    bIsFading = true;
    
    // Setup timeline
    FadeTimeline->Stop();
    FadeTimeline->SetPlaybackPosition(0.0f, false);

    UCurveFloat* FadeCurve = nullptr;
    if (CharacterAsset)
    {
        FadeCurve = CharacterAsset->VisualNovelSettings.DefaultFadeCurve;
    }
    
    if (!FadeCurve)
    {
        FadeCurve = NewObject<UCurveFloat>(this);
        FadeCurve->FloatCurve.AddKey(0.0f, 1.0f);
        FadeCurve->FloatCurve.AddKey(1.0f, 0.0f);
    }

    FOnTimelineFloat TimelineProgress;
    TimelineProgress.BindUFunction(this, FName("OnFadeTimelineUpdate"));
    FadeTimeline->AddInterpFloat(FadeCurve, TimelineProgress);

    FOnTimelineEvent TimelineFinish;
    TimelineFinish.BindUFunction(this, FName("OnFadeTimelineFinished"));
    FadeTimeline->SetTimelineFinishedFunc(TimelineFinish);

    FadeTimeline->SetTimelineLength(Duration);
    FadeTimeline->Reverse();
    FadeTimeline->Play();
}

void ACharacter2DActor::OnFadeTimelineUpdate(float Value)
{
    SetAllSpritesOpacity(Value);
    SetAllSkeletalOpacity(Value);
}

void ACharacter2DActor::OnFadeTimelineFinished()
{
    bIsFading = false;
    
    // Check if we faded out completely
    if (FadeTimeline->GetPlaybackPosition() <= 0.01f)
    {
        SetActorHiddenInGame(true);
    }
}

/* ====================================================================== */
/*                          Visual Novel Emotions                         */
/* ====================================================================== */

void ACharacter2DActor::PlayEmotion(ECharacter2DEmotionEffect EmotionType, const FCharacter2DEmotionSettings& Settings)
{
    if (bIsPlayingEmotion)
    {
        StopCurrentEmotion();
    }

    if (EmotionType == ECharacter2DEmotionEffect::None)
    {
        return;
    }

    bIsPlayingEmotion = true;
    CurrentEmotionType = EmotionType;

    // Stop and reset timeline
    EmotionTimeline->Stop();
    EmotionTimeline->SetPlaybackPosition(0.0f, false);

    // Create a default curve if none provided
    UCurveFloat* CurveToUse = Settings.AnimationCurve;
    if (!CurveToUse)
    {
        CurveToUse = NewObject<UCurveFloat>(this);
        CurveToUse->FloatCurve.AddKey(0.0f, 0.0f);
        CurveToUse->FloatCurve.AddKey(0.5f, 1.0f);
        CurveToUse->FloatCurve.AddKey(1.0f, 0.0f);
    }

    // Setup timeline
    FOnTimelineFloat EmotionProgress;
    EmotionProgress.BindUFunction(this, FName("OnEmotionTimelineUpdate"));
    EmotionTimeline->AddInterpFloat(CurveToUse, EmotionProgress);

    FOnTimelineEvent EmotionFinish;
    EmotionFinish.BindUFunction(this, FName("OnEmotionTimelineFinished"));
    EmotionTimeline->SetTimelineFinishedFunc(EmotionFinish);

    EmotionTimeline->SetTimelineLength(Settings.Duration);
    EmotionTimeline->SetLooping(Settings.bLoop);

    // Store emotion settings
    CurrentEmotionSettings = Settings;

    // Execute specific emotion setup
    switch (EmotionType)
    {
    case ECharacter2DEmotionEffect::Shake:
        ExecuteShakeEmotion(Settings);
        break;
    case ECharacter2DEmotionEffect::Pulse:
        ExecutePulseEmotion(Settings);
        break;
    case ECharacter2DEmotionEffect::ColorShift:
        ExecuteColorShiftEmotion(Settings);
        break;
    case ECharacter2DEmotionEffect::Bounce:
        ExecuteBounceEmotion(Settings);
        break;
    case ECharacter2DEmotionEffect::Flash:
        ExecuteFlashEmotion(Settings);
        break;
    default:
        bIsPlayingEmotion = false;
        return;
    }

    EmotionTimeline->Play();
}

void ACharacter2DActor::PlayEmotionWithDefaults(ECharacter2DEmotionEffect EmotionType)
{
    if (!CharacterAsset)
    {
        FCharacter2DEmotionSettings DefaultSettings;
        DefaultSettings.Duration = 2.0f;
        DefaultSettings.Intensity = 0.5f;
        PlayEmotion(EmotionType, DefaultSettings);
        return;
    }

    PlayEmotion(EmotionType, CharacterAsset->VisualNovelSettings.DefaultEmotionSettings);
}

void ACharacter2DActor::StopCurrentEmotion()
{
    if (!bIsPlayingEmotion) return;

    if (EmotionTimeline)
    {
        EmotionTimeline->Stop();
    }
    RestoreOriginalValues();
    bIsPlayingEmotion = false;
    ECharacter2DEmotionEffect PreviousEmotion = CurrentEmotionType;
    CurrentEmotionType = ECharacter2DEmotionEffect::None;

    OnEmotionFinished.Broadcast(PreviousEmotion);
}

void ACharacter2DActor::OnEmotionTimelineUpdate(float Value)
{
    if (!bIsPlayingEmotion) return;

    const float IntensityMultiplier = CurrentEmotionSettings.Intensity;

    switch (CurrentEmotionType)
    {
    case ECharacter2DEmotionEffect::Shake:
        {
            float ShakeAmount = Value * IntensityMultiplier * 10.0f;
            FVector ShakeOffset = FVector(
                FMath::FRandRange(-ShakeAmount, ShakeAmount),
                FMath::FRandRange(-ShakeAmount, ShakeAmount),
                FMath::FRandRange(-ShakeAmount, ShakeAmount)
            );
            SetActorLocation(OriginalActorLocation + ShakeOffset);
        }
        break;

    case ECharacter2DEmotionEffect::Pulse:
        {
            float ScaleFactor = 1.0f + (Value * IntensityMultiplier * 0.2f);
            SetActorScale3D(OriginalActorScale * ScaleFactor);
        }
        break;

    case ECharacter2DEmotionEffect::ColorShift:
        {
            FLinearColor CurrentColor = FMath::Lerp(FLinearColor::White, CurrentEmotionSettings.TargetColor, Value * IntensityMultiplier);
            SetAllSpritesColor(CurrentColor);
        }
        break;

    case ECharacter2DEmotionEffect::Bounce:
        {
            float BounceHeight = Value * IntensityMultiplier * 50.0f;
            FVector BounceOffset = FVector(0, 0, BounceHeight);
            SetActorLocation(OriginalActorLocation + BounceOffset);
        }
        break;

    case ECharacter2DEmotionEffect::Flash:
        {
            float Opacity = Value > 0.5f ? 1.0f : (0.3f + 0.7f * IntensityMultiplier);
            SetAllSpritesOpacity(Opacity);
        }
        break;

    default:
        // Нет действия — используется, например, для ECharacter2DEmotionEffect::None
        break;
    }
}

void ACharacter2DActor::OnEmotionTimelineFinished()
{
    if (!EmotionTimeline->IsLooping())
    {
        StopCurrentEmotion();
    }
}

/* ====================================================================== */
/*                            Visibility Control                          */
/* ====================================================================== */

void ACharacter2DActor::SetSpritesVisible(bool bVisible)
{
    bSpritesVisible = bVisible;
    
    // ═══ NEW: Update head hierarchy visibility ═══
    if (CharacterAsset)
    {
        const auto& HeadStructure = CharacterAsset->SpriteStructure.Head;
        
        // Update head root
        SpriteHead->SetVisibility(bVisible && HeadStructure.Head.bVisible);
        
        // Update head children (respect inheritance)
        SpriteEyebrow->SetVisibility(bVisible && HeadStructure.GetFinalChildVisibility(HeadStructure.Eyebrows));
        SpriteEyes->SetVisibility(bVisible && HeadStructure.GetFinalChildVisibility(HeadStructure.Eyes));
        SpriteEyelids->SetVisibility(bVisible && HeadStructure.GetFinalChildVisibility(HeadStructure.Eyelids));
        SpriteMouth->SetVisibility(bVisible && HeadStructure.GetFinalChildVisibility(HeadStructure.Mouth));
    }
    else
    {
        // Fallback for legacy or missing asset
        SpriteHead->SetVisibility(bVisible);
        SpriteEyebrow->SetVisibility(bVisible);
        SpriteEyes->SetVisibility(bVisible);
        SpriteEyelids->SetVisibility(bVisible);
        SpriteMouth->SetVisibility(bVisible);
    }
    
    // Update body/arms
    SpriteBody->SetVisibility(bVisible);
    SpriteArms->SetVisibility(bVisible);
    
    // Update animations based on current state
    if (EyelidComponent)
    {
        EyelidComponent->SetVisibility(bVisible && bIsBlinking);
    }
    if (MouthComponent)
    {
        MouthComponent->SetVisibility(bVisible && bIsTalking);
    }
}

void ACharacter2DActor::SetSkeletalVisible(bool bVisible)
{
    bSkeletalVisible = bVisible;
    
    TArray<USkeletalMeshComponent*> SkeletalComponents = GetAllSkeletalComponents();
    for (USkeletalMeshComponent* Component : SkeletalComponents)
    {
        if (Component)
        {
            Component->SetVisibility(bVisible);
        }
    }
}

void ACharacter2DActor::SetBothVisible(bool bSprites, bool bSkeletal)
{
    SetSpritesVisible(bSprites);
    SetSkeletalVisible(bSkeletal);
}

/* ====================================================================== */
/*                            Helper Methods                              */
/* ====================================================================== */

void ACharacter2DActor::StoreOriginalValues()
{
    OriginalActorLocation = GetActorLocation();
    OriginalActorScale = GetActorScale3D();
    
    TArray<UPaperSpriteComponent*> SpriteComponents = GetAllSpriteComponents();
    for (int32 i = 0; i < SpriteComponents.Num() && i < 7; i++)
    {
        if (SpriteComponents[i])
        {
            OriginalSpriteColors[i] = SpriteComponents[i]->GetSpriteColor();
        }
    }
}

void ACharacter2DActor::RestoreOriginalValues()
{
    if (!bIsMoving)
    {
        SetActorLocation(OriginalActorLocation);
    }
    
    if (!bIsPlayingEmotion || CurrentEmotionType != ECharacter2DEmotionEffect::Pulse)
    {
        SetActorScale3D(OriginalActorScale);
    }
    
    TArray<UPaperSpriteComponent*> SpriteComponents = GetAllSpriteComponents();
    for (int32 i = 0; i < SpriteComponents.Num() && i < 7; i++)
    {
        if (SpriteComponents[i])
        {
            SpriteComponents[i]->SetSpriteColor(OriginalSpriteColors[i]);
        }
    }
}

TArray<UPaperSpriteComponent*> ACharacter2DActor::GetAllSpriteComponents() const
{
    return {
        SpriteBody, SpriteArms, SpriteHead, 
        SpriteEyebrow, SpriteEyes, SpriteEyelids, SpriteMouth
    };
}

TArray<USkeletalMeshComponent*> ACharacter2DActor::GetAllSkeletalComponents() const
{
    return { BodyComponent, ArmsComponent, HeadComponent };
}

USkeletalMeshComponent* ACharacter2DActor::GetSkeletalComponentByTarget(ECharacter2DAttachmentTarget Target) const
{
    switch (Target)
    {
    case ECharacter2DAttachmentTarget::Body:
        return BodyComponent;
    case ECharacter2DAttachmentTarget::Arms:
        return ArmsComponent;
    case ECharacter2DAttachmentTarget::Head:
        return HeadComponent;
    default:
        return nullptr;
    }
}

bool ACharacter2DActor::HasValidSprites() const
{
    if (!CharacterAsset) return false;
    
    const auto& SpriteStruct = CharacterAsset->SpriteStructure;
    return (SpriteStruct.Body.Sprite != nullptr ||
            SpriteStruct.Arms.Sprite != nullptr ||
            SpriteStruct.Head.Head.Sprite != nullptr ||
            SpriteStruct.Head.Eyes.Sprite != nullptr ||
            SpriteStruct.Head.Eyebrows.Sprite != nullptr ||
            SpriteStruct.Head.Eyelids.Sprite != nullptr ||
            SpriteStruct.Head.Mouth.Sprite != nullptr);
}

bool ACharacter2DActor::HasValidSkeletalMeshes() const
{
    if (!CharacterAsset) return false;
    
    return CharacterAsset->Body.Mesh || CharacterAsset->Arms.Mesh || CharacterAsset->Head.Mesh;
}

void ACharacter2DActor::SetupSkeletalComponent(USkeletalMeshComponent* Component, const FCharacter2DSkeletalPart& Part)
{
    if (!Component || !CharacterAsset) return;

    Component->SetSkeletalMesh(Part.Mesh);
    Component->SetAnimInstanceClass(Part.AnimInstance);
    
    for (const auto& Material : Part.Materials)
    {
        Component->SetMaterial(Material.SlotIndex, Material.Material);
    }
    
    const FVector GlobalOffset = CharacterAsset->SkeletalGlobalOffset;
    const float GlobalScale = CharacterAsset->GlobalScale;
    
    Component->SetRelativeLocation(Part.Offset + GlobalOffset);
    Component->SetRelativeScale3D(FVector(Part.Scale * GlobalScale));
    Component->SetVisibility(Part.Mesh != nullptr && bSkeletalVisible);
}

void ACharacter2DActor::SetupSpriteComponentFromStruct(UPaperSpriteComponent* Component, const FCharacter2DSpriteBodyStructure& BodyStruct)
{
    if (!Component || !CharacterAsset) return;

    const FVector GlobalOffset = CharacterAsset->GetGlobalSpriteOffset();
    const float GlobalScale = CharacterAsset->GetGlobalSpriteScale();
    
    const float FinalScale = GlobalScale * BodyStruct.Scale;
    const FVector FinalOffset = GlobalOffset + BodyStruct.Offset;
    
    Component->SetSprite(BodyStruct.Sprite);
    Component->SetRelativeLocation(FinalOffset);
    Component->SetRelativeScale3D(FVector(FinalScale));
    Component->SetVisibility(BodyStruct.bVisible && bSpritesVisible);
}

void ACharacter2DActor::SetupSpriteComponentFromStruct(UPaperSpriteComponent* Component, const FCharacter2DSpriteArmsStructure& ArmsStruct)
{
    if (!Component || !CharacterAsset) return;

    const FVector GlobalOffset = CharacterAsset->GetGlobalSpriteOffset();
    const float GlobalScale = CharacterAsset->GetGlobalSpriteScale();
    
    const float FinalScale = GlobalScale * ArmsStruct.Scale;
    const FVector FinalOffset = GlobalOffset + ArmsStruct.Offset;
    
    Component->SetSprite(ArmsStruct.Sprite);
    Component->SetRelativeLocation(FinalOffset);
    Component->SetRelativeScale3D(FVector(FinalScale));
    Component->SetVisibility(ArmsStruct.bVisible && bSpritesVisible);
}

void ACharacter2DActor::ApplyAttachmentTransform(
    UPaperSpriteComponent* SpriteComponent,
    USceneComponent* TargetComponent,
    const FName& SocketName,
    bool bUseSocketTransform,
    const FVector& LocalOffset,
    float LocalScale
)
{
    if (!SpriteComponent || !TargetComponent)
    {
        return;
    }

    // Обязательно сначала отсоединяем
    SpriteComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

    if (bUseSocketTransform && !SocketName.IsNone())
    {
        // Прикрепляем к сокету
        SpriteComponent->AttachToComponent(
            TargetComponent,
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            SocketName
        );

        // Offset = дополнительное смещение от сокета
        SpriteComponent->SetRelativeLocation(LocalOffset);
        SpriteComponent->SetRelativeScale3D(FVector(LocalScale));
    }
    else
    {
        // Прикрепляем без сокета
        SpriteComponent->AttachToComponent(
            TargetComponent,
            FAttachmentTransformRules::KeepRelativeTransform
        );

        // Offset = вся позиция целиком
        SpriteComponent->SetRelativeLocation(LocalOffset);
        SpriteComponent->SetRelativeScale3D(FVector(LocalScale));
    }
}

void ACharacter2DActor::AttachSpriteToSocketFromStruct(UPaperSpriteComponent* SpriteComp, const FCharacter2DSpriteBodyStructure& BodyStruct)
{
    if (!SpriteComp || !CharacterAsset) return;

    // Если attachment не требуется, оставляем компонент прикрепленным к RootComponent
    if (BodyStruct.AttachmentTarget == ECharacter2DAttachmentTarget::None)
    {
        return;
    }

    USkeletalMeshComponent* TargetComponent = GetSkeletalComponentByTarget(BodyStruct.AttachmentTarget);
    if (!TargetComponent)
    {
        UE_LOG(LogCharacter2DActor, Warning, TEXT("AttachSpriteToSocketFromStruct (Body): Target component not found"));
        return;
    }

    // ИСПРАВЛЕНИЕ: Используем новый универсальный метод
    const FVector GlobalOffset = CharacterAsset->GetGlobalSpriteOffset();
    const float GlobalScale = CharacterAsset->GetGlobalSpriteScale();
    
    AttachComponentToSocket(
        SpriteComp,
        TargetComponent,
        BodyStruct.SocketName,
        BodyStruct.bUseSocketTransform,
        BodyStruct.Offset,
        BodyStruct.Scale,
        GlobalOffset,
        GlobalScale
    );
}

void ACharacter2DActor::AttachSpriteToSocketFromStruct(UPaperSpriteComponent* SpriteComp, const FCharacter2DSpriteArmsStructure& ArmsStruct)
{
    if (!SpriteComp || !CharacterAsset) return;

    // Если attachment не требуется, оставляем компонент прикрепленным к RootComponent
    if (ArmsStruct.AttachmentTarget == ECharacter2DAttachmentTarget::None)
    {
        return;
    }

    USkeletalMeshComponent* TargetComponent = GetSkeletalComponentByTarget(ArmsStruct.AttachmentTarget);
    if (!TargetComponent)
    {
        UE_LOG(LogCharacter2DActor, Warning, TEXT("AttachSpriteToSocketFromStruct (Arms): Target component not found"));
        return;
    }

    // ИСПРАВЛЕНИЕ: Используем новый универсальный метод
    const FVector GlobalOffset = CharacterAsset->GetGlobalSpriteOffset();
    const float GlobalScale = CharacterAsset->GetGlobalSpriteScale();
    
    AttachComponentToSocket(
        SpriteComp,
        TargetComponent,
        ArmsStruct.SocketName,
        ArmsStruct.bUseSocketTransform,
        ArmsStruct.Offset,
        ArmsStruct.Scale,
        GlobalOffset,
        GlobalScale
    );
}

/* ====================================================================== */
/*                          Emotion Implementations                       */
/* ====================================================================== */

void ACharacter2DActor::ExecuteShakeEmotion(const FCharacter2DEmotionSettings& Settings)
{
    // Shake setup is minimal - actual shaking happens in timeline update
}

void ACharacter2DActor::ExecutePulseEmotion(const FCharacter2DEmotionSettings& Settings)
{
    // Pulse setup is minimal - actual pulsing happens in timeline update
}

void ACharacter2DActor::ExecuteColorShiftEmotion(const FCharacter2DEmotionSettings& Settings)
{
    // Store target color - actual shifting happens in timeline update
}

void ACharacter2DActor::ExecuteBounceEmotion(const FCharacter2DEmotionSettings& Settings)
{
    // Bounce setup is minimal - actual bouncing happens in timeline update
}

void ACharacter2DActor::ExecuteFlashEmotion(const FCharacter2DEmotionSettings& Settings)
{
    // Flash setup is minimal - actual flashing happens in timeline update
}

void ACharacter2DActor::SetAllSpritesOpacity(float Opacity)
{
    TArray<UPaperSpriteComponent*> SpriteComponents = GetAllSpriteComponents();
    for (UPaperSpriteComponent* Component : SpriteComponents)
    {
        if (Component)
        {
            FLinearColor CurrentColor = Component->GetSpriteColor();
            CurrentColor.A = FMath::Clamp(Opacity, 0.0f, 1.0f);
            Component->SetSpriteColor(CurrentColor);
        }
    }
    
    if (EyelidComponent && bIsBlinking)
    {
        FLinearColor EyelidColor = EyelidComponent->GetSpriteColor();
        EyelidColor.A = FMath::Clamp(Opacity, 0.0f, 1.0f);
        EyelidComponent->SetSpriteColor(EyelidColor);
    }
    
    if (MouthComponent && bIsTalking)
    {
        FLinearColor MouthColor = MouthComponent->GetSpriteColor();
        MouthColor.A = FMath::Clamp(Opacity, 0.0f, 1.0f);
        MouthComponent->SetSpriteColor(MouthColor);
    }
}

void ACharacter2DActor::SetAllSpritesColor(const FLinearColor& Color)
{
    TArray<UPaperSpriteComponent*> SpriteComponents = GetAllSpriteComponents();
    for (UPaperSpriteComponent* Component : SpriteComponents)
    {
        if (Component)
        {
            FLinearColor NewColor = Color;
            NewColor.A = Component->GetSpriteColor().A; // Preserve opacity
            Component->SetSpriteColor(NewColor);
        }
    }
    
    if (EyelidComponent && bIsBlinking)
    {
        FLinearColor NewColor = Color;
        NewColor.A = EyelidComponent->GetSpriteColor().A;
        EyelidComponent->SetSpriteColor(NewColor);
    }
    
    if (MouthComponent && bIsTalking)
    {
        FLinearColor NewColor = Color;
        NewColor.A = MouthComponent->GetSpriteColor().A;
        MouthComponent->SetSpriteColor(NewColor);
    }
}

void ACharacter2DActor::SetAllSkeletalOpacity(float Opacity)
{
    // For skeletal mesh opacity, we would need to create dynamic material instances
    // This is a simplified version that uses visibility
    TArray<USkeletalMeshComponent*> SkeletalComponents = GetAllSkeletalComponents();
    for (USkeletalMeshComponent* Component : SkeletalComponents)
    {
        if (Component)
        {
            bool bShouldBeVisible = (Opacity > 0.1f) && bSkeletalVisible;
            Component->SetVisibility(bShouldBeVisible);
        }
    }
}

void ACharacter2DActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    
    // Clear all timers
    GetWorldTimerManager().ClearTimer(BlinkTimerHandle);
    GetWorldTimerManager().ClearTimer(BlinkRestoreHandle);
    
    // Stop timelines
    if (MovementTimeline)
    {
        MovementTimeline->Stop();
    }
    
    if (EmotionTimeline)
    {
        EmotionTimeline->Stop();
    }
    
    if (FadeTimeline)
    {
        FadeTimeline->Stop();
    }
}

/* ====================================================================== */
/*                        Original Animation Methods                      */
/* ====================================================================== */

void ACharacter2DActor::EnableBlinking(bool bEnable)
{
    bBlinkingActive = bEnable;
    if (bSpritesVisible)
    {
        if (bEnable && !bIsBlinking)
        {
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
    bTalkingActive = bEnable;
    if (bSpritesVisible)
    {
        if (bEnable && !bIsTalking)
        {
            StartTalking();
        }
        else if (!bEnable && bIsTalking)
        {
            StopTalking();
        }
    }
}

void ACharacter2DActor::StartBlinking()
{
    if (!IsValid(this) || !IsValid(EyelidComponent) || !CharacterAsset) return;

    bIsBlinking = true;
    const auto& Settings = CharacterAsset->GetBlinkSettings();
    const float Delay = FMath::FRandRange(Settings.BlinkIntervalMin, Settings.BlinkIntervalMax);

    GetWorldTimerManager().SetTimer(
        BlinkTimerHandle, this, &ACharacter2DActor::HandleBlink, Delay, false);
}

void ACharacter2DActor::StopBlinking()
{
    bIsBlinking = false;
    GetWorldTimerManager().ClearTimer(BlinkTimerHandle);
    GetWorldTimerManager().ClearTimer(BlinkRestoreHandle);

    if (IsValid(SpriteEyelids) && CharacterAsset)
    {
        const auto& Eyelids = CharacterAsset->GetEyelidsSprite();
        SpriteEyelids->SetSprite(Eyelids.Sprite);
        
        // ═══ NEW: Use hierarchical visibility ═══
        const auto& HeadStructure = CharacterAsset->SpriteStructure.Head;
        SpriteEyelids->SetVisibility(HeadStructure.GetFinalChildVisibility(Eyelids) && bSpritesVisible);
    }
    if (IsValid(EyelidComponent))
    {
        EyelidComponent->SetVisibility(false);
        EyelidComponent->Stop();
    }
}

void ACharacter2DActor::HandleBlink()
{
    if (!bIsBlinking || !CharacterAsset || !IsValid(EyelidComponent) || !IsValid(SpriteEyelids))
    {
        StopBlinking();
        return;
    }

    const auto& Settings = CharacterAsset->GetBlinkSettings();
    if (!Settings.BlinkFlipbook)
    {
        StopBlinking();
        return;
    }

    // ═══ NEW: Check head visibility for inheritance ═══
    const auto& HeadStructure = CharacterAsset->SpriteStructure.Head;
    bool bShouldShowAnimation = bSpritesVisible && HeadStructure.Head.bVisible;
    
    // Скрываем статичный спрайт век
    SpriteEyelids->SetVisibility(false);
    
    // Показываем и запускаем анимацию моргания
    EyelidComponent->SetFlipbook(Settings.BlinkFlipbook);
    const float Rate = FMath::FRandRange(Settings.BlinkPlayRateMin, Settings.BlinkPlayRateMax);
    EyelidComponent->SetPlayRate(Rate);
    EyelidComponent->SetVisibility(bShouldShowAnimation);
    EyelidComponent->PlayFromStart();

    const float Duration = Settings.BlinkFlipbook->GetTotalDuration() / Rate;

    // Restore static eyelids after animation
    FTimerDelegate RestoreDelegate = FTimerDelegate::CreateLambda([this]()
    {
        if (!IsValid(this) || !IsValid(EyelidComponent) || !IsValid(SpriteEyelids) || !CharacterAsset)
            return;

        // Останавливаем и скрываем анимацию моргания
        EyelidComponent->Stop();
        EyelidComponent->SetVisibility(false);
        
        // ═══ NEW: Restore with hierarchical visibility ═══
        const auto& Eyelids = CharacterAsset->GetEyelidsSprite();
        const auto& HeadStructure = CharacterAsset->SpriteStructure.Head;
        SpriteEyelids->SetSprite(Eyelids.Sprite);
        SpriteEyelids->SetVisibility(HeadStructure.GetFinalChildVisibility(Eyelids) && bSpritesVisible);

        // Chance for double blink
        if (bIsBlinking && FMath::FRand() < 0.25f)
        {
            HandleBlink();
            return;
        }

        // Schedule next blink
        if (bIsBlinking)
        {
            const auto& S = CharacterAsset->GetBlinkSettings();
            const float NextDelay = FMath::FRandRange(S.BlinkIntervalMin, S.BlinkIntervalMax);
            GetWorldTimerManager().SetTimer(
                BlinkTimerHandle, this, &ACharacter2DActor::HandleBlink, NextDelay, false);
        }
    });

    GetWorldTimerManager().SetTimer(BlinkRestoreHandle, RestoreDelegate, Duration, false);
}

void ACharacter2DActor::StartTalking()
{
    if (!IsValid(MouthComponent) || !IsValid(SpriteMouth) || !CharacterAsset) return;

    bIsTalking = true;
    const auto& Settings = CharacterAsset->GetTalkSettings();
    if (!Settings.TalkFlipbook)
    {
        bIsTalking = false;
        return;
    }

    // ═══ NEW: Check head visibility for inheritance ═══
    const auto& HeadStructure = CharacterAsset->SpriteStructure.Head;
    bool bShouldShowAnimation = bSpritesVisible && HeadStructure.Head.bVisible;

    // Скрываем статичный спрайт рта
    SpriteMouth->SetVisibility(false);
    
    // Показываем и запускаем анимацию рта
    MouthComponent->SetFlipbook(Settings.TalkFlipbook);
    MouthComponent->SetPlayRate(Settings.TalkPlayRate);
    MouthComponent->SetLooping(true);
    MouthComponent->SetVisibility(bShouldShowAnimation);
    MouthComponent->Play();
}

void ACharacter2DActor::StopTalking()
{
    bIsTalking = false;
    
    // Останавливаем и скрываем анимацию рта
    if (IsValid(MouthComponent))
    {
        MouthComponent->Stop();
        MouthComponent->SetVisibility(false);
    }
    
    // ═══ NEW: Restore with hierarchical visibility ═══
    if (IsValid(SpriteMouth) && CharacterAsset)
    {
        const auto& Mouth = CharacterAsset->GetMouthSprite();
        const auto& HeadStructure = CharacterAsset->SpriteStructure.Head;
        SpriteMouth->SetSprite(Mouth.Sprite);
        SpriteMouth->SetVisibility(HeadStructure.GetFinalChildVisibility(Mouth) && bSpritesVisible);
    }
}

// ================ ТЕСТОВЫЙ КОД ДЛЯ ПРОВЕРКИ ИСПРАВЛЕНИЙ ================
// Добавить в Character2DActor.cpp для тестирования

#if WITH_EDITOR
// НОВЫЙ МЕТОД: Тестирование трансформаций головы
void ACharacter2DActor::TestHeadTransforms()
{
    if (!CharacterAsset)
    {
        UE_LOG(LogCharacter2DActor, Error, TEXT("TestHeadTransforms: No CharacterAsset"));
        return;
    }

    UE_LOG(LogCharacter2DActor, Warning, TEXT("=== TESTING HEAD TRANSFORMS ==="));
    
    const auto& HeadRoot = CharacterAsset->SpriteStructure.Head.Head;
    
    // Тест 1: Проверка базовых настроек
    UE_LOG(LogCharacter2DActor, Log, TEXT("Test 1: Basic Head Settings"));
    UE_LOG(LogCharacter2DActor, Log, TEXT("  AttachmentTarget: %s"), *UEnum::GetValueAsString(HeadRoot.AttachmentTarget));
    UE_LOG(LogCharacter2DActor, Log, TEXT("  SocketName: %s"), *HeadRoot.SocketName.ToString());
    UE_LOG(LogCharacter2DActor, Log, TEXT("  UseSocketTransform: %s"), HeadRoot.bUseSocketTransform ? TEXT("true") : TEXT("false"));
    UE_LOG(LogCharacter2DActor, Log, TEXT("  Offset: %s"), *HeadRoot.Offset.ToString());
    UE_LOG(LogCharacter2DActor, Log, TEXT("  Scale: %f"), HeadRoot.Scale);

    // Тест 2: Проверка attachment
    if (HeadRoot.AttachmentTarget != ECharacter2DAttachmentTarget::None)
    {
        USkeletalMeshComponent* TargetComponent = GetSkeletalComponentByTarget(HeadRoot.AttachmentTarget);
        UE_LOG(LogCharacter2DActor, Log, TEXT("Test 2: Attachment Validation"));
        UE_LOG(LogCharacter2DActor, Log, TEXT("  TargetComponent: %s"), TargetComponent ? *TargetComponent->GetName() : TEXT("NULL"));
        
        if (TargetComponent)
        {
            bool bSocketExists = TargetComponent->DoesSocketExist(HeadRoot.SocketName);
            UE_LOG(LogCharacter2DActor, Log, TEXT("  SocketExists: %s"), bSocketExists ? TEXT("true") : TEXT("false"));
            
            if (bSocketExists)
            {
                FTransform SocketTransform = TargetComponent->GetSocketTransform(HeadRoot.SocketName, RTS_World);
                UE_LOG(LogCharacter2DActor, Log, TEXT("  SocketWorldLocation: %s"), *SocketTransform.GetLocation().ToString());
                UE_LOG(LogCharacter2DActor, Log, TEXT("  SocketWorldRotation: %s"), *SocketTransform.GetRotation().Rotator().ToString());
                UE_LOG(LogCharacter2DActor, Log, TEXT("  SocketWorldScale: %s"), *SocketTransform.GetScale3D().ToString());
            }
        }
    }

    // Тест 3: Проверка текущего состояния компонента головы
    UE_LOG(LogCharacter2DActor, Log, TEXT("Test 3: Current Head Component State"));
    if (SpriteHead)
    {
        USceneComponent* Parent = SpriteHead->GetAttachParent();
        UE_LOG(LogCharacter2DActor, Log, TEXT("  Head Parent: %s"), Parent ? *Parent->GetName() : TEXT("None"));
        UE_LOG(LogCharacter2DActor, Log, TEXT("  Head AttachSocketName: %s"), *SpriteHead->GetAttachSocketName().ToString());
        UE_LOG(LogCharacter2DActor, Log, TEXT("  Head RelativeLocation: %s"), *SpriteHead->GetRelativeLocation().ToString());
        UE_LOG(LogCharacter2DActor, Log, TEXT("  Head RelativeScale: %s"), *SpriteHead->GetRelativeScale3D().ToString());
        UE_LOG(LogCharacter2DActor, Log, TEXT("  Head WorldLocation: %s"), *SpriteHead->GetComponentLocation().ToString());
        UE_LOG(LogCharacter2DActor, Log, TEXT("  Head Visible: %s"), SpriteHead->IsVisible() ? TEXT("true") : TEXT("false"));
    }

    // Тест 4: Проверка дочерних элементов
    UE_LOG(LogCharacter2DActor, Log, TEXT("Test 4: Child Elements State"));
    
    auto LogChildComponent = [this](UPaperSpriteComponent* Component, const FString& Name, const FCharacter2DHeadChildSprite& Data)
    {
        if (Component)
        {
            UE_LOG(LogCharacter2DActor, Log, TEXT("  %s:"), *Name);
            UE_LOG(LogCharacter2DActor, Log, TEXT("    Parent: %s"), Component->GetAttachParent() ? *Component->GetAttachParent()->GetName() : TEXT("None"));
            UE_LOG(LogCharacter2DActor, Log, TEXT("    RelativeLocation: %s"), *Component->GetRelativeLocation().ToString());
            UE_LOG(LogCharacter2DActor, Log, TEXT("    ExpectedLocalOffset: %s"), *Data.LocalOffset.ToString());
            UE_LOG(LogCharacter2DActor, Log, TEXT("    RelativeScale: %s"), *Component->GetRelativeScale3D().ToString());
            UE_LOG(LogCharacter2DActor, Log, TEXT("    ExpectedLocalScale: %f"), Data.LocalScale);
            UE_LOG(LogCharacter2DActor, Log, TEXT("    WorldLocation: %s"), *Component->GetComponentLocation().ToString());
            UE_LOG(LogCharacter2DActor, Log, TEXT("    Visible: %s"), Component->IsVisible() ? TEXT("true") : TEXT("false"));
        }
    };
    
    const auto& HeadStruct = CharacterAsset->SpriteStructure.Head;
    LogChildComponent(SpriteEyebrow, TEXT("Eyebrows"), HeadStruct.Eyebrows);
    LogChildComponent(SpriteEyes, TEXT("Eyes"), HeadStruct.Eyes);
    LogChildComponent(SpriteEyelids, TEXT("Eyelids"), HeadStruct.Eyelids);
    LogChildComponent(SpriteMouth, TEXT("Mouth"), HeadStruct.Mouth);

    // Тест 5: Проверка глобальных настроек
    UE_LOG(LogCharacter2DActor, Log, TEXT("Test 5: Global Settings"));
    UE_LOG(LogCharacter2DActor, Log, TEXT("  GlobalSpriteOffset: %s"), *CharacterAsset->GetGlobalSpriteOffset().ToString());
    UE_LOG(LogCharacter2DActor, Log, TEXT("  GlobalSpriteScale: %f"), CharacterAsset->GetGlobalSpriteScale());

    // Тест 6: Расчеты финальных позиций
    UE_LOG(LogCharacter2DActor, Log, TEXT("Test 6: Calculated Final Positions"));
    UE_LOG(LogCharacter2DActor, Log, TEXT("  Final Eyebrow Offset: %s"), *CharacterAsset->GetFinalEyebrowOffset().ToString());
    UE_LOG(LogCharacter2DActor, Log, TEXT("  Final Eyes Offset: %s"), *CharacterAsset->GetFinalEyesOffset().ToString());
    UE_LOG(LogCharacter2DActor, Log, TEXT("  Final Eyelids Offset: %s"), *CharacterAsset->GetFinalEyelidsOffset().ToString());
    UE_LOG(LogCharacter2DActor, Log, TEXT("  Final Mouth Offset: %s"), *CharacterAsset->GetFinalMouthOffset().ToString());

    UE_LOG(LogCharacter2DActor, Warning, TEXT("=== END HEAD TRANSFORMS TEST ==="));
}

// НОВЫЙ МЕТОД: Тестирование повторного применения трансформаций
void ACharacter2DActor::TestRepeatedRefresh()
{
    UE_LOG(LogCharacter2DActor, Warning, TEXT("=== TESTING REPEATED REFRESH ==="));
    
    if (!CharacterAsset)
    {
        UE_LOG(LogCharacter2DActor, Error, TEXT("No CharacterAsset for test"));
        return;
    }

    // Сохраняем начальное положение головы
    FVector InitialHeadLocation = SpriteHead ? SpriteHead->GetComponentLocation() : FVector::ZeroVector;
    UE_LOG(LogCharacter2DActor, Log, TEXT("Initial Head World Location: %s"), *InitialHeadLocation.ToString());

    // Выполняем 5 последовательных обновлений
    for (int32 i = 1; i <= 5; i++)
    {
        UE_LOG(LogCharacter2DActor, Log, TEXT("--- Refresh Iteration %d ---"), i);
        
        // Сохраняем позицию до обновления
        FVector BeforeLocation = SpriteHead ? SpriteHead->GetComponentLocation() : FVector::ZeroVector;
        
        // Выполняем обновление
        RefreshFromAsset();
        
        // Проверяем позицию после обновления
        FVector AfterLocation = SpriteHead ? SpriteHead->GetComponentLocation() : FVector::ZeroVector;
        FVector Delta = AfterLocation - BeforeLocation;
        
        UE_LOG(LogCharacter2DActor, Log, TEXT("  Before: %s"), *BeforeLocation.ToString());
        UE_LOG(LogCharacter2DActor, Log, TEXT("  After: %s"), *AfterLocation.ToString());
        UE_LOG(LogCharacter2DActor, Log, TEXT("  Delta: %s (Length: %f)"), *Delta.ToString(), Delta.Size());
        
        // Предупреждение о дрейфе позиции
        if (Delta.Size() > 1.0f)
        {
            UE_LOG(LogCharacter2DActor, Warning, TEXT("  POSITION DRIFT DETECTED! Delta too large"));
        }
        
        // Логируем attachment информацию
        if (SpriteHead)
        {
            USceneComponent* Parent = SpriteHead->GetAttachParent();
            FVector RelLoc = SpriteHead->GetRelativeLocation();
            UE_LOG(LogCharacter2DActor, Log, TEXT("  RelativeLocation: %s, Parent: %s"), 
                   *RelLoc.ToString(), Parent ? *Parent->GetName() : TEXT("None"));
        }
    }

    // Сравниваем финальную позицию с начальной
    FVector FinalHeadLocation = SpriteHead ? SpriteHead->GetComponentLocation() : FVector::ZeroVector;
    FVector TotalDrift = FinalHeadLocation - InitialHeadLocation;
    
    UE_LOG(LogCharacter2DActor, Warning, TEXT("FINAL RESULT:"));
    UE_LOG(LogCharacter2DActor, Log, TEXT("  Initial: %s"), *InitialHeadLocation.ToString());
    UE_LOG(LogCharacter2DActor, Log, TEXT("  Final: %s"), *FinalHeadLocation.ToString());
    UE_LOG(LogCharacter2DActor, Log, TEXT("  Total Drift: %s (Length: %f)"), *TotalDrift.ToString(), TotalDrift.Size());
    
    if (TotalDrift.Size() < 1.0f)
    {
        UE_LOG(LogCharacter2DActor, Log, TEXT("  SUCCESS: No significant drift detected"));
    }
    else
    {
        UE_LOG(LogCharacter2DActor, Error, TEXT("  FAILURE: Significant drift detected!"));
    }
    
    UE_LOG(LogCharacter2DActor, Warning, TEXT("=== END REPEATED REFRESH TEST ==="));
}

// НОВЫЙ МЕТОД: Тестирование UseSocketTransform
void ACharacter2DActor::TestSocketTransformModes()
{
    UE_LOG(LogCharacter2DActor, Warning, TEXT("=== TESTING SOCKET TRANSFORM MODES ==="));
    
    if (!CharacterAsset)
    {
        UE_LOG(LogCharacter2DActor, Error, TEXT("No CharacterAsset for test"));
        return;
    }

    auto& HeadRoot = CharacterAsset->SpriteStructure.Head.Head;
    
    if (HeadRoot.AttachmentTarget == ECharacter2DAttachmentTarget::None)
    {
        UE_LOG(LogCharacter2DActor, Warning, TEXT("No attachment target set, cannot test socket modes"));
        return;
    }

    // Сохраняем начальные настройки
    bool OriginalUseSocketTransform = HeadRoot.bUseSocketTransform;
    FVector OriginalOffset = HeadRoot.Offset;

    // Тест с UseSocketTransform = true
    UE_LOG(LogCharacter2DActor, Log, TEXT("Testing UseSocketTransform = TRUE"));
    HeadRoot.bUseSocketTransform = true;
    HeadRoot.Offset = FVector(10, 0, 0); // Тестовое смещение
    
    RefreshFromAsset();
    
    if (SpriteHead)
    {
        UE_LOG(LogCharacter2DActor, Log, TEXT("  Head RelativeLocation: %s"), *SpriteHead->GetRelativeLocation().ToString());
        UE_LOG(LogCharacter2DActor, Log, TEXT("  Head WorldLocation: %s"), *SpriteHead->GetComponentLocation().ToString());
        UE_LOG(LogCharacter2DActor, Log, TEXT("  Expected RelativeLocation: %s"), *HeadRoot.Offset.ToString());
    }

    // Тест с UseSocketTransform = false
    UE_LOG(LogCharacter2DActor, Log, TEXT("Testing UseSocketTransform = FALSE"));
    HeadRoot.bUseSocketTransform = false;
    
    RefreshFromAsset();
    
    if (SpriteHead)
    {
        FVector ExpectedRelativeLocation = CharacterAsset->GetGlobalSpriteOffset() + HeadRoot.Offset;
        UE_LOG(LogCharacter2DActor, Log, TEXT("  Head RelativeLocation: %s"), *SpriteHead->GetRelativeLocation().ToString());
        UE_LOG(LogCharacter2DActor, Log, TEXT("  Head WorldLocation: %s"), *SpriteHead->GetComponentLocation().ToString());
        UE_LOG(LogCharacter2DActor, Log, TEXT("  Expected RelativeLocation: %s"), *ExpectedRelativeLocation.ToString());
    }

    // Восстанавливаем оригинальные настройки
    HeadRoot.bUseSocketTransform = OriginalUseSocketTransform;
    HeadRoot.Offset = OriginalOffset;
    RefreshFromAsset();
    
    UE_LOG(LogCharacter2DActor, Log, TEXT("Original settings restored"));
    UE_LOG(LogCharacter2DActor, Warning, TEXT("=== END SOCKET TRANSFORM MODES TEST ==="));
}

#endif // WITH_EDITOR

// ============= ДОПОЛНИТЕЛЬНЫЕ ТЕСТОВЫЕ МЕТОДЫ ДЛЯ Character2DActor.cpp =============

#if WITH_EDITOR

void ACharacter2DActor::ForceRecreateHeadAttachments()
{
    UE_LOG(LogCharacter2DActor, Warning, TEXT("=== FORCE RECREATE HEAD ATTACHMENTS ==="));
    
    if (!CharacterAsset)
    {
        UE_LOG(LogCharacter2DActor, Error, TEXT("No CharacterAsset"));
        return;
    }

    // Сохраняем текущие состояния
    bool bWasVisible = bSpritesVisible;
    
    // Принудительно отсоединяем все спрайты головы
    TArray<UPaperSpriteComponent*> HeadSprites = {
        SpriteHead, SpriteEyebrow, SpriteEyes, SpriteEyelids, SpriteMouth
    };
    
    for (UPaperSpriteComponent* Sprite : HeadSprites)
    {
        if (Sprite)
        {
            UE_LOG(LogCharacter2DActor, Log, TEXT("Detaching %s from %s"), 
                   *Sprite->GetName(), 
                   Sprite->GetAttachParent() ? *Sprite->GetAttachParent()->GetName() : TEXT("None"));
            
            Sprite->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            Sprite->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
            Sprite->SetRelativeLocation(FVector::ZeroVector);
            Sprite->SetRelativeScale3D(FVector::OneVector);
        }
    }
    
    // Принудительно отсоединяем анимационные компоненты
    if (EyelidComponent)
    {
        EyelidComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        EyelidComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
        EyelidComponent->SetRelativeLocation(FVector::ZeroVector);
        EyelidComponent->SetRelativeScale3D(FVector::OneVector);
    }
    
    if (MouthComponent)
    {
        MouthComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        MouthComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
        MouthComponent->SetRelativeLocation(FVector::ZeroVector);
        MouthComponent->SetRelativeScale3D(FVector::OneVector);
    }

    UE_LOG(LogCharacter2DActor, Log, TEXT("All head components detached and reset"));

    // Пересоздаем иерархию с нуля
    SetupHeadHierarchy();
    AttachHeadToSocket();
    SetupHeadAnimations();
    
    // Восстанавливаем видимость
    SetSpritesVisible(bWasVisible);
    
    UE_LOG(LogCharacter2DActor, Warning, TEXT("=== HEAD ATTACHMENTS RECREATED ==="));
}

void ACharacter2DActor::LogAllComponentsInfo()
{
    UE_LOG(LogCharacter2DActor, Warning, TEXT("=== ALL COMPONENTS INFO ==="));
    
    // Информация о Skeletal компонентах
    UE_LOG(LogCharacter2DActor, Log, TEXT("SKELETAL COMPONENTS:"));
    
    auto LogSkeletalComponent = [this](USkeletalMeshComponent* Component, const FString& Name)
    {
        if (Component)
        {
            UE_LOG(LogCharacter2DActor, Log, TEXT("  %s:"), *Name);
            UE_LOG(LogCharacter2DActor, Log, TEXT("    Mesh: %s"), Component->GetSkeletalMeshAsset() ? *Component->GetSkeletalMeshAsset()->GetName() : TEXT("None"));
            UE_LOG(LogCharacter2DActor, Log, TEXT("    Location: %s"), *Component->GetRelativeLocation().ToString());
            UE_LOG(LogCharacter2DActor, Log, TEXT("    Scale: %s"), *Component->GetRelativeScale3D().ToString());
            UE_LOG(LogCharacter2DActor, Log, TEXT("    Visible: %s"), Component->IsVisible() ? TEXT("true") : TEXT("false"));
            
            // Информация о сокетах
            if (Component->GetSkeletalMeshAsset())
            {
                TArray<FName> SocketNames = Component->GetAllSocketNames();
                UE_LOG(LogCharacter2DActor, Log, TEXT("    Sockets: %d"), SocketNames.Num());
                for (const FName& SocketName : SocketNames)
                {
                    UE_LOG(LogCharacter2DActor, Log, TEXT("      - %s"), *SocketName.ToString());
                }
            }
        }
        else
        {
            UE_LOG(LogCharacter2DActor, Log, TEXT("  %s: NULL"), *Name);
        }
    };
    
    LogSkeletalComponent(BodyComponent, TEXT("BodyComponent"));
    LogSkeletalComponent(ArmsComponent, TEXT("ArmsComponent"));
    LogSkeletalComponent(HeadComponent, TEXT("HeadComponent"));

    // Информация о Sprite компонентах
    UE_LOG(LogCharacter2DActor, Log, TEXT("SPRITE COMPONENTS:"));
    
    auto LogSpriteComponent = [this](UPaperSpriteComponent* Component, const FString& Name)
    {
        if (Component)
        {
            UE_LOG(LogCharacter2DActor, Log, TEXT("  %s:"), *Name);
            UE_LOG(LogCharacter2DActor, Log, TEXT("    Sprite: %s"), Component->GetSprite() ? *Component->GetSprite()->GetName() : TEXT("None"));
            UE_LOG(LogCharacter2DActor, Log, TEXT("    Parent: %s"), Component->GetAttachParent() ? *Component->GetAttachParent()->GetName() : TEXT("RootComponent"));
            UE_LOG(LogCharacter2DActor, Log, TEXT("    Socket: %s"), *Component->GetAttachSocketName().ToString());
            UE_LOG(LogCharacter2DActor, Log, TEXT("    RelativeLocation: %s"), *Component->GetRelativeLocation().ToString());
            UE_LOG(LogCharacter2DActor, Log, TEXT("    RelativeScale: %s"), *Component->GetRelativeScale3D().ToString());
            UE_LOG(LogCharacter2DActor, Log, TEXT("    WorldLocation: %s"), *Component->GetComponentLocation().ToString());
            UE_LOG(LogCharacter2DActor, Log, TEXT("    Visible: %s"), Component->IsVisible() ? TEXT("true") : TEXT("false"));
            UE_LOG(LogCharacter2DActor, Log, TEXT("    Color: %s"), *Component->GetSpriteColor().ToString());
        }
        else
        {
            UE_LOG(LogCharacter2DActor, Log, TEXT("  %s: NULL"), *Name);
        }
    };
    
    LogSpriteComponent(SpriteBody, TEXT("SpriteBody"));
    LogSpriteComponent(SpriteArms, TEXT("SpriteArms"));
    LogSpriteComponent(SpriteHead, TEXT("SpriteHead"));
    LogSpriteComponent(SpriteEyebrow, TEXT("SpriteEyebrow"));
    LogSpriteComponent(SpriteEyes, TEXT("SpriteEyes"));
    LogSpriteComponent(SpriteEyelids, TEXT("SpriteEyelids"));
    LogSpriteComponent(SpriteMouth, TEXT("SpriteMouth"));

    // Информация о Flipbook компонентах
    UE_LOG(LogCharacter2DActor, Log, TEXT("FLIPBOOK COMPONENTS:"));
    
    auto LogFlipbookComponent = [this](UPaperFlipbookComponent* Component, const FString& Name)
    {
        if (Component)
        {
            UE_LOG(LogCharacter2DActor, Log, TEXT("  %s:"), *Name);
            UE_LOG(LogCharacter2DActor, Log, TEXT("    Flipbook: %s"), Component->GetFlipbook() ? *Component->GetFlipbook()->GetName() : TEXT("None"));
            UE_LOG(LogCharacter2DActor, Log, TEXT("    Parent: %s"), Component->GetAttachParent() ? *Component->GetAttachParent()->GetName() : TEXT("RootComponent"));
            UE_LOG(LogCharacter2DActor, Log, TEXT("    RelativeLocation: %s"), *Component->GetRelativeLocation().ToString());
            UE_LOG(LogCharacter2DActor, Log, TEXT("    RelativeScale: %s"), *Component->GetRelativeScale3D().ToString());
            UE_LOG(LogCharacter2DActor, Log, TEXT("    Visible: %s"), Component->IsVisible() ? TEXT("true") : TEXT("false"));
            UE_LOG(LogCharacter2DActor, Log, TEXT("    Playing: %s"), Component->IsPlaying() ? TEXT("true") : TEXT("false"));
            UE_LOG(LogCharacter2DActor, Log, TEXT("    PlayRate: %f"), Component->GetPlayRate());
        }
        else
        {
            UE_LOG(LogCharacter2DActor, Log, TEXT("  %s: NULL"), *Name);
        }
    };
    
    LogFlipbookComponent(EyelidComponent, TEXT("EyelidComponent"));
    LogFlipbookComponent(MouthComponent, TEXT("MouthComponent"));

    // Информация об актере
    UE_LOG(LogCharacter2DActor, Log, TEXT("ACTOR INFO:"));
    UE_LOG(LogCharacter2DActor, Log, TEXT("  Location: %s"), *GetActorLocation().ToString());
    UE_LOG(LogCharacter2DActor, Log, TEXT("  Rotation: %s"), *GetActorRotation().ToString());
    UE_LOG(LogCharacter2DActor, Log, TEXT("  Scale: %s"), *GetActorScale3D().ToString());
    UE_LOG(LogCharacter2DActor, Log, TEXT("  Hidden: %s"), IsHidden() ? TEXT("true") : TEXT("false"));
    UE_LOG(LogCharacter2DActor, Log, TEXT("  SpritesVisible: %s"), bSpritesVisible ? TEXT("true") : TEXT("false"));
    UE_LOG(LogCharacter2DActor, Log, TEXT("  SkeletalVisible: %s"), bSkeletalVisible ? TEXT("true") : TEXT("false"));
    UE_LOG(LogCharacter2DActor, Log, TEXT("  BlinkingActive: %s"), bBlinkingActive ? TEXT("true") : TEXT("false"));
    UE_LOG(LogCharacter2DActor, Log, TEXT("  TalkingActive: %s"), bTalkingActive ? TEXT("true") : TEXT("false"));

    // Информация о CharacterAsset
    if (CharacterAsset)
    {
        UE_LOG(LogCharacter2DActor, Log, TEXT("CHARACTER ASSET INFO:"));
        UE_LOG(LogCharacter2DActor, Log, TEXT("  Asset: %s"), *CharacterAsset->GetName());
        UE_LOG(LogCharacter2DActor, Log, TEXT("  GlobalSpriteOffset: %s"), *CharacterAsset->GetGlobalSpriteOffset().ToString());
        UE_LOG(LogCharacter2DActor, Log, TEXT("  GlobalSpriteScale: %f"), CharacterAsset->GetGlobalSpriteScale());
        UE_LOG(LogCharacter2DActor, Log, TEXT("  SkeletalGlobalOffset: %s"), *CharacterAsset->SkeletalGlobalOffset.ToString());
        UE_LOG(LogCharacter2DActor, Log, TEXT("  GlobalScale: %f"), CharacterAsset->GlobalScale);
        UE_LOG(LogCharacter2DActor, Log, TEXT("  DualRendering: %s"), CharacterAsset->bEnableDualRendering ? TEXT("true") : TEXT("false"));
        UE_LOG(LogCharacter2DActor, Log, TEXT("  AutoBlink: %s"), CharacterAsset->bAutoBlink ? TEXT("true") : TEXT("false"));
        UE_LOG(LogCharacter2DActor, Log, TEXT("  AutoTalk: %s"), CharacterAsset->bAutoTalk ? TEXT("true") : TEXT("false"));
    }
    else
    {
        UE_LOG(LogCharacter2DActor, Warning, TEXT("CHARACTER ASSET: NULL"));
    }
    
    UE_LOG(LogCharacter2DActor, Warning, TEXT("=== END ALL COMPONENTS INFO ==="));
}

#endif // WITH_EDITOR