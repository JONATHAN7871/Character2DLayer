#include "Character2DActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TimelineComponent.h"
#include "Engine/World.h"
#include "Curves/CurveFloat.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

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

    // Initial attachment to root
    TArray<UPaperSpriteComponent*> SpriteComponents = GetAllSpriteComponents();
    for (UPaperSpriteComponent* Component : SpriteComponents)
    {
        Component->SetupAttachment(RootComponent);
        Component->SetCastShadow(false);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    /* ---------- Flipbook Components --------- */
    EyelidComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("EyelidFlipbook"));
    MouthComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("MouthFlipbook"));
    EyelidComponent->SetupAttachment(RootComponent);
    MouthComponent->SetupAttachment(RootComponent);

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

    // Setup skeletal parts
    SetupSkeletalComponent(BodyComponent, CharacterAsset->Body);
    SetupSkeletalComponent(ArmsComponent, CharacterAsset->Arms);
    SetupSkeletalComponent(HeadComponent, CharacterAsset->Head);

    // Setup sprite parts with attachment
    const FCharacter2DSpriteStructure& SpriteStruct = CharacterAsset->SpriteStructure;
    SetupSpriteComponent(SpriteBody, SpriteStruct.Body);
    SetupSpriteComponent(SpriteArms, SpriteStruct.Arms);
    SetupSpriteComponent(SpriteHead, SpriteStruct.Head);
    SetupSpriteComponent(SpriteEyebrow, SpriteStruct.Eyebrow);
    SetupSpriteComponent(SpriteEyes, SpriteStruct.Eyes);
    SetupSpriteComponent(SpriteEyelids, SpriteStruct.Eyelids);
    SetupSpriteComponent(SpriteMouth, SpriteStruct.Mouth);

    // Attach sprites to sockets if specified
    AttachSpriteToSocket(SpriteBody, SpriteStruct.Body);
    AttachSpriteToSocket(SpriteArms, SpriteStruct.Arms);
    AttachSpriteToSocket(SpriteHead, SpriteStruct.Head);
    AttachSpriteToSocket(SpriteEyebrow, SpriteStruct.Eyebrow);
    AttachSpriteToSocket(SpriteEyes, SpriteStruct.Eyes);
    AttachSpriteToSocket(SpriteEyelids, SpriteStruct.Eyelids);
    AttachSpriteToSocket(SpriteMouth, SpriteStruct.Mouth);

    // Setup flipbook components
    const auto& BlinkSettings = SpriteStruct.EyelidsBlinkSettings;
    EyelidComponent->SetFlipbook(BlinkSettings.BlinkFlipbook);
    EyelidComponent->SetRelativeLocation(BlinkSettings.Offset + SpriteStruct.GlobalOffset);
    EyelidComponent->SetRelativeScale3D(FVector(BlinkSettings.Scale * SpriteStruct.GlobalScale));
    EyelidComponent->SetVisibility(false);

    const auto& TalkSettings = SpriteStruct.MouthTalkSettings;
    MouthComponent->SetFlipbook(TalkSettings.TalkFlipbook);
    MouthComponent->SetRelativeLocation(TalkSettings.Offset + SpriteStruct.GlobalOffset);
    MouthComponent->SetRelativeScale3D(FVector(TalkSettings.Scale * SpriteStruct.GlobalScale));
    MouthComponent->SetVisibility(false);

    // Set initial visibility based on dual rendering setting
    SetSpritesVisible(CharacterAsset->bEnableDualRendering || !HasValidSkeletalMeshes());
    SetSkeletalVisible(CharacterAsset->bEnableDualRendering || !HasValidSprites());
}

void ACharacter2DActor::RefreshFromAsset()
{
    OnConstruction(GetActorTransform());
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
    
    TArray<UPaperSpriteComponent*> SpriteComponents = GetAllSpriteComponents();
    for (UPaperSpriteComponent* Component : SpriteComponents)
    {
        if (Component)
        {
            Component->SetVisibility(bVisible);
        }
    }
    
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
   return SpriteStruct.Body.Sprite || SpriteStruct.Arms.Sprite || SpriteStruct.Head.Sprite;
}

bool ACharacter2DActor::HasValidSkeletalMeshes() const
{
   if (!CharacterAsset) return false;
   
   return CharacterAsset->Body.Mesh || CharacterAsset->Arms.Mesh || CharacterAsset->Head.Mesh;
}

void ACharacter2DActor::SetupSpriteComponent(UPaperSpriteComponent* Component, const FCharacter2DSpriteLayer& Layer)
{
   if (!Component || !CharacterAsset) return;

   const FVector GlobalOffset = CharacterAsset->SpriteStructure.GlobalOffset;
   const float GlobalScale = CharacterAsset->SpriteStructure.GlobalScale;
   
   Component->SetSprite(Layer.Sprite);
   Component->SetRelativeLocation(Layer.Offset + GlobalOffset);
   Component->SetRelativeScale3D(FVector(Layer.Scale * GlobalScale));
   Component->SetVisibility(Layer.bVisible && bSpritesVisible);
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

void ACharacter2DActor::AttachSpriteToSocket(UPaperSpriteComponent* SpriteComp, const FCharacter2DSpriteLayer& Layer)
{
   if (!SpriteComp || Layer.AttachmentTarget == ECharacter2DAttachmentTarget::None) return;
   
   USkeletalMeshComponent* TargetComponent = GetSkeletalComponentByTarget(Layer.AttachmentTarget);
   if (!TargetComponent || Layer.SocketName == NAME_None) return;
   
   // Detach from root and attach to socket
   SpriteComp->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
   SpriteComp->AttachToComponent(TargetComponent, FAttachmentTransformRules::KeepRelativeTransform, Layer.SocketName);
   
   // Apply socket-specific offset if needed
   if (!Layer.bUseSocketTransform)
   {
       const FVector GlobalOffset = CharacterAsset->SpriteStructure.GlobalOffset;
       const float GlobalScale = CharacterAsset->SpriteStructure.GlobalScale;
       SpriteComp->SetRelativeLocation(Layer.Offset + GlobalOffset);
       SpriteComp->SetRelativeScale3D(FVector(Layer.Scale * GlobalScale));
   }
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
   const auto& Settings = CharacterAsset->SpriteStructure.EyelidsBlinkSettings;
   const float Delay = FMath::FRandRange(Settings.BlinkIntervalMin, Settings.BlinkIntervalMax);

   GetWorldTimerManager().SetTimer(
       BlinkTimerHandle, this, &ACharacter2DActor::HandleBlink, Delay, false);
}

void ACharacter2DActor::StopBlinking()
{
   bIsBlinking = false;
   GetWorldTimerManager().ClearTimer(BlinkTimerHandle);
   GetWorldTimerManager().ClearTimer(BlinkRestoreHandle);

   if (IsValid(SpriteEyelids))
   {
       const auto& Layer = CharacterAsset->SpriteStructure.Eyelids;
       SpriteEyelids->SetSprite(Layer.Sprite);
       SpriteEyelids->SetVisibility(Layer.bVisible && bSpritesVisible);
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

   const auto& Settings = CharacterAsset->SpriteStructure.EyelidsBlinkSettings;
   if (!Settings.BlinkFlipbook)
   {
       StopBlinking();
       return;
   }

   // Show animated blink
   SpriteEyelids->SetVisibility(false);
   EyelidComponent->SetFlipbook(Settings.BlinkFlipbook);
   const float Rate = FMath::FRandRange(Settings.BlinkPlayRateMin, Settings.BlinkPlayRateMax);
   EyelidComponent->SetPlayRate(Rate);
   EyelidComponent->SetVisibility(bSpritesVisible);
   EyelidComponent->PlayFromStart();

   const float Duration = Settings.BlinkFlipbook->GetTotalDuration() / Rate;

   // Restore static eyelids after animation
   FTimerDelegate RestoreDelegate = FTimerDelegate::CreateLambda([this]()
   {
       if (!IsValid(this) || !IsValid(EyelidComponent) || !IsValid(SpriteEyelids) || !CharacterAsset)
           return;

       const auto& Layer = CharacterAsset->SpriteStructure.Eyelids;
       EyelidComponent->Stop();
       EyelidComponent->SetVisibility(false);
       SpriteEyelids->SetSprite(Layer.Sprite);
       SpriteEyelids->SetVisibility(Layer.bVisible && bSpritesVisible);

       // Chance for double blink
       if (bIsBlinking && FMath::FRand() < 0.25f)
       {
           HandleBlink();
           return;
       }

       // Schedule next blink
       if (bIsBlinking)
       {
           const auto& S = CharacterAsset->SpriteStructure.EyelidsBlinkSettings;
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
   const auto& Settings = CharacterAsset->SpriteStructure.MouthTalkSettings;
   if (!Settings.TalkFlipbook)
   {
       bIsTalking = false;
       return;
   }

   SpriteMouth->SetVisibility(false);
   MouthComponent->SetFlipbook(Settings.TalkFlipbook);
   MouthComponent->SetPlayRate(Settings.TalkPlayRate);
   MouthComponent->SetLooping(true);
   MouthComponent->SetVisibility(bSpritesVisible);
   MouthComponent->Play();
}

void ACharacter2DActor::StopTalking()
{
   bIsTalking = false;
   if (IsValid(MouthComponent))
   {
       MouthComponent->Stop();
       MouthComponent->SetVisibility(false);
   }
   if (IsValid(SpriteMouth) && CharacterAsset)
   {
       const auto& Layer = CharacterAsset->SpriteStructure.Mouth;
       SpriteMouth->SetSprite(Layer.Sprite);
       SpriteMouth->SetVisibility(Layer.bVisible && bSpritesVisible);
   }
}