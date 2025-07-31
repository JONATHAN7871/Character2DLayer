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
	if (!MainComponent) return;

	// ШАГ 1: ПОЛНАЯ ОСТАНОВКА И СБРОС
	// Неважно, что происходило до этого, возвращаем персонажа в базовое состояние.
	StopAndResetIdleAnimations();
    
	// ШАГ 2: ОБНОВЛЕНИЕ БАЗОВОГО СОСТОЯНИЯ
	// Теперь, когда все чисто, объявляем новый спрайт новым "базовым" состоянием в кэше.
	SetCachedSprite(ComponentID, Sprite);

	// ШАГ 3: ВИЗУАЛЬНОЕ ПРИМЕНЕНИЕ
	bool bAssetChanged = false;
	// ... (логика определения bAssetChanged, как и раньше)
	const UPaperSprite* CurrentSprite = MainComponent->GetSprite();
	if ((!CurrentSprite && !Sprite.IsNull()) || (CurrentSprite && Sprite.IsNull()) || (CurrentSprite && !Sprite.IsNull() && CurrentSprite->GetPathName() != Sprite.ToString()))
	{
		bAssetChanged = true;
	}

	if (bAnimate && bAssetChanged && Duration > 0.0f && AnimationManager)
	{
		// ... (логика с PrepareSpriteTransition, как и раньше)
		if (UPaperSpriteComponent* FadeComponent = GetSpriteFadeComponent(ComponentID)) 
		{
			PrepareSpriteTransition(MainComponent, FadeComponent, Sprite);
			RequestTransitionCommit(Duration);
		} else { ValidateAndSetupSpriteComponent(MainComponent, Sprite); }
	}
	else
	{
		ValidateAndSetupSpriteComponent(MainComponent, Sprite);
	}
	
	// ШАГ 4: ПЕРЕЗАПУСК IDLE (если нужно)
	// Если мы не ждем завершения анимации перехода (Transition), то можно сразу запускать Idle.
	if (!(bAnimate && bAssetChanged && Duration > 0.0f))
	{
		if(IdleAnimationManager) IdleAnimationManager->StartAllIdleAnimations();
	}
    
	OnCharacterComponentChanged.Broadcast(ComponentID);
}


void AVNCharacter::SetFace(TSoftObjectPtr<UPaperSprite> EyesSprite, TSoftObjectPtr<UPaperSprite> MouthSprite, TSoftObjectPtr<UPaperSprite> EyebrowSprite, bool bAnimate, float Duration)
{
	// ШАГ 1: ПОЛНАЯ ОСТАНОВКА И СБРОС (один раз для всех)
	StopAndResetIdleAnimations();
    
	// ШАГ 2: ОБНОВЛЕНИЕ КЭША (для всех компонентов)
	SetCachedSprite(E_VN_ComponentID_Sprite::Eyes, EyesSprite);
	SetCachedSprite(E_VN_ComponentID_Sprite::Mouth, MouthSprite);
	SetCachedSprite(E_VN_ComponentID_Sprite::Eyebrow, EyebrowSprite);

	// ШАГ 3: ВИЗУАЛЬНОЕ ПРИМЕНЕНИЕ (вызываем базовую функцию SetSprite без анимации)
	// Это автоматически и мгновенно установит спрайты.
	// Важно: мы не вызываем полную SetSprite, так как она снова вызовет StopAndReset.
	ValidateAndSetupSpriteComponent(GetSpriteComponent(E_VN_ComponentID_Sprite::Eyes), EyesSprite);
	ValidateAndSetupSpriteComponent(GetSpriteComponent(E_VN_ComponentID_Sprite::Mouth), MouthSprite);
	ValidateAndSetupSpriteComponent(GetSpriteComponent(E_VN_ComponentID_Sprite::Eyebrow), EyebrowSprite);

	// ШАГ 4: ПЕРЕЗАПУСК IDLE (один раз для всех)
	if (IdleAnimationManager)
	{
		IdleAnimationManager->StartAllIdleAnimations();
	}
}

// === НОВЫЕ МЕТОДЫ ДЛЯ УЛУЧШЕННОГО КЭШИРОВАНИЯ ===

void AVNCharacter::UpdateCacheForComponent(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> NewSprite)
{
	// УПРОЩЕННАЯ ВЕРСЯ: Просто обновляем кэш. Без "умных" проверок.
	// Команда "UpdateCache" должна всегда выполняться.
	SetCachedSprite(ComponentID, NewSprite);
	VN_LOG_DEBUG(TEXT("UpdateCacheForComponent: New base sprite cached for %d -> %s"), 
		(int32)ComponentID, NewSprite.IsNull() ? TEXT("NULL") : *NewSprite.ToString());

	// Уведомляем IdleManager, чтобы он обновил свое внутреннее состояние.
	if (IdleAnimationManager)
	{
		IdleAnimationManager->HandleExternalSpriteChange(ComponentID, NewSprite);
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
	if (!Component) return FLinearColor::White;
    
	// Получаем базовый цвет из кэша
	FLinearColor BaseColor = GetCachedBaseColor(Component);
    
	// Применяем эффект фокуса
	return ApplyFocusToColor(BaseColor);
}

FLinearColor AVNCharacter::GetBaseColorForComponent(USceneComponent* Component) const
{
	if (!Component) return FLinearColor::White;
    
	// Возвращаем кэшированный базовый цвет
	return GetCachedBaseColor(Component);
}