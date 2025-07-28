// VNCharacter_Animation.cpp - Система анимаций, событий и управления фокусом

#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
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
			VN_LOG_DEBUG(TEXT("OnAnimationStarted: Transition animation started with %d FadingIn and %d FadingOut components"), 
				FadingInComponents.Num(), FadingOutComponents.Num());
			break;
			
		case EVNAnimationType::SpawnDespawn:
			VN_LOG_DEBUG(TEXT("OnAnimationStarted: SpawnDespawn animation started"));
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
			// Завершаем переход с правильной очисткой состояний
			FinalizeCurrentTransition();
			break;
		}
		
		case EVNAnimationType::SpawnDespawn:
		{
			VN_LOG_DEBUG(TEXT("OnAnimationFinished: SpawnDespawn animation finished. Character visible: %s"), 
				IsVisible() ? TEXT("true") : TEXT("false"));
			OnCharacterVisibilityChanged.Broadcast(IsVisible());
			break;
		}
			
		case EVNAnimationType::Focus:
		{
			VN_LOG_DEBUG(TEXT("OnAnimationFinished: Focus animation finished. Character in focus: %s"), 
				bIsInFocus ? TEXT("true") : TEXT("false"));
			OnCharacterFocusChanged.Broadcast(bIsInFocus);
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
	for (TObjectPtr<USceneComponent>& Component : FadingOutComponents)
	{
		if (!Component || !IsValid(Component.Get()))
		{
			VN_LOG_WARNING(TEXT("FinalizeCurrentTransition: Invalid component in FadingOutComponents"));
			continue;
		}
		
		SetAnimationAlpha(Component.Get(), 0.0f);
		Component->SetVisibility(false);
		
		if (USkeletalMeshComponent* SkeletalComp = Cast<USkeletalMeshComponent>(Component.Get()))
		{
			SkeletalComp->SetSkeletalMesh(nullptr);
			SkeletalComp->SetAnimInstanceClass(nullptr);
			SkeletalComp->SetLeaderPoseComponent(nullptr);
			VN_LOG_DEBUG(TEXT("FinalizeCurrentTransition: Cleared skeletal mesh for %s"), *Component->GetName());
		}
		else if (UPaperSpriteComponent* SpriteComp = Cast<UPaperSpriteComponent>(Component.Get()))
		{
			SpriteComp->SetSprite(nullptr);
			VN_LOG_DEBUG(TEXT("FinalizeCurrentTransition: Cleared sprite for %s"), *Component->GetName());
		}
		
		ResetComponentAttachmentToDefault(Component.Get());
	}
	
	// === ШАГ 2: Обработка компонентов FadingIn ===
	for (TObjectPtr<USceneComponent>& Component : FadingInComponents)
	{
		if (!Component || !IsValid(Component.Get()))
		{
			VN_LOG_WARNING(TEXT("FinalizeCurrentTransition: Invalid component in FadingInComponents"));
			continue;
		}
		
		float TargetAlpha = GetTargetAlpha(Component.Get());
		SetAnimationAlpha(Component.Get(), TargetAlpha);
		
		// Применяем финальный цвет с учетом фокуса
		SetComponentColor(Component.Get(), GetTargetColorForComponent(Component.Get()));
		
		VN_LOG_DEBUG(TEXT("FinalizeCurrentTransition: Set final alpha %.2f and focus-aware color for %s"), 
			TargetAlpha, *Component->GetName());
		
		bool bHasContent = false;
		if (USkeletalMeshComponent* SkeletalComp = Cast<USkeletalMeshComponent>(Component.Get()))
		{
			bHasContent = (SkeletalComp->GetSkeletalMeshAsset() != nullptr);
		}
		else if (UPaperSpriteComponent* SpriteComp = Cast<UPaperSpriteComponent>(Component.Get()))
		{
			bHasContent = (SpriteComp->GetSprite() != nullptr);
		}
		
		if (bHasContent && TargetAlpha > 0.01f)
		{
			Component->SetVisibility(true);
		}
	}
	
	// === ШАГ 3: Очистка анимационных данных ===
	for (TObjectPtr<USceneComponent>& Component : FadingInComponents)
	{
		if (Component && IsValid(Component.Get()))
		{
			ClearAnimationAlphas(Component.Get());
		}
	}
	
	for (TObjectPtr<USceneComponent>& Component : FadingOutComponents)
	{
		if (Component && IsValid(Component.Get()))
		{
			ClearAnimationAlphas(Component.Get());
		}
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
	if (bIsInFocus == bInFocus)
	{
		VN_LOG_DEBUG(TEXT("SetFocus: Focus state unchanged (%s)"), bInFocus ? TEXT("In Focus") : TEXT("Out of Focus"));
		return;
	}

	VN_LOG_DEBUG(TEXT("SetFocus: Changing focus to %s with duration %.2f"), 
		bInFocus ? TEXT("In Focus") : TEXT("Out of Focus"), Duration);

	// Если есть активная анимация фокуса, пропускаем её
	if (AnimationManager && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::Focus)
	{
		AnimationManager->SkipCurrentAnimation();
	}

	// Устанавливаем новое состояние фокуса
	bIsInFocus = bInFocus;

	// Запускаем анимацию
	if (Duration > 0.0f && AnimationManager)
	{
		AnimationManager->PlayFocus(bInFocus, Duration);
	}
	else
	{
		// Мгновенное применение
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
	// Применяем цвета мгновенно ко всем основным компонентам
	TArray<USceneComponent*> AllComponents = GetAllMainComponents();
	for (USceneComponent* Component : AllComponents)
	{
		if (Component && Component->IsVisible())
		{
			FLinearColor TargetColor = GetTargetColorForComponent(Component);
			SetComponentColor(Component, TargetColor);
		}
	}

	// Уведомляем о смене фокуса
	OnCharacterFocusChanged.Broadcast(bIsInFocus);
}

// =====================================================
// СИСТЕМА ВИДИМОСТИ
// =====================================================

void AVNCharacter::Appear(float Duration)
{
	VN_LOG_DEBUG(TEXT("Appear: Starting appear animation with duration %.2f"), Duration);

	// Если есть активная анимация появления/исчезновения, пропускаем её
	if (AnimationManager && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::SpawnDespawn)
	{
		AnimationManager->SkipCurrentAnimation();
	}

	// Показываем актора
	SetActorHiddenInGame(false);

	// Запускаем анимацию появления
	if (Duration > 0.0f && AnimationManager)
	{
		AnimationManager->PlaySpawnDespawn(true, Duration);
	}
	else
	{
		// Мгновенное появление
		ApplyVisibilityStateImmediate(true);
	}
}

void AVNCharacter::Disappear(float Duration)
{
	VN_LOG_DEBUG(TEXT("Disappear: Starting disappear animation with duration %.2f"), Duration);

	// Если есть активная анимация появления/исчезновения, пропускаем её
	if (AnimationManager && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::SpawnDespawn)
	{
		AnimationManager->SkipCurrentAnimation();
	}

	// Запускаем анимацию исчезновения
	if (Duration > 0.0f && AnimationManager)
	{
		AnimationManager->PlaySpawnDespawn(false, Duration);
	}
	else
	{
		// Мгновенное исчезновение
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
		// Мгновенное скрытие/показ в зависимости от текущего состояния
		bool bShouldBeVisible = !IsHidden();
		ApplyVisibilityStateImmediate(bShouldBeVisible);
	}
}

void AVNCharacter::ApplyVisibilityStateImmediate(bool bShouldBeVisible)
{
	if (bShouldBeVisible)
	{
		// Показываем все основные компоненты с правильными цветами
		TArray<USceneComponent*> AllComponents = GetAllMainComponents();
		for (USceneComponent* Component : AllComponents)
		{
			if (Component && Component != BodyShadow_Sprite)
			{
				Component->SetVisibility(true);
				FLinearColor TargetColor = GetTargetColorForComponent(Component);
				SetComponentColor(Component, TargetColor);
			}
		}
		BodyShadow_Sprite->SetVisibility(false);
	}
	else
	{
		// Скрываем актора
		SetActorHiddenInGame(true);
	}

	// Уведомляем о смене видимости
	OnCharacterVisibilityChanged.Broadcast(bShouldBeVisible);
}

bool AVNCharacter::IsVisible() const
{
	return !IsHidden();
}