#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "Components/VNCharacterIdleAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/UnrealType.h"

void AVNCharacter::CreateComponents()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	AnimationManager = CreateDefaultSubobject<UVNCharacterAnimationManager>(TEXT("AnimationManager"));
	IdleAnimationManager = CreateDefaultSubobject<UVNCharacterIdleAnimationManager>(TEXT("IdleAnimationManager"));
	CharacterRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CharacterRoot"));
	
	Body_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body_Skeletal"));
	Arms_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Arms_Skeletal"));
	Head_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Head_Skeletal"));
	Custom01_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Custom01_Skeletal"));
	Custom02_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Custom02_Skeletal"));
	Custom03_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Custom03_Skeletal"));
	Body_Skeletal_Fade = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body_Skeletal_Fade"));
	Arms_Skeletal_Fade = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Arms_Skeletal_Fade"));
	Head_Skeletal_Fade = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Head_Skeletal_Fade"));
	Custom01_Skeletal_Fade = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Custom01_Skeletal_Fade"));
	Custom02_Skeletal_Fade = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Custom02_Skeletal_Fade"));
	Custom03_Skeletal_Fade = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Custom03_Skeletal_Fade"));

	Body_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Body_Sprite"));
	Arms_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Arms_Sprite"));
	BodyShadow_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("BodyShadow_Sprite"));
	EmotionBodyEffect01_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionBodyEffect01_Sprite"));
	EmotionBodyEffect02_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionBodyEffect02_Sprite"));
	EmotionBodyEffect03_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionBodyEffect03_Sprite"));
	Body_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Body_Sprite_Fade"));
	Arms_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Arms_Sprite_Fade"));
	BodyShadow_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("BodyShadow_Sprite_Fade"));
	EmotionBodyEffect01_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionBodyEffect01_Sprite_Fade"));
	EmotionBodyEffect02_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionBodyEffect02_Sprite_Fade"));
	EmotionBodyEffect03_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionBodyEffect03_Sprite_Fade"));
	Head_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Head_Sprite"));
	Eyebrow_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Eyebrow_Sprite"));
	Eyes_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Eyes_Sprite"));
	Eyelids_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Eyelids_Sprite"));
	Wink_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Wink_Sprite"));
	Mouth_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Mouth_Sprite"));
	EmotionHeadEffect01_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionHeadEffect01_Sprite"));
	EmotionHeadEffect02_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionHeadEffect02_Sprite"));
	EmotionHeadEffect03_Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionHeadEffect03_Sprite"));
	Head_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Head_Sprite_Fade"));
	Eyebrow_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Eyebrow_Sprite_Fade"));
	Eyes_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Eyes_Sprite_Fade"));
	Eyelids_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Eyelids_Sprite_Fade"));
	Wink_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Wink_Sprite_Fade"));
	Mouth_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Mouth_Sprite_Fade"));
	EmotionHeadEffect01_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionHeadEffect01_Sprite_Fade"));
	EmotionHeadEffect02_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionHeadEffect02_Sprite_Fade"));
	EmotionHeadEffect03_Sprite_Fade = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EmotionHeadEffect03_Sprite_Fade"));
}

void AVNCharacter::SetupComponentHierarchy()
{
	CharacterRoot->SetupAttachment(RootComponent);

	Body_Skeletal->SetupAttachment(CharacterRoot);
	Arms_Skeletal->SetupAttachment(CharacterRoot);
	Head_Skeletal->SetupAttachment(CharacterRoot);
	Custom01_Skeletal->SetupAttachment(CharacterRoot);
	Custom02_Skeletal->SetupAttachment(CharacterRoot);
	Custom03_Skeletal->SetupAttachment(CharacterRoot);
	Body_Skeletal_Fade->SetupAttachment(CharacterRoot);
	Arms_Skeletal_Fade->SetupAttachment(CharacterRoot);
	Head_Skeletal_Fade->SetupAttachment(CharacterRoot);
	Custom01_Skeletal_Fade->SetupAttachment(CharacterRoot);
	Custom02_Skeletal_Fade->SetupAttachment(CharacterRoot);
	Custom03_Skeletal_Fade->SetupAttachment(CharacterRoot);

	Body_Sprite->SetupAttachment(CharacterRoot);
	Arms_Sprite->SetupAttachment(CharacterRoot);
	BodyShadow_Sprite->SetupAttachment(CharacterRoot);
	EmotionBodyEffect01_Sprite->SetupAttachment(CharacterRoot);
	EmotionBodyEffect02_Sprite->SetupAttachment(CharacterRoot);
	EmotionBodyEffect03_Sprite->SetupAttachment(CharacterRoot);
	Body_Sprite_Fade->SetupAttachment(CharacterRoot);
	Arms_Sprite_Fade->SetupAttachment(CharacterRoot);
	BodyShadow_Sprite_Fade->SetupAttachment(CharacterRoot);
	EmotionBodyEffect01_Sprite_Fade->SetupAttachment(CharacterRoot);
	EmotionBodyEffect02_Sprite_Fade->SetupAttachment(CharacterRoot);
	EmotionBodyEffect03_Sprite_Fade->SetupAttachment(CharacterRoot);
	
	Head_Sprite->SetupAttachment(CharacterRoot);
	Head_Sprite_Fade->SetupAttachment(CharacterRoot);
	
	Eyebrow_Sprite->SetupAttachment(Head_Sprite);
	Eyes_Sprite->SetupAttachment(Head_Sprite);
	Eyelids_Sprite->SetupAttachment(Head_Sprite);
	Wink_Sprite->SetupAttachment(Head_Sprite);
	Mouth_Sprite->SetupAttachment(Head_Sprite);
	EmotionHeadEffect01_Sprite->SetupAttachment(Head_Sprite);
	EmotionHeadEffect02_Sprite->SetupAttachment(Head_Sprite);
	EmotionHeadEffect03_Sprite->SetupAttachment(Head_Sprite);
	Eyebrow_Sprite_Fade->SetupAttachment(Head_Sprite);
	Eyes_Sprite_Fade->SetupAttachment(Head_Sprite);
	Eyelids_Sprite_Fade->SetupAttachment(Head_Sprite);
	Wink_Sprite_Fade->SetupAttachment(Head_Sprite);
	Mouth_Sprite_Fade->SetupAttachment(Head_Sprite);
	EmotionHeadEffect01_Sprite_Fade->SetupAttachment(Head_Sprite);
	EmotionHeadEffect02_Sprite_Fade->SetupAttachment(Head_Sprite);
	EmotionHeadEffect03_Sprite_Fade->SetupAttachment(Head_Sprite);
}

void AVNCharacter::ResetComponentAttachmentToDefault(USceneComponent* ComponentToReset)
{
	if (!ComponentToReset) return;
	USceneComponent* DefaultParent = IsChildOfHeadSprite(ComponentToReset) ? (USceneComponent*)Head_Sprite : (USceneComponent*)CharacterRoot;
	if (DefaultParent)
	{
		ComponentToReset->AttachToComponent(DefaultParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
}

USkeletalMeshComponent* AVNCharacter::GetSkeletalComponent(E_VN_ComponentID_Skeletal ComponentID) const
{
	switch (ComponentID){
	case E_VN_ComponentID_Skeletal::Body: return Body_Skeletal;
	case E_VN_ComponentID_Skeletal::Arms: return Arms_Skeletal;
	case E_VN_ComponentID_Skeletal::Head: return Head_Skeletal;
	case E_VN_ComponentID_Skeletal::Custom01: return Custom01_Skeletal;
	case E_VN_ComponentID_Skeletal::Custom02: return Custom02_Skeletal;
	case E_VN_ComponentID_Skeletal::Custom03: return Custom03_Skeletal;
	default: return nullptr;
	}
}
UPaperSpriteComponent* AVNCharacter::GetSpriteComponent(E_VN_ComponentID_Sprite ComponentID) const
{
	switch (ComponentID){
	case E_VN_ComponentID_Sprite::Body: return Body_Sprite;
	case E_VN_ComponentID_Sprite::Arms: return Arms_Sprite;
	case E_VN_ComponentID_Sprite::Head: return Head_Sprite;
	case E_VN_ComponentID_Sprite::Eyebrow: return Eyebrow_Sprite;
	case E_VN_ComponentID_Sprite::Eyes: return Eyes_Sprite;
	case E_VN_ComponentID_Sprite::Eyelids: return Eyelids_Sprite;
	case E_VN_ComponentID_Sprite::Wink: return Wink_Sprite;
	case E_VN_ComponentID_Sprite::Mouth: return Mouth_Sprite;
	case E_VN_ComponentID_Sprite::BodyShadow: return BodyShadow_Sprite;
	case E_VN_ComponentID_Sprite::EmotionHead_01: return EmotionHeadEffect01_Sprite;
	case E_VN_ComponentID_Sprite::EmotionHead_02: return EmotionHeadEffect02_Sprite;
	case E_VN_ComponentID_Sprite::EmotionHead_03: return EmotionHeadEffect03_Sprite;
	case E_VN_ComponentID_Sprite::EmotionBody_01: return EmotionBodyEffect01_Sprite;
	case E_VN_ComponentID_Sprite::EmotionBody_02: return EmotionBodyEffect02_Sprite;
	case E_VN_ComponentID_Sprite::EmotionBody_03: return EmotionBodyEffect03_Sprite;
	default: return nullptr;
	}
}
USkeletalMeshComponent* AVNCharacter::GetSkeletalFadeComponent(E_VN_ComponentID_Skeletal ComponentID) const
{
	switch (ComponentID){
	case E_VN_ComponentID_Skeletal::Body: return Body_Skeletal_Fade;
	case E_VN_ComponentID_Skeletal::Arms: return Arms_Skeletal_Fade;
	case E_VN_ComponentID_Skeletal::Head: return Head_Skeletal_Fade;
	case E_VN_ComponentID_Skeletal::Custom01: return Custom01_Skeletal_Fade;
	case E_VN_ComponentID_Skeletal::Custom02: return Custom02_Skeletal_Fade;
	case E_VN_ComponentID_Skeletal::Custom03: return Custom03_Skeletal_Fade;
	default: return nullptr;
	}
}
UPaperSpriteComponent* AVNCharacter::GetSpriteFadeComponent(E_VN_ComponentID_Sprite ComponentID) const
{
	switch (ComponentID){
	case E_VN_ComponentID_Sprite::Head: return Head_Sprite_Fade;
	case E_VN_ComponentID_Sprite::Body: return Body_Sprite_Fade;
	case E_VN_ComponentID_Sprite::Arms: return Arms_Sprite_Fade;
	case E_VN_ComponentID_Sprite::Eyebrow: return Eyebrow_Sprite_Fade;
	case E_VN_ComponentID_Sprite::Eyes: return Eyes_Sprite_Fade;
	case E_VN_ComponentID_Sprite::Eyelids: return Eyelids_Sprite_Fade;
	case E_VN_ComponentID_Sprite::Wink: return Wink_Sprite_Fade;
	case E_VN_ComponentID_Sprite::Mouth: return Mouth_Sprite_Fade;
	case E_VN_ComponentID_Sprite::BodyShadow: return BodyShadow_Sprite_Fade;
	case E_VN_ComponentID_Sprite::EmotionHead_01: return EmotionHeadEffect01_Sprite_Fade;
	case E_VN_ComponentID_Sprite::EmotionHead_02: return EmotionHeadEffect02_Sprite_Fade;
	case E_VN_ComponentID_Sprite::EmotionHead_03: return EmotionHeadEffect03_Sprite_Fade;
	case E_VN_ComponentID_Sprite::EmotionBody_01: return EmotionBodyEffect01_Sprite_Fade;
	case E_VN_ComponentID_Sprite::EmotionBody_02: return EmotionBodyEffect02_Sprite_Fade;
	case E_VN_ComponentID_Sprite::EmotionBody_03: return EmotionBodyEffect03_Sprite_Fade;
	default: return nullptr;
	}
}

TArray<USceneComponent*> AVNCharacter::GetAllMainComponents() const
{
	TArray<USceneComponent*> Components;
	
	if (Body_Skeletal) Components.Add(Body_Skeletal);
	if (Arms_Skeletal) Components.Add(Arms_Skeletal);
	if (Head_Skeletal) Components.Add(Head_Skeletal);
	if (Custom01_Skeletal) Components.Add(Custom01_Skeletal);
	if (Custom02_Skeletal) Components.Add(Custom02_Skeletal);
	if (Custom03_Skeletal) Components.Add(Custom03_Skeletal);
	
	if (Body_Sprite) Components.Add(Body_Sprite);
	if (Arms_Sprite) Components.Add(Arms_Sprite);
	if (Head_Sprite) Components.Add(Head_Sprite);
	if (Eyebrow_Sprite) Components.Add(Eyebrow_Sprite);
	if (Eyes_Sprite) Components.Add(Eyes_Sprite);
	if (Eyelids_Sprite) Components.Add(Eyelids_Sprite);
	if (Wink_Sprite) Components.Add(Wink_Sprite);
	if (Mouth_Sprite) Components.Add(Mouth_Sprite);
	
	// --- КЛЮЧЕВОЕ ИСПРАВЛЕНИЕ: ТЕНЬ БОЛЬШЕ НЕ ЯВЛЯЕТСЯ "ОСНОВНЫМ" КОМПОНЕНТОМ ---
	// if (BodyShadow_Sprite) Components.Add(BodyShadow_Sprite); 
	
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

	if (Body_Skeletal_Fade) Components.Add(Body_Skeletal_Fade);
	if (Arms_Skeletal_Fade) Components.Add(Arms_Skeletal_Fade);
	if (Head_Skeletal_Fade) Components.Add(Head_Skeletal_Fade);
	if (Custom01_Skeletal_Fade) Components.Add(Custom01_Skeletal_Fade);
	if (Custom02_Skeletal_Fade) Components.Add(Custom02_Skeletal_Fade);
	if (Custom03_Skeletal_Fade) Components.Add(Custom03_Skeletal_Fade);
	
	if (Head_Sprite_Fade) Components.Add(Head_Sprite_Fade);
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

void AVNCharacter::SetAllComponentsVisibilityTrue()
{
	// === SKELETAL MESH КОМПОНЕНТЫ ===
	if (Body_Skeletal) Body_Skeletal->SetVisibility(true);
	if (Arms_Skeletal) Arms_Skeletal->SetVisibility(true);
	if (Head_Skeletal) Head_Skeletal->SetVisibility(true);
	if (Custom01_Skeletal) Custom01_Skeletal->SetVisibility(true);
	if (Custom02_Skeletal) Custom02_Skeletal->SetVisibility(true);
	if (Custom03_Skeletal) Custom03_Skeletal->SetVisibility(true);

	// === SKELETAL MESH FADE КОМПОНЕНТЫ ===
	if (Body_Skeletal_Fade) Body_Skeletal_Fade->SetVisibility(true);
	if (Arms_Skeletal_Fade) Arms_Skeletal_Fade->SetVisibility(true);
	if (Head_Skeletal_Fade) Head_Skeletal_Fade->SetVisibility(true);
	if (Custom01_Skeletal_Fade) Custom01_Skeletal_Fade->SetVisibility(true);
	if (Custom02_Skeletal_Fade) Custom02_Skeletal_Fade->SetVisibility(true);
	if (Custom03_Skeletal_Fade) Custom03_Skeletal_Fade->SetVisibility(true);

	// === SPRITE КОМПОНЕНТЫ ТЕЛА ===
	if (Body_Sprite) Body_Sprite->SetVisibility(true);
	if (Arms_Sprite) Arms_Sprite->SetVisibility(true);
	if (BodyShadow_Sprite) BodyShadow_Sprite->SetVisibility(true); // ← ВАЖНО: Shadow тоже!
	if (EmotionBodyEffect01_Sprite) EmotionBodyEffect01_Sprite->SetVisibility(true);
	if (EmotionBodyEffect02_Sprite) EmotionBodyEffect02_Sprite->SetVisibility(true);
	if (EmotionBodyEffect03_Sprite) EmotionBodyEffect03_Sprite->SetVisibility(true);

	// === SPRITE FADE КОМПОНЕНТЫ ТЕЛА ===
	if (Body_Sprite_Fade) Body_Sprite_Fade->SetVisibility(true);
	if (Arms_Sprite_Fade) Arms_Sprite_Fade->SetVisibility(true);
	if (BodyShadow_Sprite_Fade) BodyShadow_Sprite_Fade->SetVisibility(true); // ← ВАЖНО: Shadow Fade тоже!
	if (EmotionBodyEffect01_Sprite_Fade) EmotionBodyEffect01_Sprite_Fade->SetVisibility(true);
	if (EmotionBodyEffect02_Sprite_Fade) EmotionBodyEffect02_Sprite_Fade->SetVisibility(true);
	if (EmotionBodyEffect03_Sprite_Fade) EmotionBodyEffect03_Sprite_Fade->SetVisibility(true);

	// === SPRITE КОМПОНЕНТЫ ГОЛОВЫ ===
	if (Head_Sprite) Head_Sprite->SetVisibility(true);
	if (Eyebrow_Sprite) Eyebrow_Sprite->SetVisibility(true);
	if (Eyes_Sprite) Eyes_Sprite->SetVisibility(true);
	if (Eyelids_Sprite) Eyelids_Sprite->SetVisibility(true);
	if (Wink_Sprite) Wink_Sprite->SetVisibility(true);
	if (Mouth_Sprite) Mouth_Sprite->SetVisibility(true);
	if (EmotionHeadEffect01_Sprite) EmotionHeadEffect01_Sprite->SetVisibility(true);
	if (EmotionHeadEffect02_Sprite) EmotionHeadEffect02_Sprite->SetVisibility(true);
	if (EmotionHeadEffect03_Sprite) EmotionHeadEffect03_Sprite->SetVisibility(true);

	// === SPRITE FADE КОМПОНЕНТЫ ГОЛОВЫ ===
	if (Head_Sprite_Fade) Head_Sprite_Fade->SetVisibility(true);
	if (Eyebrow_Sprite_Fade) Eyebrow_Sprite_Fade->SetVisibility(true);
	if (Eyes_Sprite_Fade) Eyes_Sprite_Fade->SetVisibility(true);
	if (Eyelids_Sprite_Fade) Eyelids_Sprite_Fade->SetVisibility(true);
	if (Wink_Sprite_Fade) Wink_Sprite_Fade->SetVisibility(true);
	if (Mouth_Sprite_Fade) Mouth_Sprite_Fade->SetVisibility(true);
	if (EmotionHeadEffect01_Sprite_Fade) EmotionHeadEffect01_Sprite_Fade->SetVisibility(true);
	if (EmotionHeadEffect02_Sprite_Fade) EmotionHeadEffect02_Sprite_Fade->SetVisibility(true);
	if (EmotionHeadEffect03_Sprite_Fade) EmotionHeadEffect03_Sprite_Fade->SetVisibility(true);

	UE_LOG(LogTemp, Warning, TEXT("SetAllComponentsVisibilityTrue: All components Visibility set to TRUE"));
}