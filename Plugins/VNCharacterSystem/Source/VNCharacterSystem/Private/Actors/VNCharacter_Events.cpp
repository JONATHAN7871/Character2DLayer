#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"

// =====================================================
// НОВАЯ ЦЕНТРАЛИЗОВАННАЯ ФУНКЦИЯ ОЧИСТКИ
// =====================================================

void AVNCharacter::FinalizeCurrentTransition()
{
	// --- ЛОГИКА ОЧИСТКИ ТЕПЕРЬ ЗДЕСЬ ---
	VN_LOG_DEBUG(TEXT("FinalizeCurrentTransition: Cleaning up %d FadingIn and %d FadingOut components."), FadingInComponents.Num(), FadingOutComponents.Num());

	// Завершаем все активные переходы
	for (USceneComponent* Component : FadingOutComponents)
	{
		if (Component)
		{
			Component->SetVisibility(false);
			SetComponentAlpha(Component, 0.0f);
			if (auto* SkeletalFade = Cast<USkeletalMeshComponent>(Component))
			{
				SkeletalFade->SetSkeletalMesh(nullptr);
				SkeletalFade->SetLeaderPoseComponent(nullptr);
			}
			else if (auto* SpriteFade = Cast<UPaperSpriteComponent>(Component))
			{
				SpriteFade->SetSprite(nullptr);
			}
			ResetComponentAttachmentToDefault(Component);
		}
	}

	// Убеждаемся, что все появившиеся компоненты имеют полную непрозрачность
	for (USceneComponent* Component : FadingInComponents)
	{
		if (Component && Component->IsVisible())
		{
			SetComponentAlpha(Component, 1.0f);
		}
	}

	// Очищаем списки для следующей анимации
	FadingInComponents.Empty();
	FadingOutComponents.Empty();
}

// =====================================================
// ОБРАБОТЧИКИ СОБЫТИЙ АНИМАЦИИ
// =====================================================

void AVNCharacter::OnAnimationStarted(EVNAnimationType AnimationType)
{
	VN_LOG_DEBUG(TEXT("OnAnimationStarted: Animation type %d started"), (int32)AnimationType);
	// Эта функция может быть расширена для пользовательской логики при старте анимации.
}

void AVNCharacter::OnAnimationFinished(EVNAnimationType AnimationType)
{
	VN_LOG_DEBUG(TEXT("OnAnimationFinished: Animation type %d finished"), (int32)AnimationType);
	
	switch (AnimationType)
	{
	case EVNAnimationType::Transition:
		{
			// Просто вызываем централизованную функцию очистки
			FinalizeCurrentTransition();
			break;
		}
	case EVNAnimationType::SpawnDespawn:
		VN_LOG_DEBUG(TEXT("OnAnimationFinished: SpawnDespawn animation finished. Character visible: %s"), IsVisible() ? TEXT("true") : TEXT("false"));
		OnCharacterVisibilityChanged.Broadcast(IsVisible());
		break;
			
	case EVNAnimationType::Focus:
		VN_LOG_DEBUG(TEXT("OnAnimationFinished: Focus animation finished. Character in focus: %s"), bIsInFocus ? TEXT("true") : TEXT("false"));
		OnCharacterFocusChanged.Broadcast(bIsInFocus);
		break;

	default:
		VN_LOG_WARNING(TEXT("OnAnimationFinished: Unknown animation type: %d"), (int32)AnimationType);
		break;
	}
}

void AVNCharacter::OnAnimationProgress(EVNAnimationType AnimationType, float Progress)
{
	// Анимация обрабатывается в AnimationManager
	// Здесь можно добавить дополнительную логику при необходимости.
	
	// Логируем только ключевые моменты, чтобы не спамить в лог
	if (FMath::IsNearlyEqual(Progress, 0.0f, 0.01f) || 
		FMath::IsNearlyEqual(Progress, 0.5f, 0.01f) || 
		FMath::IsNearlyEqual(Progress, 1.0f, 0.01f))
	{
		VN_LOG_DEBUG(TEXT("OnAnimationProgress: Animation type %d progress: %.1f%%"), (int32)AnimationType, Progress * 100.0f);
	}
}

// =====================================================
// ОТЛАДОЧНЫЕ МЕТОДЫ
// =====================================================

#if WITH_EDITOR
void AVNCharacter::PrintDebugInfo()
{
	FString DebugInfo = TEXT("=== VN Character Debug Info ===\n");
	
	DebugInfo += FString::Printf(TEXT("Actor: %s\n"), *GetName());
	DebugInfo += FString::Printf(TEXT("Character Name: %s\n"), *CharacterName);
	DebugInfo += FString::Printf(TEXT("Is In Focus: %s\n"), bIsInFocus ? TEXT("Yes") : TEXT("No"));
	DebugInfo += FString::Printf(TEXT("Is Visible: %s\n"), IsVisible() ? TEXT("Yes") : TEXT("No"));
	DebugInfo += FString::Printf(TEXT("Is Animating: %s\n"), IsAnimating() ? TEXT("Yes") : TEXT("No"));
	
	if (AnimationManager)
	{
		FString CurrentAnimationTypeName;
		EVNAnimationType CurrentAnimationType = AnimationManager->GetCurrentAnimationType();
		switch (CurrentAnimationType)
		{
			case EVNAnimationType::None: CurrentAnimationTypeName = TEXT("None"); break;
			case EVNAnimationType::Transition: CurrentAnimationTypeName = TEXT("Transition"); break;
			case EVNAnimationType::SpawnDespawn: CurrentAnimationTypeName = TEXT("SpawnDespawn"); break;
			case EVNAnimationType::Focus: CurrentAnimationTypeName = TEXT("Focus"); break;
			default: CurrentAnimationTypeName = TEXT("Unknown"); break;
		}

		DebugInfo += FString::Printf(TEXT("Current Animation: %s\n"), *CurrentAnimationTypeName);
		DebugInfo += FString::Printf(TEXT("Animation Progress: %.2f%%\n"), AnimationManager->GetCurrentAnimationProgress() * 100.0f);
		DebugInfo += FString::Printf(TEXT("Queued Animations: %d\n"), AnimationManager->GetQueuedAnimationsCount());
	}
	
	TArray<USceneComponent*> MainComponents = GetAllMainComponents();
	TArray<USceneComponent*> FadeComponents = GetAllFadeComponents();
	
	int32 VisibleMainCount = 0;
	for (USceneComponent* Component : MainComponents)
	{
		if (Component && Component->IsVisible()) VisibleMainCount++;
	}
	
	int32 VisibleFadeCount = 0;
	for (USceneComponent* Component : FadeComponents)
	{
		if (Component && Component->IsVisible()) VisibleFadeCount++;
	}
	
	DebugInfo += FString::Printf(TEXT("Main Components: %d total, %d visible\n"), MainComponents.Num(), VisibleMainCount);
	DebugInfo += FString::Printf(TEXT("Fade Components: %d total, %d visible\n"), FadeComponents.Num(), VisibleFadeCount);
	
	// Добавляем информацию о компонентах в анимации
	DebugInfo += FString::Printf(TEXT("Fading In Components: %d\n"), FadingInComponents.Num());
	DebugInfo += FString::Printf(TEXT("Fading Out Components: %d\n"), FadingOutComponents.Num());
	
	// Детальная информация о компонентах в анимации
	if (FadingInComponents.Num() > 0)
	{
		DebugInfo += TEXT("FadingIn Components:\n");
		for (USceneComponent* Component : FadingInComponents)
		{
			if (Component)
			{
				DebugInfo += FString::Printf(TEXT("  - %s (Visible: %s)\n"), 
					*Component->GetName(), Component->IsVisible() ? TEXT("Yes") : TEXT("No"));
			}
		}
	}
	
	if (FadingOutComponents.Num() > 0)
	{
		DebugInfo += TEXT("FadingOut Components:\n");
		for (USceneComponent* Component : FadingOutComponents)
		{
			if (Component)
			{
				DebugInfo += FString::Printf(TEXT("  - %s (Visible: %s)\n"), 
					*Component->GetName(), Component->IsVisible() ? TEXT("Yes") : TEXT("No"));
			}
		}
	}
	
	VN_LOG(Log, TEXT("%s"), *DebugInfo);
}

void AVNCharacter::ValidateAllComponents()
{
	TArray<FString> ValidationErrors;
	
	VN_LOG_DEBUG(TEXT("ValidateAllComponents: Starting validation for character %s"), *GetName());
	
	TArray<USceneComponent*> MainComponents = GetAllMainComponents();
	for (USceneComponent* Component : MainComponents)
	{
		if (!Component)
		{
			ValidationErrors.Add(TEXT("Main component is null"));
			continue;
		}
		
		if (USkeletalMeshComponent* SkeletalComp = Cast<USkeletalMeshComponent>(Component))
		{
			if (SkeletalComp->IsVisible() && !SkeletalComp->GetSkeletalMeshAsset())
			{
				ValidationErrors.Add(FString::Printf(TEXT("Skeletal component %s is visible but has no mesh"), *SkeletalComp->GetName()));
			}
		}
		else if (UPaperSpriteComponent* SpriteComp = Cast<UPaperSpriteComponent>(Component))
		{
			if (SpriteComp->IsVisible() && !SpriteComp->GetSprite())
			{
				ValidationErrors.Add(FString::Printf(TEXT("Sprite component %s is visible but has no sprite"), *SpriteComp->GetName()));
			}
		}
	}
	
	TArray<USceneComponent*> FadeComponents = GetAllFadeComponents();
	for (USceneComponent* Component : FadeComponents)
	{
		if (!Component)
		{
			ValidationErrors.Add(TEXT("Fade component is null"));
			continue;
		}
		if (Component->IsVisible() && !IsAnimating())
		{
			ValidationErrors.Add(FString::Printf(TEXT("Fade component %s is visible but no animation is running"), *Component->GetName()));
		}
	}
	
	// Проверяем состояние списков анимации
	if (FadingInComponents.Num() > 0 && !IsAnimating())
	{
		ValidationErrors.Add(FString::Printf(TEXT("FadingInComponents contains %d components but no animation is running"), FadingInComponents.Num()));
	}
	
	if (FadingOutComponents.Num() > 0 && !IsAnimating())
	{
		ValidationErrors.Add(FString::Printf(TEXT("FadingOutComponents contains %d components but no animation is running"), FadingOutComponents.Num()));
	}
	
	if (ValidationErrors.Num() == 0)
	{
		VN_LOG(Log, TEXT("ValidateAllComponents: All components validation passed for %s"), *GetName());
	}
	else
	{
		FString ErrorMessage = FString::Printf(TEXT("ValidateAllComponents: Component validation failed for %s:"), *GetName());
		for (const FString& Error : ValidationErrors)
		{
			ErrorMessage += FString::Printf(TEXT("\n- %s"), *Error);
		}
		VN_LOG(Warning, TEXT("%s"), *ErrorMessage);
	}
}
#endif