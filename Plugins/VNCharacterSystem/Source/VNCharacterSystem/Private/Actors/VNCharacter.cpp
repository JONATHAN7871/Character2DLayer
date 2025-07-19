#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

AVNCharacter::AVNCharacter()
{
	// Включаем тик для LOD системы
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Создаем все компоненты
	CreateComponents();

	// Настраиваем иерархию
	SetupComponentHierarchy();

	// Инициализация значений по умолчанию
	bIsInFocus = true;
	DimColorMultiplier = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	// Создаем пустое состояние по умолчанию
	CurrentState = F_VN_CharacterState::CreateEmpty(TEXT("DefaultState"));

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

	// Обновляем LOD систему если включена
	if (RenderSettings.bEnableLOD)
	{
		UpdateLOD();
	}
}

#if WITH_EDITOR
void AVNCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Если изменилось состояние в редакторе, применяем его
	if (PropertyChangedEvent.Property && 
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AVNCharacter, CurrentState))
	{
		ApplyCharacterState(CurrentState);
		VN_LOG_DEBUG(TEXT("Character state updated in editor"));
	}

	// Если изменились настройки рендеринга
	if (PropertyChangedEvent.Property && 
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AVNCharacter, RenderSettings))
	{
		ApplyMobileOptimizations();
		VN_LOG_DEBUG(TEXT("Render settings updated in editor"));
	}
}
#endif

// =====================================================
// ОСНОВНОЕ API - УПРАВЛЕНИЕ СОСТОЯНИЕМ
// =====================================================

void AVNCharacter::SetCharacterState(const F_VN_CharacterState& NewState, float TransitionDuration)
{
	VN_LOG_DEBUG(TEXT("=== SetCharacterState START ==="));
	VN_LOG_DEBUG(TEXT("New State ID: %s"), *NewState.StateID.ToString());
	VN_LOG_DEBUG(TEXT("Current State ID: %s"), *CurrentState.StateID.ToString());
	VN_LOG_DEBUG(TEXT("Transition Duration: %.2f"), TransitionDuration);

	// Валидация нового состояния
	if (!ValidateCharacterState(NewState))
	{
		VN_LOG_WARNING(TEXT("SetCharacterState: Invalid state provided for %s"), *GetName());
		return;
	}

	// Проверяем, действительно ли состояние изменилось
	bool bStatesAreDifferent = (CurrentState.StateID != NewState.StateID);
	VN_LOG_DEBUG(TEXT("States are different: %s"), bStatesAreDifferent ? TEXT("Yes") : TEXT("No"));

	// Если состояния одинаковые, все равно применяем (может быть изменились настройки)
	if (!bStatesAreDifferent)
	{
		VN_LOG_DEBUG(TEXT("Same StateID, but applying anyway in case of configuration changes"));
	}

	// Если есть активная анимация, пропускаем её
	if (AnimationManager && AnimationManager->IsAnimating())
	{
		VN_LOG_DEBUG(TEXT("Skipping current animation to start state transition"));
		AnimationManager->SkipCurrentAnimation();
	}

	// Сохраняем новое состояние
	F_VN_CharacterState PreviousState = CurrentState;
	CurrentState = NewState;

	VN_LOG_DEBUG(TEXT("State saved, now applying configuration"));

	// Мгновенно применяем новое состояние (пока без анимации для отладки)
	ApplyCharacterState(CurrentState);

	VN_LOG_DEBUG(TEXT("=== SetCharacterState END ==="));

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
// ВНУТРЕННИЕ МЕТОДЫ - ПРИМЕНЕНИЕ СОСТОЯНИЯ
// =====================================================

void AVNCharacter::ApplyCharacterState(const F_VN_CharacterState& State)
{
	VN_LOG_DEBUG(TEXT("ApplyCharacterState: Applying state '%s'"), *State.StateID.ToString());

	// Применяем конфигурации Skeletal Mesh компонентов
	SetupComponentFromConfig(Body_Skeletal, State.BodyConfig);
	SetupComponentFromConfig(Arms_Skeletal, State.ArmsConfig);
	SetupComponentFromConfig(Head_Skeletal, State.HeadConfig);
	SetupComponentFromConfig(Custom01_Skeletal, State.Custom01Config);
	SetupComponentFromConfig(Custom02_Skeletal, State.Custom02Config);
	SetupComponentFromConfig(Custom03_Skeletal, State.Custom03Config);

	// Применяем конфигурации Sprite компонентов (Attachment)
	SetupComponentFromConfig(Body_Sprite, State.BodySpriteConfig);
	SetupComponentFromConfig(Arms_Sprite, State.ArmsSpriteConfig);
	SetupComponentFromConfig(BodyShadow_Sprite, State.BodyShadowSpriteConfig);
	SetupComponentFromConfig(EmotionBody01_Sprite, State.EmotionBodyEffect01SpriteConfig);
	SetupComponentFromConfig(EmotionBody02_Sprite, State.EmotionBodyEffect02SpriteConfig);
	SetupComponentFromConfig(EmotionBody03_Sprite, State.EmotionBodyEffect03SpriteConfig);

	// Head_Sprite ТЕПЕРЬ использует Attachment конфигурацию
	SetupComponentFromConfig(Head_Sprite, State.HeadSpriteConfig);

	// Применяем конфигурации Sprite компонентов (Simple)
	SetupComponentFromConfig(Eyebrow_Sprite, State.EyebrowSpriteConfig);
	SetupComponentFromConfig(Eyes_Sprite, State.EyesSpriteConfig);
	SetupComponentFromConfig(Eyelids_Sprite, State.EyelidsSpriteConfig);
	SetupComponentFromConfig(Wink_Sprite, State.WinkSpriteConfig);
	SetupComponentFromConfig(Mouth_Sprite, State.MouthSpriteConfig);
	SetupComponentFromConfig(EmotionHead01_Sprite, State.EmotionHeadEffect01SpriteConfig);
	SetupComponentFromConfig(EmotionHead02_Sprite, State.EmotionHeadEffect02SpriteConfig);
	SetupComponentFromConfig(EmotionHead03_Sprite, State.EmotionHeadEffect03SpriteConfig);

	// Применяем глобальные настройки трансформации
	ApplyGlobalTransforms();

	VN_LOG_DEBUG(TEXT("Character state applied successfully"));
}

void AVNCharacter::ApplyGlobalTransforms()
{
	// НЕ применяем глобальные настройки к корневым трансформам!
	// GlobalSkeletalMeshTransform и GlobalSpriteTransform используются только для организации иерархии
	
	VN_LOG_DEBUG(TEXT("Applying global transforms individually to components"));
	
	// Применяем глобальные настройки к каждому Skeletal Mesh компоненту индивидуально
	ApplyGlobalSkeletalTransforms();
	
	// Применяем глобальные настройки к каждому спрайту индивидуально
	ApplyGlobalSpriteTransforms();

	VN_LOG_DEBUG(TEXT("Global transforms applied: SkeletalOffset=%s, SkeletalScale=%.2f, SpriteOffset=%s, SpriteScale=%.2f"),
		*GlobalSkeletalOffset.ToString(), GlobalSkeletalScale,
		*GlobalSpriteOffset.ToString(), GlobalSpriteScale);
}

void AVNCharacter::ApplyGlobalSkeletalTransforms()
{
	// Получаем все Skeletal Mesh компоненты
	TArray<USkeletalMeshComponent*> AllSkeletalMeshes = GetAllSkeletalComponents();
	
	VN_LOG_DEBUG(TEXT("Applying global skeletal transforms to %d components"), AllSkeletalMeshes.Num());
	
	for (USkeletalMeshComponent* SkeletalMesh : AllSkeletalMeshes)
	{
		if (!SkeletalMesh) continue;
		
		// Получаем текущий relative transform компонента
		FTransform CurrentTransform = SkeletalMesh->GetRelativeTransform();
		
		// Применяем глобальные настройки ПОВЕРХ индивидуальных настроек
		FVector NewLocation = CurrentTransform.GetLocation() + GlobalSkeletalOffset;
		FVector NewScale = CurrentTransform.GetScale3D() * GlobalSkeletalScale;
		
		// Создаем новый трансформ
		FTransform NewTransform = CurrentTransform;
		NewTransform.SetLocation(NewLocation);
		NewTransform.SetScale3D(NewScale);
		
		// Применяем
		SkeletalMesh->SetRelativeTransform(NewTransform);
		
		VN_LOG_DEBUG(TEXT("Applied global skeletal transform to %s: Offset=%s, Scale=%.2f"), 
			*SkeletalMesh->GetName(), *GlobalSkeletalOffset.ToString(), GlobalSkeletalScale);
	}
}

void AVNCharacter::ApplyGlobalSpriteTransforms()
{
	// Получаем все спрайт компоненты
	TArray<UPaperSpriteComponent*> AllSprites = GetAllSpriteComponents();
	
	VN_LOG_DEBUG(TEXT("Applying global sprite transforms to %d components"), AllSprites.Num());
	
	for (UPaperSpriteComponent* Sprite : AllSprites)
	{
		if (!Sprite) continue;
		
		// ИСКЛЮЧАЕМ дочерние элементы Head_Sprite из глобальных трансформаций
		// Они получат трансформации через наследование от Head_Sprite
		if (IsChildOfHeadSprite(Sprite))
		{
			VN_LOG_DEBUG(TEXT("Skipping global transform for %s (child of Head_Sprite)"), *Sprite->GetName());
			continue;
		}
		
		// Получаем текущий relative transform спрайта
		FTransform CurrentTransform = Sprite->GetRelativeTransform();
		
		// Применяем глобальные настройки ПОВЕРХ индивидуальных настроек
		FVector NewLocation = CurrentTransform.GetLocation() + GlobalSpriteOffset;
		FVector NewScale = CurrentTransform.GetScale3D() * GlobalSpriteScale;
		
		// Создаем новый трансформ
		FTransform NewTransform = CurrentTransform;
		NewTransform.SetLocation(NewLocation);
		NewTransform.SetScale3D(NewScale);
		
		// Применяем
		Sprite->SetRelativeTransform(NewTransform);
		
		VN_LOG_DEBUG(TEXT("Applied global sprite transform to %s: Offset=%s, Scale=%.2f"), 
			*Sprite->GetName(), *GlobalSpriteOffset.ToString(), GlobalSpriteScale);
	}
}

void AVNCharacter::PrepareTransitionComponents(const F_VN_CharacterState& NewState)
{
	VN_LOG_DEBUG(TEXT("PrepareTransitionComponents: Preparing transition components"));

	// Очищаем массивы компонентов для анимации
	SkeletalMeshesToFadeOut.Empty();
	SpritesToFadeOut.Empty();
	SkeletalMeshesToFadeIn.Empty();
	SpritesToFadeIn.Empty();

	// Сравниваем Skeletal Mesh конфигурации и подготавливаем компоненты для анимации
	
	// Body
	if (CurrentState.BodyConfig != NewState.BodyConfig)
	{
		USkeletalMeshComponent* TempComponent = DuplicateObject<USkeletalMeshComponent>(Body_Skeletal, this);
		if (TempComponent)
		{
			SkeletalMeshesToFadeOut.Add(TempComponent);
			SetupComponentFromConfig(Body_Skeletal, NewState.BodyConfig);
			SetComponentAlpha(Body_Skeletal, 0.0f);
			SkeletalMeshesToFadeIn.Add(Body_Skeletal);
		}
	}

	// Arms
	if (CurrentState.ArmsConfig != NewState.ArmsConfig)
	{
		USkeletalMeshComponent* TempComponent = DuplicateObject<USkeletalMeshComponent>(Arms_Skeletal, this);
		if (TempComponent)
		{
			SkeletalMeshesToFadeOut.Add(TempComponent);
			SetupComponentFromConfig(Arms_Skeletal, NewState.ArmsConfig);
			SetComponentAlpha(Arms_Skeletal, 0.0f);
			SkeletalMeshesToFadeIn.Add(Arms_Skeletal);
		}
	}

	// Head
	if (CurrentState.HeadConfig != NewState.HeadConfig)
	{
		USkeletalMeshComponent* TempComponent = DuplicateObject<USkeletalMeshComponent>(Head_Skeletal, this);
		if (TempComponent)
		{
			SkeletalMeshesToFadeOut.Add(TempComponent);
			SetupComponentFromConfig(Head_Skeletal, NewState.HeadConfig);
			SetComponentAlpha(Head_Skeletal, 0.0f);
			SkeletalMeshesToFadeIn.Add(Head_Skeletal);
		}
	}

	// Сравниваем Sprite конфигурации и подготавливаем компоненты для анимации
	
	// Eyes
	if (CurrentState.EyesSpriteConfig != NewState.EyesSpriteConfig)
	{
		UPaperSpriteComponent* TempComponent = DuplicateObject<UPaperSpriteComponent>(Eyes_Sprite, this);
		if (TempComponent)
		{
			SpritesToFadeOut.Add(TempComponent);
			SetupComponentFromConfig(Eyes_Sprite, NewState.EyesSpriteConfig);
			SetComponentAlpha(Eyes_Sprite, 0.0f);
			SpritesToFadeIn.Add(Eyes_Sprite);
		}
	}

	// Eyebrow
	if (CurrentState.EyebrowSpriteConfig != NewState.EyebrowSpriteConfig)
	{
		UPaperSpriteComponent* TempComponent = DuplicateObject<UPaperSpriteComponent>(Eyebrow_Sprite, this);
		if (TempComponent)
		{
			SpritesToFadeOut.Add(TempComponent);
			SetupComponentFromConfig(Eyebrow_Sprite, NewState.EyebrowSpriteConfig);
			SetComponentAlpha(Eyebrow_Sprite, 0.0f);
			SpritesToFadeIn.Add(Eyebrow_Sprite);
		}
	}

	// Mouth
	if (CurrentState.MouthSpriteConfig != NewState.MouthSpriteConfig)
	{
		UPaperSpriteComponent* TempComponent = DuplicateObject<UPaperSpriteComponent>(Mouth_Sprite, this);
		if (TempComponent)
		{
			SpritesToFadeOut.Add(TempComponent);
			SetupComponentFromConfig(Mouth_Sprite, NewState.MouthSpriteConfig);
			SetComponentAlpha(Mouth_Sprite, 0.0f);
			SpritesToFadeIn.Add(Mouth_Sprite);
		}
	}

	// Можно добавить остальные компоненты по аналогии...

	VN_LOG_DEBUG(TEXT("Transition components prepared: %d SkeletalMesh FadeOut, %d Sprite FadeOut"), 
		SkeletalMeshesToFadeOut.Num(), SpritesToFadeOut.Num());
}

void AVNCharacter::FinishAndCleanupTransition()
{
	VN_LOG_DEBUG(TEXT("FinishAndCleanupTransition: Cleaning up transition components"));

	// Устанавливаем полную непрозрачность для fade-in компонентов
	for (USkeletalMeshComponent* Component : SkeletalMeshesToFadeIn)
	{
		if (Component)
		{
			SetComponentAlpha(Component, 1.0f);
		}
	}

	for (UPaperSpriteComponent* Component : SpritesToFadeIn)
	{
		if (Component)
		{
			SetComponentAlpha(Component, 1.0f);
		}
	}

	// Удаляем fade-out компоненты
	for (USkeletalMeshComponent* Component : SkeletalMeshesToFadeOut)
	{
		if (Component && IsValid(Component))
		{
			Component->DestroyComponent();
		}
	}

	for (UPaperSpriteComponent* Component : SpritesToFadeOut)
	{
		if (Component && IsValid(Component))
		{
			Component->DestroyComponent();
		}
	}

	// Очищаем массивы
	SkeletalMeshesToFadeOut.Empty();
	SpritesToFadeOut.Empty();
	SkeletalMeshesToFadeIn.Empty();
	SpritesToFadeIn.Empty();

	VN_LOG_DEBUG(TEXT("Transition cleanup completed"));
}

// =====================================================
// МЕТОДЫ НАСТРОЙКИ КОМПОНЕНТОВ
// =====================================================

void AVNCharacter::SetupComponentFromConfig(USkeletalMeshComponent* Component, const F_VN_SkeletalConfig_Body& Config)
{
	if (!Component)
	{
		VN_LOG_WARNING(TEXT("SetupComponentFromConfig: SkeletalMeshComponent is null"));
		return;
	}

	// Устанавливаем видимость
	Component->SetVisibility(Config.bVisible);
	
	if (!Config.bVisible)
	{
		return; // Если компонент невидим, дальше не настраиваем
	}

	// Загружаем и устанавливаем Skeletal Mesh
	if (!Config.SkeletalMesh.IsNull())
	{
		USkeletalMesh* LoadedMesh = Config.SkeletalMesh.LoadSynchronous();
		if (LoadedMesh)
		{
			Component->SetSkeletalMesh(LoadedMesh);
		}
		else
		{
			VN_LOG_WARNING(TEXT("Failed to load SkeletalMesh for component %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSkeletalMesh(nullptr);
	}

	// Устанавливаем AnimInstance класс
	if (Config.AnimInstanceClass)
	{
		Component->SetAnimInstanceClass(Config.AnimInstanceClass);
	}

	// Применяем переопределения материалов
	for (const auto& MaterialPair : Config.MaterialOverrides)
	{
		int32 MaterialIndex = MaterialPair.Key;
		TSoftObjectPtr<UMaterialInterface> MaterialPtr = MaterialPair.Value;

		if (!MaterialPtr.IsNull())
		{
			UMaterialInterface* LoadedMaterial = MaterialPtr.LoadSynchronous();
			if (LoadedMaterial)
			{
				Component->SetMaterial(MaterialIndex, LoadedMaterial);
			}
		}
	}

	// Устанавливаем трансформ
	FTransform ComponentTransform = Component->GetRelativeTransform();
	ComponentTransform.SetLocation(Config.Offset);
	ComponentTransform.SetScale3D(FVector(Config.Scale));
	Component->SetRelativeTransform(ComponentTransform);

	// Устанавливаем цвет
	FLinearColor FinalColor = GetTargetColorForComponent(Component);
	SetComponentColor(Component, FinalColor);

	VN_LOG_DEBUG(TEXT("SkeletalMesh component %s configured from Body config"), *Component->GetName());
}

void AVNCharacter::SetupComponentFromConfig(USkeletalMeshComponent* Component, const F_VN_SkeletalConfig_Attachment& Config)
{
	if (!Component)
	{
		VN_LOG_WARNING(TEXT("SetupComponentFromConfig: SkeletalMeshComponent is null"));
		return;
	}

	// Устанавливаем видимость
	Component->SetVisibility(Config.bVisible);
	
	if (!Config.bVisible)
	{
		return; // Если компонент невидим, дальше не настраиваем
	}

	// Загружаем и устанавливаем Skeletal Mesh
	if (!Config.SkeletalMesh.IsNull())
	{
		USkeletalMesh* LoadedMesh = Config.SkeletalMesh.LoadSynchronous();
		if (LoadedMesh)
		{
			Component->SetSkeletalMesh(LoadedMesh);
		}
		else
		{
			VN_LOG_WARNING(TEXT("Failed to load SkeletalMesh for component %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSkeletalMesh(nullptr);
	}

	// Устанавливаем AnimInstance класс
	if (Config.AnimInstanceClass)
	{
		Component->SetAnimInstanceClass(Config.AnimInstanceClass);
	}

	// Применяем переопределения материалов
	for (const auto& MaterialPair : Config.MaterialOverrides)
	{
		int32 MaterialIndex = MaterialPair.Key;
		TSoftObjectPtr<UMaterialInterface> MaterialPtr = MaterialPair.Value;

		if (!MaterialPtr.IsNull())
		{
			UMaterialInterface* LoadedMaterial = MaterialPtr.LoadSynchronous();
			if (LoadedMaterial)
			{
				Component->SetMaterial(MaterialIndex, LoadedMaterial);
			}
		}
	}

	// Настраиваем прикрепление
	if (Config.AttachTo != E_SkeletalAttachmentTarget::None)
	{
		USkeletalMeshComponent* AttachTarget = nullptr;
		
		switch (Config.AttachTo)
		{
			case E_SkeletalAttachmentTarget::Body:
				AttachTarget = Body_Skeletal;
				break;
			default:
				VN_LOG_WARNING(TEXT("Unknown attachment target for component %s"), *Component->GetName());
				break;
		}

		if (AttachTarget && Config.bUseSocketTransform && !Config.SocketName.IsNone())
		{
			// Прикрепляем к сокету
			Component->AttachToComponent(AttachTarget, 
				FAttachmentTransformRules::KeepWorldTransform, Config.SocketName);
		}
		else if (AttachTarget)
		{
			// Прикрепляем к компоненту без сокета
			Component->AttachToComponent(AttachTarget, 
				FAttachmentTransformRules::KeepRelativeTransform);
		}
	}

	// Устанавливаем трансформ
	FTransform ComponentTransform = Component->GetRelativeTransform();
	ComponentTransform.SetLocation(Config.Offset);
	ComponentTransform.SetScale3D(FVector(Config.Scale));
	Component->SetRelativeTransform(ComponentTransform);

	// Устанавливаем цвет
	FLinearColor FinalColor = GetTargetColorForComponent(Component);
	SetComponentColor(Component, FinalColor);

	VN_LOG_DEBUG(TEXT("SkeletalMesh component %s configured from Attachment config"), *Component->GetName());
}

void AVNCharacter::SetupComponentFromConfig(UPaperSpriteComponent* Component, const F_VN_SpriteConfig_Attachment& Config)
{
	if (!Component)
	{
		VN_LOG_WARNING(TEXT("SetupComponentFromConfig: PaperSpriteComponent is null"));
		return;
	}

	// Устанавливаем видимость
	Component->SetVisibility(Config.bVisible);
	
	if (!Config.bVisible)
	{
		return; // Если компонент невидим, дальше не настраиваем
	}

	// Загружаем и устанавливаем Sprite
	if (!Config.Sprite.IsNull())
	{
		if (UPaperSprite* LoadedSprite = Config.Sprite.LoadSynchronous())
		{
			Component->SetSprite(LoadedSprite);
		}
		else
		{
			VN_LOG_WARNING(TEXT("Failed to load Sprite for component %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSprite(nullptr);
	}

	// Настраиваем прикрепление к Skeletal Mesh компонентам
	if (Config.AttachTo != E_SpriteAttachmentTarget::None)
	{
		USkeletalMeshComponent* AttachTarget = nullptr;
		
		switch (Config.AttachTo)
		{
			case E_SpriteAttachmentTarget::Body_Skeletal:
				AttachTarget = Body_Skeletal;
				break;
			case E_SpriteAttachmentTarget::Arms_Skeletal:
				AttachTarget = Arms_Skeletal;
				break;
			case E_SpriteAttachmentTarget::Head_Skeletal:
				AttachTarget = Head_Skeletal;
				break;
			case E_SpriteAttachmentTarget::Custom01_Skeletal:
				AttachTarget = Custom01_Skeletal;
				break;
			case E_SpriteAttachmentTarget::Custom02_Skeletal:
				AttachTarget = Custom02_Skeletal;
				break;
			case E_SpriteAttachmentTarget::Custom03_Skeletal:
				AttachTarget = Custom03_Skeletal;
				break;
			default:
				VN_LOG_WARNING(TEXT("Unknown sprite attachment target for component %s"), *Component->GetName());
				break;
		}

		if (AttachTarget && Config.bUseSocketTransform && !Config.SocketName.IsNone())
		{
			// Прикрепляем к сокету
			Component->AttachToComponent(AttachTarget, 
				FAttachmentTransformRules::KeepWorldTransform, Config.SocketName);
		}
		else if (AttachTarget)
		{
			// Прикрепляем к компоненту без сокета
			Component->AttachToComponent(AttachTarget, 
				FAttachmentTransformRules::KeepRelativeTransform);
		}
	}

	// Устанавливаем трансформ
	FTransform ComponentTransform = Component->GetRelativeTransform();
	ComponentTransform.SetLocation(Config.Offset);
	ComponentTransform.SetScale3D(FVector(Config.Scale));
	Component->SetRelativeTransform(ComponentTransform);

	// Устанавливаем цвет
	FLinearColor FinalColor = GetTargetColorForComponent(Component);
	SetComponentColor(Component, FinalColor);

	VN_LOG_DEBUG(TEXT("Sprite component %s configured from Attachment config"), *Component->GetName());
}

void AVNCharacter::SetupComponentFromConfig(UPaperSpriteComponent* Component, const F_VN_SpriteConfig_Simple& Config)
{
	if (!Component)
	{
		VN_LOG_WARNING(TEXT("SetupComponentFromConfig: PaperSpriteComponent is null"));
		return;
	}

	// Устанавливаем видимость
	Component->SetVisibility(Config.bVisible);
	
	if (!Config.bVisible)
	{
		return; // Если компонент невидим, дальше не настраиваем
	}

	// Загружаем и устанавливаем Sprite
	if (!Config.Sprite.IsNull())
	{
		if (UPaperSprite* LoadedSprite = Config.Sprite.LoadSynchronous())
		{
			Component->SetSprite(LoadedSprite);
		}
		else
		{
			VN_LOG_WARNING(TEXT("Failed to load Sprite for component %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSprite(nullptr);
	}

	// Устанавливаем трансформ
	FTransform ComponentTransform = Component->GetRelativeTransform();
	ComponentTransform.SetLocation(Config.Offset);
	ComponentTransform.SetScale3D(FVector(Config.Scale));
	Component->SetRelativeTransform(ComponentTransform);

	// Устанавливаем цвет
	FLinearColor FinalColor = GetTargetColorForComponent(Component);
	SetComponentColor(Component, FinalColor);

	VN_LOG_DEBUG(TEXT("Sprite component %s configured from Simple config"), *Component->GetName());
}

// =====================================================
// УТИЛИТЫ ДЛЯ РАБОТЫ С КОМПОНЕНТАМИ
// =====================================================

void AVNCharacter::SetComponentAlpha(USceneComponent* Component, float Alpha)
{
	if (!Component)
	{
		return;
	}

	Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

	// Для Skeletal Mesh компонентов
	if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(Component))
	{
		// Создаем или получаем динамический материал
		for (int32 i = 0; i < SkeletalMesh->GetNumMaterials(); ++i)
		{
			UMaterialInterface* BaseMaterial = SkeletalMesh->GetMaterial(i);
			if (BaseMaterial)
			{
				UMaterialInstanceDynamic* DynamicMaterial = SkeletalMesh->CreateAndSetMaterialInstanceDynamic(i);
				if (DynamicMaterial)
				{
					DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), Alpha);
				}
			}
		}
	}
	// Для Sprite компонентов
	else if (UPaperSpriteComponent* SpriteComponent = Cast<UPaperSpriteComponent>(Component))
	{
		// Получаем текущий цвет и изменяем альфа
		FLinearColor CurrentColor = SpriteComponent->GetSpriteColor();
		CurrentColor.A = Alpha;
		SpriteComponent->SetSpriteColor(CurrentColor);
	}
}

void AVNCharacter::SetComponentColor(USceneComponent* Component, const FLinearColor& Color)
{
	if (!Component)
	{
		return;
	}

	// Для Skeletal Mesh компонентов
	if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(Component))
	{
		// Создаем или получаем динамический материал
		for (int32 i = 0; i < SkeletalMesh->GetNumMaterials(); ++i)
		{
			UMaterialInterface* BaseMaterial = SkeletalMesh->GetMaterial(i);
			if (BaseMaterial)
			{
				UMaterialInstanceDynamic* DynamicMaterial = SkeletalMesh->CreateAndSetMaterialInstanceDynamic(i);
				if (DynamicMaterial)
				{
					DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
					DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), Color.A);
				}
			}
		}
	}
	// Для Sprite компонентов
	else if (UPaperSpriteComponent* SpriteComponent = Cast<UPaperSpriteComponent>(Component))
	{
		SpriteComponent->SetSpriteColor(Color);
	}
}

USkeletalMeshComponent* AVNCharacter::GetSkeletalComponent(E_VN_ComponentID_Skeletal ComponentID) const
{
	switch (ComponentID)
	{
		case E_VN_ComponentID_Skeletal::Body:
			return Body_Skeletal;
		case E_VN_ComponentID_Skeletal::Arms:
			return Arms_Skeletal;
		case E_VN_ComponentID_Skeletal::Head:
			return Head_Skeletal;
		case E_VN_ComponentID_Skeletal::Custom01:
			return Custom01_Skeletal;
		case E_VN_ComponentID_Skeletal::Custom02:
			return Custom02_Skeletal;
		case E_VN_ComponentID_Skeletal::Custom03:
			return Custom03_Skeletal;
		default:
			return nullptr;
	}
}

UPaperSpriteComponent* AVNCharacter::GetSpriteComponent(E_VN_ComponentID_Sprite ComponentID) const
{
	switch (ComponentID)
	{
		case E_VN_ComponentID_Sprite::Body:
			return Body_Sprite;
		case E_VN_ComponentID_Sprite::Arms:
			return Arms_Sprite;
		case E_VN_ComponentID_Sprite::Head:
			return Head_Sprite;
		case E_VN_ComponentID_Sprite::Eyebrow:
			return Eyebrow_Sprite;
		case E_VN_ComponentID_Sprite::Eyes:
			return Eyes_Sprite;
		case E_VN_ComponentID_Sprite::Eyelids:
			return Eyelids_Sprite;
		case E_VN_ComponentID_Sprite::Wink:
			return Wink_Sprite;
		case E_VN_ComponentID_Sprite::Mouth:
			return Mouth_Sprite;
		case E_VN_ComponentID_Sprite::BodyShadow:
			return BodyShadow_Sprite;
		case E_VN_ComponentID_Sprite::EmotionHead_01:
			return EmotionHead01_Sprite;
		case E_VN_ComponentID_Sprite::EmotionHead_02:
			return EmotionHead02_Sprite;
		case E_VN_ComponentID_Sprite::EmotionHead_03:
			return EmotionHead03_Sprite;
		case E_VN_ComponentID_Sprite::EmotionBody_01:
			return EmotionBody01_Sprite;
		case E_VN_ComponentID_Sprite::EmotionBody_02:
			return EmotionBody02_Sprite;
		case E_VN_ComponentID_Sprite::EmotionBody_03:
			return EmotionBody03_Sprite;
		default:
			return nullptr;
	}
}

TArray<USceneComponent*> AVNCharacter::GetAllRenderComponents() const
{
	TArray<USceneComponent*> Components;
	
	// Добавляем все Skeletal Mesh компоненты
	TArray<USkeletalMeshComponent*> SkeletalComponents = GetAllSkeletalComponents();
	for (USkeletalMeshComponent* Component : SkeletalComponents)
	{
		if (Component)
		{
			Components.Add(Component);
		}
	}
	
	// Добавляем все Sprite компоненты
	TArray<UPaperSpriteComponent*> SpriteComponents = GetAllSpriteComponents();
	for (UPaperSpriteComponent* Component : SpriteComponents)
	{
		if (Component)
		{
			Components.Add(Component);
		}
	}
	
	return Components;
}

TArray<USkeletalMeshComponent*> AVNCharacter::GetAllSkeletalComponents() const
{
	TArray<USkeletalMeshComponent*> Components;
	
	Components.Add(Body_Skeletal);
	Components.Add(Arms_Skeletal);
	Components.Add(Head_Skeletal);
	Components.Add(Custom01_Skeletal);
	Components.Add(Custom02_Skeletal);
	Components.Add(Custom03_Skeletal);
	
	// Удаляем null указатели
	Components.RemoveAll([](USkeletalMeshComponent* Component) { return Component == nullptr; });
	
	return Components;
}

TArray<UPaperSpriteComponent*> AVNCharacter::GetAllSpriteComponents() const
{
	TArray<UPaperSpriteComponent*> Components;
	
	Components.Add(Body_Sprite);
	Components.Add(Arms_Sprite);
	Components.Add(Head_Sprite);
	Components.Add(Eyebrow_Sprite);
	Components.Add(Eyes_Sprite);
	Components.Add(Eyelids_Sprite);
	Components.Add(Wink_Sprite);
	Components.Add(Mouth_Sprite);
	Components.Add(BodyShadow_Sprite);
	Components.Add(EmotionHead01_Sprite);
	Components.Add(EmotionHead02_Sprite);
	Components.Add(EmotionHead03_Sprite);
	Components.Add(EmotionBody01_Sprite);
	Components.Add(EmotionBody02_Sprite);
	Components.Add(EmotionBody03_Sprite);
	
	// Удаляем null указатели
	Components.RemoveAll([](UPaperSpriteComponent* Component) { return Component == nullptr; });
	
	return Components;
}

// =====================================================
// ВАЛИДАЦИЯ И ОБРАБОТКА ОШИБОК
// =====================================================

bool AVNCharacter::ValidateCharacterState(const F_VN_CharacterState& State) const
{
	if (!State.IsValid())
	{
		VN_LOG_WARNING(TEXT("Character state is invalid: StateID is None"));
		return false;
	}

	// Проверяем, что состояние содержит хотя бы один видимый компонент
	if (!State.HasVisibleComponents())
	{
		VN_LOG_WARNING(TEXT("Character state '%s' has no visible components"), *State.StateID.ToString());
		return false;
	}

	// Детальная валидация ассетов
	return ValidateAssets(State);
}

bool AVNCharacter::ValidateAssets(const F_VN_CharacterState& State) const
{
	bool bIsValid = true;
	TArray<FString> ValidationErrors = State.GetDetailedValidationErrors();
	
	if (ValidationErrors.Num() > 0)
	{
		FString ErrorMessage = FString::Printf(TEXT("Asset validation failed for state '%s':"), 
			*State.StateID.ToString());
		
		for (const FString& Error : ValidationErrors)
		{
			ErrorMessage += FString::Printf(TEXT("\n- %s"), *Error);
		}
		
		VN_LOG_WARNING(TEXT("%s"), *ErrorMessage);
		bIsValid = false;
	}
	
	return bIsValid;
}

void AVNCharacter::ApplyMobileOptimizations()
{
#if VN_CHARACTER_SYSTEM_MOBILE
	if (RenderSettings.bDisableShadowsOnMobile)
	{
		// Отключаем тени для всех Sprite компонентов
		TArray<UPaperSpriteComponent*> SpriteComponents = GetAllSpriteComponents();
		for (UPaperSpriteComponent* Sprite : SpriteComponents)
		{
			if (Sprite)
			{
				Sprite->SetCastShadow(false);
			}
		}
		
		VN_LOG_DEBUG(TEXT("Mobile optimization: Shadows disabled for sprites"));
	}
	
	if (RenderSettings.bUseSimplifiedMaterialsOnMobile)
	{
		// Здесь можно добавить логику замены материалов на упрощенные версии
		VN_LOG_DEBUG(TEXT("Mobile optimization: Simplified materials applied"));
	}
#endif
}

void AVNCharacter::UpdateLOD()
{
	if (!RenderSettings.bEnableLOD)
	{
		return;
	}

	// Получаем расстояние до камеры игрока
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (APawn* PlayerPawn = PC->GetPawn())
		{
			float Distance = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());
			bool bShouldUseHighLOD = Distance < RenderSettings.LODDistance;
			
			// Переключаем LOD для всех Skeletal Mesh компонентов
			TArray<USkeletalMeshComponent*> SkeletalComponents = GetAllSkeletalComponents();
			for (USkeletalMeshComponent* Component : SkeletalComponents)
			{
				if (Component)
				{
					Component->SetForcedLOD(bShouldUseHighLOD ? 0 : 1);
				}
			}
		}
	}
}

bool AVNCharacter::IsChildOfHeadSprite(UPaperSpriteComponent* Sprite) const
{
	if (!Sprite || !Head_Sprite) return false;
	
	// Проверяем, является ли спрайт одним из лицевых элементов
	return (Sprite == Eyebrow_Sprite ||
			Sprite == Eyes_Sprite ||
			Sprite == Eyelids_Sprite ||
			Sprite == Wink_Sprite ||
			Sprite == Mouth_Sprite ||
			Sprite == EmotionHead01_Sprite ||
			Sprite == EmotionHead02_Sprite ||
			Sprite == EmotionHead03_Sprite);
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

// =====================================================
// ОТЛАДОЧНЫЕ МЕТОДЫ
// =====================================================

#if WITH_EDITOR
void AVNCharacter::PrintDebugInfo()
{
	FString DebugInfo = TEXT("=== VN Character Debug Info ===\n");
	
	DebugInfo += FString::Printf(TEXT("Actor: %s\n"), *GetName());
	DebugInfo += FString::Printf(TEXT("Current State: %s\n"), *CurrentState.StateID.ToString());
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
	
	DebugInfo += TEXT("\nState Summary: ") + CurrentState.GetSummary() + TEXT("\n");
	
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