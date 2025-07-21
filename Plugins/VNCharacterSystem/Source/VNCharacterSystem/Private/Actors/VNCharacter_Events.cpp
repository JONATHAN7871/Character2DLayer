#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"

// =====================================================
// ОБРАБОТЧИКИ СОБЫТИЙ АНИМАЦИИ
// =====================================================

void AVNCharacter::OnAnimationStarted(EVNAnimationType AnimationType)
{
	FString AnimationTypeName;
	switch (AnimationType)
	{
		case EVNAnimationType::None:
			AnimationTypeName = TEXT("None");
			break;
		case EVNAnimationType::Transition:
			AnimationTypeName = TEXT("Transition");
			break;
		case EVNAnimationType::SpawnDespawn:
			AnimationTypeName = TEXT("SpawnDespawn");
			break;
		case EVNAnimationType::Focus:
			AnimationTypeName = TEXT("Focus");
			break;
		default:
			AnimationTypeName = TEXT("Unknown");
			break;
	}

	VN_LOG_DEBUG(TEXT("Animation started: %s"), *AnimationTypeName);
}

void AVNCharacter::OnAnimationFinished(EVNAnimationType AnimationType)
{
	FString AnimationTypeName;
	switch (AnimationType)
	{
	case EVNAnimationType::None:
		AnimationTypeName = TEXT("None");
		break;
	case EVNAnimationType::Transition:
		AnimationTypeName = TEXT("Transition");
		break;
	case EVNAnimationType::SpawnDespawn:
		AnimationTypeName = TEXT("SpawnDespawn");
		break;
	case EVNAnimationType::Focus:
		AnimationTypeName = TEXT("Focus");
		break;
	default:
		AnimationTypeName = TEXT("Unknown");
		break;
	}

	VN_LOG_DEBUG(TEXT("Animation finished: %s"), *AnimationTypeName);
	
	switch (AnimationType)
	{
	case EVNAnimationType::Transition:
		{
			// ⚠️ ИСПРАВЛЕНИЕ: Добавляем фигурные скобки для создания локального scope
			// Завершаем все активные переходы
			HideAllFadeComponents();
			// Убеждаемся, что все основные компоненты имеют полную непрозрачность
			TArray<USceneComponent*> MainComponents = GetAllMainComponents();
			for (USceneComponent* Component : MainComponents)
			{
				if (Component && Component->IsVisible())
				{
					SetComponentAlpha(Component, 1.0f);
				}
			}
			break;
		}
	case EVNAnimationType::SpawnDespawn:
		OnCharacterVisibilityChanged.Broadcast(IsVisible());
		break;
			
	case EVNAnimationType::Focus:
		OnCharacterFocusChanged.Broadcast(bIsInFocus);
		break;

	default:
		break;
	}
}

void AVNCharacter::OnAnimationProgress(EVNAnimationType AnimationType, float Progress)
{
	// Анимация обрабатывается в AnimationManager
	// Здесь можем добавить дополнительную логику если нужно
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
			case EVNAnimationType::None:
				CurrentAnimationTypeName = TEXT("None");
				break;
			case EVNAnimationType::Transition:
				CurrentAnimationTypeName = TEXT("Transition");
				break;
			case EVNAnimationType::SpawnDespawn:
				CurrentAnimationTypeName = TEXT("SpawnDespawn");
				break;
			case EVNAnimationType::Focus:
				CurrentAnimationTypeName = TEXT("Focus");
				break;
			default:
				CurrentAnimationTypeName = TEXT("Unknown");
				break;
		}

		DebugInfo += FString::Printf(TEXT("Current Animation: %s\n"), *CurrentAnimationTypeName);
		DebugInfo += FString::Printf(TEXT("Animation Progress: %.2f%%\n"), 
			AnimationManager->GetCurrentAnimationProgress() * 100.0f);
		DebugInfo += FString::Printf(TEXT("Queued Animations: %d\n"), 
			AnimationManager->GetQueuedAnimationsCount());
	}
	
	// Информация о компонентах
	TArray<USceneComponent*> MainComponents = GetAllMainComponents();
	TArray<USceneComponent*> FadeComponents = GetAllFadeComponents();
	
	int32 VisibleMainCount = 0;
	int32 VisibleFadeCount = 0;
	
	for (USceneComponent* Component : MainComponents)
	{
		if (Component && Component->IsVisible())
		{
			VisibleMainCount++;
		}
	}
	
	for (USceneComponent* Component : FadeComponents)
	{
		if (Component && Component->IsVisible())
		{
			VisibleFadeCount++;
		}
	}
	
	DebugInfo += FString::Printf(TEXT("Main Components: %d total, %d visible\n"), MainComponents.Num(), VisibleMainCount);
	DebugInfo += FString::Printf(TEXT("Fade Components: %d total, %d visible\n"), FadeComponents.Num(), VisibleFadeCount);
	
	VN_LOG(Log, TEXT("%s"), *DebugInfo);
}

void AVNCharacter::ValidateAllComponents()
{
	TArray<FString> ValidationErrors;
	
	// Проверяем основные компоненты
	TArray<USceneComponent*> MainComponents = GetAllMainComponents();
	for (USceneComponent* Component : MainComponents)
	{
		if (!Component)
		{
			ValidationErrors.Add(TEXT("Main component is null"));
			continue;
		}
		
		// Для Skeletal Mesh компонентов
		if (USkeletalMeshComponent* SkeletalComp = Cast<USkeletalMeshComponent>(Component))
		{
			if (SkeletalComp->IsVisible() && !SkeletalComp->GetSkeletalMeshAsset())
			{
				ValidationErrors.Add(FString::Printf(TEXT("Skeletal component %s is visible but has no mesh"), 
					*SkeletalComp->GetName()));
			}
		}
		// Для Sprite компонентов
		else if (UPaperSpriteComponent* SpriteComp = Cast<UPaperSpriteComponent>(Component))
		{
			if (SpriteComp->IsVisible() && !SpriteComp->GetSprite())
			{
				ValidationErrors.Add(FString::Printf(TEXT("Sprite component %s is visible but has no sprite"), 
					*SpriteComp->GetName()));
			}
		}
	}
	
	// Проверяем fade компоненты (должны быть скрыты когда не используются)
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
			ValidationErrors.Add(FString::Printf(TEXT("Fade component %s is visible but no animation is running"), 
				*Component->GetName()));
		}
	}
	
	// Выводим результаты валидации
	if (ValidationErrors.Num() == 0)
	{
		VN_LOG(Log, TEXT("All components validation passed for %s"), *GetName());
	}
	else
	{
		FString ErrorMessage = FString::Printf(TEXT("Component validation failed for %s:"), *GetName());
		for (const FString& Error : ValidationErrors)
		{
			ErrorMessage += FString::Printf(TEXT("\n- %s"), *Error);
		}
		VN_LOG(Warning, TEXT("%s"), *ErrorMessage);
	}
}
#endif