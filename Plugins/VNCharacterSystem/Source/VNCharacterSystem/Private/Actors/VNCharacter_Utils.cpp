#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

void AVNCharacter::UpdateComponentTransform(USceneComponent* Component, const FVector& LocalOffset, float LocalScale)
{
    if (!Component) return;

    FVector FinalOffset = LocalOffset;
    float FinalScale = LocalScale;

    if (Component->GetAttachSocketName().IsNone())
    {
        if (Cast<USkeletalMeshComponent>(Component))
        {
            FinalOffset = GlobalSkeletalOffset + LocalOffset;
            FinalScale = GlobalSkeletalScale * LocalScale;
        }
        else if (Cast<UPaperSpriteComponent>(Component))
        {
            if (!IsChildOfHeadSprite(Component))
            {
                FinalOffset = GlobalSpriteOffset + LocalOffset;
                FinalScale = GlobalSpriteScale * LocalScale;
            }
        }
    }
    
    Component->SetRelativeLocation(FinalOffset);
    Component->SetRelativeScale3D(FVector(FinalScale));
}

void AVNCharacter::ApplyIndividualSpriteTransform(UPaperSpriteComponent* SpriteComponent, E_VN_ComponentID_Sprite ComponentID)
{
	if (!SpriteComponent) return;
	UpdateComponentTransform(SpriteComponent, FVector::ZeroVector, 1.0f);
}

bool AVNCharacter::IsChildOfHeadSprite(E_VN_ComponentID_Sprite ComponentID) const
{
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

bool AVNCharacter::IsChildOfHeadSprite(USceneComponent* Component) const
{
    if (!Component) return false;
    return Component->GetAttachParent() == Head_Sprite;
}

void AVNCharacter::ValidateAndSetupSkeletalComponent(USkeletalMeshComponent* Component, TSoftObjectPtr<USkeletalMesh> SkeletalMesh)
{
	if (!Component) return;
	
	if (!SkeletalMesh.IsNull())
	{
		USkeletalMesh* LoadedMesh = SkeletalMesh.LoadSynchronous();
		if (LoadedMesh)
		{
			VN_LOG_DEBUG(TEXT("ValidateAndSetupSkeletalComponent: Setting mesh for %s to %s"), *Component->GetName(), *LoadedMesh->GetName());
			Component->SetSkeletalMesh(LoadedMesh);
			Component->SetVisibility(true);
			SetComponentColor(Component, GetTargetColorForComponent(Component));
		}
		else
		{
			VN_LOG_WARNING(TEXT("ValidateAndSetupSkeletalComponent: Failed to load mesh for %s"), *Component->GetName());
			Component->SetSkeletalMesh(nullptr);
			Component->SetVisibility(false);
		}
	}
	else
	{
		VN_LOG_DEBUG(TEXT("ValidateAndSetupSkeletalComponent: Clearing mesh for %s"), *Component->GetName());
		Component->SetSkeletalMesh(nullptr);
		Component->SetVisibility(false);
	}
}

void AVNCharacter::ValidateAndSetupSpriteComponent(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite> Sprite)
{
	if (!Component) return;
	
	if (!Sprite.IsNull())
	{
		UPaperSprite* LoadedSprite = Sprite.LoadSynchronous();
		if (LoadedSprite)
		{
			VN_LOG_DEBUG(TEXT("ValidateAndSetupSpriteComponent: Setting sprite for %s to %s"), *Component->GetName(), *LoadedSprite->GetName());
			Component->SetSprite(LoadedSprite);
			Component->SetVisibility(true);
			SetComponentColor(Component, GetTargetColorForComponent(Component));
		}
		else
		{
			VN_LOG_WARNING(TEXT("ValidateAndSetupSpriteComponent: Failed to load sprite for %s"), *Component->GetName());
			Component->SetSprite(nullptr);
			Component->SetVisibility(false);
		}
	}
	else
	{
		VN_LOG_DEBUG(TEXT("ValidateAndSetupSpriteComponent: Clearing sprite for %s"), *Component->GetName());
		Component->SetSprite(nullptr);
		Component->SetVisibility(false);
	}
}

void AVNCharacter::SetComponentAlpha(USceneComponent* Component, float Alpha)
{
	if (!Component) return;
	Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	
	if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(Component))
	{
		for (int32 i = 0; i < SkeletalMesh->GetNumMaterials(); ++i)
		{
			if (UMaterialInterface* BaseMaterial = SkeletalMesh->GetMaterial(i))
			{
				if (UMaterialInstanceDynamic* DynamicMaterial = SkeletalMesh->CreateAndSetMaterialInstanceDynamic(i))
				{
					DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), Alpha);
				}
			}
		}
	}
	else if (UPaperSpriteComponent* SpriteComponent = Cast<UPaperSpriteComponent>(Component))
	{
		FLinearColor CurrentColor = SpriteComponent->GetSpriteColor();
		CurrentColor.A = Alpha;
		SpriteComponent->SetSpriteColor(CurrentColor);
	}
}

void AVNCharacter::SetComponentColor(USceneComponent* Component, const FLinearColor& Color)
{
	if (!Component) return;
	
	if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(Component))
	{
		for (int32 i = 0; i < SkeletalMesh->GetNumMaterials(); ++i)
		{
			if (UMaterialInterface* BaseMaterial = SkeletalMesh->GetMaterial(i))
			{
				if (UMaterialInstanceDynamic* DynamicMaterial = SkeletalMesh->CreateAndSetMaterialInstanceDynamic(i))
				{
					DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
					DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), Color.A);
				}
			}
		}
	}
	else if (UPaperSpriteComponent* SpriteComponent = Cast<UPaperSpriteComponent>(Component))
	{
		SpriteComponent->SetSpriteColor(Color);
	}
}

void AVNCharacter::CopySkeletalComponentSettings(USkeletalMeshComponent* Source, USkeletalMeshComponent* Target)
{
	if (!Source || !Target) return;
	
	VN_LOG_DEBUG(TEXT("CopySkeletalComponentSettings: Copying from %s to %s"), *Source->GetName(), *Target->GetName());
	
	Target->SetSkeletalMesh(Source->GetSkeletalMeshAsset());
	Target->SetAnimInstanceClass(Source->GetAnimClass());
	Target->SetVisibility(Source->IsVisible());
	
	for (int32 i = 0; i < Source->GetNumMaterials(); ++i)
	{
		Target->SetMaterial(i, Source->GetMaterial(i));
	}
	
	if (Source->GetAttachParent())
	{
		Target->AttachToComponent(Source->GetAttachParent(), FAttachmentTransformRules::KeepWorldTransform, Source->GetAttachSocketName());
		Target->SetRelativeTransform(Source->GetRelativeTransform());
	}
	else
	{
		Target->SetWorldTransform(Source->GetComponentTransform());
	}
}

void AVNCharacter::CopySpriteComponentSettings(UPaperSpriteComponent* Source, UPaperSpriteComponent* Target)
{
	if (!Source || !Target) return;
	
	VN_LOG_DEBUG(TEXT("CopySpriteComponentSettings: Copying from %s to %s"), *Source->GetName(), *Target->GetName());
	
	Target->SetSprite(Source->GetSprite());
	Target->SetSpriteColor(Source->GetSpriteColor());
	Target->SetVisibility(Source->IsVisible());
	
	if (Source->GetAttachParent())
	{
		Target->AttachToComponent(Source->GetAttachParent(), FAttachmentTransformRules::KeepWorldTransform, Source->GetAttachSocketName());
		Target->SetRelativeTransform(Source->GetRelativeTransform());
	}
	else
	{
		Target->SetWorldTransform(Source->GetComponentTransform());
	}
}

void AVNCharacter::PrepareSkeletalTransition(USkeletalMeshComponent* MainComponent, USkeletalMeshComponent* FadeComponent, TSoftObjectPtr<USkeletalMesh> NewMesh)
{
	if (!MainComponent || !FadeComponent) return;
	
	VN_LOG_DEBUG(TEXT("PrepareSkeletalTransition: Starting transition for MainComponent=%s, FadeComponent=%s"), *MainComponent->GetName(), *FadeComponent->GetName());
	
	FadingInComponents.Add(MainComponent);
	FadingOutComponents.Add(FadeComponent);

	// --- ДЕТАЛЬНОЕ ЛОГИРОВАНИЕ ---
	VN_LOG_DEBUG(TEXT("PrepareSkeletalTransition: Added to transition sets. FadingIn [%s], FadingOut [%s]. Total sets: FadingIn=%d, FadingOut=%d"), 
		*MainComponent->GetName(), *FadeComponent->GetName(), FadingInComponents.Num(), FadingOutComponents.Num());

	CopySkeletalComponentSettings(MainComponent, FadeComponent);

	// --- ИЗМЕНЕНИЕ: Используем Leader Pose ТОЛЬКО при переходе на валидный меш ---
	if (!NewMesh.IsNull())
	{
		VN_LOG_DEBUG(TEXT("PrepareSkeletalTransition: Setting Leader Pose Component for %s"), *FadeComponent->GetName());
		// Это предотвратит "скачок" позы при переходе с одного меша на другой.
		FadeComponent->SetLeaderPoseComponent(MainComponent);
	}
	else
	{
		VN_LOG_DEBUG(TEXT("PrepareSkeletalTransition: Skipping Leader Pose setup (NewMesh is null)"));
	}
	// Если NewMesh == nullptr, мы НЕ устанавливаем Leader, т.к. лидер будет невалидным.
	// FadeComponent просто исчезнет со своей последней валидной позой.

	// Смещаем Fade-компонент назад для решения проблемы Z-fighting
	FVector CurrentLocation = FadeComponent->GetRelativeLocation();
	FadeComponent->SetRelativeLocation(CurrentLocation + FVector(0.f, -1.f, 0.f));
	VN_LOG_DEBUG(TEXT("PrepareSkeletalTransition: Applied Z-offset to %s"), *FadeComponent->GetName());
	
	SetComponentAlpha(FadeComponent, 1.0f);
	FadeComponent->SetVisibility(true, true);
	VN_LOG_DEBUG(TEXT("PrepareSkeletalTransition: FadeComponent %s set to visible with alpha 1.0"), *FadeComponent->GetName());

	ResetComponentAttachmentToDefault(MainComponent);
	ValidateAndSetupSkeletalComponent(MainComponent, NewMesh);
	SetComponentAlpha(MainComponent, 0.0f);
	VN_LOG_DEBUG(TEXT("PrepareSkeletalTransition: MainComponent %s set to alpha 0.0"), *MainComponent->GetName());
}

void AVNCharacter::PrepareSpriteTransition(UPaperSpriteComponent* MainComponent, UPaperSpriteComponent* FadeComponent, TSoftObjectPtr<UPaperSprite> NewSprite)
{
	if (!MainComponent || !FadeComponent) return;
	
	VN_LOG_DEBUG(TEXT("PrepareSpriteTransition: Starting transition for MainComponent=%s, FadeComponent=%s"), *MainComponent->GetName(), *FadeComponent->GetName());
	
	FadingInComponents.Add(MainComponent);
	FadingOutComponents.Add(FadeComponent);

	// --- ДЕТАЛЬНОЕ ЛОГИРОВАНИЕ ---
	VN_LOG_DEBUG(TEXT("PrepareSpriteTransition: Added to transition sets. FadingIn [%s], FadingOut [%s]. Total sets: FadingIn=%d, FadingOut=%d"), 
		*MainComponent->GetName(), *FadeComponent->GetName(), FadingInComponents.Num(), FadingOutComponents.Num());

	CopySpriteComponentSettings(MainComponent, FadeComponent);

	// Смещаем Fade-компонент назад для решения проблемы Z-fighting
	FVector CurrentLocation = FadeComponent->GetRelativeLocation();
	FadeComponent->SetRelativeLocation(CurrentLocation + FVector(0.f, -1.f, 0.f));
	VN_LOG_DEBUG(TEXT("PrepareSpriteTransition: Applied Z-offset to %s"), *FadeComponent->GetName());
	
	SetComponentAlpha(FadeComponent, 1.0f);
	FadeComponent->SetVisibility(true, true);
	VN_LOG_DEBUG(TEXT("PrepareSpriteTransition: FadeComponent %s set to visible with alpha 1.0"), *FadeComponent->GetName());
	
	ResetComponentAttachmentToDefault(MainComponent);
	ValidateAndSetupSpriteComponent(MainComponent, NewSprite);
	SetComponentAlpha(MainComponent, 0.0f);
	VN_LOG_DEBUG(TEXT("PrepareSpriteTransition: MainComponent %s set to alpha 0.0"), *MainComponent->GetName());
}

void AVNCharacter::FinishTransition(USceneComponent* MainComponent, USceneComponent* FadeComponent)
{
	if (!MainComponent || !FadeComponent) return;
	
	VN_LOG_DEBUG(TEXT("FinishTransition: Finishing transition for MainComponent=%s, FadeComponent=%s"), *MainComponent->GetName(), *FadeComponent->GetName());
	
	SetComponentAlpha(MainComponent, 1.0f);
	FadeComponent->SetVisibility(false);
	SetComponentAlpha(FadeComponent, 0.0f);
	
	if (USkeletalMeshComponent* SkeletalFade = Cast<USkeletalMeshComponent>(FadeComponent))
	{
		SkeletalFade->SetSkeletalMesh(nullptr);
		VN_LOG_DEBUG(TEXT("FinishTransition: Cleared skeletal mesh for %s"), *FadeComponent->GetName());
	}
	else if (UPaperSpriteComponent* SpriteFade = Cast<UPaperSpriteComponent>(FadeComponent))
	{
		SpriteFade->SetSprite(nullptr);
		VN_LOG_DEBUG(TEXT("FinishTransition: Cleared sprite for %s"), *FadeComponent->GetName());
	}
}

void AVNCharacter::HideAllFadeComponents()
{
	VN_LOG_DEBUG(TEXT("HideAllFadeComponents: Hiding all fade components"));
	
	TArray<USceneComponent*> FadeComponents = GetAllFadeComponents();
	for (USceneComponent* Component : FadeComponents)
	{
		if (Component)
		{
			Component->SetVisibility(false);
			SetComponentAlpha(Component, 0.0f);
			
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
	
	VN_LOG_DEBUG(TEXT("HideAllFadeComponents: Hidden %d fade components"), FadeComponents.Num());
}