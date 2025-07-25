#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"

// =====================================================
// ИСПРАВЛЕННАЯ ЦЕНТРАЛИЗОВАННАЯ ФУНКЦИЯ ОЧИСТКИ
// =====================================================

void AVNCharacter::FinalizeCurrentTransition()
{
	UE_LOG(LogTemp, Warning, TEXT("FinalizeCurrentTransition: Cleaning up %d FadingIn and %d FadingOut components."), 
		FadingInComponents.Num(), FadingOutComponents.Num());

	// ПЕРВЫЙ ЭТАП: Обрабатываем FadingIn компоненты
	// Применяем целевую альфу ПЕРЕД очисткой fade компонентов
	for (USceneComponent* Component : FadingInComponents)
	{
		if (Component && Component->IsVisible())
		{
			// Применяем целевую альфу из системы анимации
			float TargetAlpha = GetTargetAlpha(Component);
			SetAnimationAlpha(Component, TargetAlpha);
			
			UE_LOG(LogTemp, Warning, TEXT("FinalizeCurrentTransition: Applied target alpha %.2f to %s"), 
				TargetAlpha, *Component->GetName());
		}
	}

	// ВТОРОЙ ЭТАП: Обрабатываем FadingOut компоненты
	for (USceneComponent* Component : FadingOutComponents)
	{
		if (Component)
		{
			// КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Правильно очищаем контент
			if (auto* SkeletalComp = Cast<USkeletalMeshComponent>(Component))
			{
				// Если это основной компонент (не fade) - очищаем его
				if (Component == Body_Skeletal || Component == Arms_Skeletal || Component == Head_Skeletal ||
					Component == Custom01_Skeletal || Component == Custom02_Skeletal || Component == Custom03_Skeletal)
				{
					UE_LOG(LogTemp, Log, TEXT("FinalizeCurrentTransition: Clearing main skeletal component %s"), *Component->GetName());
					SkeletalComp->SetSkeletalMesh(nullptr);
					SkeletalComp->SetAnimInstanceClass(nullptr);
					SkeletalComp->SetVisibility(false);
				}
				else
				{
					// Это fade компонент - просто очищаем и скрываем
					UE_LOG(LogTemp, Log, TEXT("FinalizeCurrentTransition: Clearing fade skeletal component %s"), *Component->GetName());
					SkeletalComp->SetSkeletalMesh(nullptr);
					SkeletalComp->SetAnimInstanceClass(nullptr);
					SkeletalComp->SetLeaderPoseComponent(nullptr);
					SkeletalComp->SetVisibility(false);
				}
			}
			else if (auto* SpriteComp = Cast<UPaperSpriteComponent>(Component))
			{
				// Если это основной компонент (не fade) - очищаем его
				if (Component == Body_Sprite || Component == Arms_Sprite || Component == Head_Sprite ||
					Component == Eyebrow_Sprite || Component == Eyes_Sprite || Component == Eyelids_Sprite ||
					Component == Wink_Sprite || Component == Mouth_Sprite || Component == BodyShadow_Sprite ||
					Component == EmotionHeadEffect01_Sprite || Component == EmotionHeadEffect02_Sprite || Component == EmotionHeadEffect03_Sprite ||
					Component == EmotionBodyEffect01_Sprite || Component == EmotionBodyEffect02_Sprite || Component == EmotionBodyEffect03_Sprite)
				{
					UE_LOG(LogTemp, Log, TEXT("FinalizeCurrentTransition: Clearing main sprite component %s"), *Component->GetName());
					SpriteComp->SetSprite(nullptr);
					SpriteComp->SetVisibility(false);
				}
				else
				{
					// Это fade компонент - просто очищаем и скрываем
					UE_LOG(LogTemp, Log, TEXT("FinalizeCurrentTransition: Clearing fade sprite component %s"), *Component->GetName());
					SpriteComp->SetSprite(nullptr);
					SpriteComp->SetVisibility(false);
				}
			}
			
			// Сбрасываем альфу и attachment
			SetAnimationAlpha(Component, 0.0f);
			ResetComponentAttachmentToDefault(Component);
			
			UE_LOG(LogTemp, Log, TEXT("FinalizeCurrentTransition: Cleaned up FadingOut component %s"), *Component->GetName());
		}
	}

	// ТРЕТИЙ ЭТАП: Очищаем данные анимации для ВСЕХ компонентов
	for (USceneComponent* Component : FadingInComponents)
	{
		if (Component)
		{
			ClearAnimationAlphas(Component);
		}
	}
	
	for (USceneComponent* Component : FadingOutComponents)
	{
		if (Component)
		{
			ClearAnimationAlphas(Component);
		}
	}

	// Очищаем списки для следующей анимации
	FadingInComponents.Empty();
	FadingOutComponents.Empty();
	
	UE_LOG(LogTemp, Warning, TEXT("FinalizeCurrentTransition: Transition cleanup complete"));
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