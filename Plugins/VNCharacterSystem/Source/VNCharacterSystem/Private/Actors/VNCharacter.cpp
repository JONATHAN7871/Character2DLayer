#include "Actors/VNCharacter.h"
#include "Data/VNCharacterDataAsset.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Components/VNCharacterIdleAnimationManager.h"
#include "Data/VNCharacterIdleAnimationDataAsset.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

AVNCharacter::AVNCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	CreateComponents();
	SetupComponentHierarchy();

	bIsInFocus = true;
	DimColorMultiplier = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	// Настройка Tick для поддержки движения (по умолчанию выключен)
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	// Инициализация переменных движения
	bIsMoving = false;
	StartLocation = FVector::ZeroVector;
	TargetLocation = FVector::ZeroVector;
	StartScale = FVector::OneVector;
	TargetScale = FVector::OneVector;
	bShouldInterpolateScale = false;
	MovementStartTime = 0.0f;
	MovementDuration = 1.0f;

	// Инициализация автоинициализации
	AutoInitCharacterData = nullptr;
	AutoInitIdleData = nullptr;
	bAutoApplyOnBeginPlay = true;
	bAutoInitWithAnimation = false;
	AutoInitAnimationDuration = 1.0f;
	AutoInitDelay = 0.0f;
	bAutoStartIdleAnimations = true;
}

void AVNCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AnimationManager)
	{
		AnimationManager->OnAnimationStarted.AddDynamic(this, &AVNCharacter::OnAnimationStarted);
		AnimationManager->OnAnimationFinished.AddDynamic(this, &AVNCharacter::OnAnimationFinished);
		AnimationManager->OnAnimationProgress.AddDynamic(this, &AVNCharacter::OnAnimationProgress);
	}

	HideAllFadeComponents();

	if (bAutoApplyOnBeginPlay && (AutoInitCharacterData || AutoInitIdleData))
	{
		if (AutoInitDelay > 0.0f)
		{
			// Запускаем с задержкой
			GetWorld()->GetTimerManager().SetTimer(
				AutoInitTimerHandle,
				this,
				&AVNCharacter::PerformAutoInitialization,
				AutoInitDelay,
				false
			);
            
			VN_LOG_DEBUG(TEXT("BeginPlay: Auto-initialization scheduled with %.2f second delay"), AutoInitDelay);
		}
		else
		{
			// Выполняем немедленно
			PerformAutoInitialization();
		}
	}
}