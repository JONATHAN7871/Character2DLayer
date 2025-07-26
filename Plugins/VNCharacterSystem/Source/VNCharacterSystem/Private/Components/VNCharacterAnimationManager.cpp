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
		// ПРИНУДИТЕЛЬНО СКРЫВАЕМ ВСЕ FADING IN КОМПОНЕНТЫ
		for (const TObjectPtr<USceneComponent>& Component : Character->GetFadingInComponents())
		{
			if (Component)
			{
				Component->SetVisibility(false);
				Character->SetAnimationAlpha(Component.Get(), 0.0f);
			}
		}
		
		// ПРИНУДИТЕЛЬНО ПОКАЗЫВАЕМ ВСЕ FADING OUT КОМПОНЕНТЫ
		for (const TObjectPtr<USceneComponent>& Component : Character->GetFadingOutComponents())
		{
			if (Component)
			{
				Component->SetVisibility(true);
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

	// Простые кривые для плавности
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
		
		// ИСПРАВЛЕНИЕ: Скрываем fade компоненты только когда альфа становится очень низкой
		if (CurrentAlpha <= 0.01f)
		{
			FadeComponent->SetVisibility(false);
			UE_LOG(LogTemp, Warning, TEXT("FadeOut[%d]: %s - HIDDEN (alpha too low)"), FadeOutIndex, *FadeComponent->GetName());
		}
		else
		{
			FadeComponent->SetVisibility(true);
			UE_LOG(LogTemp, Warning, TEXT("FadeOut[%d]: %s - VISIBLE"), FadeOutIndex, *FadeComponent->GetName());
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
		
		// === КЛЮЧЕВОЕ ИСПРАВЛЕНИЕ: Минимальный порог видимости ===
		const float VISIBILITY_THRESHOLD = 0.1f; // Компонент станет видимым только при альфе >= 10%
		
		if (bHasContent && CurrentAlpha >= VISIBILITY_THRESHOLD)
		{
			MainComponent->SetVisibility(true);
			UE_LOG(LogTemp, Warning, TEXT("FadeIn[%d]: %s - VISIBLE (alpha %.3f >= threshold %.3f)"), 
				FadeInIndex, *MainComponent->GetName(), CurrentAlpha, VISIBILITY_THRESHOLD);
		}
		else
		{
			MainComponent->SetVisibility(false);
			if (bHasContent)
			{
				UE_LOG(LogTemp, Warning, TEXT("FadeIn[%d]: %s - HIDDEN (alpha %.3f < threshold %.3f)"), 
					FadeInIndex, *MainComponent->GetName(), CurrentAlpha, VISIBILITY_THRESHOLD);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("FadeIn[%d]: %s - HIDDEN (no content)"), 
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