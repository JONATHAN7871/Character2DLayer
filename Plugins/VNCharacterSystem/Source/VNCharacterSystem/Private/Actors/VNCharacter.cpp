#include "Actors/VNCharacter.h"
#include "Data/VNCharacterDataAsset.h"
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