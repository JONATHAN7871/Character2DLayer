#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Components/VNCharacterIdleAnimationManager.h"

void AVNCharacter::SetSkeletalMesh(E_VN_ComponentID_Skeletal ComponentID, TSoftObjectPtr<USkeletalMesh> SkeletalMesh, bool bAnimate, float Duration)
{
	USkeletalMeshComponent* MainComponent = GetSkeletalComponent(ComponentID);
	if (!MainComponent)
	{
		VN_LOG_WARNING(TEXT("SetSkeletalMesh: Component not found for ID %d"), (int32)ComponentID);
		return;
	}

	if (AnimationManager && AnimationManager->IsAnimating() && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::Transition)
	{
		VN_LOG_WARNING(TEXT("SetSkeletalMesh: Forcing completion of ongoing transition"));
		AnimationManager->ClearAnimationQueue();
		FinalizeCurrentTransition();
	}

	bool bAssetChanged = false;
	const USkeletalMesh* CurrentMesh = MainComponent->GetSkeletalMeshAsset();
	
	if (!CurrentMesh && !SkeletalMesh.IsNull())
	{
		bAssetChanged = true;
	}
	else if (CurrentMesh && SkeletalMesh.IsNull())
	{
		bAssetChanged = true;
	}
	else if (CurrentMesh && !SkeletalMesh.IsNull())
	{
		bAssetChanged = (CurrentMesh->GetPathName() != SkeletalMesh.ToString());
	}

	if (bAnimate && bAssetChanged && Duration > 0.0f && AnimationManager)
	{
		VN_LOG_DEBUG(TEXT("SetSkeletalMesh: Preparing transition for component: %s"), *MainComponent->GetName());
		
		if (USkeletalMeshComponent* FadeComponent = GetSkeletalFadeComponent(ComponentID))
		{
			PrepareSkeletalTransition(MainComponent, FadeComponent, SkeletalMesh);
			RequestTransitionCommit(Duration);
		}
		else
		{
			VN_LOG_WARNING(TEXT("SetSkeletalMesh: Fade component not found for ID %d"), (int32)ComponentID);
			ValidateAndSetupSkeletalComponent(MainComponent, SkeletalMesh);
		}
	}
	else
	{
		if (!bAssetChanged)
		{
			VN_LOG_DEBUG(TEXT("SetSkeletalMesh: Asset unchanged for component: %s"), *MainComponent->GetName());
		}
		ValidateAndSetupSkeletalComponent(MainComponent, SkeletalMesh);
	}
}

void AVNCharacter::SetSprite(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> Sprite, bool bAnimate, float Duration)
{
	UPaperSpriteComponent* MainComponent = GetSpriteComponent(ComponentID);
	if (!MainComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("SetSprite: Component not found for ID %d"), (int32)ComponentID);
		return;
	}

	// КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Автоматическое кэширование спрайта ВСЕГДА
	CacheSpriteOnSet(ComponentID, Sprite);
	
	// ДОПОЛНИТЕЛЬНОЕ ИСПРАВЛЕНИЕ: Если idle анимации активны - обновляем их кэш тоже
	if (IdleAnimationManager)
	{
		// Проверяем, какие анимации активны и обновляем их состояние
		const FVNIdleAnimationsConfig& Config = IdleAnimationManager->GetIdleAnimationsConfig();
		
		// Если меняем спрайт рта во время Talk анимации
		if (ComponentID == E_VN_ComponentID_Sprite::Mouth && Config.TalkConfig.bEnabled)
		{
			VN_LOG_DEBUG(TEXT("SetSprite: Mouth sprite changed during Talk animation - updating cache"));
			IdleAnimationManager->UpdateSavedSprites();
		}
		
		// Если меняем спрайт глаз во время Eyes анимации
		if (ComponentID == E_VN_ComponentID_Sprite::Eyes && Config.EyesRandomConfig.bEnabled)
		{
			VN_LOG_DEBUG(TEXT("SetSprite: Eyes sprite changed during Eyes animation - updating cache"));
			IdleAnimationManager->UpdateSavedSprites();
		}
		
		// Если меняем спрайт век во время Blink анимации
		if (ComponentID == E_VN_ComponentID_Sprite::Eyelids && Config.BlinkConfig.bEnabled)
		{
			VN_LOG_DEBUG(TEXT("SetSprite: Eyelids sprite changed during Blink animation - updating cache"));
			IdleAnimationManager->UpdateSavedSprites();
		}
	}

	if (AnimationManager && AnimationManager->IsAnimating() && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::Transition)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetSprite: Forcing completion of ongoing transition"));
		AnimationManager->ClearAnimationQueue();
		FinalizeCurrentTransition();
	}

	bool bAssetChanged = false;
	const UPaperSprite* CurrentSprite = MainComponent->GetSprite();
	
	if (!CurrentSprite && !Sprite.IsNull())
	{
		bAssetChanged = true;
	}
	else if (CurrentSprite && Sprite.IsNull())
	{
		bAssetChanged = true;
	}
	else if (CurrentSprite && !Sprite.IsNull())
	{
		bAssetChanged = (CurrentSprite->GetPathName() != Sprite.ToString());
	}

	if (bAnimate && bAssetChanged && Duration > 0.0f && AnimationManager)
	{
		if (UPaperSpriteComponent* FadeComponent = GetSpriteFadeComponent(ComponentID))
		{
			PrepareSpriteTransition(MainComponent, FadeComponent, Sprite);
			RequestTransitionCommit(Duration);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("SetSprite: Fade component not found for ID %d"), (int32)ComponentID);
			ValidateAndSetupSpriteComponent(MainComponent, Sprite);
		}
	}
	else
	{
		if (!bAssetChanged)
		{
			UE_LOG(LogTemp, Warning, TEXT("SetSprite: Asset unchanged, skipping"));
		}
		ValidateAndSetupSpriteComponent(MainComponent, Sprite);
	}

	OnCharacterComponentChanged.Broadcast(ComponentID);
}

void AVNCharacter::SetFace(TSoftObjectPtr<UPaperSprite> EyesSprite, TSoftObjectPtr<UPaperSprite> MouthSprite, TSoftObjectPtr<UPaperSprite> EyebrowSprite, bool bAnimate, float Duration)
{
    VN_LOG_DEBUG(TEXT("SetFace: Setting multiple face components with animate=%s, duration=%.2f"), bAnimate ? TEXT("true") : TEXT("false"), Duration);

    if (bAnimate && Duration > 0.0f && AnimationManager)
    {
        bool bAnyComponentChanged = false;

        // Проверяем и подготавливаем ГЛАЗА
        UPaperSpriteComponent* EyesComp = GetSpriteComponent(E_VN_ComponentID_Sprite::Eyes);
        if (EyesComp)
        {
            const UPaperSprite* CurrentSprite = EyesComp->GetSprite();
            const bool bAssetChanged = (!CurrentSprite && !EyesSprite.IsNull()) ||
                                       (CurrentSprite && EyesSprite.IsNull()) ||
                                       (CurrentSprite && !EyesSprite.IsNull() && CurrentSprite->GetPathName() != EyesSprite.ToString());
            if (bAssetChanged)
            {
                if (UPaperSpriteComponent* FadeComp = GetSpriteFadeComponent(E_VN_ComponentID_Sprite::Eyes))
                {
                    PrepareSpriteTransition(EyesComp, FadeComp, EyesSprite);
                    bAnyComponentChanged = true;
                }
            }
        }

        // Проверяем и подготавливаем РОТ
        UPaperSpriteComponent* MouthComp = GetSpriteComponent(E_VN_ComponentID_Sprite::Mouth);
        if (MouthComp)
        {
            const UPaperSprite* CurrentSprite = MouthComp->GetSprite();
            const bool bAssetChanged = (!CurrentSprite && !MouthSprite.IsNull()) ||
                                       (CurrentSprite && MouthSprite.IsNull()) ||
                                       (CurrentSprite && !MouthSprite.IsNull() && CurrentSprite->GetPathName() != MouthSprite.ToString());
            if (bAssetChanged)
            {
                if (UPaperSpriteComponent* FadeComp = GetSpriteFadeComponent(E_VN_ComponentID_Sprite::Mouth))
                {
                    PrepareSpriteTransition(MouthComp, FadeComp, MouthSprite);
                    bAnyComponentChanged = true;
                }
            }
        }

        // Проверяем и подготавливаем БРОВИ
        UPaperSpriteComponent* EyebrowComp = GetSpriteComponent(E_VN_ComponentID_Sprite::Eyebrow);
        if (EyebrowComp)
        {
            const UPaperSprite* CurrentSprite = EyebrowComp->GetSprite();
            const bool bAssetChanged = (!CurrentSprite && !EyebrowSprite.IsNull()) ||
                                       (CurrentSprite && EyebrowSprite.IsNull()) ||
                                       (CurrentSprite && !EyebrowSprite.IsNull() && CurrentSprite->GetPathName() != EyebrowSprite.ToString());
            if (bAssetChanged)
            {
                if (UPaperSpriteComponent* FadeComp = GetSpriteFadeComponent(E_VN_ComponentID_Sprite::Eyebrow))
                {
                    PrepareSpriteTransition(EyebrowComp, FadeComp, EyebrowSprite);
                    bAnyComponentChanged = true;
                }
            }
        }

        if (bAnyComponentChanged)
        {
            RequestTransitionCommit(Duration);
        }
    }
    else
    {
        SetSprite(E_VN_ComponentID_Sprite::Eyes, EyesSprite, false, 0.0f);
        SetSprite(E_VN_ComponentID_Sprite::Mouth, MouthSprite, false, 0.0f);
        SetSprite(E_VN_ComponentID_Sprite::Eyebrow, EyebrowSprite, false, 0.0f);
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