#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"

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
		SkipFocusAnimation();
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
		SkipSpawnDespawnAnimation();
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
		SkipSpawnDespawnAnimation();
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
}

bool AVNCharacter::IsVisible() const
{
	return !IsHidden();
}