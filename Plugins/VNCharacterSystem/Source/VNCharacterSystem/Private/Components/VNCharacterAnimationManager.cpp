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

	// ДОБАВЛЯЕМ ЛОГИРОВАНИЕ КАЖДЫЕ 0.5 СЕКУНДЫ
	static float LogTimer = 0.0f;
	LogTimer += DeltaTime;
	if (LogTimer >= 0.5f)
	{
		UE_LOG(LogTemp, Error, TEXT("TickComponent: IsAnimating=%s, CurrentType=%d"), 
			IsAnimating() ? TEXT("TRUE") : TEXT("FALSE"), (int32)GetCurrentAnimationType());
		LogTimer = 0.0f;
	}

	// Обновляем текущую анимацию
	if (IsAnimating())
	{
		UE_LOG(LogTemp, Warning, TEXT("TickComponent: Updating animation (DeltaTime=%.4f)"), DeltaTime);
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
	UE_LOG(LogTemp, Error, TEXT("=== PlayTransition CALLED ==="));
	UE_LOG(LogTemp, Error, TEXT("PlayTransition: Duration=%.2f"), Duration);
	
	FVNAnimationRequest Request;
	Request.AnimationType = EVNAnimationType::Transition;
	Request.Duration = FMath::Max(Duration, MinAnimationDuration);

	UE_LOG(LogTemp, Error, TEXT("PlayTransition: Request Duration=%.2f"), Request.Duration);
	UE_LOG(LogTemp, Error, TEXT("PlayTransition: Calling EnqueueAnimationRequest"));
	
	bool bEnqueued = EnqueueAnimationRequest(Request);
	
	UE_LOG(LogTemp, Error, TEXT("PlayTransition: EnqueueAnimationRequest returned %s"), bEnqueued ? TEXT("TRUE") : TEXT("FALSE"));
	UE_LOG(LogTemp, Error, TEXT("=== PlayTransition END ==="));
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
	if (!ValidateAnimationRequest(Request) || bDisableAnimations)
		return;

	AVNCharacter* Character = GetVNCharacterOwner();
	if (!Character)
		return;

	CurrentAnimation = Request;
	CurrentAnimationTime = 0.0f;

	UE_LOG(LogTemp, Warning, TEXT("StartAnimation: %s"), *Request.ToString());
	
	if (Request.AnimationType == EVNAnimationType::Transition)
	{
		// Устанавливаем начальную альфу 0 для FadingIn компонентов
		for (const TObjectPtr<USceneComponent>& Component : Character->GetFadingInComponents())
		{
			if (Component)
			{
				Character->SetAnimationAlpha(Component.Get(), 0.0f);
				UE_LOG(LogTemp, Warning, TEXT("StartAnimation: Set alpha 0.0 for FadingIn component %s"), *Component->GetName());
			}
		}
		
		// КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: ПРИНУДИТЕЛЬНО ПОКАЗЫВАЕМ ВСЕ FADING OUT КОМПОНЕНТЫ
		for (const TObjectPtr<USceneComponent>& Component : Character->GetFadingOutComponents())
		{
			if (Component)
			{
				Component->SetHiddenInGame(false); // ИСПРАВЛЕНО: SetHiddenInGame вместо SetVisibility
				UE_LOG(LogTemp, Warning, TEXT("StartAnimation: FORCE VISIBLE FadingOut component %s"), *Component->GetName());
				
				// Дополнительная проверка - выводим состояние видимости
				UE_LOG(LogTemp, Warning, TEXT("StartAnimation: %s HiddenInGame=%s"), 
					*Component->GetName(),
					Component->bHiddenInGame ? TEXT("YES") : TEXT("NO"));
			}
		}
	}

	OnAnimationStarted.Broadcast(Request.AnimationType);
}

void UVNCharacterAnimationManager::UpdateCurrentAnimation(float DeltaTime)
{
	if (!IsAnimating())
	{
		UE_LOG(LogTemp, Error, TEXT("UpdateCurrentAnimation: Not animating!"));
		return;
	}

	// Обновляем время
	CurrentAnimationTime += DeltaTime;
	
	// Вычисляем прогресс
	float Alpha = FMath::Clamp(CurrentAnimationTime / CurrentAnimation.Duration, 0.0f, 1.0f);

	UE_LOG(LogTemp, Warning, TEXT("UpdateCurrentAnimation: Time=%.3f/%.3f, Alpha=%.3f, Type=%d"), 
		CurrentAnimationTime, CurrentAnimation.Duration, Alpha, (int32)CurrentAnimation.AnimationType);

	// Уведомляем о прогрессе
	OnAnimationProgress.Broadcast(CurrentAnimation.AnimationType, Alpha);

	// Обновляем специфичную анимацию
	switch (CurrentAnimation.AnimationType)
	{
	case EVNAnimationType::Transition:
		UE_LOG(LogTemp, Warning, TEXT("UpdateCurrentAnimation: Calling UpdateTransitionAnimation"));
		 UpdateTransitionAnimation(Alpha);
		break;

	case EVNAnimationType::SpawnDespawn:
		UpdateSpawnDespawnAnimation(Alpha);
		break;

	case EVNAnimationType::Focus:
		UpdateFocusAnimation(Alpha);
		break;

	default:
		UE_LOG(LogTemp, Error, TEXT("UpdateCurrentAnimation: Unknown animation type: %d"), (int32)CurrentAnimation.AnimationType);
		break;
	}

	// Проверяем завершение анимации
	if (Alpha >= 1.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("UpdateCurrentAnimation: Animation finished, calling FinishCurrentAnimation"));
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
		UE_LOG(LogTemp, Error, TEXT("UpdateTransitionAnimation: No Character owner!"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("=== UpdateTransitionAnimation: Progress %.2f%% ==="), Alpha * 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("FadingIn components: %d, FadingOut components: %d"), 
		Character->GetFadingInComponents().Num(), Character->GetFadingOutComponents().Num());

	// УЛУЧШЕННЫЕ КРИВЫЕ ДЛЯ ПЛАВНОГО CROSSFADE
	const float SmoothInAlpha = Alpha;      
	const float SmoothOutAlpha = 1.0f - Alpha; 

	UE_LOG(LogTemp, Warning, TEXT("SmoothInAlpha: %.3f, SmoothOutAlpha: %.3f"), SmoothInAlpha, SmoothOutAlpha);

	// === FADE OUT: Анимируем исчезающие компоненты ===
	int32 FadeOutIndex = 0;
	for (const TObjectPtr<USceneComponent>& FadeComponent : Character->GetFadingOutComponents())
	{
		if (!FadeComponent || !IsValid(FadeComponent.Get()))
		{
			UE_LOG(LogTemp, Error, TEXT("UpdateTransitionAnimation: Invalid FadingOut component at index %d"), FadeOutIndex);
			FadeOutIndex++;
			continue;
		}

		float CurrentAlpha = FMath::Clamp(SmoothOutAlpha, 0.0f, 1.0f);
		
		UE_LOG(LogTemp, Warning, TEXT("FadeOut[%d]: %s - Setting alpha %.3f"), 
			FadeOutIndex, *FadeComponent->GetName(), CurrentAlpha);
		
		Character->SetAnimationAlpha(FadeComponent.Get(), CurrentAlpha);
		
		// ИСПРАВЛЕНИЕ: Более агрессивное скрытие при низкой альфе
		if (CurrentAlpha <= 0.05f) // Увеличили порог с 0.001f до 0.05f
		{
			FadeComponent->SetHiddenInGame(true); // ИСПРАВЛЕНО: SetHiddenInGame
			UE_LOG(LogTemp, Warning, TEXT("FadeOut[%d]: %s - HIDDEN via HiddenInGame (alpha %.3f <= 0.05)"), 
				FadeOutIndex, *FadeComponent->GetName(), CurrentAlpha);
		}
		else
		{
			FadeComponent->SetHiddenInGame(false); // ИСПРАВЛЕНО: SetHiddenInGame
			UE_LOG(LogTemp, Warning, TEXT("FadeOut[%d]: %s - VISIBLE via HiddenInGame"), FadeOutIndex, *FadeComponent->GetName());
		}
		
		FadeOutIndex++;
	}

	// === FADE IN: Анимируем появляющиеся компоненты ===
	int32 FadeInIndex = 0;
	for (const TObjectPtr<USceneComponent>& MainComponent : Character->GetFadingInComponents())
	{
		if (!MainComponent || !IsValid(MainComponent.Get()))
		{
			UE_LOG(LogTemp, Error, TEXT("UpdateTransitionAnimation: Invalid FadingIn component at index %d"), FadeInIndex);
			FadeInIndex++;
			continue;
		}

		float TargetAlpha = Character->GetTargetAlpha(MainComponent.Get());
		float CurrentAlpha = FMath::Lerp(0.0f, TargetAlpha, SmoothInAlpha);
		CurrentAlpha = FMath::Clamp(CurrentAlpha, 0.0f, 1.0f);
		
		UE_LOG(LogTemp, Warning, TEXT("FadeIn[%d]: %s - Target: %.3f, Current: %.3f"), 
			FadeInIndex, *MainComponent->GetName(), TargetAlpha, CurrentAlpha);
		
		Character->SetAnimationAlpha(MainComponent.Get(), CurrentAlpha);
		
		// Проверяем контент
		bool bHasContent = false;
		if (USkeletalMeshComponent* SkeletalComp = Cast<USkeletalMeshComponent>(MainComponent.Get()))
		{
			bHasContent = (SkeletalComp->GetSkeletalMeshAsset() != nullptr);
		}
		else if (UPaperSpriteComponent* SpriteComp = Cast<UPaperSpriteComponent>(MainComponent.Get()))
		{
			bHasContent = (SpriteComp->GetSprite() != nullptr);
		}
		
		UE_LOG(LogTemp, Warning, TEXT("FadeIn[%d]: %s - HasContent: %s"), 
			FadeInIndex, *MainComponent->GetName(), bHasContent ? TEXT("YES") : TEXT("NO"));
		
		// === КЛЮЧЕВОЕ ИСПРАВЛЕНИЕ: Более раннее появление компонентов ===
		if (bHasContent && CurrentAlpha > 0.01f) // Снизили порог с 0.001f до 0.01f
		{
			MainComponent->SetHiddenInGame(false); // ИСПРАВЛЕНО: SetHiddenInGame
			UE_LOG(LogTemp, Warning, TEXT("FadeIn[%d]: %s - VISIBLE via HiddenInGame (alpha %.3f > 0.01)"), 
				FadeInIndex, *MainComponent->GetName(), CurrentAlpha);
		}
		else
		{
			if (bHasContent)
			{
				UE_LOG(LogTemp, Warning, TEXT("FadeIn[%d]: %s - HIDDEN via HiddenInGame (alpha %.3f <= 0.01)"), 
					FadeInIndex, *MainComponent->GetName(), CurrentAlpha);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("FadeIn[%d]: %s - HIDDEN via HiddenInGame (no content)"), 
					FadeInIndex, *MainComponent->GetName());
			}
		}
		
		FadeInIndex++;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("=== UpdateTransitionAnimation END ==="));
}

void UVNCharacterAnimationManager::UpdateSpawnDespawnAnimation(float Alpha)
{
	AVNCharacter* Character = GetVNCharacterOwner();
	if (!Character) return;

	TArray<USceneComponent*> AllMainComponents = Character->GetAllMainComponents();

	if (CurrentAnimation.bIsSpawnAnimation)
	{
		// === ПОЯВЛЕНИЕ (Appear) ===
		Character->SetActorHiddenInGame(false);

		if (Alpha <= 0.5f)
		{
			// ФАЗА 1: Появляется тень
			float PhaseAlpha = Alpha * 2.0f; // от 0 до 1

			// Все основные компоненты скрыты
			for (USceneComponent* Comp : AllMainComponents)
			{
				if(Comp) Comp->SetVisibility(false);
			}

			// Показываем и анимируем тень
			if (Character->BodyShadow_Sprite)
			{
				Character->BodyShadow_Sprite->SetVisibility(true);
				Character->SetComponentAlpha(Character->BodyShadow_Sprite, PhaseAlpha);
			}
		}
		else
		{
			// ФАЗА 2: Тень исчезает, появляются основные компоненты
			float PhaseAlpha = (Alpha - 0.5f) * 2.0f; // от 0 до 1

			// Скрываем тень
			if (Character->BodyShadow_Sprite)
			{
				Character->BodyShadow_Sprite->SetVisibility(false);
			}

			// Показываем основные компоненты и анимируем их цвет
			for (USceneComponent* Comp : AllMainComponents)
			{
				if (!Comp) continue;

				bool bHasContent = false;
				if (auto* SkelComp = Cast<USkeletalMeshComponent>(Comp)) bHasContent = (SkelComp->GetSkeletalMeshAsset() != nullptr);
				else if (auto* SpriteComp = Cast<UPaperSpriteComponent>(Comp)) bHasContent = (SpriteComp->GetSprite() != nullptr);

				if (bHasContent)
				{
					Comp->SetVisibility(true);
					
					FLinearColor TargetColor = Character->GetTargetColorForComponent(Comp);
					FLinearColor BlackColor = FLinearColor::Black;
					BlackColor.A = TargetColor.A;
					
					FLinearColor CurrentColor = FMath::Lerp(BlackColor, TargetColor, PhaseAlpha);
					Character->SetComponentColor(Comp, CurrentColor);
				}
			}
		}
	}
	else
	{
		// === ИСЧЕЗНОВЕНИЕ (Disappear) ===
		if (Alpha <= 0.5f)
		{
			// ФАЗА 1: Основные компоненты темнеют
			float PhaseAlpha = Alpha * 2.0f; // от 0 до 1

			// Тень скрыта
			if (Character->BodyShadow_Sprite)
			{
				Character->BodyShadow_Sprite->SetVisibility(false);
			}

			// Затемняем основные компоненты
			for (USceneComponent* Comp : AllMainComponents)
			{
				if (Comp && Comp->IsVisible())
				{
					FLinearColor TargetColor = Character->GetTargetColorForComponent(Comp);
					FLinearColor BlackColor = FLinearColor::Black;
					BlackColor.A = TargetColor.A;
					
					FLinearColor CurrentColor = FMath::Lerp(TargetColor, BlackColor, PhaseAlpha);
					Character->SetComponentColor(Comp, CurrentColor);
				}
			}
		}
		else
		{
			// ФАЗА 2: Основные компоненты скрыты, появляется и исчезает тень
			float PhaseAlpha = (Alpha - 0.5f) * 2.0f; // от 0 до 1

			// Скрываем основные компоненты
			for (USceneComponent* Comp : AllMainComponents)
			{
				if(Comp) Comp->SetVisibility(false);
			}
			
			// Показываем и анимируем исчезновение тени
			if (Character->BodyShadow_Sprite)
			{
				Character->BodyShadow_Sprite->SetVisibility(true);
				Character->SetComponentAlpha(Character->BodyShadow_Sprite, 1.0f - PhaseAlpha);
			}

			// В самом конце скрываем всего актора
			if (Alpha >= 1.0f)
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