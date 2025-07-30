#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "Components/VNCharacterIdleAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "TimerManager.h"

// =====================================================
// ПРИНЦИП 2: СИСТЕМА ГРУППИРОВКИ (BATCHING)
// =====================================================

// === ОТЛАДОЧНАЯ ВЕРСИЯ RequestTransitionCommit ===
void AVNCharacter::RequestTransitionCommit(float Duration)
{
	UE_LOG(LogTemp, Error, TEXT("=== RequestTransitionCommit CALLED ==="));
	UE_LOG(LogTemp, Error, TEXT("RequestTransitionCommit: Duration=%.2f"), Duration);
	
	// Агрегируем максимальную длительность
	PendingTransitionDuration = FMath::Max(PendingTransitionDuration, Duration);
	
	UE_LOG(LogTemp, Error, TEXT("RequestTransitionCommit: PendingTransitionDuration now %.2f"), PendingTransitionDuration);
	
	// Проверяем мир
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("RequestTransitionCommit: GetWorld() returned NULL!"));
		return;
	}
	
	FTimerManager& TimerManager = World->GetTimerManager();
	bool bTimerActive = TimerManager.IsTimerActive(CommitTransitionTimerHandle);
	
	UE_LOG(LogTemp, Error, TEXT("RequestTransitionCommit: Timer active = %s"), bTimerActive ? TEXT("TRUE") : TEXT("FALSE"));
	
	// Если таймер еще не запущен, запускаем его
	if (!bTimerActive)
	{
		UE_LOG(LogTemp, Error, TEXT("RequestTransitionCommit: Setting timer with 1-frame delay"));
		
		// ИСПРАВЛЕНИЕ: Используем время одного кадра (~0.016f для 60 FPS)
		// Это гарантирует, что fade компоненты успеют полностью загрузиться
		TimerManager.SetTimer(
			CommitTransitionTimerHandle,
			this,
			&AVNCharacter::CommitTransitions,
			0.016f, // Задержка на ~1 кадр (60 FPS)
			false   // Не повторять
		);
		
		UE_LOG(LogTemp, Error, TEXT("RequestTransitionCommit: Timer set successfully with 0.016f delay"));
		
		// Дополнительная проверка
		bool bTimerActiveAfter = TimerManager.IsTimerActive(CommitTransitionTimerHandle);
		UE_LOG(LogTemp, Error, TEXT("RequestTransitionCommit: Timer active after setting = %s"), bTimerActiveAfter ? TEXT("TRUE") : TEXT("FALSE"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("RequestTransitionCommit: Timer already active, just updating duration"));
	}
	
	UE_LOG(LogTemp, Error, TEXT("=== RequestTransitionCommit END ==="));
}

void AVNCharacter::CommitTransitions()
{
	UE_LOG(LogTemp, Error, TEXT("=== CommitTransitions CALLED ==="));
	UE_LOG(LogTemp, Error, TEXT("CommitTransitions: Duration=%.2f"), PendingTransitionDuration);
	
	// Сбрасываем таймер
	GetWorld()->GetTimerManager().ClearTimer(CommitTransitionTimerHandle);
	
	// ДЕТАЛЬНОЕ ЛОГИРОВАНИЕ
	UE_LOG(LogTemp, Error, TEXT("FadingInComponents: %d"), FadingInComponents.Num());
	for (const TObjectPtr<USceneComponent>& Comp : FadingInComponents)
	{
		if (Comp)
		{
			UE_LOG(LogTemp, Error, TEXT("  - FadingIn: %s (TargetAlpha: %.3f)"), *Comp->GetName(), GetTargetAlpha(Comp.Get()));
		}
	}
	
	UE_LOG(LogTemp, Error, TEXT("FadingOutComponents: %d"), FadingOutComponents.Num());
	for (const TObjectPtr<USceneComponent>& Comp : FadingOutComponents)
	{
		if (Comp)
		{
			UE_LOG(LogTemp, Error, TEXT("  - FadingOut: %s"), *Comp->GetName());
		}
	}
	
	// Проверяем AnimationManager
	if (!AnimationManager)
	{
		UE_LOG(LogTemp, Error, TEXT("CommitTransitions: AnimationManager is NULL!"));
		return;
	}
	
	UE_LOG(LogTemp, Error, TEXT("CommitTransitions: AnimationManager exists"));
	UE_LOG(LogTemp, Error, TEXT("CommitTransitions: AnimationManager IsAnimating = %s"), 
		AnimationManager->IsAnimating() ? TEXT("TRUE") : TEXT("FALSE"));
	
	// Проверяем, есть ли ожидающие изменения
	if ((FadingInComponents.Num() > 0 || FadingOutComponents.Num() > 0))
	{
		UE_LOG(LogTemp, Error, TEXT("CommitTransitions: Starting transition with duration %.2f"), PendingTransitionDuration);
		
		// ВЫЗЫВАЕМ PlayTransition
		AnimationManager->PlayTransition(PendingTransitionDuration);
		
		UE_LOG(LogTemp, Error, TEXT("CommitTransitions: PlayTransition called"));
		UE_LOG(LogTemp, Error, TEXT("CommitTransitions: AnimationManager IsAnimating after PlayTransition = %s"), 
			AnimationManager->IsAnimating() ? TEXT("TRUE") : TEXT("FALSE"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("CommitTransitions: No components to animate"));
		HideAllFadeComponents();
	}
	
	// Сбрасываем накопленную длительность
	PendingTransitionDuration = 0.0f;
	
	UE_LOG(LogTemp, Error, TEXT("=== CommitTransitions END ==="));
}


// =====================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// =====================================================

void AVNCharacter::UpdateComponentTransform(USceneComponent* Component, const FVector& LocalOffset, float LocalScale)
{
    if (!Component) return;

    FVector FinalOffset = LocalOffset;
    float FinalScale = LocalScale;

    if (Component->GetAttachSocketName().IsNone())
    {
        if (Cast<USkeletalMeshComponent>(Component))
        {
            FinalOffset = GlobalSkeletalOffset + LocalOffset;
            FinalScale = GlobalSkeletalScale * LocalScale;
        }
        else if (Cast<UPaperSpriteComponent>(Component))
        {
            if (!IsChildOfHeadSprite(Component))
            {
                FinalOffset = GlobalSpriteOffset + LocalOffset;
                FinalScale = GlobalSpriteScale * LocalScale;
            }
        }
    }
    
    Component->SetRelativeLocation(FinalOffset);
    Component->SetRelativeScale3D(FVector(FinalScale));
}

bool AVNCharacter::IsChildOfHeadSprite(E_VN_ComponentID_Sprite ComponentID) const
{
	switch (ComponentID)
	{
		case E_VN_ComponentID_Sprite::Eyebrow:
		case E_VN_ComponentID_Sprite::Eyes:
		case E_VN_ComponentID_Sprite::Eyelids:
		case E_VN_ComponentID_Sprite::Wink:
		case E_VN_ComponentID_Sprite::Mouth:
		case E_VN_ComponentID_Sprite::EmotionHead_01:
		case E_VN_ComponentID_Sprite::EmotionHead_02:
		case E_VN_ComponentID_Sprite::EmotionHead_03:
			return true;
		default:
			return false;
	}
}

bool AVNCharacter::IsChildOfHeadSprite(USceneComponent* Component) const
{
    if (!Component) return false;
    return Component->GetAttachParent() == Head_Sprite;
}

// =====================================================
// СИСТЕМА УПРАВЛЕНИЯ АЛЬФОЙ ДЛЯ АНИМАЦИИ
// =====================================================

void AVNCharacter::SetAnimationAlpha(USceneComponent* Component, float Alpha)
{
	if (!Component) return;
    
	Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	ComponentAnimationAlphas.Add(Component, Alpha);
    
	UE_LOG(LogTemp, Warning, TEXT("SetAnimationAlpha: %s alpha set to %.3f"), *Component->GetName(), Alpha);
    
	// Применяем альфу к компоненту
	if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(Component))
	{
		UE_LOG(LogTemp, Warning, TEXT("SetAnimationAlpha: Processing SkeletalMesh %s"), *Component->GetName());
		for (int32 i = 0; i < SkeletalMesh->GetNumMaterials(); ++i)
		{
			if (UMaterialInstanceDynamic* DynamicMaterial = SkeletalMesh->CreateAndSetMaterialInstanceDynamicFromMaterial(i, SkeletalMesh->GetMaterial(i)))
			{
				DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), Alpha);
				UE_LOG(LogTemp, Warning, TEXT("SetAnimationAlpha: Material %d opacity set to %.3f"), i, Alpha);
			}
		}
	}
	else if (UPaperSpriteComponent* SpriteComponent = Cast<UPaperSpriteComponent>(Component))
	{
		FLinearColor CurrentColor = SpriteComponent->GetSpriteColor();
		CurrentColor.A = Alpha;
		SpriteComponent->SetSpriteColor(CurrentColor);
		UE_LOG(LogTemp, Warning, TEXT("SetAnimationAlpha: Sprite %s color alpha set to %.3f"), *Component->GetName(), Alpha);
	}
}

void AVNCharacter::SetTargetAlpha(USceneComponent* Component, float TargetAlpha)
{
    if (!Component) return;
    TargetAlpha = FMath::Clamp(TargetAlpha, 0.0f, 1.0f);
    ComponentTargetAlphas.Add(Component, TargetAlpha);
    VN_LOG_DEBUG(TEXT("SetTargetAlpha: %s target alpha set to %.2f"), *Component->GetName(), TargetAlpha);
}

float AVNCharacter::GetAnimationAlpha(USceneComponent* Component) const
{
    if (!Component) return 1.0f;
    if (const float* Alpha = ComponentAnimationAlphas.Find(Component))
    {
        return *Alpha;
    }
    return 1.0f; // По умолчанию полная непрозрачность
}

float AVNCharacter::GetTargetAlpha(USceneComponent* Component) const
{
    if (!Component) return 1.0f;
    if (const float* Alpha = ComponentTargetAlphas.Find(Component))
    {
        return *Alpha;
    }
    return 1.0f; // По умолчанию полная непрозрачность
}

void AVNCharacter::ClearAnimationAlphas(USceneComponent* Component)
{
    if (!Component) return;
    ComponentAnimationAlphas.Remove(Component);
    ComponentTargetAlphas.Remove(Component);
}

// =====================================================
// УПРОЩЕННЫЕ ФУНКЦИИ НАСТРОЙКИ КОМПОНЕНТОВ
// =====================================================

void AVNCharacter::ValidateAndSetupSkeletalComponent(USkeletalMeshComponent* Component, TSoftObjectPtr<USkeletalMesh> SkeletalMesh)
{
	if (!Component) return;
	
	// ИСПРАВЛЕНИЕ: Принудительно сбрасываем состояние перед изменениями
	Component->SetHiddenInGame(false);
	Component->SetVisibility(true);
	
	if (!SkeletalMesh.IsNull())
	{
		USkeletalMesh* LoadedMesh = SkeletalMesh.LoadSynchronous();
		if (LoadedMesh)
		{
			Component->SetSkeletalMesh(LoadedMesh);
			
			// ИСПРАВЛЕНИЕ: Применяем цвет с учетом фокуса
			SetComponentColor(Component, GetTargetColorForComponent(Component));
			
			VN_LOG_DEBUG(TEXT("ValidateAndSetupSkeletalComponent: Set mesh for %s with focus-aware color"), *Component->GetName());
		}
		else
		{
			Component->SetSkeletalMesh(nullptr);
			Component->SetAnimInstanceClass(nullptr);
			Component->SetVisibility(false);
			VN_LOG_WARNING(TEXT("ValidateAndSetupSkeletalComponent: Failed to load mesh for %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSkeletalMesh(nullptr);
		Component->SetAnimInstanceClass(nullptr);
		Component->SetVisibility(false);
		VN_LOG_DEBUG(TEXT("ValidateAndSetupSkeletalComponent: Cleared mesh for %s"), *Component->GetName());
	}
}

void AVNCharacter::ValidateAndSetupSpriteComponent(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite> Sprite)
{
	if (!Component) return;
    
	// ПРОВЕРЬТЕ ЭТОТ МЕТОД: возможно здесь тоже нужно заменить
	Component->SetHiddenInGame(false);  // Вместо SetVisibility(true)
    
	if (!Sprite.IsNull())
	{
		UPaperSprite* LoadedSprite = Sprite.LoadSynchronous();
		if (LoadedSprite)
		{
			Component->SetSprite(LoadedSprite);
			Component->SetSpriteColor(GetTargetColorForComponent(Component));
			VN_LOG_DEBUG(TEXT("ValidateAndSetupSpriteComponent: Set sprite for %s"), *Component->GetName());
		}
		else
		{
			Component->SetSprite(nullptr);
			Component->SetHiddenInGame(true);  // Вместо SetVisibility(false)
			VN_LOG_WARNING(TEXT("ValidateAndSetupSpriteComponent: Failed to load sprite for %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSprite(nullptr);
		Component->SetHiddenInGame(true);  // Вместо SetVisibility(false)
		VN_LOG_DEBUG(TEXT("ValidateAndSetupSpriteComponent: Cleared sprite for %s"), *Component->GetName());
	}
}

// =====================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ РАБОТЫ С КОМПОНЕНТАМИ
// =====================================================

void AVNCharacter::SetComponentAlpha(USceneComponent* Component, float Alpha)
{
	if (!Component) return;
	Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	
	if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(Component))
	{
		for (int32 i = 0; i < SkeletalMesh->GetNumMaterials(); ++i)
		{
			if (UMaterialInstanceDynamic* DynamicMaterial = SkeletalMesh->CreateAndSetMaterialInstanceDynamicFromMaterial(i, SkeletalMesh->GetMaterial(i)))
			{
				DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), Alpha);
			}
		}
	}
	else if (UPaperSpriteComponent* SpriteComponent = Cast<UPaperSpriteComponent>(Component))
	{
		FLinearColor CurrentColor = SpriteComponent->GetSpriteColor();
		CurrentColor.A = Alpha;
		SpriteComponent->SetSpriteColor(CurrentColor);
	}
}

void AVNCharacter::SetComponentColor(USceneComponent* Component, const FLinearColor& Color)
{
	if (!Component) return;
	
	if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(Component))
	{
		for (int32 i = 0; i < SkeletalMesh->GetNumMaterials(); ++i)
		{
			if (UMaterialInstanceDynamic* DynamicMaterial = SkeletalMesh->CreateAndSetMaterialInstanceDynamicFromMaterial(i, SkeletalMesh->GetMaterial(i)))
			{
				DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
				DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), Color.A);
			}
		}
	}
	else if (UPaperSpriteComponent* SpriteComponent = Cast<UPaperSpriteComponent>(Component))
	{
		SpriteComponent->SetSpriteColor(Color);
	}
}

void AVNCharacter::CopySkeletalComponentSettings(USkeletalMeshComponent* Source, USkeletalMeshComponent* Target)
{
	if (!Source || !Target) return;
	
	UE_LOG(LogTemp, Warning, TEXT("CopySkeletalComponentSettings: Copying from %s to %s"), *Source->GetName(), *Target->GetName());
	
	Target->SetSkeletalMesh(Source->GetSkeletalMeshAsset());
	Target->SetAnimInstanceClass(Source->GetAnimClass());
	
	Target->SetHiddenInGame(false);
	Target->SetVisibility(true);
	
	for (int32 i = 0; i < Source->GetNumMaterials(); ++i)
	{
		Target->SetMaterial(i, Source->GetMaterial(i));
	}
	
	// ИСПРАВЛЕНИЕ ФОКУСА: Применяем цвет с учетом фокуса
	SetComponentColor(Target, GetTargetColorForComponent(Target));
	
	Target->MarkRenderStateDirty();
	Target->RecreateRenderState_Concurrent();
	
	if (Source->GetAttachParent())
	{
		Target->AttachToComponent(Source->GetAttachParent(), FAttachmentTransformRules::KeepWorldTransform, Source->GetAttachSocketName());
		Target->SetRelativeTransform(Source->GetRelativeTransform());
	}
	else
	{
		Target->SetWorldTransform(Source->GetComponentTransform());
	}
	
	UE_LOG(LogTemp, Warning, TEXT("CopySkeletalComponentSettings: Copy completed for %s with focus-aware color"), *Target->GetName());
}

void AVNCharacter::CopySpriteComponentSettings(UPaperSpriteComponent* Source, UPaperSpriteComponent* Target)
{
	if (!Source || !Target) return;
	
	UE_LOG(LogTemp, Warning, TEXT("CopySpriteComponentSettings: Copying from %s to %s"), *Source->GetName(), *Target->GetName());
	
	Target->SetSprite(Source->GetSprite());
	
	Target->SetHiddenInGame(false);
	Target->SetVisibility(true);
	
	// ИСПРАВЛЕНИЕ ФОКУСА: Применяем цвет с учетом фокуса, но с альфой 1.0 для fade компонента
	FLinearColor TargetColor = GetTargetColorForComponent(Target);
	TargetColor.A = 1.0f; // Fade компонент начинает с полной видимости
	Target->SetSpriteColor(TargetColor);
	
	Target->MarkRenderStateDirty();
	Target->RecreateRenderState_Concurrent();
	
	if (Source->GetAttachParent())
	{
		Target->AttachToComponent(Source->GetAttachParent(), FAttachmentTransformRules::KeepWorldTransform, Source->GetAttachSocketName());
		Target->SetRelativeTransform(Source->GetRelativeTransform());
	}
	else
	{
		Target->SetWorldTransform(Source->GetComponentTransform());
	}
	
	UE_LOG(LogTemp, Warning, TEXT("CopySpriteComponentSettings: Copy completed for %s with focus-aware color"), *Target->GetName());
}

// =====================================================
// УНИВЕРСАЛЬНАЯ СИСТЕМА ПЕРЕХОДОВ
// =====================================================

void AVNCharacter::PrepareSkeletalTransition(USkeletalMeshComponent* MainComponent, USkeletalMeshComponent* FadeComponent, TSoftObjectPtr<USkeletalMesh> NewMesh)
{
	if (!MainComponent || !FadeComponent) 
	{
		UE_LOG(LogTemp, Error, TEXT("PrepareSkeletalTransition: Invalid components passed"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("=== PrepareSkeletalTransition: %s ==="), *MainComponent->GetName());
	
	// === КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: ПРАВИЛЬНЫЙ АНАЛИЗ СОСТОЯНИЯ ===
	// Проверяем не только наличие меша, но и видимость компонента
	bool bCurrentlyHasMesh = (MainComponent->GetSkeletalMeshAsset() != nullptr) && 
							(!MainComponent->bHiddenInGame) && 
							MainComponent->IsVisible();
	bool bWillHaveMesh = !NewMesh.IsNull();
	
	UE_LOG(LogTemp, Warning, TEXT("PrepareSkeletalTransition: %s - Current: %s, Target: %s"), 
		*MainComponent->GetName(),
		bCurrentlyHasMesh ? TEXT("HasMesh") : TEXT("Empty"),
		bWillHaveMesh ? TEXT("HasMesh") : TEXT("Empty"));
	
	// === СЛУЧАЙ 1: КОНТЕНТ → ПУСТОЕ ===
	if (bCurrentlyHasMesh && !bWillHaveMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("PrepareSkeletalTransition: %s - Content to Empty transition"), *MainComponent->GetName());
		
		CopySkeletalComponentSettings(MainComponent, FadeComponent);
		
		// ИСПРАВЛЕНИЕ: Правильная видимость для fade компонента
		FadeComponent->SetHiddenInGame(false);
		FadeComponent->SetVisibility(true);
		SetAnimationAlpha(FadeComponent, 1.0f);
		SetTargetAlpha(FadeComponent, 0.0f);
		FadingOutComponents.Add(FadeComponent);
		
		// Очищаем и скрываем main компонент
		MainComponent->SetSkeletalMesh(nullptr);
		MainComponent->SetAnimInstanceClass(nullptr);
		MainComponent->SetHiddenInGame(true);
		
		UE_LOG(LogTemp, Warning, TEXT("PrepareSkeletalTransition: Added %s to FadingOut"), *FadeComponent->GetName());
		return;
	}
	
	// === СЛУЧАЙ 2: ПУСТОЕ → КОНТЕНТ ===
	if (!bCurrentlyHasMesh && bWillHaveMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("PrepareSkeletalTransition: %s - Empty to Content transition"), *MainComponent->GetName());
		
		// Устанавливаем новый mesh БЕЗ видимости
		ValidateAndSetupSkeletalComponentSilent(MainComponent, NewMesh);
		
		// ИСПРАВЛЕНИЕ: Принудительно скрываем для анимации появления
		MainComponent->SetHiddenInGame(true);
		SetAnimationAlpha(MainComponent, 0.0f);
		
		// Настраиваем для появления
		FLinearColor TargetColor = GetTargetColorForComponent(MainComponent);
		SetTargetAlpha(MainComponent, TargetColor.A);
		FadingInComponents.Add(MainComponent);
		
		UE_LOG(LogTemp, Warning, TEXT("PrepareSkeletalTransition: Added %s to FadingIn (target alpha: %.3f, HIDDEN for animation)"), 
			*MainComponent->GetName(), TargetColor.A);
		return;
	}
	
	// === СЛУЧАЙ 3: КОНТЕНТ → ДРУГОЙ КОНТЕНТ ===
	if (bCurrentlyHasMesh && bWillHaveMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("PrepareSkeletalTransition: %s - Content to Content crossfade"), *MainComponent->GetName());
		
		// Переносим старый контент в fade компонент
		CopySkeletalComponentSettings(MainComponent, FadeComponent);
		
		// ИСПРАВЛЕНИЕ: Правильная видимость для fade компонента
		FadeComponent->SetHiddenInGame(false);
		FadeComponent->SetVisibility(true);
		SetAnimationAlpha(FadeComponent, 1.0f);
		SetTargetAlpha(FadeComponent, 0.0f);
		FadingOutComponents.Add(FadeComponent);
		
		// Устанавливаем новый контент БЕЗ видимости
		ValidateAndSetupSkeletalComponentSilent(MainComponent, NewMesh);
		
		// ИСПРАВЛЕНИЕ: Принудительно скрываем для анимации появления
		MainComponent->SetHiddenInGame(true);
		SetAnimationAlpha(MainComponent, 0.0f);
		
		ResetComponentAttachmentToDefault(MainComponent);
		
		FLinearColor TargetColor = GetTargetColorForComponent(MainComponent);
		SetTargetAlpha(MainComponent, TargetColor.A);
		FadingInComponents.Add(MainComponent);
		
		UE_LOG(LogTemp, Warning, TEXT("PrepareSkeletalTransition: Added %s to FadingIn (target alpha: %.3f, HIDDEN for animation)"), 
			*MainComponent->GetName(), TargetColor.A);
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("PrepareSkeletalTransition: %s - Empty to Empty (no action)"), *MainComponent->GetName());
}

void AVNCharacter::PrepareSpriteTransition(UPaperSpriteComponent* MainComponent, UPaperSpriteComponent* FadeComponent, TSoftObjectPtr<UPaperSprite> NewSprite)
{
	if (!MainComponent || !FadeComponent) 
	{
		UE_LOG(LogTemp, Error, TEXT("PrepareSpriteTransition: Invalid components passed"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("=== PrepareSpriteTransition: %s ==="), *MainComponent->GetName());
	
	// === КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: ПРАВИЛЬНЫЙ АНАЛИЗ СОСТОЯНИЯ ===
	// Проверяем не только наличие спрайта, но и видимость компонента
	bool bCurrentlyHasSprite = (MainComponent->GetSprite() != nullptr) && 
							  (!MainComponent->bHiddenInGame) && 
							  MainComponent->IsVisible();
	bool bWillHaveSprite = !NewSprite.IsNull();
	
	UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: %s - Current: %s, Target: %s"), 
		*MainComponent->GetName(),
		bCurrentlyHasSprite ? TEXT("HasSprite") : TEXT("Empty"),
		bWillHaveSprite ? TEXT("HasSprite") : TEXT("Empty"));
	
	// ДОПОЛНИТЕЛЬНОЕ ЛОГИРОВАНИЕ ДЛЯ ОТЛАДКИ
	UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: %s - Sprite=%s, HiddenInGame=%s, IsVisible=%s"), 
		*MainComponent->GetName(),
		MainComponent->GetSprite() ? TEXT("YES") : TEXT("NO"),
		MainComponent->bHiddenInGame ? TEXT("YES") : TEXT("NO"),
		MainComponent->IsVisible() ? TEXT("YES") : TEXT("NO"));
	
	// === СЛУЧАЙ 1: КОНТЕНТ → ПУСТОЕ ===
	if (bCurrentlyHasSprite && !bWillHaveSprite)
	{
		UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: %s - Content to Empty transition"), *MainComponent->GetName());
		
		CopySpriteComponentSettings(MainComponent, FadeComponent);
		
		// ИСПРАВЛЕНИЕ: Правильная видимость для fade компонента
		FadeComponent->SetHiddenInGame(false);
		FadeComponent->SetVisibility(true);
		SetAnimationAlpha(FadeComponent, 1.0f);
		SetTargetAlpha(FadeComponent, 0.0f);
		FadingOutComponents.Add(FadeComponent);
		
		// Очищаем и скрываем main компонент
		MainComponent->SetSprite(nullptr);
		MainComponent->SetHiddenInGame(true);
		
		UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: Added %s to FadingOut"), *FadeComponent->GetName());
		return;
	}
	
	// === СЛУЧАЙ 2: ПУСТОЕ → КОНТЕНТ ===
	if (!bCurrentlyHasSprite && bWillHaveSprite)
	{
		UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: %s - Empty to Content transition"), *MainComponent->GetName());
		
		// Устанавливаем новый sprite БЕЗ видимости
		ValidateAndSetupSpriteComponentSilent(MainComponent, NewSprite);
		
		// ИСПРАВЛЕНИЕ: Принудительно скрываем для анимации появления
		MainComponent->SetHiddenInGame(true);
		SetAnimationAlpha(MainComponent, 0.0f);
		
		FLinearColor TargetColor = GetTargetColorForComponent(MainComponent);
		SetTargetAlpha(MainComponent, TargetColor.A);
		FadingInComponents.Add(MainComponent);
		
		UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: Added %s to FadingIn (target alpha: %.3f, HIDDEN for animation)"), 
			*MainComponent->GetName(), TargetColor.A);
		return;
	}
	
	// === СЛУЧАЙ 3: КОНТЕНТ → ДРУГОЙ КОНТЕНТ ===
	if (bCurrentlyHasSprite && bWillHaveSprite)
	{
		UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: %s - Content to Content crossfade"), *MainComponent->GetName());
		
		CopySpriteComponentSettings(MainComponent, FadeComponent);
		
		// ИСПРАВЛЕНИЕ: Правильная видимость для fade компонента
		FadeComponent->SetHiddenInGame(false);
		FadeComponent->SetVisibility(true);
		SetAnimationAlpha(FadeComponent, 1.0f);
		SetTargetAlpha(FadeComponent, 0.0f);
		FadingOutComponents.Add(FadeComponent);
		
		// Устанавливаем новый контент БЕЗ видимости
		ValidateAndSetupSpriteComponentSilent(MainComponent, NewSprite);
		
		// ИСПРАВЛЕНИЕ: Принудительно скрываем для анимации появления
		MainComponent->SetHiddenInGame(true);
		SetAnimationAlpha(MainComponent, 0.0f);
		
		ResetComponentAttachmentToDefault(MainComponent);
		
		FLinearColor TargetColor = GetTargetColorForComponent(MainComponent);
		SetTargetAlpha(MainComponent, TargetColor.A);
		FadingInComponents.Add(MainComponent);
		
		UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: Added %s to FadingIn (target alpha: %.3f, HIDDEN for animation)"), 
			*MainComponent->GetName(), TargetColor.A);
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: %s - Empty to Empty (no action)"), *MainComponent->GetName());
}

void AVNCharacter::HideAllFadeComponents()
{
	VN_LOG_DEBUG(TEXT("HideAllFadeComponents: Hiding all fade components"));
	
	TArray<USceneComponent*> FadeComponents = GetAllFadeComponents();
	for (USceneComponent* Component : FadeComponents)
	{
		if (Component)
		{
			Component->SetVisibility(false);
			Component->SetHiddenInGame(true); // ДОБАВЛЕНО: Дополнительное сокрытие
			SetAnimationAlpha(Component, 0.0f);
			
			if (USkeletalMeshComponent* SkeletalFade = Cast<USkeletalMeshComponent>(Component))
			{
				SkeletalFade->SetSkeletalMesh(nullptr);
				SkeletalFade->SetAnimInstanceClass(nullptr);
				SkeletalFade->SetLeaderPoseComponent(nullptr);
			}
			else if (UPaperSpriteComponent* SpriteFade = Cast<UPaperSpriteComponent>(Component))
			{
				SpriteFade->SetSprite(nullptr);
			}
		}
	}
	
	// ИСПРАВЛЕНИЕ: Убеждаемся, что все main компоненты правильно отображаются
	for (USceneComponent* Component : FadingInComponents)
	{
		if (Component)
		{
			// Применяем целевую альфу, если она есть
			float TargetAlpha = GetTargetAlpha(Component);
			if (TargetAlpha != 1.0f || ComponentTargetAlphas.Contains(Component))
			{
				SetAnimationAlpha(Component, TargetAlpha);
			}
			
			// ИСПРАВЛЕНИЕ: Устанавливаем правильный цвет с учетом фокуса
			SetComponentColor(Component, GetTargetColorForComponent(Component));
			
			// КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Убеждаемся, что компонент виден
			if (TargetAlpha > 0.01f)
			{
				Component->SetHiddenInGame(false);
				Component->SetVisibility(true);
			}
		}
	}
	
	// Очищаем все данные анимации
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
	
	FadingInComponents.Empty();
	FadingOutComponents.Empty();
}

// =====================================================
// "ТИХИЕ" ФУНКЦИИ НАСТРОЙКИ КОМПОНЕНТОВ (БЕЗ АВТОМАТИЧЕСКОГО ПРИМЕНЕНИЯ ЦВЕТА)
// =====================================================

void AVNCharacter::ValidateAndSetupSkeletalComponentSilent(USkeletalMeshComponent* Component, TSoftObjectPtr<USkeletalMesh> SkeletalMesh)
{
	if (!Component) 
	{
		UE_LOG(LogTemp, Error, TEXT("ValidateAndSetupSkeletalComponentSilent: Component is null"));
		return;
	}
    
	UE_LOG(LogTemp, Warning, TEXT("ValidateAndSetupSkeletalComponentSilent: BEFORE - %s HiddenInGame=%s"), 
		*Component->GetName(), Component->bHiddenInGame ? TEXT("YES") : TEXT("NO"));
    
	if (!SkeletalMesh.IsNull())
	{
		USkeletalMesh* LoadedMesh = SkeletalMesh.LoadSynchronous();
		if (LoadedMesh)
		{
			// ИСПРАВЛЕНИЕ: Сначала устанавливаем меш, потом скрываем
			Component->SetSkeletalMesh(LoadedMesh);
			Component->SetHiddenInGame(true); // Скрываем для анимации
            
			UE_LOG(LogTemp, Warning, TEXT("ValidateAndSetupSkeletalComponentSilent: AFTER - %s HiddenInGame=%s"), 
				*Component->GetName(), Component->bHiddenInGame ? TEXT("YES") : TEXT("NO"));
		}
		else
		{
			Component->SetSkeletalMesh(nullptr);
			Component->SetAnimInstanceClass(nullptr);
			Component->SetHiddenInGame(true);
			UE_LOG(LogTemp, Warning, TEXT("ValidateAndSetupSkeletalComponentSilent: Failed to load mesh for %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSkeletalMesh(nullptr);
		Component->SetAnimInstanceClass(nullptr);
		Component->SetHiddenInGame(true);
		UE_LOG(LogTemp, Warning, TEXT("ValidateAndSetupSkeletalComponentSilent: Cleared mesh for %s"), *Component->GetName());
	}
}

void AVNCharacter::ValidateAndSetupSpriteComponentSilent(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite> Sprite)
{
	if (!Component) 
	{
		UE_LOG(LogTemp, Error, TEXT("ValidateAndSetupSpriteComponentSilent: Component is null"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("ValidateAndSetupSpriteComponentSilent: BEFORE - %s HiddenInGame=%s"), 
		*Component->GetName(), Component->bHiddenInGame ? TEXT("YES") : TEXT("NO"));
	
	if (!Sprite.IsNull())
	{
		UPaperSprite* LoadedSprite = Sprite.LoadSynchronous();
		if (LoadedSprite)
		{
			// ИСПРАВЛЕНИЕ: Сначала устанавливаем спрайт, потом скрываем
			Component->SetSprite(LoadedSprite);
			Component->SetHiddenInGame(true); // Скрываем для анимации
			
			UE_LOG(LogTemp, Warning, TEXT("ValidateAndSetupSpriteComponentSilent: AFTER - %s HiddenInGame=%s"), 
				*Component->GetName(), Component->bHiddenInGame ? TEXT("YES") : TEXT("NO"));
		}
		else
		{
			Component->SetSprite(nullptr);
			Component->SetHiddenInGame(true);
			UE_LOG(LogTemp, Warning, TEXT("ValidateAndSetupSpriteComponentSilent: Failed to load sprite for %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSprite(nullptr);
		Component->SetHiddenInGame(true);
		UE_LOG(LogTemp, Warning, TEXT("ValidateAndSetupSpriteComponentSilent: Cleared sprite for %s"), *Component->GetName());
	}
}

FString AVNCharacter::GetSpritesStatusReport() const
{
	TArray<FString> StatusLines;
	
	// Основные спрайты лица
	StatusLines.Add(TEXT("=== FACIAL SPRITES ==="));
	StatusLines.Add(FString::Printf(TEXT("Eyes: %s"), 
		(Eyes_Sprite && Eyes_Sprite->GetSprite()) ? *Eyes_Sprite->GetSprite()->GetName() : TEXT("MISSING")));
	
	StatusLines.Add(FString::Printf(TEXT("Mouth: %s"), 
		(Mouth_Sprite && Mouth_Sprite->GetSprite()) ? *Mouth_Sprite->GetSprite()->GetName() : TEXT("MISSING")));
	
	StatusLines.Add(FString::Printf(TEXT("Eyebrows: %s"), 
		(Eyebrow_Sprite && Eyebrow_Sprite->GetSprite()) ? *Eyebrow_Sprite->GetSprite()->GetName() : TEXT("MISSING")));

	StatusLines.Add(FString::Printf(TEXT("Eyelids: %s"), 
		(Eyelids_Sprite && Eyelids_Sprite->GetSprite()) ? *Eyelids_Sprite->GetSprite()->GetName() : TEXT("MISSING")));

	StatusLines.Add(FString::Printf(TEXT("Wink: %s"), 
		(Wink_Sprite && Wink_Sprite->GetSprite()) ? *Wink_Sprite->GetSprite()->GetName() : TEXT("MISSING")));

	// Основные спрайты тела
	StatusLines.Add(TEXT(""));
	StatusLines.Add(TEXT("=== BODY SPRITES ==="));
	StatusLines.Add(FString::Printf(TEXT("Head: %s"), 
		(Head_Sprite && Head_Sprite->GetSprite()) ? *Head_Sprite->GetSprite()->GetName() : TEXT("MISSING")));
	
	StatusLines.Add(FString::Printf(TEXT("Body: %s"), 
		(Body_Sprite && Body_Sprite->GetSprite()) ? *Body_Sprite->GetSprite()->GetName() : TEXT("MISSING")));
	
	StatusLines.Add(FString::Printf(TEXT("Arms: %s"), 
		(Arms_Sprite && Arms_Sprite->GetSprite()) ? *Arms_Sprite->GetSprite()->GetName() : TEXT("MISSING")));

	// Эмоциональные эффекты
	StatusLines.Add(TEXT(""));
	StatusLines.Add(TEXT("=== EMOTION EFFECTS ==="));
	StatusLines.Add(FString::Printf(TEXT("Head Effect 01: %s"), 
		(EmotionHeadEffect01_Sprite && EmotionHeadEffect01_Sprite->GetSprite()) ? *EmotionHeadEffect01_Sprite->GetSprite()->GetName() : TEXT("MISSING")));
	
	StatusLines.Add(FString::Printf(TEXT("Head Effect 02: %s"), 
		(EmotionHeadEffect02_Sprite && EmotionHeadEffect02_Sprite->GetSprite()) ? *EmotionHeadEffect02_Sprite->GetSprite()->GetName() : TEXT("MISSING")));
	
	StatusLines.Add(FString::Printf(TEXT("Head Effect 03: %s"), 
		(EmotionHeadEffect03_Sprite && EmotionHeadEffect03_Sprite->GetSprite()) ? *EmotionHeadEffect03_Sprite->GetSprite()->GetName() : TEXT("MISSING")));

	StatusLines.Add(FString::Printf(TEXT("Body Effect 01: %s"), 
		(EmotionBodyEffect01_Sprite && EmotionBodyEffect01_Sprite->GetSprite()) ? *EmotionBodyEffect01_Sprite->GetSprite()->GetName() : TEXT("MISSING")));
	
	StatusLines.Add(FString::Printf(TEXT("Body Effect 02: %s"), 
		(EmotionBodyEffect02_Sprite && EmotionBodyEffect02_Sprite->GetSprite()) ? *EmotionBodyEffect02_Sprite->GetSprite()->GetName() : TEXT("MISSING")));
	
	StatusLines.Add(FString::Printf(TEXT("Body Effect 03: %s"), 
		(EmotionBodyEffect03_Sprite && EmotionBodyEffect03_Sprite->GetSprite()) ? *EmotionBodyEffect03_Sprite->GetSprite()->GetName() : TEXT("MISSING")));

	// Статистика
	int32 FilledSprites = 0;
	int32 TotalSprites = 0;
	
	TArray<UPaperSpriteComponent*> AllSpriteComponents = {
		Eyes_Sprite, Mouth_Sprite, Eyebrow_Sprite, Eyelids_Sprite, Wink_Sprite,
		Head_Sprite, Body_Sprite, Arms_Sprite,
		EmotionHeadEffect01_Sprite, EmotionHeadEffect02_Sprite, EmotionHeadEffect03_Sprite,
		EmotionBodyEffect01_Sprite, EmotionBodyEffect02_Sprite, EmotionBodyEffect03_Sprite
	};
	
	for (UPaperSpriteComponent* Component : AllSpriteComponents)
	{
		if (Component)
		{
			TotalSprites++;
			if (Component->GetSprite())
			{
				FilledSprites++;
			}
		}
	}

	StatusLines.Add(TEXT(""));
	StatusLines.Add(TEXT("=== SUMMARY ==="));
	StatusLines.Add(FString::Printf(TEXT("Filled Sprites: %d/%d (%.1f%%)"), 
		FilledSprites, TotalSprites, TotalSprites > 0 ? (float)FilledSprites / TotalSprites * 100.0f : 0.0f));

	return FString::Join(StatusLines, TEXT("\n"));
}