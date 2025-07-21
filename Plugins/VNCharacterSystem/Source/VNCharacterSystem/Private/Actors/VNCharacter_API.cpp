#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"

void AVNCharacter::SetSkeletalMesh(E_VN_ComponentID_Skeletal ComponentID, TSoftObjectPtr<USkeletalMesh> SkeletalMesh, bool bAnimate, float Duration)
{
	USkeletalMeshComponent* MainComponent = GetSkeletalComponent(ComponentID);
	if (!MainComponent)
	{
		VN_LOG_WARNING(TEXT("SetSkeletalMesh: Component not found for ID %d"), (int32)ComponentID);
		return;
	}

	// --- КЛЮЧЕВАЯ ПРОВЕРКА: Изменился ли ассет ---
	bool bAssetChanged = false;
	const USkeletalMesh* CurrentMesh = MainComponent->GetSkeletalMeshAsset();
	
	if (!CurrentMesh && !SkeletalMesh.IsNull())
	{
		bAssetChanged = true; // Был пустым, стал непустым
	}
	else if (CurrentMesh && SkeletalMesh.IsNull())
	{
		bAssetChanged = true; // Был непустым, стал пустым
	}
	else if (CurrentMesh && !SkeletalMesh.IsNull())
	{
		bAssetChanged = (CurrentMesh->GetPathName() != SkeletalMesh.ToString()); // Сравниваем пути
	}

	if (bAnimate && bAssetChanged && Duration > 0.0f && AnimationManager)
	{
		if (USkeletalMeshComponent* FadeComponent = GetSkeletalFadeComponent(ComponentID))
		{
			PrepareSkeletalTransition(MainComponent, FadeComponent, SkeletalMesh);
			AnimationManager->PlayTransition(Duration);
		}
		else
		{
			ValidateAndSetupSkeletalComponent(MainComponent, SkeletalMesh);
		}
	}
	else
	{
		// Мгновенное применение, если анимация отключена или ассет не изменился
		ValidateAndSetupSkeletalComponent(MainComponent, SkeletalMesh);
	}
}

void AVNCharacter::SetSprite(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> Sprite, bool bAnimate, float Duration)
{
	UPaperSpriteComponent* MainComponent = GetSpriteComponent(ComponentID);
	if (!MainComponent)
	{
		VN_LOG_WARNING(TEXT("SetSprite: Component not found for ID %d"), (int32)ComponentID);
		return;
	}

	// --- КЛЮЧЕВАЯ ПРОВЕРКА: Изменился ли ассет ---
	bool bAssetChanged = false;
	const UPaperSprite* CurrentSprite = MainComponent->GetSprite();
	
	if (!CurrentSprite && !Sprite.IsNull())
	{
		bAssetChanged = true; // Был пустым, стал непустым
	}
	else if (CurrentSprite && Sprite.IsNull())
	{
		bAssetChanged = true; // Был непустым, стал пустым
	}
	else if (CurrentSprite && !Sprite.IsNull())
	{
		bAssetChanged = (CurrentSprite->GetPathName() != Sprite.ToString()); // Сравниваем пути
	}

	if (bAnimate && bAssetChanged && Duration > 0.0f && AnimationManager)
	{
		if (UPaperSpriteComponent* FadeComponent = GetSpriteFadeComponent(ComponentID))
		{
			PrepareSpriteTransition(MainComponent, FadeComponent, Sprite);
			AnimationManager->PlayTransition(Duration);
		}
		else
		{
			ValidateAndSetupSpriteComponent(MainComponent, Sprite);
		}
	}
	else
	{
		// Мгновенное применение, если анимация отключена или ассет не изменился
		ValidateAndSetupSpriteComponent(MainComponent, Sprite);
	}

	OnCharacterComponentChanged.Broadcast(ComponentID);
}

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
	SetSprite(E_VN_ComponentID_Sprite::Eyes, EyesSprite, false, 0.0f);
	SetSprite(E_VN_ComponentID_Sprite::Mouth, MouthSprite, false, 0.0f);
	SetSprite(E_VN_ComponentID_Sprite::Eyebrow, EyebrowSprite, false, 0.0f);

	if (bAnimate && Duration > 0.0f && AnimationManager)
	{
		AnimationManager->PlayTransition(Duration);
	}
}

bool AVNCharacter::IsAnimating() const
{
	return AnimationManager && AnimationManager->IsAnimating();
}

FLinearColor AVNCharacter::GetTargetColorForComponent(USceneComponent* Component) const
{
	FLinearColor BaseColor = GetBaseColorForComponent(Component);
	return bIsInFocus ? BaseColor : BaseColor * DimColorMultiplier;
}

FLinearColor AVNCharacter::GetBaseColorForComponent(USceneComponent* Component) const
{
	return FLinearColor::White;
}