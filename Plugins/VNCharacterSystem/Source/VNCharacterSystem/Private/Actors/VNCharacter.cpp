#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

AVNCharacter::AVNCharacter()
{
	// Отключаем тик - LOD система больше не нужна
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Создаем все компоненты
	CreateComponents();

	// Настраиваем иерархию
	SetupComponentHierarchy();

	// Инициализация значений по умолчанию
	bIsInFocus = true;
	DimColorMultiplier = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	// Создаем пустое состояние по умолчанию
	CurrentState = F_VN_CharacterState::CreateEmpty(TEXT("DefaultState"));
	
	// Инициализируем главный пресет как пустой
	MainPosePreset = F_VN_CharacterState::CreateEmpty(TEXT("MainPose"));

	VN_LOG_DEBUG(TEXT("VNCharacter created: %s"), *GetName());
}

void AVNCharacter::CreateComponents()
{
	// Создаем корневой компонент
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Создаем менеджер анимаций
	AnimationManager = CreateDefaultSubobject<UVNCharacterAnimationManager>(TEXT("AnimationManager"));

	// Создаем корневые трансформы
	GlobalSkeletalMeshTransform = CreateDefaultSubobject<USceneComponent>(TEXT("GlobalSkeletalMeshTransform"));
	GlobalSpriteTransform = CreateDefaultSubobject<USceneComponent>(TEXT("GlobalSpriteTransform"));

	// =====================================================
	// СОЗДАНИЕ SKELETAL MESH КОМПОНЕНТОВ
	// =====================================================

	Body_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body_Skeletal"));
	Arms_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Arms_Skeletal"));
	Head_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Head_Skeletal"));
	Custom01_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Custom01_Skeletal"));
	Custom02_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Custom02_Skeletal"));
	Custom03_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Custom03_Skeletal"));

	// =====================================================
	// СОЗДАНИЕ SPRITE КОМПОНЕНТОВ
	// =====================================================

	Body_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Body_Sprite"));
	Arms_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Arms_Sprite"));
	Head_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Head_Sprite"));
	Eyebrow_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Eyebrow_Sprite"));
	Eyes_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Eyes_Sprite"));
	Eyelids_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Eyelids_Sprite"));
	Wink_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Wink_Sprite"));
	Mouth_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Mouth_Sprite"));
	BodyShadow_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("BodyShadow_Sprite"));
	EmotionHead01_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionHead01_Sprite"));
	EmotionHead02_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionHead02_Sprite"));
	EmotionHead03_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionHead03_Sprite"));
	EmotionBody01_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionBody01_Sprite"));
	EmotionBody02_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionBody02_Sprite"));
	EmotionBody03_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionBody03_Sprite"));

	VN_LOG_DEBUG(TEXT("All components created for VNCharacter"));
}

void AVNCharacter::SetupComponentHierarchy()
{
	// Настройка корневых трансформов
	GlobalSkeletalMeshTransform->SetupAttachment(RootComponent);
	GlobalSpriteTransform->SetupAttachment(RootComponent);

	// =====================================================
	// SKELETAL MESH ИЕРАРХИЯ
	// =====================================================

	Body_Skeletal->SetupAttachment(GlobalSkeletalMeshTransform);
	Arms_Skeletal->SetupAttachment(GlobalSkeletalMeshTransform);
	Head_Skeletal->SetupAttachment(GlobalSkeletalMeshTransform);
	Custom01_Skeletal->SetupAttachment(GlobalSkeletalMeshTransform);
	Custom02_Skeletal->SetupAttachment(GlobalSkeletalMeshTransform);
	Custom03_Skeletal->SetupAttachment(GlobalSkeletalMeshTransform);

	// =====================================================
	// SPRITE ИЕРАРХИЯ
	// =====================================================

	// Спрайты тела и эффектов прикрепляются к корневому sprite transform
	Body_Sprite->SetupAttachment(GlobalSpriteTransform);
	Arms_Sprite->SetupAttachment(GlobalSpriteTransform);
	BodyShadow_Sprite->SetupAttachment(GlobalSpriteTransform);
	EmotionBody01_Sprite->SetupAttachment(GlobalSpriteTransform);
	EmotionBody02_Sprite->SetupAttachment(GlobalSpriteTransform);
	EmotionBody03_Sprite->SetupAttachment(GlobalSpriteTransform);

	// Head_Sprite прикрепляется к GlobalSpriteTransform (получит глобальные настройки)
	Head_Sprite->SetupAttachment(GlobalSpriteTransform);
	
	// Спрайты головы прикрепляются к Head_Sprite (НЕ получат дополнительные глобальные настройки)
	Eyebrow_Sprite->SetupAttachment(Head_Sprite);
	Eyes_Sprite->SetupAttachment(Head_Sprite);
	Eyelids_Sprite->SetupAttachment(Head_Sprite);
	Wink_Sprite->SetupAttachment(Head_Sprite);
	Mouth_Sprite->SetupAttachment(Head_Sprite);
	EmotionHead01_Sprite->SetupAttachment(Head_Sprite);
	EmotionHead02_Sprite->SetupAttachment(Head_Sprite);
	EmotionHead03_Sprite->SetupAttachment(Head_Sprite);

	VN_LOG_DEBUG(TEXT("Component hierarchy setup complete"));
}

void AVNCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Подключаем события анимации
	if (AnimationManager)
	{
		AnimationManager->OnAnimationStarted.AddDynamic(this, &AVNCharacter::OnAnimationStarted);
		AnimationManager->OnAnimationFinished.AddDynamic(this, &AVNCharacter::OnAnimationFinished);
		AnimationManager->OnAnimationProgress.AddDynamic(this, &AVNCharacter::OnAnimationProgress);
	}

	// Применяем начальное состояние
	ApplyCharacterState(CurrentState);

	// Применяем мобильные оптимизации при необходимости
	ApplyMobileOptimizations();

	VN_LOG_DEBUG(TEXT("VNCharacter BeginPlay completed: %s"), *GetName());
}

void AVNCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// LOD система удалена - тик не нужен
}

#if WITH_EDITOR
void AVNCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Если изменилось состояние в редакторе, применяем его
	if (PropertyChangedEvent.Property && 
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AVNCharacter, CurrentState))
	{
		UpdateCharacterPreview();
		VN_LOG_DEBUG(TEXT("Character state updated in editor"));
	}

	// Если изменились глобальные трансформации
	if (PropertyChangedEvent.Property && 
		(PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AVNCharacter, GlobalSkeletalOffset) ||
		 PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AVNCharacter, GlobalSkeletalScale) ||
		 PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AVNCharacter, GlobalSpriteOffset) ||
		 PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AVNCharacter, GlobalSpriteScale)))
	{
		UpdateCharacterPreview();
		VN_LOG_DEBUG(TEXT("Global transforms updated in editor"));
	}
}

void AVNCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	// Обновляем превью персонажа при изменениях в редакторе
	UpdateCharacterPreview();
}
#endif

// =====================================================
// ОСНОВНОЕ API - УПРАВЛЕНИЕ СОСТОЯНИЕМ
// =====================================================

void AVNCharacter::SetCharacterState(const F_VN_CharacterState& NewState, float TransitionDuration)
{
	SetCharacterStateInternal(NewState, TransitionDuration, true); // true = с валидацией
}

void AVNCharacter::SetCharacterStateInternal(const F_VN_CharacterState& NewState, float TransitionDuration, bool bValidate)
{
	VN_LOG_DEBUG(TEXT("=== SetCharacterStateInternal START ==="));
	VN_LOG_DEBUG(TEXT("New State ID: %s"), *NewState.StateID.ToString());
	VN_LOG_DEBUG(TEXT("Current State ID: %s"), *CurrentState.StateID.ToString());
	VN_LOG_DEBUG(TEXT("Transition Duration: %.2f"), TransitionDuration);
	VN_LOG_DEBUG(TEXT("Validate: %s"), bValidate ? TEXT("Yes") : TEXT("No"));

	// Упрощенная валидация и автокоррекция
	F_VN_CharacterState CorrectedState = NewState;
	if (bValidate)
	{
		if (!ValidateCharacterStateSimple(NewState))
		{
			VN_LOG_WARNING(TEXT("SetCharacterState: Invalid state provided, applying auto-correction"));
		}
		CorrectedState = CorrectCharacterState(NewState);
	}

	// Проверяем, действительно ли состояние изменилось
	bool bStatesAreDifferent = (CurrentState.StateID != CorrectedState.StateID);
	VN_LOG_DEBUG(TEXT("States are different: %s"), bStatesAreDifferent ? TEXT("Yes") : TEXT("No"));

	// Если есть активная анимация, пропускаем её
	if (AnimationManager && AnimationManager->IsAnimating())
	{
		VN_LOG_DEBUG(TEXT("Skipping current animation to start state transition"));
		AnimationManager->SkipCurrentAnimation();
	}

	// Подготавливаем компоненты для анимации перехода если нужна анимация
	if (TransitionDuration > 0.0f && bStatesAreDifferent)
	{
		PrepareTransitionComponents(CorrectedState);
	}

	// Сохраняем новое состояние
	F_VN_CharacterState PreviousState = CurrentState;
	CurrentState = CorrectedState;

	VN_LOG_DEBUG(TEXT("State saved, now applying configuration"));

	// Мгновенно применяем новое состояние
	ApplyCharacterState(CurrentState);

	// Запускаем анимацию перехода если нужно
	if (TransitionDuration > 0.0f && bStatesAreDifferent && AnimationManager)
	{
		AnimationManager->PlayTransition(TransitionDuration);
	}

	VN_LOG_DEBUG(TEXT("=== SetCharacterStateInternal END ==="));

	// Уведомляем о смене состояния
	OnCharacterStateChanged.Broadcast(CurrentState);
}

void AVNCharacter::SkipTransition()
{
	if (AnimationManager && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::Transition)
	{
		AnimationManager->SkipCurrentAnimation();
	}
	else
	{
		// Если анимация не идет, завершаем переход вручную
		FinishAndCleanupTransition();
	}
}

// =====================================================
// СИСТЕМА ФОКУСА
// =====================================================

void AVNCharacter::SetFocus(bool bInFocus, float Duration)
{
	// Если фокус не изменился, ничего не делаем
	if (bIsInFocus == bInFocus)
	{
		VN_LOG_DEBUG(TEXT("SetFocus: Focus state unchanged (%s)"), bInFocus ? TEXT("In Focus") : TEXT("Out of Focus"));
		return;
	}

	VN_LOG_DEBUG(TEXT("SetFocus: Changing focus to %s with duration %.2f"), 
		bInFocus ? TEXT("In Focus") : TEXT("Out of Focus"), Duration);

	// Если есть активная анимация фокуса, пропускаем её
	if (AnimationManager && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::Focus)
	{
		AnimationManager->SkipCurrentAnimation();
	}

	// Устанавливаем новое состояние фокуса
	bIsInFocus = bInFocus;

	// Запускаем анимацию
	if (Duration > 0.0f && AnimationManager)
	{
		AnimationManager->PlayFocus(bInFocus, Duration);
	}
	else
	{
		// Мгновенное применение
		SkipFocusAnimation();
	}
}

void AVNCharacter::SkipFocusAnimation()
{
	if (AnimationManager && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::Focus)
	{
		AnimationManager->SkipCurrentAnimation();
	}
	else
	{
		// Применяем цвета мгновенно
		TArray<USceneComponent*> AllComponents = GetAllRenderComponents();
		for (USceneComponent* Component : AllComponents)
		{
			if (Component)
			{
				FLinearColor TargetColor = GetTargetColorForComponent(Component);
				SetComponentColor(Component, TargetColor);
			}
		}

		// Уведомляем о смене фокуса
		OnCharacterFocusChanged.Broadcast(bIsInFocus);
	}
}

// =====================================================
// СИСТЕМА ВИДИМОСТИ
// =====================================================

void AVNCharacter::Appear(float Duration)
{
	VN_LOG_DEBUG(TEXT("Appear: Starting appear animation with duration %.2f"), Duration);

	// Если есть активная анимация появления/исчезновения, пропускаем её
	if (AnimationManager && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::SpawnDespawn)
	{
		AnimationManager->SkipCurrentAnimation();
	}

	// Показываем актора
	SetActorHiddenInGame(false);

	// Запускаем анимацию появления
	if (Duration > 0.0f && AnimationManager)
	{
		AnimationManager->PlaySpawnDespawn(true, Duration);
	}
	else
	{
		// Мгновенное появление
		SkipSpawnDespawnAnimation();
	}
}

void AVNCharacter::Disappear(float Duration)
{
	VN_LOG_DEBUG(TEXT("Disappear: Starting disappear animation with duration %.2f"), Duration);

	// Если есть активная анимация появления/исчезновения, пропускаем её
	if (AnimationManager && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::SpawnDespawn)
	{
		AnimationManager->SkipCurrentAnimation();
	}

	// Запускаем анимацию исчезновения
	if (Duration > 0.0f && AnimationManager)
	{
		AnimationManager->PlaySpawnDespawn(false, Duration);
	}
	else
	{
		// Мгновенное исчезновение
		SkipSpawnDespawnAnimation();
	}
}

void AVNCharacter::SkipSpawnDespawnAnimation()
{
	if (AnimationManager && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::SpawnDespawn)
	{
		AnimationManager->SkipCurrentAnimation();
	}
	else
	{
		// Мгновенное скрытие/показ в зависимости от текущего состояния
		bool bShouldBeVisible = !IsHidden();
		
		if (bShouldBeVisible)
		{
			// Показываем все компоненты с правильными цветами
			TArray<USceneComponent*> AllComponents = GetAllRenderComponents();
			for (USceneComponent* Component : AllComponents)
			{
				if (Component && Component != BodyShadow_Sprite)
				{
					Component->SetVisibility(true);
					FLinearColor TargetColor = GetTargetColorForComponent(Component);
					SetComponentColor(Component, TargetColor);
				}
			}
			BodyShadow_Sprite->SetVisibility(false);
		}
		else
		{
			// Скрываем актора
			SetActorHiddenInGame(true);
		}

		// Уведомляем о смене видимости
		OnCharacterVisibilityChanged.Broadcast(bShouldBeVisible);
	}
}

bool AVNCharacter::IsVisible() const
{
	return !IsHidden();
}

// =====================================================
// УТИЛИТЫ И ИНФОРМАЦИЯ
// =====================================================

bool AVNCharacter::IsAnimating() const
{
	return AnimationManager && AnimationManager->IsAnimating();
}

FLinearColor AVNCharacter::GetTargetColorForComponent(USceneComponent* Component) const
{
	if (!Component)
	{
		return FLinearColor::White;
	}

	// Получаем базовый цвет
	FLinearColor BaseColor = GetBaseColorForComponent(Component);

	// Применяем модификатор фокуса
	if (bIsInFocus)
	{
		return BaseColor;
	}
	else
	{
		return BaseColor * DimColorMultiplier;
	}
}

FLinearColor AVNCharacter::GetBaseColorForComponent(USceneComponent* Component) const
{
	if (!Component)
	{
		return FLinearColor::White;
	}

	// Определяем тип компонента и возвращаем соответствующий цвет из CurrentState
	
	// Skeletal Mesh компоненты
	if (Component == Body_Skeletal)
		return CurrentState.BodyConfig.Color;
	if (Component == Arms_Skeletal)
		return CurrentState.ArmsConfig.Color;
	if (Component == Head_Skeletal)
		return CurrentState.HeadConfig.Color;
	if (Component == Custom01_Skeletal)
		return CurrentState.Custom01Config.Color;
	if (Component == Custom02_Skeletal)
		return CurrentState.Custom02Config.Color;
	if (Component == Custom03_Skeletal)
		return CurrentState.Custom03Config.Color;

	// Sprite компоненты
	if (Component == Body_Sprite)
		return CurrentState.BodySpriteConfig.Color;
	if (Component == Arms_Sprite)
		return CurrentState.ArmsSpriteConfig.Color;
	if (Component == Head_Sprite)
		return CurrentState.HeadSpriteConfig.Color;
	if (Component == Eyebrow_Sprite)
		return CurrentState.EyebrowSpriteConfig.Color;
	if (Component == Eyes_Sprite)
		return CurrentState.EyesSpriteConfig.Color;
	if (Component == Eyelids_Sprite)
		return CurrentState.EyelidsSpriteConfig.Color;
	if (Component == Wink_Sprite)
		return CurrentState.WinkSpriteConfig.Color;
	if (Component == Mouth_Sprite)
		return CurrentState.MouthSpriteConfig.Color;
	if (Component == BodyShadow_Sprite)
		return CurrentState.BodyShadowSpriteConfig.Color;
	if (Component == EmotionHead01_Sprite)
		return CurrentState.EmotionHeadEffect01SpriteConfig.Color;
	if (Component == EmotionHead02_Sprite)
		return CurrentState.EmotionHeadEffect02SpriteConfig.Color;
	if (Component == EmotionHead03_Sprite)
		return CurrentState.EmotionHeadEffect03SpriteConfig.Color;
	if (Component == EmotionBody01_Sprite)
		return CurrentState.EmotionBodyEffect01SpriteConfig.Color;
	if (Component == EmotionBody02_Sprite)
		return CurrentState.EmotionBodyEffect02SpriteConfig.Color;
	if (Component == EmotionBody03_Sprite)
		return CurrentState.EmotionBodyEffect03SpriteConfig.Color;

	// По умолчанию возвращаем белый
	return FLinearColor::White;
}

// =====================================================
// УПРОЩЕННАЯ ВАЛИДАЦИЯ И АВТОКОРРЕКЦИЯ
// =====================================================

bool AVNCharacter::ValidateCharacterStateSimple(const F_VN_CharacterState& State) const
{
	// Упрощенная валидация - проверяем только базовые требования
	if (State.StateID.IsNone())
	{
		VN_LOG_WARNING(TEXT("Character state has no StateID"));
		return false;
	}

	return true; // Все остальное исправляем автоматически
}

F_VN_CharacterState AVNCharacter::CorrectCharacterState(const F_VN_CharacterState& State) const
{
	F_VN_CharacterState CorrectedState = State;

	// Если StateID пустой, задаем дефолтный
	if (CorrectedState.StateID.IsNone())
	{
		CorrectedState.StateID = FName("CorrectedState");
	}

	// Автокоррекция для Skeletal Mesh компонентов
	// Если ассет отсутствует - скрываем компонент
	if (CorrectedState.BodyConfig.bVisible && CorrectedState.BodyConfig.SkeletalMesh.IsNull())
	{
		CorrectedState.BodyConfig.bVisible = false;
		VN_LOG_DEBUG(TEXT("Auto-corrected: BodyConfig set to invisible (no mesh)"));
	}

	if (CorrectedState.ArmsConfig.bVisible && CorrectedState.ArmsConfig.SkeletalMesh.IsNull())
	{
		CorrectedState.ArmsConfig.bVisible = false;
		VN_LOG_DEBUG(TEXT("Auto-corrected: ArmsConfig set to invisible (no mesh)"));
	}

	if (CorrectedState.HeadConfig.bVisible && CorrectedState.HeadConfig.SkeletalMesh.IsNull())
	{
		CorrectedState.HeadConfig.bVisible = false;
		VN_LOG_DEBUG(TEXT("Auto-corrected: HeadConfig set to invisible (no mesh)"));
	}

	// Автокоррекция для Sprite компонентов
	// Если ассет отсутствует - скрываем компонент, КРОМЕ Head_Sprite
	if (CorrectedState.BodySpriteConfig.bVisible && CorrectedState.BodySpriteConfig.Sprite.IsNull())
	{
		CorrectedState.BodySpriteConfig.bVisible = false;
		VN_LOG_DEBUG(TEXT("Auto-corrected: BodySpriteConfig set to invisible (no sprite)"));
	}

	if (CorrectedState.ArmsSpriteConfig.bVisible && CorrectedState.ArmsSpriteConfig.Sprite.IsNull())
	{
		CorrectedState.ArmsSpriteConfig.bVisible = false;
		VN_LOG_DEBUG(TEXT("Auto-corrected: ArmsSpriteConfig set to invisible (no sprite)"));
	}

	// Head_Sprite НЕ скрываем автоматически, чтобы не потерять вложенные элементы
	// Просто убираем спрайт, но оставляем видимость
	if (CorrectedState.HeadSpriteConfig.Sprite.IsNull())
	{
		VN_LOG_DEBUG(TEXT("Auto-corrected: HeadSpriteConfig has no sprite but keeping visible for child components"));
	}

	// Обычные спрайты лица - скрываем если нет ассета
	if (CorrectedState.EyesSpriteConfig.bVisible && CorrectedState.EyesSpriteConfig.Sprite.IsNull())
	{
		CorrectedState.EyesSpriteConfig.bVisible = false;
		VN_LOG_DEBUG(TEXT("Auto-corrected: EyesSpriteConfig set to invisible (no sprite)"));
	}

	if (CorrectedState.MouthSpriteConfig.bVisible && CorrectedState.MouthSpriteConfig.Sprite.IsNull())
	{
		CorrectedState.MouthSpriteConfig.bVisible = false;
		VN_LOG_DEBUG(TEXT("Auto-corrected: MouthSpriteConfig set to invisible (no sprite)"));
	}

	if (CorrectedState.EyebrowSpriteConfig.bVisible && CorrectedState.EyebrowSpriteConfig.Sprite.IsNull())
	{
		CorrectedState.EyebrowSpriteConfig.bVisible = false;
		VN_LOG_DEBUG(TEXT("Auto-corrected: EyebrowSpriteConfig set to invisible (no sprite)"));
	}

	if (CorrectedState.EyelidsSpriteConfig.bVisible && CorrectedState.EyelidsSpriteConfig.Sprite.IsNull())
	{
		CorrectedState.EyelidsSpriteConfig.bVisible = false;
		VN_LOG_DEBUG(TEXT("Auto-corrected: EyelidsSpriteConfig set to invisible (no sprite)"));
	}

	if (CorrectedState.WinkSpriteConfig.bVisible && CorrectedState.WinkSpriteConfig.Sprite.IsNull())
	{
		CorrectedState.WinkSpriteConfig.bVisible = false;
		VN_LOG_DEBUG(TEXT("Auto-corrected: WinkSpriteConfig set to invisible (no sprite)"));
	}

	// Эмоциональные эффекты
	if (CorrectedState.EmotionHeadEffect01SpriteConfig.bVisible && CorrectedState.EmotionHeadEffect01SpriteConfig.Sprite.IsNull())
	{
		CorrectedState.EmotionHeadEffect01SpriteConfig.bVisible = false;
	}

	if (CorrectedState.EmotionHeadEffect02SpriteConfig.bVisible && CorrectedState.EmotionHeadEffect02SpriteConfig.Sprite.IsNull())
	{
		CorrectedState.EmotionHeadEffect02SpriteConfig.bVisible = false;
	}

	if (CorrectedState.EmotionHeadEffect03SpriteConfig.bVisible && CorrectedState.EmotionHeadEffect03SpriteConfig.Sprite.IsNull())
	{
		CorrectedState.EmotionHeadEffect03SpriteConfig.bVisible = false;
	}

	if (CorrectedState.EmotionBodyEffect01SpriteConfig.bVisible && CorrectedState.EmotionBodyEffect01SpriteConfig.Sprite.IsNull())
	{
		CorrectedState.EmotionBodyEffect01SpriteConfig.bVisible = false;
	}

	if (CorrectedState.EmotionBodyEffect02SpriteConfig.bVisible && CorrectedState.EmotionBodyEffect02SpriteConfig.Sprite.IsNull())
	{
		CorrectedState.EmotionBodyEffect02SpriteConfig.bVisible = false;
	}

	if (CorrectedState.EmotionBodyEffect03SpriteConfig.bVisible && CorrectedState.EmotionBodyEffect03SpriteConfig.Sprite.IsNull())
	{
		CorrectedState.EmotionBodyEffect03SpriteConfig.bVisible = false;
	}

	if (CorrectedState.BodyShadowSpriteConfig.bVisible && CorrectedState.BodyShadowSpriteConfig.Sprite.IsNull())
	{
		CorrectedState.BodyShadowSpriteConfig.bVisible = false;
	}

	return CorrectedState;
}

void AVNCharacter::UpdateCharacterPreview()
{
	// Применяем скорректированное состояние
	F_VN_CharacterState CorrectedState = CorrectCharacterState(CurrentState);
	ApplyCharacterState(CorrectedState);
	
	VN_LOG_DEBUG(TEXT("Character preview updated"));
}

// =====================================================
// ОБРАБОТЧИКИ СОБЫТИЙ АНИМАЦИИ
// =====================================================

void AVNCharacter::OnAnimationStarted(EVNAnimationType AnimationType)
{
	// Преобразуем enum в строку вручную
	FString AnimationTypeName;
	switch (AnimationType)
	{
		case EVNAnimationType::None:
			AnimationTypeName = TEXT("None");
			break;
		case EVNAnimationType::Transition:
			AnimationTypeName = TEXT("Transition");
			break;
		case EVNAnimationType::SpawnDespawn:
			AnimationTypeName = TEXT("SpawnDespawn");
			break;
		case EVNAnimationType::Focus:
			AnimationTypeName = TEXT("Focus");
			break;
		default:
			AnimationTypeName = TEXT("Unknown");
			break;
	}

	VN_LOG_DEBUG(TEXT("Animation started: %s"), *AnimationTypeName);
	
	switch (AnimationType)
	{
		case EVNAnimationType::Transition:
			// Дополнительная логика для начала перехода состояний
			break;
			
		case EVNAnimationType::SpawnDespawn:
			// Дополнительная логика для начала появления/исчезновения
			break;
			
		case EVNAnimationType::Focus:
			// Дополнительная логика для начала смены фокуса
			break;

		default:
			VN_LOG_WARNING(TEXT("Unknown animation type in OnAnimationStarted: %d"), (int32)AnimationType);
			break;
	}
}

void AVNCharacter::OnAnimationFinished(EVNAnimationType AnimationType)
{
	// Преобразуем enum в строку вручную
	FString AnimationTypeName;
	switch (AnimationType)
	{
		case EVNAnimationType::None:
			AnimationTypeName = TEXT("None");
			break;
		case EVNAnimationType::Transition:
			AnimationTypeName = TEXT("Transition");
			break;
		case EVNAnimationType::SpawnDespawn:
			AnimationTypeName = TEXT("SpawnDespawn");
			break;
		case EVNAnimationType::Focus:
			AnimationTypeName = TEXT("Focus");
			break;
		default:
			AnimationTypeName = TEXT("Unknown");
			break;
	}

	VN_LOG_DEBUG(TEXT("Animation finished: %s"), *AnimationTypeName);
	
	switch (AnimationType)
	{
		case EVNAnimationType::Transition:
			FinishAndCleanupTransition();
			OnCharacterStateChanged.Broadcast(CurrentState);
			break;
			
		case EVNAnimationType::SpawnDespawn:
			// Проверяем, было ли это исчезновение
			if (AnimationManager && !AnimationManager->IsAnimating())
			{
				// Логика завершения appears/disappear будет в AnimationManager
				OnCharacterVisibilityChanged.Broadcast(IsVisible());
			}
			break;
			
		case EVNAnimationType::Focus:
			OnCharacterFocusChanged.Broadcast(bIsInFocus);
			break;

		default:
			VN_LOG_WARNING(TEXT("Unknown animation type in OnAnimationFinished: %d"), (int32)AnimationType);
			break;
	}
}

void AVNCharacter::OnAnimationProgress(EVNAnimationType AnimationType, float Progress)
{
	// Анимация теперь полностью обрабатывается в AnimationManager
	// Здесь можем добавить дополнительную логику если нужно
	
	switch (AnimationType)
	{
		case EVNAnimationType::Transition:
			// Дополнительная логика для прогресса перехода состояний
			break;
			
		case EVNAnimationType::SpawnDespawn:
			// Дополнительная логика для прогресса появления/исчезновения
			break;
			
		case EVNAnimationType::Focus:
			// Дополнительная логика для прогресса смены фокуса
			break;

		default:
			VN_LOG_WARNING(TEXT("Unknown animation type in OnAnimationProgress: %d"), (int32)AnimationType);
			break;
	}
}

#if WITH_EDITOR
void AVNCharacter::PrintDebugInfo()
{
	FString DebugInfo = TEXT("=== VN Character Debug Info ===\n");
	
	DebugInfo += FString::Printf(TEXT("Actor: %s\n"), *GetName());
	DebugInfo += FString::Printf(TEXT("Character Name: %s\n"), *CharacterName);
	DebugInfo += FString::Printf(TEXT("Current State: %s\n"), *CurrentState.StateID.ToString());
	DebugInfo += FString::Printf(TEXT("Main Pose Preset: %s\n"), *MainPosePreset.StateID.ToString());
	DebugInfo += FString::Printf(TEXT("Is In Focus: %s\n"), bIsInFocus ? TEXT("Yes") : TEXT("No"));
	DebugInfo += FString::Printf(TEXT("Is Visible: %s\n"), IsVisible() ? TEXT("Yes") : TEXT("No"));
	DebugInfo += FString::Printf(TEXT("Is Animating: %s\n"), IsAnimating() ? TEXT("Yes") : TEXT("No"));
	
	if (AnimationManager)
	{
		// Преобразуем тип анимации в строку вручную
		FString CurrentAnimationTypeName;
		EVNAnimationType CurrentAnimationType = AnimationManager->GetCurrentAnimationType();
		switch (CurrentAnimationType)
		{
			case EVNAnimationType::None:
				CurrentAnimationTypeName = TEXT("None");
				break;
			case EVNAnimationType::Transition:
				CurrentAnimationTypeName = TEXT("Transition");
				break;
			case EVNAnimationType::SpawnDespawn:
				CurrentAnimationTypeName = TEXT("SpawnDespawn");
				break;
			case EVNAnimationType::Focus:
				CurrentAnimationTypeName = TEXT("Focus");
				break;
			default:
				CurrentAnimationTypeName = TEXT("Unknown");
				break;
		}

		DebugInfo += FString::Printf(TEXT("Current Animation: %s\n"), *CurrentAnimationTypeName);
		DebugInfo += FString::Printf(TEXT("Animation Progress: %.2f%%\n"), 
			AnimationManager->GetCurrentAnimationProgress() * 100.0f);
		DebugInfo += FString::Printf(TEXT("Queued Animations: %d\n"), 
			AnimationManager->GetQueuedAnimationsCount());
	}
	
	// Информация о компонентах
	TArray<USkeletalMeshComponent*> SkeletalComponents = GetAllSkeletalComponents();
	TArray<UPaperSpriteComponent*> SpriteComponents = GetAllSpriteComponents();
	
	DebugInfo += FString::Printf(TEXT("Skeletal Components: %d\n"), SkeletalComponents.Num());
	DebugInfo += FString::Printf(TEXT("Sprite Components: %d\n"), SpriteComponents.Num());
	
	DebugInfo += TEXT("\nCurrent State Summary: ") + CurrentState.GetSummary() + TEXT("\n");
	DebugInfo += TEXT("Main Pose Preset Summary: ") + MainPosePreset.GetSummary() + TEXT("\n");
	
	VN_LOG(Log, TEXT("%s"), *DebugInfo);
}

void AVNCharacter::ValidateAllComponents()
{
	TArray<FString> ValidationErrors;
	
	// Проверяем все Skeletal Mesh компоненты
	TArray<USkeletalMeshComponent*> SkeletalComponents = GetAllSkeletalComponents();
	for (USkeletalMeshComponent* Component : SkeletalComponents)
	{
		if (!Component)
		{
			ValidationErrors.Add(TEXT("Skeletal Mesh component is null"));
			continue;
		}
		
		if (!Component->GetSkeletalMeshAsset())
		{
			ValidationErrors.Add(FString::Printf(TEXT("Skeletal Mesh component %s has no mesh asset"), 
				*Component->GetName()));
		}
	}
	
	// Проверяем все Sprite компоненты
	TArray<UPaperSpriteComponent*> SpriteComponents = GetAllSpriteComponents();
	for (UPaperSpriteComponent* Component : SpriteComponents)
	{
		if (!Component)
		{
			ValidationErrors.Add(TEXT("Sprite component is null"));
			continue;
		}
		
		if (!Component->GetSprite())
		{
			ValidationErrors.Add(FString::Printf(TEXT("Sprite component %s has no sprite asset"), 
				*Component->GetName()));
		}
	}
	
	// Выводим результаты валидации
	if (ValidationErrors.Num() == 0)
	{
		VN_LOG(Log, TEXT("All components validation passed for %s"), *GetName());
	}
	else
	{
		FString ErrorMessage = FString::Printf(TEXT("Component validation failed for %s:"), *GetName());
		for (const FString& Error : ValidationErrors)
		{
			ErrorMessage += FString::Printf(TEXT("\n- %s"), *Error);
		}
		VN_LOG(Warning, TEXT("%s"), *ErrorMessage);
	}
}
#endif

// =====================================================
// ЗАГЛУШКИ ДЛЯ МЕТОДОВ, РЕАЛИЗОВАННЫХ В ДРУГИХ ФАЙЛАХ
// =====================================================

// ПРИМЕЧАНИЕ: Эти методы реализованы в соответствующих .cpp файлах:
// - VNCharacter_ComponentSetup.cpp
// - VNCharacter_DialogueSystem.cpp  
// - VNCharacter_Utilities.cpp

// Здесь остаются только объявления, если они нужны для связывания