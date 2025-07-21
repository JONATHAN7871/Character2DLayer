#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

AVNCharacter::AVNCharacter()
{
	// Отключаем тик
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Создаем все компоненты
	CreateComponents();

	// Настраиваем иерархию
	SetupComponentHierarchy();

	// Инициализация значений по умолчанию
	bIsInFocus = true;
	DimColorMultiplier = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

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
	// СОЗДАНИЕ SKELETAL MESH КОМПОНЕНТОВ (ОСНОВНЫЕ + FADE)
	// =====================================================

	// Основные компоненты
	Body_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body_Skeletal"));
	Arms_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Arms_Skeletal"));
	Head_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Head_Skeletal"));
	Custom01_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Custom01_Skeletal"));
	Custom02_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Custom02_Skeletal"));
	Custom03_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Custom03_Skeletal"));

	// Fade компоненты
	Body_Skeletal_Fade = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body_Skeletal_Fade"));
	Arms_Skeletal_Fade = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Arms_Skeletal_Fade"));
	Head_Skeletal_Fade = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Head_Skeletal_Fade"));
	Custom01_Skeletal_Fade = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Custom01_Skeletal_Fade"));
	Custom02_Skeletal_Fade = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Custom02_Skeletal_Fade"));
	Custom03_Skeletal_Fade = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Custom03_Skeletal_Fade"));

	// =====================================================
	// СОЗДАНИЕ SPRITE КОМПОНЕНТОВ (ОСНОВНЫЕ + FADE)
	// =====================================================

	// Основные спрайты тела
	Body_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Body_Sprite"));
	Arms_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Arms_Sprite"));
	BodyShadow_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("BodyShadow_Sprite"));
	EmotionBodyEffect01_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionBodyEffect01_Sprite"));
	EmotionBodyEffect02_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionBodyEffect02_Sprite"));
	EmotionBodyEffect03_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionBodyEffect03_Sprite"));

	// Fade спрайты тела
	Body_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Body_Sprite_Fade"));
	Arms_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Arms_Sprite_Fade"));
	BodyShadow_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("BodyShadow_Sprite_Fade"));
	EmotionBodyEffect01_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionBodyEffect01_Sprite_Fade"));
	EmotionBodyEffect02_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionBodyEffect02_Sprite_Fade"));
	EmotionBodyEffect03_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionBodyEffect03_Sprite_Fade"));

	// Основные спрайты головы
	Head_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Head_Sprite"));
	Eyebrow_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Eyebrow_Sprite"));
	Eyes_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Eyes_Sprite"));
	Eyelids_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Eyelids_Sprite"));
	Wink_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Wink_Sprite"));
	Mouth_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Mouth_Sprite"));
	EmotionHeadEffect01_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionHeadEffect01_Sprite"));
	EmotionHeadEffect02_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionHeadEffect02_Sprite"));
	EmotionHeadEffect03_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionHeadEffect03_Sprite"));

	// Fade спрайты головы
	Eyebrow_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Eyebrow_Sprite_Fade"));
	Eyes_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Eyes_Sprite_Fade"));
	Eyelids_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Eyelids_Sprite_Fade"));
	Wink_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Wink_Sprite_Fade"));
	Mouth_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Mouth_Sprite_Fade"));
	EmotionHeadEffect01_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionHeadEffect01_Sprite_Fade"));
	EmotionHeadEffect02_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionHeadEffect02_Sprite_Fade"));
	EmotionHeadEffect03_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionHeadEffect03_Sprite_Fade"));

	VN_LOG_DEBUG(TEXT("All components created for VNCharacter"));
}

void AVNCharacter::SetupComponentHierarchy()
{
	// Настройка корневых трансформов
	GlobalSkeletalMeshTransform->SetupAttachment(RootComponent);
	GlobalSpriteTransform->SetupAttachment(RootComponent);

	// =====================================================
	// SKELETAL MESH ИЕРАРХИЯ (ОСНОВНЫЕ + FADE)
	// =====================================================

	// Основные компоненты
	Body_Skeletal->SetupAttachment(GlobalSkeletalMeshTransform);
	Arms_Skeletal->SetupAttachment(GlobalSkeletalMeshTransform);
	Head_Skeletal->SetupAttachment(GlobalSkeletalMeshTransform);
	Custom01_Skeletal->SetupAttachment(GlobalSkeletalMeshTransform);
	Custom02_Skeletal->SetupAttachment(GlobalSkeletalMeshTransform);
	Custom03_Skeletal->SetupAttachment(GlobalSkeletalMeshTransform);

	// Fade компоненты
	Body_Skeletal_Fade->SetupAttachment(GlobalSkeletalMeshTransform);
	Arms_Skeletal_Fade->SetupAttachment(GlobalSkeletalMeshTransform);
	Head_Skeletal_Fade->SetupAttachment(GlobalSkeletalMeshTransform);
	Custom01_Skeletal_Fade->SetupAttachment(GlobalSkeletalMeshTransform);
	Custom02_Skeletal_Fade->SetupAttachment(GlobalSkeletalMeshTransform);
	Custom03_Skeletal_Fade->SetupAttachment(GlobalSkeletalMeshTransform);

	// =====================================================
	// SPRITE ИЕРАРХИЯ (ОСНОВНЫЕ + FADE)
	// =====================================================

	// Основные спрайты тела прикрепляются к корневому sprite transform
	Body_Sprite->SetupAttachment(GlobalSpriteTransform);
	Arms_Sprite->SetupAttachment(GlobalSpriteTransform);
	BodyShadow_Sprite->SetupAttachment(GlobalSpriteTransform);
	EmotionBodyEffect01_Sprite->SetupAttachment(GlobalSpriteTransform);
	EmotionBodyEffect02_Sprite->SetupAttachment(GlobalSpriteTransform);
	EmotionBodyEffect03_Sprite->SetupAttachment(GlobalSpriteTransform);

	// Fade спрайты тела
	Body_Sprite_Fade->SetupAttachment(GlobalSpriteTransform);
	Arms_Sprite_Fade->SetupAttachment(GlobalSpriteTransform);
	BodyShadow_Sprite_Fade->SetupAttachment(GlobalSpriteTransform);
	EmotionBodyEffect01_Sprite_Fade->SetupAttachment(GlobalSpriteTransform);
	EmotionBodyEffect02_Sprite_Fade->SetupAttachment(GlobalSpriteTransform);
	EmotionBodyEffect03_Sprite_Fade->SetupAttachment(GlobalSpriteTransform);

	// Head_Sprite прикрепляется к GlobalSpriteTransform
	Head_Sprite->SetupAttachment(GlobalSpriteTransform);
	
	// Основные спрайты головы прикрепляются к Head_Sprite
	Eyebrow_Sprite->SetupAttachment(Head_Sprite);
	Eyes_Sprite->SetupAttachment(Head_Sprite);
	Eyelids_Sprite->SetupAttachment(Head_Sprite);
	Wink_Sprite->SetupAttachment(Head_Sprite);
	Mouth_Sprite->SetupAttachment(Head_Sprite);
	EmotionHeadEffect01_Sprite->SetupAttachment(Head_Sprite);
	EmotionHeadEffect02_Sprite->SetupAttachment(Head_Sprite);
	EmotionHeadEffect03_Sprite->SetupAttachment(Head_Sprite);

	// Fade спрайты головы тоже прикрепляются к Head_Sprite
	Eyebrow_Sprite_Fade->SetupAttachment(Head_Sprite);
	Eyes_Sprite_Fade->SetupAttachment(Head_Sprite);
	Eyelids_Sprite_Fade->SetupAttachment(Head_Sprite);
	Wink_Sprite_Fade->SetupAttachment(Head_Sprite);
	Mouth_Sprite_Fade->SetupAttachment(Head_Sprite);
	EmotionHeadEffect01_Sprite_Fade->SetupAttachment(Head_Sprite);
	EmotionHeadEffect02_Sprite_Fade->SetupAttachment(Head_Sprite);
	EmotionHeadEffect03_Sprite_Fade->SetupAttachment(Head_Sprite);

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

	// Скрываем все fade компоненты изначально
	HideAllFadeComponents();

	// Применяем глобальные трансформации
	ApplyGlobalTransforms();

	VN_LOG_DEBUG(TEXT("VNCharacter BeginPlay completed: %s"), *GetName());
}

#if WITH_EDITOR
void AVNCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Если изменились глобальные трансформации
	if (PropertyChangedEvent.Property && 
		(PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AVNCharacter, GlobalSkeletalOffset) ||
		 PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AVNCharacter, GlobalSkeletalScale) ||
		 PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AVNCharacter, GlobalSpriteOffset) ||
		 PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AVNCharacter, GlobalSpriteScale)))
	{
		ApplyGlobalTransforms();
		VN_LOG_DEBUG(TEXT("Global transforms updated in editor"));
	}
}

void AVNCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	// Скрываем fade компоненты в редакторе
	HideAllFadeComponents();
	
	// Применяем глобальные трансформации
	ApplyGlobalTransforms();
}
#endif

// =====================================================
// ИСПРАВЛЕННЫЕ МЕТОДЫ С ENUM'АМИ
// =====================================================

void AVNCharacter::SetSkeletalMesh(E_VN_ComponentID_Skeletal ComponentID, TSoftObjectPtr<USkeletalMesh> SkeletalMesh, bool bAnimate, float Duration)
{
	VN_LOG_DEBUG(TEXT("SetSkeletalMesh: ComponentID %d, Animate: %s"), (int32)ComponentID, bAnimate ? TEXT("Yes") : TEXT("No"));

	USkeletalMeshComponent* MainComponent = GetSkeletalComponent(ComponentID);
	if (!MainComponent)
	{
		VN_LOG_WARNING(TEXT("SetSkeletalMesh: Component not found for ID %d"), (int32)ComponentID);
		return;
	}

	if (bAnimate && Duration > 0.0f && AnimationManager)
	{
		// Подготавливаем анимацию перехода
		USkeletalMeshComponent* FadeComponent = GetSkeletalFadeComponent(ComponentID);
		if (FadeComponent)
		{
			PrepareSkeletalTransition(MainComponent, FadeComponent, SkeletalMesh);
			AnimationManager->PlayTransition(Duration);
		}
		else
		{
			// Если нет fade компонента, применяем мгновенно
			ValidateAndSetupSkeletalComponent(MainComponent, SkeletalMesh);
		}
	}
	else
	{
		// Мгновенное применение
		ValidateAndSetupSkeletalComponent(MainComponent, SkeletalMesh);
	}
}

void AVNCharacter::SetSprite(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> Sprite, bool bAnimate, float Duration)
{
	VN_LOG_DEBUG(TEXT("SetSprite: ComponentID %d, Animate: %s"), (int32)ComponentID, bAnimate ? TEXT("Yes") : TEXT("No"));

	UPaperSpriteComponent* MainComponent = GetSpriteComponent(ComponentID);
	if (!MainComponent)
	{
		VN_LOG_WARNING(TEXT("SetSprite: Component not found for ID %d"), (int32)ComponentID);
		return;
	}

	if (bAnimate && Duration > 0.0f && AnimationManager)
	{
		// Подготавливаем анимацию перехода
		UPaperSpriteComponent* FadeComponent = GetSpriteFadeComponent(ComponentID);
		if (FadeComponent)
		{
			PrepareSpriteTransition(MainComponent, FadeComponent, Sprite);
			AnimationManager->PlayTransition(Duration);
		}
		else
		{
			// Если нет fade компонента, применяем мгновенно
			ValidateAndSetupSpriteComponent(MainComponent, Sprite);
			ApplyIndividualSpriteTransform(MainComponent, ComponentID);
		}
	}
	else
	{
		// Мгновенное применение
		ValidateAndSetupSpriteComponent(MainComponent, Sprite);
		ApplyIndividualSpriteTransform(MainComponent, ComponentID);
	}

	// Уведомляем о изменении
	OnCharacterComponentChanged.Broadcast(ComponentID);
}

// =====================================================
// УПРОЩЕННЫЕ МЕТОДЫ (ИСПРАВЛЕНЫ ДЛЯ ИСПОЛЬЗОВАНИЯ ENUM'ОВ)
// =====================================================

void AVNCharacter::SetEyes(TSoftObjectPtr<UPaperSprite> EyesSprite, bool bAnimate, float Duration)
{
	SetSprite(E_VN_ComponentID_Sprite::Eyes, EyesSprite, bAnimate, Duration);
}

void AVNCharacter::SetMouth(TSoftObjectPtr<UPaperSprite> MouthSprite, bool bAnimate, float Duration)
{
	SetSprite(E_VN_ComponentID_Sprite::Mouth, MouthSprite, bAnimate, Duration);
}

void AVNCharacter::SetEyebrows(TSoftObjectPtr<UPaperSprite> EyebrowSprite, bool bAnimate, float Duration)
{
	SetSprite(E_VN_ComponentID_Sprite::Eyebrow, EyebrowSprite, bAnimate, Duration);
}

void AVNCharacter::SetBody(TSoftObjectPtr<USkeletalMesh> BodyMesh, bool bAnimate, float Duration)
{
	SetSkeletalMesh(E_VN_ComponentID_Skeletal::Body, BodyMesh, bAnimate, Duration);
}

void AVNCharacter::SetArms(TSoftObjectPtr<USkeletalMesh> ArmsMesh, bool bAnimate, float Duration)
{
	SetSkeletalMesh(E_VN_ComponentID_Skeletal::Arms, ArmsMesh, bAnimate, Duration);
}

void AVNCharacter::SetFace(TSoftObjectPtr<UPaperSprite> EyesSprite, TSoftObjectPtr<UPaperSprite> MouthSprite, TSoftObjectPtr<UPaperSprite> EyebrowSprite, bool bAnimate, float Duration)
{
	VN_LOG_DEBUG(TEXT("SetFace: Setting multiple facial components"));

	// Устанавливаем все элементы лица
	// ВАЖНО: Используем bAnimate=false для индивидуальных вызовов, 
	// чтобы избежать множественных анимаций
	SetSprite(E_VN_ComponentID_Sprite::Eyes, EyesSprite, false, 0.0f);
	SetSprite(E_VN_ComponentID_Sprite::Mouth, MouthSprite, false, 0.0f);
	SetSprite(E_VN_ComponentID_Sprite::Eyebrow, EyebrowSprite, false, 0.0f);

	// Если нужна анимация, запускаем её один раз для всех изменений
	if (bAnimate && Duration > 0.0f && AnimationManager)
	{
		AnimationManager->PlayTransition(Duration);
	}
}

// =====================================================
// МЕТОДЫ ДЛЯ ПОЛУЧЕНИЯ КОМПОНЕНТОВ (ИСПРАВЛЕНЫ ДЛЯ ENUM'ОВ)
// =====================================================

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
			return EmotionHeadEffect01_Sprite;
		case E_VN_ComponentID_Sprite::EmotionHead_02:
			return EmotionHeadEffect02_Sprite;
		case E_VN_ComponentID_Sprite::EmotionHead_03:
			return EmotionHeadEffect03_Sprite;
		case E_VN_ComponentID_Sprite::EmotionBody_01:
			return EmotionBodyEffect01_Sprite;
		case E_VN_ComponentID_Sprite::EmotionBody_02:
			return EmotionBodyEffect02_Sprite;
		case E_VN_ComponentID_Sprite::EmotionBody_03:
			return EmotionBodyEffect03_Sprite;
		default:
			return nullptr;
	}
}

USkeletalMeshComponent* AVNCharacter::GetSkeletalFadeComponent(E_VN_ComponentID_Skeletal ComponentID) const
{
	switch (ComponentID)
	{
		case E_VN_ComponentID_Skeletal::Body:
			return Body_Skeletal_Fade;
		case E_VN_ComponentID_Skeletal::Arms:
			return Arms_Skeletal_Fade;
		case E_VN_ComponentID_Skeletal::Head:
			return Head_Skeletal_Fade;
		case E_VN_ComponentID_Skeletal::Custom01:
			return Custom01_Skeletal_Fade;
		case E_VN_ComponentID_Skeletal::Custom02:
			return Custom02_Skeletal_Fade;
		case E_VN_ComponentID_Skeletal::Custom03:
			return Custom03_Skeletal_Fade;
		default:
			return nullptr;
	}
}

UPaperSpriteComponent* AVNCharacter::GetSpriteFadeComponent(E_VN_ComponentID_Sprite ComponentID) const
{
	switch (ComponentID)
	{
		case E_VN_ComponentID_Sprite::Body:
			return Body_Sprite_Fade;
		case E_VN_ComponentID_Sprite::Arms:
			return Arms_Sprite_Fade;
		case E_VN_ComponentID_Sprite::Eyebrow:
			return Eyebrow_Sprite_Fade;
		case E_VN_ComponentID_Sprite::Eyes:
			return Eyes_Sprite_Fade;
		case E_VN_ComponentID_Sprite::Eyelids:
			return Eyelids_Sprite_Fade;
		case E_VN_ComponentID_Sprite::Wink:
			return Wink_Sprite_Fade;
		case E_VN_ComponentID_Sprite::Mouth:
			return Mouth_Sprite_Fade;
		case E_VN_ComponentID_Sprite::BodyShadow:
			return BodyShadow_Sprite_Fade;
		case E_VN_ComponentID_Sprite::EmotionHead_01:
			return EmotionHeadEffect01_Sprite_Fade;
		case E_VN_ComponentID_Sprite::EmotionHead_02:
			return EmotionHeadEffect02_Sprite_Fade;
		case E_VN_ComponentID_Sprite::EmotionHead_03:
			return EmotionHeadEffect03_Sprite_Fade;
		case E_VN_ComponentID_Sprite::EmotionBody_01:
			return EmotionBodyEffect01_Sprite_Fade;
		case E_VN_ComponentID_Sprite::EmotionBody_02:
			return EmotionBodyEffect02_Sprite_Fade;
		case E_VN_ComponentID_Sprite::EmotionBody_03:
			return EmotionBodyEffect03_Sprite_Fade;
		// Примечание: Head_Sprite не имеет fade версии, так как используется как контейнер
		default:
			return nullptr;
	}
}

// =====================================================
// ИСПРАВЛЕННАЯ ВАЛИДАЦИЯ И НАСТРОЙКА SPRITE КОМПОНЕНТОВ
// =====================================================

void AVNCharacter::ValidateAndSetupSpriteComponent(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite> Sprite)
{
	if (!Component)
	{
		VN_LOG_WARNING(TEXT("ValidateAndSetupSpriteComponent: Component is null"));
		return;
	}

	// УПРОЩЕННАЯ ВАЛИДАЦИЯ: Если есть спрайт - показываем, если нет - скрываем
	if (!Sprite.IsNull())
	{
		// Загружаем и устанавливаем спрайт
		UPaperSprite* LoadedSprite = Sprite.LoadSynchronous();
		if (LoadedSprite)
		{
			Component->SetSprite(LoadedSprite);
			Component->SetVisibility(true);
			
			// Устанавливаем цвет с учетом фокуса
			FLinearColor TargetColor = GetTargetColorForComponent(Component);
			SetComponentColor(Component, TargetColor);
			
			VN_LOG_DEBUG(TEXT("Sprite component %s: Sprite set and visible"), *Component->GetName());
		}
		else
		{
			VN_LOG_WARNING(TEXT("Failed to load Sprite for component %s"), *Component->GetName());
			Component->SetVisibility(false);
		}
	}
	else
	{
		// Нет спрайта - скрываем компонент
		Component->SetSprite(nullptr);
		Component->SetVisibility(false);
		VN_LOG_DEBUG(TEXT("Sprite component %s: Hidden (no sprite)"), *Component->GetName());
	}
	
	// НЕ применяем трансформации здесь - это делается в ApplyIndividualSpriteTransform
}

// =====================================================
// ИСПРАВЛЕННОЕ ПРИМЕНЕНИЕ ГЛОБАЛЬНЫХ ТРАНСФОРМАЦИЙ
// =====================================================

void AVNCharacter::ApplyGlobalTransforms()
{
	VN_LOG_DEBUG(TEXT("Applying global transforms to individual components"));
	
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
	// Получаем все Skeletal Mesh компоненты (основные + fade)
	TArray<USkeletalMeshComponent*> AllSkeletalMeshes;
	
	// Основные компоненты
	if (Body_Skeletal) AllSkeletalMeshes.Add(Body_Skeletal);
	if (Arms_Skeletal) AllSkeletalMeshes.Add(Arms_Skeletal);
	if (Head_Skeletal) AllSkeletalMeshes.Add(Head_Skeletal);
	if (Custom01_Skeletal) AllSkeletalMeshes.Add(Custom01_Skeletal);
	if (Custom02_Skeletal) AllSkeletalMeshes.Add(Custom02_Skeletal);
	if (Custom03_Skeletal) AllSkeletalMeshes.Add(Custom03_Skeletal);
	
	// Fade компоненты
	if (Body_Skeletal_Fade) AllSkeletalMeshes.Add(Body_Skeletal_Fade);
	if (Arms_Skeletal_Fade) AllSkeletalMeshes.Add(Arms_Skeletal_Fade);
	if (Head_Skeletal_Fade) AllSkeletalMeshes.Add(Head_Skeletal_Fade);
	if (Custom01_Skeletal_Fade) AllSkeletalMeshes.Add(Custom01_Skeletal_Fade);
	if (Custom02_Skeletal_Fade) AllSkeletalMeshes.Add(Custom02_Skeletal_Fade);
	if (Custom03_Skeletal_Fade) AllSkeletalMeshes.Add(Custom03_Skeletal_Fade);
	
	VN_LOG_DEBUG(TEXT("Applying global skeletal transforms to %d components"), AllSkeletalMeshes.Num());
	
	for (USkeletalMeshComponent* SkeletalMesh : AllSkeletalMeshes)
	{
		if (!SkeletalMesh) continue;
		
		// Создаем трансформ с глобальными настройками
		FTransform ComponentTransform = FTransform::Identity;
		ComponentTransform.SetLocation(GlobalSkeletalOffset);
		ComponentTransform.SetScale3D(FVector(GlobalSkeletalScale));
		
		// Применяем к компоненту
		SkeletalMesh->SetRelativeTransform(ComponentTransform);
		
		VN_LOG_DEBUG(TEXT("Applied global skeletal transform to %s: Offset=%s, Scale=%.2f"), 
			*SkeletalMesh->GetName(), *GlobalSkeletalOffset.ToString(), GlobalSkeletalScale);
	}
}

void AVNCharacter::ApplyGlobalSpriteTransforms()
{
	// Получаем все спрайт компоненты (основные + fade)
	TArray<UPaperSpriteComponent*> AllSprites;
	
	// Основные спрайты тела (прикреплены к GlobalSpriteTransform)
	if (Body_Sprite) AllSprites.Add(Body_Sprite);
	if (Arms_Sprite) AllSprites.Add(Arms_Sprite);
	if (BodyShadow_Sprite) AllSprites.Add(BodyShadow_Sprite);
	if (EmotionBodyEffect01_Sprite) AllSprites.Add(EmotionBodyEffect01_Sprite);
	if (EmotionBodyEffect02_Sprite) AllSprites.Add(EmotionBodyEffect02_Sprite);
	if (EmotionBodyEffect03_Sprite) AllSprites.Add(EmotionBodyEffect03_Sprite);
	
	// Fade спрайты тела
	if (Body_Sprite_Fade) AllSprites.Add(Body_Sprite_Fade);
	if (Arms_Sprite_Fade) AllSprites.Add(Arms_Sprite_Fade);
	if (BodyShadow_Sprite_Fade) AllSprites.Add(BodyShadow_Sprite_Fade);
	if (EmotionBodyEffect01_Sprite_Fade) AllSprites.Add(EmotionBodyEffect01_Sprite_Fade);
	if (EmotionBodyEffect02_Sprite_Fade) AllSprites.Add(EmotionBodyEffect02_Sprite_Fade);
	if (EmotionBodyEffect03_Sprite_Fade) AllSprites.Add(EmotionBodyEffect03_Sprite_Fade);
	
	// Head_Sprite (получает глобальные трансформации)
	if (Head_Sprite) AllSprites.Add(Head_Sprite);
	
	VN_LOG_DEBUG(TEXT("Applying global sprite transforms to %d components"), AllSprites.Num());
	
	for (UPaperSpriteComponent* Sprite : AllSprites)
	{
		if (!Sprite) continue;
		
		// Создаем трансформ с глобальными настройками
		FTransform ComponentTransform = FTransform::Identity;
		ComponentTransform.SetLocation(GlobalSpriteOffset);
		ComponentTransform.SetScale3D(FVector(GlobalSpriteScale));
		
		// Применяем к компоненту
		Sprite->SetRelativeTransform(ComponentTransform);
		
		VN_LOG_DEBUG(TEXT("Applied global sprite transform to %s: Offset=%s, Scale=%.2f"), 
			*Sprite->GetName(), *GlobalSpriteOffset.ToString(), GlobalSpriteScale);
	}
	
	// ВАЖНО: Дочерние спрайты головы НЕ получают глобальные трансформации напрямую
	// Они наследуют их через Head_Sprite, поэтому оставляем их трансформ как Identity
	ApplyChildHeadSpriteTransforms();
}

void AVNCharacter::ApplyChildHeadSpriteTransforms()
{
	// Дочерние спрайты головы - оставляем их трансформы как Identity
	// Они получат глобальные трансформации через наследование от Head_Sprite
	TArray<UPaperSpriteComponent*> ChildHeadSprites;
	
	// Основные дочерние спрайты
	if (Eyebrow_Sprite) ChildHeadSprites.Add(Eyebrow_Sprite);
	if (Eyes_Sprite) ChildHeadSprites.Add(Eyes_Sprite);
	if (Eyelids_Sprite) ChildHeadSprites.Add(Eyelids_Sprite);
	if (Wink_Sprite) ChildHeadSprites.Add(Wink_Sprite);
	if (Mouth_Sprite) ChildHeadSprites.Add(Mouth_Sprite);
	if (EmotionHeadEffect01_Sprite) ChildHeadSprites.Add(EmotionHeadEffect01_Sprite);
	if (EmotionHeadEffect02_Sprite) ChildHeadSprites.Add(EmotionHeadEffect02_Sprite);
	if (EmotionHeadEffect03_Sprite) ChildHeadSprites.Add(EmotionHeadEffect03_Sprite);
	
	// Fade дочерние спрайты
	if (Eyebrow_Sprite_Fade) ChildHeadSprites.Add(Eyebrow_Sprite_Fade);
	if (Eyes_Sprite_Fade) ChildHeadSprites.Add(Eyes_Sprite_Fade);
	if (Eyelids_Sprite_Fade) ChildHeadSprites.Add(Eyelids_Sprite_Fade);
	if (Wink_Sprite_Fade) ChildHeadSprites.Add(Wink_Sprite_Fade);
	if (Mouth_Sprite_Fade) ChildHeadSprites.Add(Mouth_Sprite_Fade);
	if (EmotionHeadEffect01_Sprite_Fade) ChildHeadSprites.Add(EmotionHeadEffect01_Sprite_Fade);
	if (EmotionHeadEffect02_Sprite_Fade) ChildHeadSprites.Add(EmotionHeadEffect02_Sprite_Fade);
	if (EmotionHeadEffect03_Sprite_Fade) ChildHeadSprites.Add(EmotionHeadEffect03_Sprite_Fade);
	
	VN_LOG_DEBUG(TEXT("Setting identity transforms for %d child head sprites"), ChildHeadSprites.Num());
	
	for (UPaperSpriteComponent* ChildSprite : ChildHeadSprites)
	{
		if (!ChildSprite) continue;
		
		// Устанавливаем Identity трансформ - они получат глобальные настройки через Head_Sprite
		FTransform IdentityTransform = FTransform::Identity;
		ChildSprite->SetRelativeTransform(IdentityTransform);
		
		VN_LOG_DEBUG(TEXT("Set identity transform for child sprite: %s"), *ChildSprite->GetName());
	}
}

void AVNCharacter::ApplyIndividualSpriteTransform(UPaperSpriteComponent* SpriteComponent, E_VN_ComponentID_Sprite ComponentID)
{
	if (!SpriteComponent)
	{
		return;
	}

	// Дочерние элементы Head_Sprite получают только Identity трансформ
	// Глобальные трансформации они наследуют через Head_Sprite
	if (IsChildOfHeadSprite(ComponentID))
	{
		FTransform IdentityTransform = FTransform::Identity;
		SpriteComponent->SetRelativeTransform(IdentityTransform);
		VN_LOG_DEBUG(TEXT("Applied identity transform to child sprite %s"), *SpriteComponent->GetName());
		return;
	}

	// Остальные спрайты получают глобальные трансформации
	FTransform ComponentTransform = FTransform::Identity;
	ComponentTransform.SetLocation(GlobalSpriteOffset);
	ComponentTransform.SetScale3D(FVector(GlobalSpriteScale));
	SpriteComponent->SetRelativeTransform(ComponentTransform);
	
	VN_LOG_DEBUG(TEXT("Applied global sprite transform to %s: Offset=%s, Scale=%.2f"), 
		*SpriteComponent->GetName(), *GlobalSpriteOffset.ToString(), GlobalSpriteScale);
}

bool AVNCharacter::IsChildOfHeadSprite(E_VN_ComponentID_Sprite ComponentID) const
{
	// Проверяем, является ли спрайт одним из лицевых элементов
	switch (ComponentID)
	{
		case E_VN_ComponentID_Sprite::Eyebrow:
		case E_VN_ComponentID_Sprite::Eyes:
		case E_VN_ComponentID_Sprite::Eyelids:
		case E_VN_ComponentID_Sprite::Wink:
		case E_VN_ComponentID_Sprite::Mouth:
		case E_VN_ComponentID_Sprite::EmotionHead_01:
		case E_VN_ComponentID_Sprite::EmotionHead_02:
		case E_VN_ComponentID_Sprite::EmotionHead_03:
			return true;
		default:
			return false;
	}
}

// =====================================================
// ИСПРАВЛЕННАЯ ПОДГОТОВКА SPRITE ПЕРЕХОДА
// =====================================================

void AVNCharacter::PrepareSpriteTransition(UPaperSpriteComponent* MainComponent, UPaperSpriteComponent* FadeComponent, TSoftObjectPtr<UPaperSprite> NewSprite)
{
	if (!MainComponent || !FadeComponent)
	{
		VN_LOG_WARNING(TEXT("PrepareSpriteTransition: Invalid components"));
		return;
	}

	VN_LOG_DEBUG(TEXT("PrepareSpriteTransition: %s -> new sprite"), *MainComponent->GetName());

	// Копируем текущие настройки главного компонента в fade компонент
	CopySpriteComponentSettings(MainComponent, FadeComponent);
	
	// Fade компонент будет исчезать (alpha от 1 до 0)
	SetComponentAlpha(FadeComponent, 1.0f);
	FadeComponent->SetVisibility(true);

	// Настраиваем главный компонент с новым спрайтом
	ValidateAndSetupSpriteComponent(MainComponent, NewSprite);
	
	// Определяем ComponentID для применения правильных трансформаций
	E_VN_ComponentID_Sprite ComponentID = E_VN_ComponentID_Sprite::Body; // Значение по умолчанию
	
	// Находим ComponentID по указателю на компонент
	if (MainComponent == Eyes_Sprite) ComponentID = E_VN_ComponentID_Sprite::Eyes;
	else if (MainComponent == Mouth_Sprite) ComponentID = E_VN_ComponentID_Sprite::Mouth;
	else if (MainComponent == Eyebrow_Sprite) ComponentID = E_VN_ComponentID_Sprite::Eyebrow;
	else if (MainComponent == Body_Sprite) ComponentID = E_VN_ComponentID_Sprite::Body;
	else if (MainComponent == Arms_Sprite) ComponentID = E_VN_ComponentID_Sprite::Arms;
	// ... добавить остальные компоненты при необходимости
	
	// Применяем правильные трансформации
	ApplyIndividualSpriteTransform(MainComponent, ComponentID);
	
	// Главный компонент будет появляться (alpha от 0 до 1)
	SetComponentAlpha(MainComponent, 0.0f);
}

// =====================================================
// СИСТЕМА ФОКУСА
// =====================================================

void AVNCharacter::SetFocus(bool bInFocus, float Duration)
{
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
		// Применяем цвета мгновенно ко всем основным компонентам
		TArray<USceneComponent*> AllComponents = GetAllMainComponents();
		for (USceneComponent* Component : AllComponents)
		{
			if (Component && Component->IsVisible())
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
			// Показываем все основные компоненты с правильными цветами
			TArray<USceneComponent*> AllComponents = GetAllMainComponents();
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

	// Получаем базовый цвет (пока всегда белый, так как нет системы состояний)
	FLinearColor BaseColor = FLinearColor::White;

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
	// Пока всегда возвращаем белый, так как система состояний убрана
	return FLinearColor::White;
}

// =====================================================
// ВНУТРЕННИЕ МЕТОДЫ - ВАЛИДАЦИЯ И НАСТРОЙКА
// =====================================================

void AVNCharacter::ValidateAndSetupSkeletalComponent(USkeletalMeshComponent* Component, TSoftObjectPtr<USkeletalMesh> SkeletalMesh)
{
	if (!Component)
	{
		VN_LOG_WARNING(TEXT("ValidateAndSetupSkeletalComponent: Component is null"));
		return;
	}

	// УПРОЩЕННАЯ ВАЛИДАЦИЯ: Если есть меш - показываем, если нет - скрываем
	if (!SkeletalMesh.IsNull())
	{
		// Загружаем и устанавливаем меш
		USkeletalMesh* LoadedMesh = SkeletalMesh.LoadSynchronous();
		if (LoadedMesh)
		{
			Component->SetSkeletalMesh(LoadedMesh);
			Component->SetVisibility(true);
			
			// Устанавливаем цвет с учетом фокуса
			FLinearColor TargetColor = GetTargetColorForComponent(Component);
			SetComponentColor(Component, TargetColor);
			
			VN_LOG_DEBUG(TEXT("Skeletal component %s: Mesh set and visible"), *Component->GetName());
		}
		else
		{
			VN_LOG_WARNING(TEXT("Failed to load SkeletalMesh for component %s"), *Component->GetName());
			Component->SetVisibility(false);
		}
	}
	else
	{
		// Нет меша - скрываем компонент
		Component->SetSkeletalMesh(nullptr);
		Component->SetVisibility(false);
		VN_LOG_DEBUG(TEXT("Skeletal component %s: Hidden (no mesh)"), *Component->GetName());
	}
}

void AVNCharacter::CopySkeletalComponentSettings(USkeletalMeshComponent* Source, USkeletalMeshComponent* Target)
{
	if (!Source || !Target)
	{
		return;
	}

	// Копируем основные настройки
	Target->SetSkeletalMesh(Source->GetSkeletalMeshAsset());
	Target->SetAnimInstanceClass(Source->GetAnimClass());
	Target->SetRelativeTransform(Source->GetRelativeTransform());
	Target->SetVisibility(Source->IsVisible());

	// Копируем материалы
	for (int32 i = 0; i < Source->GetNumMaterials(); ++i)
	{
		Target->SetMaterial(i, Source->GetMaterial(i));
	}

	VN_LOG_DEBUG(TEXT("Copied settings from %s to %s"), *Source->GetName(), *Target->GetName());
}

void AVNCharacter::CopySpriteComponentSettings(UPaperSpriteComponent* Source, UPaperSpriteComponent* Target)
{
	if (!Source || !Target)
	{
		return;
	}

	// Копируем основные настройки
	Target->SetSprite(Source->GetSprite());
	Target->SetSpriteColor(Source->GetSpriteColor());
	Target->SetRelativeTransform(Source->GetRelativeTransform());
	Target->SetVisibility(Source->IsVisible());

	VN_LOG_DEBUG(TEXT("Copied settings from %s to %s"), *Source->GetName(), *Target->GetName());
}

void AVNCharacter::PrepareSkeletalTransition(USkeletalMeshComponent* MainComponent, USkeletalMeshComponent* FadeComponent, TSoftObjectPtr<USkeletalMesh> NewMesh)
{
	if (!MainComponent || !FadeComponent)
	{
		VN_LOG_WARNING(TEXT("PrepareSkeletalTransition: Invalid components"));
		return;
	}

	VN_LOG_DEBUG(TEXT("PrepareSkeletalTransition: %s -> new mesh"), *MainComponent->GetName());

	// Копируем текущие настройки главного компонента в fade компонент
	CopySkeletalComponentSettings(MainComponent, FadeComponent);
	
	// Fade компонент будет исчезать (alpha от 1 до 0)
	SetComponentAlpha(FadeComponent, 1.0f);
	FadeComponent->SetVisibility(true);

	// Настраиваем главный компонент с новым мешем
	ValidateAndSetupSkeletalComponent(MainComponent, NewMesh);
	
	// Главный компонент будет появляться (alpha от 0 до 1)
	SetComponentAlpha(MainComponent, 0.0f);
}

void AVNCharacter::FinishTransition(USceneComponent* MainComponent, USceneComponent* FadeComponent)
{
	if (!MainComponent || !FadeComponent)
	{
		return;
	}

	VN_LOG_DEBUG(TEXT("FinishTransition: %s"), *MainComponent->GetName());

	// Устанавливаем полную непрозрачность для главного компонента
	SetComponentAlpha(MainComponent, 1.0f);

	// Скрываем и очищаем fade компонент
	FadeComponent->SetVisibility(false);
	SetComponentAlpha(FadeComponent, 0.0f);

	// Очищаем содержимое fade компонента
	if (USkeletalMeshComponent* SkeletalFade = Cast<USkeletalMeshComponent>(FadeComponent))
	{
		SkeletalFade->SetSkeletalMesh(nullptr);
	}
	else if (UPaperSpriteComponent* SpriteFade = Cast<UPaperSpriteComponent>(FadeComponent))
	{
		SpriteFade->SetSprite(nullptr);
	}

	VN_LOG_DEBUG(TEXT("Transition finished for %s"), *MainComponent->GetName());
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

TArray<USceneComponent*> AVNCharacter::GetAllMainComponents() const
{
	TArray<USceneComponent*> Components;
	
	// Добавляем все основные Skeletal Mesh компоненты
	if (Body_Skeletal) Components.Add(Body_Skeletal);
	if (Arms_Skeletal) Components.Add(Arms_Skeletal);
	if (Head_Skeletal) Components.Add(Head_Skeletal);
	if (Custom01_Skeletal) Components.Add(Custom01_Skeletal);
	if (Custom02_Skeletal) Components.Add(Custom02_Skeletal);
	if (Custom03_Skeletal) Components.Add(Custom03_Skeletal);
	
	// Добавляем все основные Sprite компоненты
	if (Body_Sprite) Components.Add(Body_Sprite);
	if (Arms_Sprite) Components.Add(Arms_Sprite);
	if (Head_Sprite) Components.Add(Head_Sprite);
	if (Eyebrow_Sprite) Components.Add(Eyebrow_Sprite);
	if (Eyes_Sprite) Components.Add(Eyes_Sprite);
	if (Eyelids_Sprite) Components.Add(Eyelids_Sprite);
	if (Wink_Sprite) Components.Add(Wink_Sprite);
	if (Mouth_Sprite) Components.Add(Mouth_Sprite);
	if (BodyShadow_Sprite) Components.Add(BodyShadow_Sprite);
	if (EmotionHeadEffect01_Sprite) Components.Add(EmotionHeadEffect01_Sprite);
	if (EmotionHeadEffect02_Sprite) Components.Add(EmotionHeadEffect02_Sprite);
	if (EmotionHeadEffect03_Sprite) Components.Add(EmotionHeadEffect03_Sprite);
	if (EmotionBodyEffect01_Sprite) Components.Add(EmotionBodyEffect01_Sprite);
	if (EmotionBodyEffect02_Sprite) Components.Add(EmotionBodyEffect02_Sprite);
	if (EmotionBodyEffect03_Sprite) Components.Add(EmotionBodyEffect03_Sprite);
	
	return Components;
}

TArray<USceneComponent*> AVNCharacter::GetAllFadeComponents() const
{
	TArray<USceneComponent*> Components;
	
	// Добавляем все Fade Skeletal Mesh компоненты
	if (Body_Skeletal_Fade) Components.Add(Body_Skeletal_Fade);
	if (Arms_Skeletal_Fade) Components.Add(Arms_Skeletal_Fade);
	if (Head_Skeletal_Fade) Components.Add(Head_Skeletal_Fade);
	if (Custom01_Skeletal_Fade) Components.Add(Custom01_Skeletal_Fade);
	if (Custom02_Skeletal_Fade) Components.Add(Custom02_Skeletal_Fade);
	if (Custom03_Skeletal_Fade) Components.Add(Custom03_Skeletal_Fade);
	
	// Добавляем все Fade Sprite компоненты
	if (Body_Sprite_Fade) Components.Add(Body_Sprite_Fade);
	if (Arms_Sprite_Fade) Components.Add(Arms_Sprite_Fade);
	if (Eyebrow_Sprite_Fade) Components.Add(Eyebrow_Sprite_Fade);
	if (Eyes_Sprite_Fade) Components.Add(Eyes_Sprite_Fade);
	if (Eyelids_Sprite_Fade) Components.Add(Eyelids_Sprite_Fade);
	if (Wink_Sprite_Fade) Components.Add(Wink_Sprite_Fade);
	if (Mouth_Sprite_Fade) Components.Add(Mouth_Sprite_Fade);
	if (BodyShadow_Sprite_Fade) Components.Add(BodyShadow_Sprite_Fade);
	if (EmotionHeadEffect01_Sprite_Fade) Components.Add(EmotionHeadEffect01_Sprite_Fade);
	if (EmotionHeadEffect02_Sprite_Fade) Components.Add(EmotionHeadEffect02_Sprite_Fade);
	if (EmotionHeadEffect03_Sprite_Fade) Components.Add(EmotionHeadEffect03_Sprite_Fade);
	if (EmotionBodyEffect01_Sprite_Fade) Components.Add(EmotionBodyEffect01_Sprite_Fade);
	if (EmotionBodyEffect02_Sprite_Fade) Components.Add(EmotionBodyEffect02_Sprite_Fade);
	if (EmotionBodyEffect03_Sprite_Fade) Components.Add(EmotionBodyEffect03_Sprite_Fade);
	
	return Components;
}

void AVNCharacter::HideAllFadeComponents()
{
	TArray<USceneComponent*> FadeComponents = GetAllFadeComponents();
	for (USceneComponent* Component : FadeComponents)
	{
		if (Component)
		{
			Component->SetVisibility(false);
			SetComponentAlpha(Component, 0.0f);
			
			// Очищаем содержимое fade компонентов
			if (USkeletalMeshComponent* SkeletalFade = Cast<USkeletalMeshComponent>(Component))
			{
				SkeletalFade->SetSkeletalMesh(nullptr);
			}
			else if (UPaperSpriteComponent* SpriteFade = Cast<UPaperSpriteComponent>(Component))
			{
				SpriteFade->SetSprite(nullptr);
			}
		}
	}
	
	VN_LOG_DEBUG(TEXT("All fade components hidden and cleared"));
}

// =====================================================
// ОБРАБОТЧИКИ СОБЫТИЙ АНИМАЦИИ
// =====================================================

void AVNCharacter::OnAnimationStarted(EVNAnimationType AnimationType)
{
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
}

void AVNCharacter::OnAnimationFinished(EVNAnimationType AnimationType)
{
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
		{
			// ⚠️ ИСПРАВЛЕНИЕ: Добавляем фигурные скобки для создания локального scope
			// Завершаем все активные переходы
			HideAllFadeComponents();
			// Убеждаемся, что все основные компоненты имеют полную непрозрачность
			TArray<USceneComponent*> MainComponents = GetAllMainComponents();
			for (USceneComponent* Component : MainComponents)
			{
				if (Component && Component->IsVisible())
				{
					SetComponentAlpha(Component, 1.0f);
				}
			}
			break;
		}
	case EVNAnimationType::SpawnDespawn:
		OnCharacterVisibilityChanged.Broadcast(IsVisible());
		break;
			
	case EVNAnimationType::Focus:
		OnCharacterFocusChanged.Broadcast(bIsInFocus);
		break;

	default:
		break;
	}
}

void AVNCharacter::OnAnimationProgress(EVNAnimationType AnimationType, float Progress)
{
	// Анимация обрабатывается в AnimationManager
	// Здесь можем добавить дополнительную логику если нужно
}

// =====================================================
// ОТЛАДОЧНЫЕ МЕТОДЫ
// =====================================================

#if WITH_EDITOR
void AVNCharacter::PrintDebugInfo()
{
	FString DebugInfo = TEXT("=== VN Character Debug Info ===\n");
	
	DebugInfo += FString::Printf(TEXT("Actor: %s\n"), *GetName());
	DebugInfo += FString::Printf(TEXT("Character Name: %s\n"), *CharacterName);
	DebugInfo += FString::Printf(TEXT("Is In Focus: %s\n"), bIsInFocus ? TEXT("Yes") : TEXT("No"));
	DebugInfo += FString::Printf(TEXT("Is Visible: %s\n"), IsVisible() ? TEXT("Yes") : TEXT("No"));
	DebugInfo += FString::Printf(TEXT("Is Animating: %s\n"), IsAnimating() ? TEXT("Yes") : TEXT("No"));
	
	if (AnimationManager)
	{
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
	TArray<USceneComponent*> MainComponents = GetAllMainComponents();
	TArray<USceneComponent*> FadeComponents = GetAllFadeComponents();
	
	int32 VisibleMainCount = 0;
	int32 VisibleFadeCount = 0;
	
	for (USceneComponent* Component : MainComponents)
	{
		if (Component && Component->IsVisible())
		{
			VisibleMainCount++;
		}
	}
	
	for (USceneComponent* Component : FadeComponents)
	{
		if (Component && Component->IsVisible())
		{
			VisibleFadeCount++;
		}
	}
	
	DebugInfo += FString::Printf(TEXT("Main Components: %d total, %d visible\n"), MainComponents.Num(), VisibleMainCount);
	DebugInfo += FString::Printf(TEXT("Fade Components: %d total, %d visible\n"), FadeComponents.Num(), VisibleFadeCount);
	
	VN_LOG(Log, TEXT("%s"), *DebugInfo);
}

void AVNCharacter::ValidateAllComponents()
{
	TArray<FString> ValidationErrors;
	
	// Проверяем основные компоненты
	TArray<USceneComponent*> MainComponents = GetAllMainComponents();
	for (USceneComponent* Component : MainComponents)
	{
		if (!Component)
		{
			ValidationErrors.Add(TEXT("Main component is null"));
			continue;
		}
		
		// Для Skeletal Mesh компонентов
		if (USkeletalMeshComponent* SkeletalComp = Cast<USkeletalMeshComponent>(Component))
		{
			if (SkeletalComp->IsVisible() && !SkeletalComp->GetSkeletalMeshAsset())
			{
				ValidationErrors.Add(FString::Printf(TEXT("Skeletal component %s is visible but has no mesh"), 
					*SkeletalComp->GetName()));
			}
		}
		// Для Sprite компонентов
		else if (UPaperSpriteComponent* SpriteComp = Cast<UPaperSpriteComponent>(Component))
		{
			if (SpriteComp->IsVisible() && !SpriteComp->GetSprite())
			{
				ValidationErrors.Add(FString::Printf(TEXT("Sprite component %s is visible but has no sprite"), 
					*SpriteComp->GetName()));
			}
		}
	}
	
	// Проверяем fade компоненты (должны быть скрыты когда не используются)
	TArray<USceneComponent*> FadeComponents = GetAllFadeComponents();
	for (USceneComponent* Component : FadeComponents)
	{
		if (!Component)
		{
			ValidationErrors.Add(TEXT("Fade component is null"));
			continue;
		}
		
		if (Component->IsVisible() && !IsAnimating())
		{
			ValidationErrors.Add(FString::Printf(TEXT("Fade component %s is visible but no animation is running"), 
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