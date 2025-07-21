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
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	CreateComponents();
	SetupComponentHierarchy();

	bIsInFocus = true;
	DimColorMultiplier = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
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
}

#if WITH_EDITOR
void AVNCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Примечание: Для корректного обновления в редакторе при изменении Global transforms
	// теперь рекомендуется повторно применить DataAsset, так как логика трансформации
	// централизована и применяется при вызове ApplyDataAsset.
}
#endif