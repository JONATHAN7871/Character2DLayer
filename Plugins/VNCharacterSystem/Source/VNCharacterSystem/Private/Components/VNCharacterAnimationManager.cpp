#include "Components/VNCharacterAnimationManager.h"
#include "Actors/VNCharacter.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Data/VNCharacterTypes.h"

UVNCharacterAnimationManager::UVNCharacterAnimationManager()
{
	// Включаем тик для обновления анимаций
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	// Инициализация значений по умолчанию
	CurrentAnimationTime = 0.0f;
	MaxQueueSize = 10;
	bAutoSkipOnQueueOverflow = true;
	MinAnimationDuration = 0.1f;
	bDisableAnimations = false;
	bVerboseLogging = false;

	// Инициализация пустого состояния анимации
	CurrentAnimation.AnimationType = EVNAnimationType::None;
	CurrentAnimation.Duration = 0.0f;
}

void UVNCharacterAnimationManager::BeginPlay()
{
	Super::BeginPlay();

	// Кэшируем ссылку на владельца
	OwnerCharacter = Cast<AVNCharacter>(GetOwner());
	
	if (!OwnerCharacter.IsValid())
	{
		VN_LOG_ERROR(TEXT("VNCharacterAnimationManager: Owner is not a VNCharacter! Component will not function properly."));
	}
	else
	{
		LogAnimation(TEXT("Animation Manager initialized successfully"));
	}
}

void UVNCharacterAnimationManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Обновляем текущую анимацию
	if (IsAnimating())
	{
		UpdateCurrentAnimation(DeltaTime);
	}
	else
	{
		// Если анимация не выполняется, проверяем очередь
		ProcessAnimationQueue();
	}
}

// =====================================================
// ПУБЛИЧНЫЕ МЕТОДЫ
// =====================================================

void UVNCharacterAnimationManager::PlayTransition(float Duration)
{
	FVNAnimationRequest Request;
	Request.AnimationType = EVNAnimationType::Transition;
	Request.Duration = FMath::Max(Duration, MinAnimationDuration);

	EnqueueAnimationRequest(Request);
}

void UVNCharacterAnimationManager::PlaySpawnDespawn(bool bAppear, float Duration)
{
	FVNAnimationRequest Request;
	Request.AnimationType = EVNAnimationType::SpawnDespawn;
	Request.Duration = FMath::Max(Duration, MinAnimationDuration);
	Request.bIsSpawnAnimation = bAppear;

	EnqueueAnimationRequest(Request);
}

void UVNCharacterAnimationManager::PlayFocus(bool bInFocus, float Duration)
{
	FVNAnimationRequest Request;
	Request.AnimationType = EVNAnimationType::Focus;
	Request.Duration = FMath::Max(Duration, MinAnimationDuration);
	Request.bIsInFocus = bInFocus;

	EnqueueAnimationRequest(Request);
}

void UVNCharacterAnimationManager::SkipCurrentAnimation()
{
	if (!IsAnimating())
	{
		LogAnimation(TEXT("SkipCurrentAnimation called but no animation is playing"));
		return;
	}

	LogAnimation(FString::Printf(TEXT("Skipping animation: %s"), *CurrentAnimation.ToString()));

	// Завершаем анимацию с прогрессом 1.0
	CurrentAnimationTime = CurrentAnimation.Duration;
	UpdateCurrentAnimation(0.0f);
}

void UVNCharacterAnimationManager::ClearAnimationQueue()
{
	LogAnimation(FString::Printf(TEXT("Clearing animation queue (%d items)"), AnimationQueue.Num()));

	// Останавливаем текущую анимацию
	if (IsAnimating())
	{
		FinishCurrentAnimation();
	}

	// Очищаем очередь
	AnimationQueue.Empty();
}

float UVNCharacterAnimationManager::GetCurrentAnimationProgress() const
{
	if (!IsAnimating())
	{
		return -1.0f;
	}

	if (CurrentAnimation.Duration <= 0.0f)
	{
		return 1.0f;
	}

	return FMath::Clamp(CurrentAnimationTime / CurrentAnimation.Duration, 0.0f, 1.0f);
}

// =====================================================
// ВНУТРЕННИЕ МЕТОДЫ
// =====================================================

void UVNCharacterAnimationManager::ProcessAnimationQueue()
{
	if (AnimationQueue.Num() == 0)
	{
		return;
	}

	// Берем первый запрос из очереди
	FVNAnimationRequest NextRequest = AnimationQueue[0];
	AnimationQueue.RemoveAt(0);

	// Запускаем анимацию
	StartAnimation(NextRequest);
}

void UVNCharacterAnimationManager::StartAnimation(const FVNAnimationRequest& Request)
{
	if (!ValidateAnimationRequest(Request))
	{
		VN_LOG_WARNING(TEXT("Invalid animation request: %s"), *Request.ToString());
		return;
	}

	if (bDisableAnimations)
	{
		LogAnimation(TEXT("Animations disabled, skipping: ") + Request.ToString());
		return;
	}

	AVNCharacter* Character = GetVNCharacterOwner();
	if (!Character)
	{
		VN_LOG_ERROR(TEXT("Cannot start animation - VNCharacter owner not found"));
		return;
	}

	// Устанавливаем текущую анимацию
	CurrentAnimation = Request;
	CurrentAnimationTime = 0.0f;

	LogAnimation(FString::Printf(TEXT("Starting animation: %s"), *Request.ToString()));

	// Уведомляем о начале анимации
	OnAnimationStarted.Broadcast(Request.AnimationType);
}

void UVNCharacterAnimationManager::UpdateCurrentAnimation(float DeltaTime)
{
	if (!IsAnimating())
	{
		return;
	}

	// Обновляем время
	CurrentAnimationTime += DeltaTime;
	
	// Вычисляем прогресс
	float Alpha = FMath::Clamp(CurrentAnimationTime / CurrentAnimation.Duration, 0.0f, 1.0f);

	// Уведомляем о прогрессе
	OnAnimationProgress.Broadcast(CurrentAnimation.AnimationType, Alpha);

	// Обновляем специфичную анимацию
	switch (CurrentAnimation.AnimationType)
	{
		case EVNAnimationType::Transition:
			UpdateTransitionAnimation(Alpha);
			break;

		case EVNAnimationType::SpawnDespawn:
			UpdateSpawnDespawnAnimation(Alpha);
			break;

		case EVNAnimationType::Focus:
			UpdateFocusAnimation(Alpha);
			break;

		default:
			VN_LOG_WARNING(TEXT("Unknown animation type in UpdateCurrentAnimation"));
			break;
	}

	// Проверяем завершение анимации
	if (Alpha >= 1.0f)
	{
		FinishCurrentAnimation();
	}
}

void UVNCharacterAnimationManager::FinishCurrentAnimation()
{
	if (!IsAnimating())
	{
		return;
	}

	EVNAnimationType FinishedType = CurrentAnimation.AnimationType;
	
	LogAnimation(FString::Printf(TEXT("Finishing animation: %s"), *CurrentAnimation.ToString()));

	// Уведомляем о завершении анимации
	OnAnimationFinished.Broadcast(FinishedType);

	// Сбрасываем текущую анимацию
	CurrentAnimation.AnimationType = EVNAnimationType::None;
	CurrentAnimation.Duration = 0.0f;
	CurrentAnimationTime = 0.0f;
}

bool UVNCharacterAnimationManager::EnqueueAnimationRequest(const FVNAnimationRequest& Request)
{
	if (!ValidateAnimationRequest(Request))
	{
		VN_LOG_WARNING(TEXT("Cannot enqueue invalid animation request: %s"), *Request.ToString());
		return false;
	}

	// Проверяем переполнение очереди
	if (AnimationQueue.Num() >= MaxQueueSize)
	{
		if (bAutoSkipOnQueueOverflow)
		{
			LogAnimation(TEXT("Queue overflow, skipping current animation to make space"));
			SkipCurrentAnimation();
		}
		else
		{
			VN_LOG_WARNING(TEXT("Animation queue is full (%d/%d), request rejected"), 
				AnimationQueue.Num(), MaxQueueSize);
			return false;
		}
	}

	// Добавляем в очередь
	AnimationQueue.Add(Request);
	
	LogAnimation(FString::Printf(TEXT("Enqueued animation: %s (Queue size: %d)"), 
		*Request.ToString(), AnimationQueue.Num()));

	return true;
}

// =====================================================
// СПЕЦИАЛИЗИРОВАННЫЕ МЕТОДЫ АНИМАЦИИ (ПЕРЕРАБОТАННЫЕ)
// =====================================================

void UVNCharacterAnimationManager::UpdateTransitionAnimation(float Alpha)
{
	AVNCharacter* Character = GetVNCharacterOwner();
	if (!Character)
	{
		return;
	}

	// --- НОВАЯ ЛОГИКА ---
	// Анимируем только те компоненты, которые были помечены для перехода

	// Fade Out: анимируем компоненты из списка FadingOutComponents
	for (USceneComponent* FadeComponent : Character->GetFadingOutComponents())
	{
		if (FadeComponent)
		{
			Character->SetComponentAlpha(FadeComponent, 1.0f - Alpha);
		}
	}

	// Fade In: анимируем компоненты из списка FadingInComponents
	for (USceneComponent* MainComponent : Character->GetFadingInComponents())
	{
		if (MainComponent)
		{
			Character->SetComponentAlpha(MainComponent, Alpha);
		}
	}
	
	LogAnimation(FString::Printf(TEXT("Transition progress: %.2f (FadingIn: %d, FadingOut: %d)"), 
		Alpha, Character->GetFadingInComponents().Num(), Character->GetFadingOutComponents().Num()), false);
}

void UVNCharacterAnimationManager::UpdateSpawnDespawnAnimation(float Alpha)
{
	AVNCharacter* Character = GetVNCharacterOwner();
	if (!Character)
	{
		return;
	}

	if (CurrentAnimation.bIsSpawnAnimation)
	{
		// ПОЯВЛЕНИЕ (Appear) - двухфазная анимация
		if (Alpha <= 0.5f)
		{
			// ФАЗА 1: BodyShadow появляется (0-50% времени)
			float PhaseAlpha = Alpha * 2.0f; // Преобразуем 0-0.5 в 0-1
			
			// Скрываем все основные компоненты кроме BodyShadow
			TArray<USceneComponent*> AllComponents = Character->GetAllMainComponents();
			for (USceneComponent* Component : AllComponents)
			{
				if (Component && Component != Character->BodyShadow_Sprite)
				{
					Component->SetVisibility(false);
				}
			}
			
			// Показываем и анимируем BodyShadow
			if (Character->BodyShadow_Sprite)
			{
				Character->BodyShadow_Sprite->SetVisibility(true);
				Character->SetComponentAlpha(Character->BodyShadow_Sprite, PhaseAlpha);
			}
			
			LogAnimation(FString::Printf(TEXT("Appear Phase 1: %.2f"), PhaseAlpha), false);
		}
		else
		{
			// ФАЗА 2: Все остальные компоненты появляются (50-100% времени)
			float PhaseAlpha = (Alpha - 0.5f) * 2.0f; // Преобразуем 0.5-1 в 0-1
			
			// Скрываем BodyShadow
			if (Character->BodyShadow_Sprite)
			{
				Character->BodyShadow_Sprite->SetVisibility(false);
			}
			
			// Показываем все остальные компоненты и анимируем цвет от черного к целевому
			TArray<USceneComponent*> AllComponents = Character->GetAllMainComponents();
			for (USceneComponent* Component : AllComponents)
			{
				if (Component && Component != Character->BodyShadow_Sprite)
				{
					// Проверяем, есть ли содержимое в компоненте
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
						
						// Интерполируем цвет от черного к целевому
						FLinearColor TargetColor = Character->GetTargetColorForComponent(Component);
						FLinearColor BlackColor = FLinearColor::Black;
						BlackColor.A = TargetColor.A; // Сохраняем альфу
						
						FLinearColor CurrentColor = FMath::Lerp(BlackColor, TargetColor, PhaseAlpha);
						Character->SetComponentColor(Component, CurrentColor);
					}
				}
			}
			
			LogAnimation(FString::Printf(TEXT("Appear Phase 2: %.2f"), PhaseAlpha), false);
		}
	}
	else
	{
		// ИСЧЕЗНОВЕНИЕ (Disappear) - двухфазная анимация
		if (Alpha <= 0.5f)
		{
			// ФАЗА 1: Все компоненты темнеют до черного (0-50% времени)
			float PhaseAlpha = Alpha * 2.0f; // Преобразуем 0-0.5 в 0-1
			
			// Скрываем BodyShadow
			if (Character->BodyShadow_Sprite)
			{
				Character->BodyShadow_Sprite->SetVisibility(false);
			}
			
			// Затемняем все остальные компоненты
			TArray<USceneComponent*> AllComponents = Character->GetAllMainComponents();
			for (USceneComponent* Component : AllComponents)
			{
				if (Component && Component != Character->BodyShadow_Sprite && Component->IsVisible())
				{
					// Интерполируем цвет от целевого к черному
					FLinearColor TargetColor = Character->GetTargetColorForComponent(Component);
					FLinearColor BlackColor = FLinearColor::Black;
					BlackColor.A = TargetColor.A; // Сохраняем альфу
					
					FLinearColor CurrentColor = FMath::Lerp(TargetColor, BlackColor, PhaseAlpha);
					Character->SetComponentColor(Component, CurrentColor);
				}
			}
			
			LogAnimation(FString::Printf(TEXT("Disappear Phase 1: %.2f"), PhaseAlpha), false);
		}
		else
		{
			// ФАЗА 2: BodyShadow исчезает (50-100% времени)
			float PhaseAlpha = (Alpha - 0.5f) * 2.0f; // Преобразуем 0.5-1 в 0-1
			
			// Скрываем все компоненты кроме BodyShadow
			TArray<USceneComponent*> AllComponents = Character->GetAllMainComponents();
			for (USceneComponent* Component : AllComponents)
			{
				if (Component && Component != Character->BodyShadow_Sprite)
				{
					Component->SetVisibility(false);
				}
			}
			
			// Показываем и анимируем исчезновение BodyShadow
			if (Character->BodyShadow_Sprite)
			{
				Character->BodyShadow_Sprite->SetVisibility(true);
				Character->SetComponentAlpha(Character->BodyShadow_Sprite, 1.0f - PhaseAlpha);
			}
			
			LogAnimation(FString::Printf(TEXT("Disappear Phase 2: %.2f"), PhaseAlpha), false);
			
			// В конце анимации скрываем весь актор
			if (PhaseAlpha >= 1.0f)
			{
				Character->SetActorHiddenInGame(true);
			}
		}
	}
}

void UVNCharacterAnimationManager::UpdateFocusAnimation(float Alpha)
{
	AVNCharacter* Character = GetVNCharacterOwner();
	if (!Character)
	{
		return;
	}

	TArray<USceneComponent*> AllComponents = Character->GetAllMainComponents();
	
	for (USceneComponent* Component : AllComponents)
	{
		if (Component && Component->IsVisible())
		{
			FLinearColor BaseColor = Character->GetBaseColorForComponent(Component);
			FLinearColor DimmedColor = BaseColor * Character->DimColorMultiplier;
			
			// Определяем начальный и конечный цвета
			FLinearColor StartColor = CurrentAnimation.bIsInFocus ? DimmedColor : BaseColor;
			FLinearColor EndColor = CurrentAnimation.bIsInFocus ? BaseColor : DimmedColor;
			
			// Интерполируем цвет
			FLinearColor CurrentColor = FMath::Lerp(StartColor, EndColor, Alpha);
			Character->SetComponentColor(Component, CurrentColor);
		}
	}
	
	LogAnimation(FString::Printf(TEXT("Focus progress: %.2f (%s)"), Alpha, 
		CurrentAnimation.bIsInFocus ? TEXT("In Focus") : TEXT("Out of Focus")), false);
}

// =====================================================
// УТИЛИТЫ
// =====================================================

AVNCharacter* UVNCharacterAnimationManager::GetVNCharacterOwner() const
{
	return OwnerCharacter.Get();
}

bool UVNCharacterAnimationManager::ValidateAnimationRequest(const FVNAnimationRequest& Request) const
{
	if (!Request.IsValid())
	{
		return false;
	}

	if (Request.Duration < 0.0f)
	{
		VN_LOG_WARNING(TEXT("Animation duration cannot be negative: %f"), Request.Duration);
		return false;
	}

	return true;
}

void UVNCharacterAnimationManager::LogAnimation(const FString& Message, bool bForceLog) const
{
	if (bVerboseLogging || bForceLog)
	{
		VN_LOG_DEBUG(TEXT("AnimManager [%s]: %s"), 
			OwnerCharacter.IsValid() ? *OwnerCharacter->GetName() : TEXT("Unknown"), 
			*Message);
	}
}

// =====================================================
// ОТЛАДОЧНЫЕ МЕТОДЫ
// =====================================================

#if WITH_EDITOR
void UVNCharacterAnimationManager::PrintDebugInfo()
{
	FString DebugInfo = TEXT("=== VN Animation Manager Debug Info ===\n");
	
	DebugInfo += FString::Printf(TEXT("Owner: %s\n"), 
		OwnerCharacter.IsValid() ? *OwnerCharacter->GetName() : TEXT("None"));
	
	DebugInfo += FString::Printf(TEXT("Is Animating: %s\n"), 
		IsAnimating() ? TEXT("Yes") : TEXT("No"));
	
	if (IsAnimating())
	{
		DebugInfo += FString::Printf(TEXT("Current Animation: %s\n"), *CurrentAnimation.ToString());
		DebugInfo += FString::Printf(TEXT("Progress: %.2f%%\n"), GetCurrentAnimationProgress() * 100.0f);
		DebugInfo += FString::Printf(TEXT("Time: %.2f / %.2f\n"), CurrentAnimationTime, CurrentAnimation.Duration);
	}
	
	DebugInfo += FString::Printf(TEXT("Queue Size: %d / %d\n"), AnimationQueue.Num(), MaxQueueSize);
	
	if (AnimationQueue.Num() > 0)
	{
		DebugInfo += TEXT("Queued Animations:\n");
		for (int32 i = 0; i < AnimationQueue.Num(); ++i)
		{
			DebugInfo += FString::Printf(TEXT("  %d: %s\n"), i + 1, *AnimationQueue[i].ToString());
		}
	}
	
	DebugInfo += FString::Printf(TEXT("Settings: MaxQueue=%d, MinDuration=%.2f, DisableAnims=%s, VerboseLog=%s\n"),
		MaxQueueSize, MinAnimationDuration, 
		bDisableAnimations ? TEXT("Yes") : TEXT("No"),
		bVerboseLogging ? TEXT("Yes") : TEXT("No"));

	// Добавляем информацию о компонентах в анимации
	if (OwnerCharacter.IsValid())
	{
		DebugInfo += FString::Printf(TEXT("Fading In Components: %d\n"), OwnerCharacter->GetFadingInComponents().Num());
		DebugInfo += FString::Printf(TEXT("Fading Out Components: %d\n"), OwnerCharacter->GetFadingOutComponents().Num());
	}

	VN_LOG(Log, TEXT("%s"), *DebugInfo);
}
#endif

FString UVNCharacterAnimationManager::GetQueueDebugString() const
{
	if (AnimationQueue.Num() == 0)
	{
		return TEXT("Animation Queue: Empty");
	}

	FString Result = FString::Printf(TEXT("Animation Queue (%d items):\n"), AnimationQueue.Num());
	
	for (int32 i = 0; i < AnimationQueue.Num(); ++i)
	{
		Result += FString::Printf(TEXT("  %d: %s\n"), i + 1, *AnimationQueue[i].ToString());
	}

	return Result;
}