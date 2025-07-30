#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "Components/VNCharacterIdleAnimationManager.h"
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

	// === КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: УМНОЕ КЭШИРОВАНИЕ ===
	// Всегда обновляем кэш при установке нового спрайта, независимо от анимаций
	UpdateCacheForComponent(ComponentID, Sprite);

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
			UE_LOG(LogTemp, Warning, TEXT("SetSprite: Asset unchanged, but cache updated"));
		}
		ValidateAndSetupSpriteComponent(MainComponent, Sprite);
	}

	// === НОВОЕ: Уведомляем Idle Manager об изменении ===
	NotifyIdleManagerAboutSpriteChange(ComponentID, Sprite);

	OnCharacterComponentChanged.Broadcast(ComponentID);
}

void AVNCharacter::SetFace(TSoftObjectPtr<UPaperSprite> EyesSprite, TSoftObjectPtr<UPaperSprite> MouthSprite, TSoftObjectPtr<UPaperSprite> EyebrowSprite, bool bAnimate, float Duration)
{
    VN_LOG_DEBUG(TEXT("SetFace: Setting multiple face components with animate=%s, duration=%.2f"), bAnimate ? TEXT("true") : TEXT("false"), Duration);

    // === КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Обновляем кэш для всех компонентов ===
    UpdateCacheForComponent(E_VN_ComponentID_Sprite::Eyes, EyesSprite);
    UpdateCacheForComponent(E_VN_ComponentID_Sprite::Mouth, MouthSprite);
    UpdateCacheForComponent(E_VN_ComponentID_Sprite::Eyebrow, EyebrowSprite);

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
        // Мгновенное применение - используем SetSprite с выключенной анимацией
        // Это автоматически обработает кэширование
        SetSprite(E_VN_ComponentID_Sprite::Eyes, EyesSprite, false, 0.0f);
        SetSprite(E_VN_ComponentID_Sprite::Mouth, MouthSprite, false, 0.0f);
        SetSprite(E_VN_ComponentID_Sprite::Eyebrow, EyebrowSprite, false, 0.0f);
    }

    // === НОВОЕ: Уведомляем обо всех изменениях ===
    NotifyIdleManagerAboutSpriteChange(E_VN_ComponentID_Sprite::Eyes, EyesSprite);
    NotifyIdleManagerAboutSpriteChange(E_VN_ComponentID_Sprite::Mouth, MouthSprite);
    NotifyIdleManagerAboutSpriteChange(E_VN_ComponentID_Sprite::Eyebrow, EyebrowSprite);
}

// === НОВЫЕ МЕТОДЫ ДЛЯ УЛУЧШЕННОГО КЭШИРОВАНИЯ ===

void AVNCharacter::UpdateCacheForComponent(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> NewSprite)
{
	VN_LOG_DEBUG(TEXT("UpdateCacheForComponent: %d -> %s"), 
		(int32)ComponentID, 
		NewSprite.IsNull() ? TEXT("NULL") : *NewSprite.ToString());

	// Проверяем, не является ли текущий спрайт частью активной анимации
	if (IdleAnimationManager && IsComponentInActiveIdleAnimation(ComponentID))
	{
		VN_LOG_DEBUG(TEXT("UpdateCacheForComponent: Component %d is in active idle animation, updating cache directly"), (int32)ComponentID);
		
		// Если компонент участвует в idle анимации, обновляем кэш напрямую
		SetCachedSprite(ComponentID, NewSprite);
		
		// Дополнительно обновляем состояние в IdleAnimationManager
		if (IdleAnimationManager)
		{
			IdleAnimationManager->HandleExternalSpriteChange(ComponentID, NewSprite);
		}
	}
	else
	{
		VN_LOG_DEBUG(TEXT("UpdateCacheForComponent: Component %d not in active animation, normal cache update"), (int32)ComponentID);
		SetCachedSprite(ComponentID, NewSprite);
	}
}

bool AVNCharacter::IsComponentInActiveIdleAnimation(E_VN_ComponentID_Sprite ComponentID) const
{
	if (!IdleAnimationManager) return false;
	
	const FVNIdleAnimationsConfig& Config = IdleAnimationManager->GetIdleAnimationsConfig();
	
	switch (ComponentID)
	{
		case E_VN_ComponentID_Sprite::Eyelids:
			return Config.BlinkConfig.bEnabled && IdleAnimationManager->IsBlinkActive();
			
		case E_VN_ComponentID_Sprite::Mouth:
			return Config.TalkConfig.bEnabled && IdleAnimationManager->IsTalkActive();
			
		case E_VN_ComponentID_Sprite::Eyes:
			return Config.EyesRandomConfig.bEnabled && IdleAnimationManager->IsEyesRandomActive();
			
		case E_VN_ComponentID_Sprite::Eyebrow:
		case E_VN_ComponentID_Sprite::Wink:
			// Эти компоненты могут участвовать в будущих анимациях
			return false;
			
		default:
			return false;
	}
}

void AVNCharacter::NotifyIdleManagerAboutSpriteChange(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> NewSprite)
{
	if (!IdleAnimationManager) return;
	
	VN_LOG_DEBUG(TEXT("NotifyIdleManagerAboutSpriteChange: Component %d changed"), (int32)ComponentID);
	
	// Специальная обработка для компонентов с активными анимациями
	switch (ComponentID)
	{
		case E_VN_ComponentID_Sprite::Eyelids:
		{
			const FVNIdleAnimationsConfig& Config = IdleAnimationManager->GetIdleAnimationsConfig();
			if (Config.BlinkConfig.bEnabled)
			{
				VN_LOG_DEBUG(TEXT("NotifyIdleManagerAboutSpriteChange: Updating blink animation mode for new eyelids state"));
				// IdleAnimationManager сам определит новый режим моргания (2-phase или 3-phase)
				IdleAnimationManager->UpdateBlinkModeForNewEyelidsState(NewSprite);
			}
			break;
		}
		
		case E_VN_ComponentID_Sprite::Mouth:
		{
			const FVNIdleAnimationsConfig& Config = IdleAnimationManager->GetIdleAnimationsConfig();
			if (Config.TalkConfig.bEnabled)
			{
				VN_LOG_DEBUG(TEXT("NotifyIdleManagerAboutSpriteChange: Talk animation active, mouth sprite updated"));
				// Кэш уже обновлен, ничего дополнительного не нужно
			}
			break;
		}
		
		case E_VN_ComponentID_Sprite::Eyes:
		{
			const FVNIdleAnimationsConfig& Config = IdleAnimationManager->GetIdleAnimationsConfig();
			if (Config.EyesRandomConfig.bEnabled)
			{
				VN_LOG_DEBUG(TEXT("NotifyIdleManagerAboutSpriteChange: Eyes animation active, eyes sprite updated"));
				// Кэш уже обновлен, ничего дополнительного не нужно
			}
			break;
		}
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