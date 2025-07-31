// VNCharacter_Animation.cpp - Система анимаций, событий и управления фокусом

#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "Components/VNCharacterIdleAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"

// =====================================================
// СОБЫТИЯ АНИМАЦИЙ
// =====================================================

void AVNCharacter::OnAnimationStarted(EVNAnimationType AnimationType)
{
	VN_LOG_DEBUG(TEXT("OnAnimationStarted: Animation type %d started"), (int32)AnimationType);
	
	switch (AnimationType)
	{
	case EVNAnimationType::Transition:
	case EVNAnimationType::SpawnDespawn:
		if (IdleAnimationManager)
		{
			VN_LOG_DEBUG(TEXT("OnAnimationStarted: Stopping and resetting idle states."));
			StopAndResetIdleAnimations(); // <--- ИСПОЛЬЗУЕМ НОВУЮ ФУНКЦИЮ
		}
		break;
			
	case EVNAnimationType::Focus:
		VN_LOG_DEBUG(TEXT("OnAnimationStarted: Focus animation started"));
		break;
			
	default:
		break;
	}
}

void AVNCharacter::OnAnimationFinished(EVNAnimationType AnimationType)
{
	VN_LOG_DEBUG(TEXT("OnAnimationFinished: Animation type %d finished"), (int32)AnimationType);
	
	switch (AnimationType)
	{
	case EVNAnimationType::Transition:
		{
			FinalizeCurrentTransition();
			// ВАЖНО: Перезапускаем Idle после любого перехода.
			if (IdleAnimationManager)
			{
				VN_LOG_DEBUG(TEXT("OnAnimationFinished (Transition): Restarting idle animations."));
				IdleAnimationManager->StartAllIdleAnimations();
			}
			break;
		}
		
	case EVNAnimationType::SpawnDespawn:
		{
			RefreshAllComponentColors();
			OnCharacterVisibilityChanged.Broadcast(IsVisible());
			// ВАЖНО: Перезапускаем Idle, если персонаж стал видимым.
			if (IsVisible() && IdleAnimationManager)
			{
				VN_LOG_DEBUG(TEXT("OnAnimationFinished (SpawnDespawn): Restarting idle animations."));
				IdleAnimationManager->StartAllIdleAnimations();
			}
			break;
		}
			
	case EVNAnimationType::Focus:
		{
			OnCharacterFocusChanged.Broadcast(bIsInFocus);
			// Анимация фокуса не должна останавливать/запускать Idle.
			break;
		}

	default:
		VN_LOG_WARNING(TEXT("OnAnimationFinished: Unknown animation type: %d"), (int32)AnimationType);
		break;
	}
}

void AVNCharacter::OnAnimationProgress(EVNAnimationType AnimationType, float Progress)
{
	// Логируем только ключевые моменты прогресса для уменьшения спама
	if (FMath::IsNearlyEqual(Progress, 0.0f, 0.01f))
	{
		VN_LOG_DEBUG(TEXT("OnAnimationProgress: Animation type %d started (0%%)"), (int32)AnimationType);
	}
	else if (FMath::IsNearlyEqual(Progress, 0.25f, 0.01f))
	{
		VN_LOG_DEBUG(TEXT("OnAnimationProgress: Animation type %d at 25%%"), (int32)AnimationType);
	}
	else if (FMath::IsNearlyEqual(Progress, 0.5f, 0.01f))
	{
		VN_LOG_DEBUG(TEXT("OnAnimationProgress: Animation type %d at 50%%"), (int32)AnimationType);
	}
	else if (FMath::IsNearlyEqual(Progress, 0.75f, 0.01f))
	{
		VN_LOG_DEBUG(TEXT("OnAnimationProgress: Animation type %d at 75%%"), (int32)AnimationType);
	}
	else if (FMath::IsNearlyEqual(Progress, 1.0f, 0.01f))
	{
		VN_LOG_DEBUG(TEXT("OnAnimationProgress: Animation type %d completed (100%%)"), (int32)AnimationType);
	}
}

// =====================================================
// ЗАВЕРШЕНИЕ ПЕРЕХОДОВ
// =====================================================

void AVNCharacter::FinalizeCurrentTransition()
{
	VN_LOG_DEBUG(TEXT("FinalizeCurrentTransition: Starting cleanup of %d FadingIn and %d FadingOut components"), 
		FadingInComponents.Num(), FadingOutComponents.Num());

	// === ШАГ 1: Обработка компонентов FadingOut ===
	// Здесь логика не меняется, так как компоненты просто исчезают.
	for (TObjectPtr<USceneComponent>& Component : FadingOutComponents)
	{
		if (!Component || !IsValid(Component.Get()))
		{
			continue;
		}
		
		Component->SetHiddenInGame(true); // ИСПРАВЛЕНО
		if (USkeletalMeshComponent* SkeletalComp = Cast<USkeletalMeshComponent>(Component.Get()))
		{
			SkeletalComp->SetSkeletalMesh(nullptr);
		}
		else if (UPaperSpriteComponent* SpriteComp = Cast<UPaperSpriteComponent>(Component.Get()))
		{
			SpriteComp->SetSprite(nullptr);
		}
		ResetComponentAttachmentToDefault(Component.Get());
	}
	
	// === ШАГ 2: Обработка компонентов FadingIn ===
	for (TObjectPtr<USceneComponent>& Component : FadingInComponents)
	{
		if (!Component || !IsValid(Component.Get()))
		{
			continue;
		}
		
		float TargetAlpha = GetTargetAlpha(Component.Get());
		
		// === ИСПРАВЛЕНИЕ: Устанавливаем цвет из кэша с учетом фокуса ===
		// Вместо прямого применения цвета, мы используем новую систему,
		// которая возьмет базовый цвет из кэша и применит к нему эффект фокуса.
		ApplyComponentColorWithFocus(Component.Get());
		
		// Альфу все еще нужно установить, так как она может быть не 1.0
		// Наша новая система цвета не управляет альфой напрямую в этом контексте.
		SetComponentAlpha(Component.Get(), TargetAlpha);

		VN_LOG_DEBUG(TEXT("FinalizeCurrentTransition: Set final state for %s with cached color and target alpha %.2f"), 
			*Component->GetName(), TargetAlpha);
		
		bool bHasContent = false;
		if (USkeletalMeshComponent* SkeletalComp = Cast<USkeletalMeshComponent>(Component.Get()))
		{
			bHasContent = (SkeletalComp->GetSkeletalMeshAsset() != nullptr);
		}
		else if (UPaperSpriteComponent* SpriteComp = Cast<UPaperSpriteComponent>(Component.Get()))
		{
			bHasContent = (SpriteComp->GetSprite() != nullptr);
		}
		
		// Устанавливаем видимость в зависимости от наличия контента и альфы
		Component->SetHiddenInGame(!(bHasContent && TargetAlpha > 0.01f)); // ИСПРАВЛЕНО
	}
	
	// === ШАГ 3: Очистка анимационных данных ===
	// Логика не меняется
	for (TObjectPtr<USceneComponent>& Component : FadingInComponents)
	{
		if (Component && IsValid(Component.Get())) ClearAnimationAlphas(Component.Get());
	}
	for (TObjectPtr<USceneComponent>& Component : FadingOutComponents)
	{
		if (Component && IsValid(Component.Get())) ClearAnimationAlphas(Component.Get());
	}
	
	FadingInComponents.Empty();
	FadingOutComponents.Empty();
	
	VN_LOG_DEBUG(TEXT("FinalizeCurrentTransition: Cleanup complete with focus-aware colors applied"));
}

// =====================================================
// СИСТЕМА ФОКУСА
// =====================================================

void AVNCharacter::SetFocus(bool bInFocus, float Duration)
{
	if (bIsInFocus == bInFocus) return;

	if (AnimationManager && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::Focus)
	{
		AnimationManager->SkipCurrentAnimation();
	}

	bIsInFocus = bInFocus;

	if (Duration > 0.0f && AnimationManager)
	{
		AnimationManager->PlayFocus(bInFocus, Duration);
	}
	else
	{
		ApplyFocusStateImmediate();
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
		ApplyFocusStateImmediate();
	}
}

void AVNCharacter::ApplyFocusStateImmediate()
{
	// === ИСПРАВЛЕНИЕ: Используем новую централизованную функцию ===
	// Вместо ручного перебора компонентов, вызываем функцию, которая сделает это за нас.
	RefreshAllComponentColors();
	OnCharacterFocusChanged.Broadcast(bIsInFocus);
}

// =====================================================
// СИСТЕМА ВИДИМОСТИ
// =====================================================

void AVNCharacter::Appear(float Duration)
{
	if (AnimationManager && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::SpawnDespawn)
	{
		AnimationManager->SkipCurrentAnimation();
	}

	// 1. Принудительно скрываем все визуальные компоненты ПЕРЕД тем, как сделать актора видимым.
	// Это предотвращает мерцание на один кадр.
	HideAllVisualComponents();

	// 2. Делаем самого актора видимым. Его компоненты все еще скрыты, так что он "пустой".
	SetActorHiddenInGame(false);

	if (Duration > 0.0f && AnimationManager)
	{
		// 3. Запускаем анимацию. AnimationManager теперь будет работать с уже "чистым" состоянием.
		AnimationManager->PlaySpawnDespawn(true, Duration);
	}
	else
	{
		ApplyVisibilityStateImmediate(true);
	}
}

void AVNCharacter::Disappear(float Duration)
{
	if (AnimationManager && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::SpawnDespawn)
	{
		AnimationManager->SkipCurrentAnimation();
	}

	if (Duration > 0.0f && AnimationManager)
	{
		AnimationManager->PlaySpawnDespawn(false, Duration);
	}
	else
	{
		ApplyVisibilityStateImmediate(false);
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
		ApplyVisibilityStateImmediate(!IsHidden());
	}
}

void AVNCharacter::ApplyVisibilityStateImmediate(bool bShouldBeVisible)
{
	if (bShouldBeVisible)
	{
		SetActorHiddenInGame(false);
		
		// Показываем все ОСНОВНЫЕ компоненты с контентом
		TArray<USceneComponent*> AllComponents = GetAllMainComponents();
		for (USceneComponent* Component : AllComponents)
		{
			if (Component)
			{
				bool bHasContent = false;
				if (USkeletalMeshComponent* SkeletalComp = Cast<USkeletalMeshComponent>(Component))
				{
					bHasContent = (SkeletalComp->GetSkeletalMeshAsset() != nullptr);
				}
				else if (UPaperSpriteComponent* SpriteComp = Cast<UPaperSpriteComponent>(Component))
				{
					bHasContent = (SpriteComp->GetSprite() != nullptr);
				}
				
				if (bHasContent)
				{
					Component->SetVisibility(true);
					ApplyComponentColorWithFocus(Component);
				}
				else
				{
					Component->SetVisibility(false);
				}
			}
		}
		// Явно скрываем тень при мгновенном появлении
		if (BodyShadow_Sprite)
		{
			BodyShadow_Sprite->SetVisibility(false);
		}
	}
	else
	{
		// При мгновенном исчезновении скрываем всего актора, что скроет и тень
		SetActorHiddenInGame(true);
	}

	OnCharacterVisibilityChanged.Broadcast(bShouldBeVisible);
}



bool AVNCharacter::IsVisible() const
{
	return !IsHidden();
}

void AVNCharacter::HideAllVisualComponents()
{
	TArray<USceneComponent*> AllComps = GetAllMainComponents();
	if (BodyShadow_Sprite)
	{
		AllComps.Add(BodyShadow_Sprite);
	}

	for (USceneComponent* Comp : AllComps)
	{
		if (Comp)
		{
			Comp->SetVisibility(false, true); // Рекурсивно скрываем
		}
	}
	VN_LOG_DEBUG(TEXT("HideAllVisualComponents: All components forced to be invisible."));
}