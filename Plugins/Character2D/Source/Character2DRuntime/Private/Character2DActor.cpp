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
	TransitionTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("TransitionTimeline"));
	EmotionTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("EmotionTimeline"));
}

void ACharacter2DActor::BeginPlay()
{
	Super::BeginPlay();
	
	StoreOriginalValues();
	
	if (CharacterAsset)
	{
		if (CharacterAsset->VisualNovelSettings.bAutoPlayEntranceTransition)
		{
			PlayTransitionWithDefaults(CharacterAsset->VisualNovelSettings.DefaultEntranceTransition);
		}
		
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

	// Setup sprite parts
	const FCharacter2DSpriteStructure& SpriteStruct = CharacterAsset->SpriteStructure;
	SetupSpriteComponent(SpriteBody, SpriteStruct.Body);
	SetupSpriteComponent(SpriteArms, SpriteStruct.Arms);
	SetupSpriteComponent(SpriteHead, SpriteStruct.Head);
	SetupSpriteComponent(SpriteEyebrow, SpriteStruct.Eyebrow);
	SetupSpriteComponent(SpriteEyes, SpriteStruct.Eyes);
	SetupSpriteComponent(SpriteEyelids, SpriteStruct.Eyelids);
	SetupSpriteComponent(SpriteMouth, SpriteStruct.Mouth);

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
/*                        Visual Novel Transitions                        */
/* ====================================================================== */

void ACharacter2DActor::PlayTransition(ECharacter2DTransitionType TransitionType, 
                                     const FCharacter2DTransitionSettings& Settings)
{
    if (bIsInTransition) 
    {
        TransitionTimeline->Stop();
        RestoreOriginalValues();
    }
    
    bIsInTransition = true;
    CurrentTransitionType = TransitionType;

    // Stop and reset timeline
    TransitionTimeline->Stop();
    TransitionTimeline->SetPlaybackPosition(0.0f, false);

    // Create a default curve if none provided
    UCurveFloat* CurveToUse = Settings.AnimationCurve;
    if (!CurveToUse)
    {
        // Create a simple linear curve in-memory
        CurveToUse = NewObject<UCurveFloat>(this);
        CurveToUse->FloatCurve.AddKey(0.0f, 0.0f);
        CurveToUse->FloatCurve.AddKey(1.0f, 1.0f);
    }

    // Setup timeline
    FOnTimelineFloat TimelineProgress;
    TimelineProgress.BindUFunction(this, FName("OnTransitionTimelineUpdate"));
    TransitionTimeline->AddInterpFloat(CurveToUse, TimelineProgress);

    FOnTimelineEvent TimelineFinish;
    TimelineFinish.BindUFunction(this, FName("OnTransitionTimelineFinished"));
    TransitionTimeline->SetTimelineFinishedFunc(TimelineFinish);

    TransitionTimeline->SetTimelineLength(Settings.Duration);

    // Execute specific transition setup
    switch (TransitionType)
    {
    case ECharacter2DTransitionType::FadeIn:
        ExecuteFadeTransition(true, Settings);
        break;
    case ECharacter2DTransitionType::FadeOut:
        ExecuteFadeTransition(false, Settings);
        break;
    case ECharacter2DTransitionType::SlideInLeft:
        ExecuteSlideTransition(true, true, Settings);
        break;
    case ECharacter2DTransitionType::SlideInRight:
        ExecuteSlideTransition(true, false, Settings);
        break;
    case ECharacter2DTransitionType::SlideOutLeft:
        ExecuteSlideTransition(false, true, Settings);
        break;
    case ECharacter2DTransitionType::SlideOutRight:
        ExecuteSlideTransition(false, false, Settings);
        break;
    case ECharacter2DTransitionType::ScaleIn:
        ExecuteScaleTransition(true, Settings);
        break;
    case ECharacter2DTransitionType::ScaleOut:
        ExecuteScaleTransition(false, Settings);
        break;
    default:
        bIsInTransition = false;
        return;
    }

    // Start with delay if specified
    if (Settings.StartDelay > 0.0f)
    {
        FTimerHandle DelayHandle;
        GetWorldTimerManager().SetTimer(DelayHandle, [this]()
        {
            if (TransitionTimeline)
            {
                TransitionTimeline->Play();
            }
        }, Settings.StartDelay, false);
    }
    else
    {
        TransitionTimeline->Play();
    }
}

void ACharacter2DActor::PlayTransitionWithDefaults(ECharacter2DTransitionType TransitionType)
{
	if (!CharacterAsset) return;

	FCharacter2DTransitionSettings Settings;
	switch (TransitionType)
	{
	case ECharacter2DTransitionType::FadeOut:
	case ECharacter2DTransitionType::SlideOutLeft:
	case ECharacter2DTransitionType::SlideOutRight:
	case ECharacter2DTransitionType::ScaleOut:
		Settings = CharacterAsset->VisualNovelSettings.DefaultDisappearanceTransition;
		break;
	default:
		Settings = CharacterAsset->VisualNovelSettings.DefaultAppearanceTransition;
		break;
	}

	PlayTransition(TransitionType, Settings);
}

/* ====================================================================== */
/*                          Visual Novel Emotions                         */
/* ====================================================================== */

void ACharacter2DActor::PlayEmotion(ECharacter2DEmotionEffect EmotionType, 
                                   const FCharacter2DEmotionSettings& Settings)
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
        // Create a simple sine-wave like curve for emotions
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

    // Store emotion settings for use in timeline update
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
		// Create basic default settings if no asset
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

/* ====================================================================== */
/*                            Timeline Callbacks                          */
/* ====================================================================== */

void ACharacter2DActor::OnTransitionTimelineUpdate(float Value)
{
	switch (CurrentTransitionType)
	{
	case ECharacter2DTransitionType::FadeIn:
		SetActorHiddenInGame(false);
		SetAllSpritesOpacity(Value);
		SetAllSkeletalOpacity(Value);
		break;
	case ECharacter2DTransitionType::FadeOut:
		SetAllSpritesOpacity(1.0f - Value);
		SetAllSkeletalOpacity(1.0f - Value);
		if (Value >= 1.0f)
		{
			SetActorHiddenInGame(true);
		}
		break;
	case ECharacter2DTransitionType::SlideInLeft:
	case ECharacter2DTransitionType::SlideInRight:
		{
			SetActorHiddenInGame(false);
			FVector CurrentLocation = FMath::Lerp(SlideStartLocation, OriginalActorLocation, Value);
			SetActorLocation(CurrentLocation);
		}
		break;
	case ECharacter2DTransitionType::SlideOutLeft:
	case ECharacter2DTransitionType::SlideOutRight:
		{
			FVector CurrentLocation = FMath::Lerp(OriginalActorLocation, SlideTargetLocation, Value);
			SetActorLocation(CurrentLocation);
			if (Value >= 1.0f)
			{
				SetActorHiddenInGame(true);
			}
		}
		break;
	case ECharacter2DTransitionType::ScaleIn:
		{
			SetActorHiddenInGame(false);
			float Scale = Value;
			SetActorScale3D(FVector(Scale));
		}
		break;
	case ECharacter2DTransitionType::ScaleOut:
		{
			float Scale = 1.0f - Value;
			SetActorScale3D(FVector(FMath::Max(Scale, 0.001f))); // Prevent zero scale
			if (Value >= 1.0f)
			{
				SetActorHiddenInGame(true);
			}
		}
		break;
	default:
		break;
	}
}

void ACharacter2DActor::OnTransitionTimelineFinished()
{
	bIsInTransition = false;
	ECharacter2DTransitionType CompletedTransition = CurrentTransitionType;
	CurrentTransitionType = ECharacter2DTransitionType::None;
	
	// Ensure proper final state
	switch (CompletedTransition)
	{
	case ECharacter2DTransitionType::FadeIn:
	case ECharacter2DTransitionType::SlideInLeft:
	case ECharacter2DTransitionType::SlideInRight:
	case ECharacter2DTransitionType::ScaleIn:
		SetActorHiddenInGame(false);
		SetAllSpritesOpacity(1.0f);
		SetAllSkeletalOpacity(1.0f);
		SetActorLocation(OriginalActorLocation);
		SetActorScale3D(OriginalActorScale);
		break;
	case ECharacter2DTransitionType::FadeOut:
	case ECharacter2DTransitionType::SlideOutLeft:
	case ECharacter2DTransitionType::SlideOutRight:
	case ECharacter2DTransitionType::ScaleOut:
		SetActorHiddenInGame(true);
		break;
	}
	
	OnTransitionFinished.Broadcast(CompletedTransition);
}

void ACharacter2DActor::OnEmotionTimelineUpdate(float Value)
{
	if (!bIsPlayingEmotion) return;

	const float IntensityMultiplier = CurrentEmotionSettings.Intensity;
	
	switch (CurrentEmotionType)
	{
	case ECharacter2DEmotionEffect::Shake:
		{
			// Create random shake offset
			float ShakeAmount = Value * IntensityMultiplier * 10.0f; // Max 10 units shake
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
			// Pulse scale
			float ScaleFactor = 1.0f + (Value * IntensityMultiplier * 0.2f); // Max 20% scale increase
			SetActorScale3D(OriginalActorScale * ScaleFactor);
		}
		break;
	case ECharacter2DEmotionEffect::ColorShift:
		{
			// Interpolate to target color
			FLinearColor CurrentColor = FMath::Lerp(FLinearColor::White, CurrentEmotionSettings.TargetColor, Value * IntensityMultiplier);
			SetAllSpritesColor(CurrentColor);
		}
		break;
	case ECharacter2DEmotionEffect::Bounce:
		{
			// Bounce up and down
			float BounceHeight = Value * IntensityMultiplier * 50.0f; // Max 50 units bounce
			FVector BounceOffset = FVector(0, 0, BounceHeight);
			SetActorLocation(OriginalActorLocation + BounceOffset);
		}
		break;
	case ECharacter2DEmotionEffect::Flash:
		{
			// Flash opacity
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
	
	// Only show flipbook components if sprites are visible and the respective animation is active
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
/*                            Quick Actions                               */
/* ====================================================================== */

void ACharacter2DActor::AppearInstantly()
{
	// Stop any ongoing transitions/emotions
	if (bIsInTransition && TransitionTimeline)
	{
		TransitionTimeline->Stop();
		bIsInTransition = false;
	}
	
	SetActorHiddenInGame(false);
	SetAllSpritesOpacity(1.0f);
	SetAllSkeletalOpacity(1.0f);
	SetActorLocation(OriginalActorLocation);
	SetActorScale3D(OriginalActorScale);
	
	// Restore sprite colors
	RestoreOriginalValues();
}

void ACharacter2DActor::DisappearInstantly()
{
	// Stop any ongoing transitions/emotions
	if (bIsInTransition && TransitionTimeline)
	{
		TransitionTimeline->Stop();
		bIsInTransition = false;
	}
	if (bIsPlayingEmotion)
	{
		StopCurrentEmotion();
	}
	
	SetActorHiddenInGame(true);
}

void ACharacter2DActor::AppearWithDefaultTransition()
{
	if (!CharacterAsset) 
	{
		AppearInstantly();
		return;
	}
	PlayTransitionWithDefaults(CharacterAsset->VisualNovelSettings.DefaultEntranceTransition);
}

void ACharacter2DActor::DisappearWithDefaultTransition()
{
	PlayTransitionWithDefaults(ECharacter2DTransitionType::FadeOut);
}

/* ====================================================================== */
/*                            Helper Methods                              */
/* ====================================================================== */

void ACharacter2DActor::StoreOriginalValues()
{
	OriginalActorLocation = GetActorLocation();
	OriginalActorScale = GetActorScale3D();
	
	// Store original sprite colors
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
	if (!bIsInTransition) // Don't restore location/scale during transitions
	{
		SetActorLocation(OriginalActorLocation);
		SetActorScale3D(OriginalActorScale);
	}
	
	// Always restore sprite colors
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

/* ====================================================================== */
/*                          Transition Implementations                    */
/* ====================================================================== */

void ACharacter2DActor::ExecuteFadeTransition(bool bFadeIn, const FCharacter2DTransitionSettings& Settings)
{
	if (bFadeIn)
	{
		SetAllSpritesOpacity(0.0f);
		SetAllSkeletalOpacity(0.0f);
		SetActorHiddenInGame(false);
	}
	// FadeOut setup is handled in timeline update
}

void ACharacter2DActor::ExecuteSlideTransition(bool bSlideIn, bool bFromLeft, const FCharacter2DTransitionSettings& Settings)
{
	if (bSlideIn)
	{
		// Start off-screen
		SlideStartLocation = OriginalActorLocation;
		SlideStartLocation.Y += bFromLeft ? -Settings.SlideDistance : Settings.SlideDistance;
		SetActorLocation(SlideStartLocation);
		SetActorHiddenInGame(false);
	}
	else
	{
		// End off-screen
		SlideTargetLocation = OriginalActorLocation;
		SlideTargetLocation.Y += bFromLeft ? -Settings.SlideDistance : Settings.SlideDistance;
	}
}

void ACharacter2DActor::ExecuteScaleTransition(bool bScaleIn, const FCharacter2DTransitionSettings& Settings)
{
	if (bScaleIn)
	{
		SetActorScale3D(FVector(0.001f)); // Start very small
		SetActorHiddenInGame(false);
	}
	// ScaleOut is handled in timeline update
}

void ACharacter2DActor::ExecuteShakeEmotion(const FCharacter2DEmotionSettings& Settings)
{
	// Store shake parameters - actual shaking happens in timeline update
}

void ACharacter2DActor::ExecutePulseEmotion(const FCharacter2DEmotionSettings& Settings)
{
	// Store pulse parameters - actual pulsing happens in timeline update
}

void ACharacter2DActor::ExecuteColorShiftEmotion(const FCharacter2DEmotionSettings& Settings)
{
	// Store color parameters - actual color shift happens in timeline update
}

void ACharacter2DActor::ExecuteBounceEmotion(const FCharacter2DEmotionSettings& Settings)
{
	// Store bounce parameters - actual bouncing happens in timeline update
}

void ACharacter2DActor::ExecuteFlashEmotion(const FCharacter2DEmotionSettings& Settings)
{
	// Store flash parameters - actual flashing happens in timeline update
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
	
	// Also handle flipbook components
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
			Component->SetSpriteColor(Color);
		}
	}
	
	// Also handle flipbook components
	if (EyelidComponent && bIsBlinking)
	{
		EyelidComponent->SetSpriteColor(Color);
	}
	
	if (MouthComponent && bIsTalking)
	{
		MouthComponent->SetSpriteColor(Color);
	}
}

void ACharacter2DActor::SetAllSkeletalOpacity(float Opacity)
{
	// For skeletal mesh opacity, we would need to create dynamic material instances
	// and modify their opacity parameters. This is more complex and depends on
	// the materials being used. For now, we'll just control visibility.
	
	TArray<USkeletalMeshComponent*> SkeletalComponents = GetAllSkeletalComponents();
	for (USkeletalMeshComponent* Component : SkeletalComponents)
	{
		if (Component)
		{
			// Simple visibility control - for true opacity control,
			// materials would need opacity parameters
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
	if (TransitionTimeline)
	{
		TransitionTimeline->Stop();
	}
	
	if (EmotionTimeline)
	{
		EmotionTimeline->Stop();
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