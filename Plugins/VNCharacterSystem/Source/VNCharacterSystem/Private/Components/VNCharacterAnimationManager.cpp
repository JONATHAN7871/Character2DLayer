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

	// --- СПЕЦИАЛЬНОЕ ЛОГИРОВАНИЕ ДЛЯ TRANSITION ---
	if (Request.AnimationType == EVNAnimationType::Transition)
	{
		LogAnimation(FString::Printf(TEXT("Transition animation starting. Fading in %d components, fading out %d components."), 
			Character->GetFadingInComponents().Num(), Character->GetFadingOutComponents().Num()));
		
		// Дополнительное детальное логирование компонентов
		for (const TObjectPtr<USceneComponent>& Component : Character->GetFadingInComponents())
		{
			if (Component)
			{
				VN_LOG_DEBUG(TEXT("StartAnimation: FadingIn component: %s"), *Component->GetName());
			}
		}
		
		for (const TObjectPtr<USceneComponent>& Component : Character->GetFadingOutComponents())
		{
			if (Component)
			{
				VN_LOG_DEBUG(TEXT("StartAnimation: FadingOut component: %s"), *Component->GetName());
			}
		}
	}
	else
	{
		LogAnimation(FString::Printf(TEXT("Starting animation: %s"), *Request.ToString()));
	}

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

	UE_LOG(LogTemp, Log, TEXT("UpdateTransitionAnimation: Progress %.2f%% - FadingIn=%d, FadingOut=%d"), 
		Alpha * 100.0f, Character->GetFadingInComponents().Num(), Character->GetFadingOutComponents().Num());

	// === НОВАЯ УЛУЧШЕННАЯ СИСТЕМА КРИВЫХ ===
	// Используем более плавные кривые для предотвращения резких переходов
	const float SmoothInAlpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);     // Smooth появление
	const float SmoothOutAlpha = FMath::SmoothStep(1.0f, 0.0f, Alpha);    // Smooth исчезновение

	// === ДОПОЛНИТЕЛЬНАЯ ЗАЩИТА: Проверяем валидность компонентов ===
	TArray<TObjectPtr<USceneComponent>> ValidFadingIn;
	TArray<TObjectPtr<USceneComponent>> ValidFadingOut;

	// Собираем только валидные компоненты
	for (const TObjectPtr<USceneComponent>& Component : Character->GetFadingInComponents())
	{
		if (Component && IsValid(Component.Get()))
		{
			ValidFadingIn.Add(Component);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UpdateTransitionAnimation: Invalid FadingIn component detected, skipping"));
		}
	}

	for (const TObjectPtr<USceneComponent>& Component : Character->GetFadingOutComponents())
	{
		if (Component && IsValid(Component.Get()))
		{
			ValidFadingOut.Add(Component);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UpdateTransitionAnimation: Invalid FadingOut component detected, skipping"));
		}
	}

	// === FADE OUT: Анимируем исчезающие компоненты ===
	for (const TObjectPtr<USceneComponent>& FadeComponent : ValidFadingOut)
	{
		if (FadeComponent && FadeComponent->IsVisible())
		{
			// Плавное исчезновение с проверкой границ
			float CurrentAlpha = FMath::Clamp(SmoothOutAlpha, 0.0f, 1.0f);
			Character->SetAnimationAlpha(FadeComponent.Get(), CurrentAlpha);
			
			UE_LOG(LogTemp, Verbose, TEXT("UpdateTransitionAnimation: FadeOut %s alpha %.2f"), 
				*FadeComponent->GetName(), CurrentAlpha);
		}
	}

	// === FADE IN: Анимируем появляющиеся компоненты ===
	for (const TObjectPtr<USceneComponent>& MainComponent : ValidFadingIn)
	{
		if (MainComponent)
		{
			// КРИТИЧЕСКАЯ ПРОВЕРКА: Убеждаемся, что компонент готов к анимации
			float TargetAlpha = Character->GetTargetAlpha(MainComponent.Get());
			
			// Интерполируем от 0.0 к целевой альфе с плавной кривой
			float CurrentAlpha = FMath::Lerp(0.0f, TargetAlpha, SmoothInAlpha);
			CurrentAlpha = FMath::Clamp(CurrentAlpha, 0.0f, 1.0f);
			
			Character->SetAnimationAlpha(MainComponent.Get(), CurrentAlpha);
			
			// Убеждаемся, что компонент видим только если у него есть контент
			bool bHasContent = false;
			if (USkeletalMeshComponent* SkeletalComp = Cast<USkeletalMeshComponent>(MainComponent.Get()))
			{
				bHasContent = (SkeletalComp->GetSkeletalMeshAsset() != nullptr);
			}
			else if (UPaperSpriteComponent* SpriteComp = Cast<UPaperSpriteComponent>(MainComponent.Get()))
			{
				bHasContent = (SpriteComp->GetSprite() != nullptr);
			}
			
			// ДОПОЛНИТЕЛЬНАЯ ЗАЩИТА: показываем компонент только если есть контент и альфа > 0
			if (bHasContent && CurrentAlpha > 0.01f)
			{
				MainComponent->SetVisibility(true);
			}
			else if (!bHasContent)
			{
				MainComponent->SetVisibility(false);
			}
			
			UE_LOG(LogTemp, Verbose, TEXT("UpdateTransitionAnimation: FadeIn %s alpha %.2f (target: %.2f, hasContent: %s)"), 
				*MainComponent->GetName(), CurrentAlpha, TargetAlpha, bHasContent ? TEXT("Yes") : TEXT("No"));
		}
	}

	// === ЗАЩИТА ОТ ЗАВИСАНИЯ: Проверяем завершение анимации ===
	if (Alpha >= 0.99f)
	{
		UE_LOG(LogTemp, Warning, TEXT("UpdateTransitionAnimation: Animation near completion, preparing for finalization"));
		
		// Предварительно применяем финальные альфы для предотвращения мерцания
		for (const TObjectPtr<USceneComponent>& MainComponent : ValidFadingIn)
		{
			if (MainComponent)
			{
				float TargetAlpha = Character->GetTargetAlpha(MainComponent.Get());
				Character->SetAnimationAlpha(MainComponent.Get(), TargetAlpha);
			}
		}
		
		for (const TObjectPtr<USceneComponent>& FadeComponent : ValidFadingOut)
		{
			if (FadeComponent)
			{
				Character->SetAnimationAlpha(FadeComponent.Get(), 0.0f);
			}
		}
	}
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