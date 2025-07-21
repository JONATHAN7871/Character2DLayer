#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Components/SceneComponent.h"

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

// =====================================================
// МЕТОДЫ ДЛЯ ПОЛУЧЕНИЯ КОМПОНЕНТОВ
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