#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "TimerManager.h"

// =====================================================
// ПРИНЦИП 2: СИСТЕМА ГРУППИРОВКИ (BATCHING)
// =====================================================

void AVNCharacter::RequestTransitionCommit(float Duration)
{
	// Агрегируем максимальную длительность
	PendingTransitionDuration = FMath::Max(PendingTransitionDuration, Duration);
	
	// Если таймер еще не запущен, запускаем его на следующий tick
	if (!GetWorld()->GetTimerManager().IsTimerActive(CommitTransitionTimerHandle))
	{
		VN_LOG_DEBUG(TEXT("RequestTransitionCommit: Scheduling transition commit for next tick (duration: %.2f)"), Duration);
		
		GetWorld()->GetTimerManager().SetTimer(
			CommitTransitionTimerHandle,
			this,
			&AVNCharacter::CommitTransitions,
			0.0f, // Задержка 0 = следующий tick
			false // Не повторять
		);
	}
	else
	{
		VN_LOG_DEBUG(TEXT("RequestTransitionCommit: Transition commit already scheduled, updating duration to %.2f"), 
			PendingTransitionDuration);
	}
}

void AVNCharacter::CommitTransitions()
{
	VN_LOG_DEBUG(TEXT("CommitTransitions: Committing batched transitions (duration: %.2f)"), PendingTransitionDuration);
	
	// Сбрасываем таймер
	GetWorld()->GetTimerManager().ClearTimer(CommitTransitionTimerHandle);
	
	// Проверяем, есть ли ожидающие изменения
	if ((FadingInComponents.Num() > 0 || FadingOutComponents.Num() > 0) && AnimationManager)
	{
		VN_LOG_DEBUG(TEXT("CommitTransitions: Starting batched transition with %d FadingIn and %d FadingOut components"), 
			FadingInComponents.Num(), FadingOutComponents.Num());
		
		// Запускаем одну общую анимацию с накопленной длительностью
		AnimationManager->PlayTransition(PendingTransitionDuration);
	}
	else
	{
		VN_LOG_DEBUG(TEXT("CommitTransitions: No components to animate, skipping transition"));
	}
	
	// Сбрасываем накопленную длительность
	PendingTransitionDuration = 0.0f;
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
    
    // Применяем альфу к компоненту
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
	
	if (!SkeletalMesh.IsNull())
	{
		USkeletalMesh* LoadedMesh = SkeletalMesh.LoadSynchronous();
		if (LoadedMesh)
		{
			Component->SetSkeletalMesh(LoadedMesh);
			Component->SetVisibility(true);
			// Применяем полный цвет БЕЗ модификации альфы - альфа управляется отдельно
			SetComponentColor(Component, GetTargetColorForComponent(Component));
			VN_LOG_DEBUG(TEXT("ValidateAndSetupSkeletalComponent: Set mesh for %s"), *Component->GetName());
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
	
	if (!Sprite.IsNull())
	{
		UPaperSprite* LoadedSprite = Sprite.LoadSynchronous();
		if (LoadedSprite)
		{
			Component->SetSprite(LoadedSprite);
			Component->SetVisibility(true);
			// Применяем полный цвет БЕЗ модификации альфы - альфа управляется отдельно
			Component->SetSpriteColor(GetTargetColorForComponent(Component));
			VN_LOG_DEBUG(TEXT("ValidateAndSetupSpriteComponent: Set sprite for %s"), *Component->GetName());
		}
		else
		{
			Component->SetSprite(nullptr);
			Component->SetVisibility(false);
			VN_LOG_WARNING(TEXT("ValidateAndSetupSpriteComponent: Failed to load sprite for %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSprite(nullptr);
		Component->SetVisibility(false);
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
	
	VN_LOG_DEBUG(TEXT("CopySkeletalComponentSettings: Copying from %s to %s"), *Source->GetName(), *Target->GetName());
	
	Target->SetSkeletalMesh(Source->GetSkeletalMeshAsset());
	Target->SetAnimInstanceClass(Source->GetAnimClass());
	Target->SetVisibility(Source->IsVisible());
	
	for (int32 i = 0; i < Source->GetNumMaterials(); ++i)
	{
		Target->SetMaterial(i, Source->GetMaterial(i));
	}
	
	if (Source->GetAttachParent())
	{
		Target->AttachToComponent(Source->GetAttachParent(), FAttachmentTransformRules::KeepWorldTransform, Source->GetAttachSocketName());
		Target->SetRelativeTransform(Source->GetRelativeTransform());
	}
	else
	{
		Target->SetWorldTransform(Source->GetComponentTransform());
	}
}

void AVNCharacter::CopySpriteComponentSettings(UPaperSpriteComponent* Source, UPaperSpriteComponent* Target)
{
	if (!Source || !Target) return;
	
	VN_LOG_DEBUG(TEXT("CopySpriteComponentSettings: Copying from %s to %s"), *Source->GetName(), *Target->GetName());
	
	Target->SetSprite(Source->GetSprite());
	Target->SetVisibility(Source->IsVisible());
	
	// Копируем цвет с альфой 1.0 (fade компонент начинает полностью видимым)
	FLinearColor SourceColor = Source->GetSpriteColor();
	FLinearColor TargetColor = SourceColor;
	TargetColor.A = 1.0f; // Fade компонент всегда начинает с полной видимости
	Target->SetSpriteColor(TargetColor);
	
	if (Source->GetAttachParent())
	{
		Target->AttachToComponent(Source->GetAttachParent(), FAttachmentTransformRules::KeepWorldTransform, Source->GetAttachSocketName());
		Target->SetRelativeTransform(Source->GetRelativeTransform());
	}
	else
	{
		Target->SetWorldTransform(Source->GetComponentTransform());
	}
}

// =====================================================
// УНИВЕРСАЛЬНАЯ СИСТЕМА ПЕРЕХОДОВ
// =====================================================

void AVNCharacter::PrepareSkeletalTransition(USkeletalMeshComponent* MainComponent, USkeletalMeshComponent* FadeComponent, TSoftObjectPtr<USkeletalMesh> NewMesh)
{
	if (!MainComponent || !FadeComponent) 
	{
		VN_LOG_ERROR(TEXT("PrepareSkeletalTransition: Invalid components passed"));
		return;
	}
	
	VN_LOG_DEBUG(TEXT("PrepareSkeletalTransition: Starting transition for %s"), *MainComponent->GetName());
	
	// === АНАЛИЗ СОСТОЯНИЯ ===
	bool bCurrentlyHasMesh = (MainComponent->GetSkeletalMeshAsset() != nullptr && MainComponent->IsVisible());
	bool bWillHaveMesh = !NewMesh.IsNull();
	
	VN_LOG_DEBUG(TEXT("PrepareSkeletalTransition: %s - Current: %s, Target: %s"), 
		*MainComponent->GetName(),
		bCurrentlyHasMesh ? TEXT("HasMesh") : TEXT("Empty"),
		bWillHaveMesh ? TEXT("HasMesh") : TEXT("Empty"));
	
	// === СЛУЧАЙ 1: КОНТЕНТ → ПУСТОЕ (Перенос в fade + исчезновение) ===
	if (bCurrentlyHasMesh && !bWillHaveMesh)
	{
		VN_LOG_DEBUG(TEXT("PrepareSkeletalTransition: %s - Content to Empty transition"), *MainComponent->GetName());
		
		// Переносим текущий контент в fade компонент
		CopySkeletalComponentSettings(MainComponent, FadeComponent);
		
		// Настраиваем позицию fade компонента (чуть смещаем для слоя)
		FVector CurrentLocation = FadeComponent->GetRelativeLocation();
		FadeComponent->SetRelativeLocation(CurrentLocation + FVector(0.f, -1.f, 0.f));
		
		// Fade компонент начинает с полной видимости и будет исчезать
		FadeComponent->SetVisibility(true, true);
		SetAnimationAlpha(FadeComponent, 1.0f);
		SetTargetAlpha(FadeComponent, 0.0f);
		FadingOutComponents.Add(FadeComponent);
		
		// Очищаем главный компонент
		MainComponent->SetSkeletalMesh(nullptr);
		MainComponent->SetAnimInstanceClass(nullptr);
		MainComponent->SetVisibility(false);
		
		return;
	}
	
	// === СЛУЧАЙ 2: ПУСТОЕ → КОНТЕНТ (Простое появление) ===
	if (!bCurrentlyHasMesh && bWillHaveMesh)
	{
		VN_LOG_DEBUG(TEXT("PrepareSkeletalTransition: %s - Empty to Content transition"), *MainComponent->GetName());
		
		// Устанавливаем новый mesh
		ValidateAndSetupSkeletalComponentSilent(MainComponent, NewMesh);
		
		// Устанавливаем альфу 0 ПОСЛЕ установки mesh
		SetAnimationAlpha(MainComponent, 0.0f);
		
		// Настраиваем для появления
		FLinearColor TargetColor = GetTargetColorForComponent(MainComponent);
		SetTargetAlpha(MainComponent, TargetColor.A);
		FadingInComponents.Add(MainComponent);
		
		return;
	}
	
	// === СЛУЧАЙ 3: КОНТЕНТ → ДРУГОЙ КОНТЕНТ (Классический нахлест) ===
	if (bCurrentlyHasMesh && bWillHaveMesh)
	{
		VN_LOG_DEBUG(TEXT("PrepareSkeletalTransition: %s - Content to Content crossfade"), *MainComponent->GetName());
		
		// Переносим старый контент в fade компонент
		CopySkeletalComponentSettings(MainComponent, FadeComponent);
		
		// Настраиваем позицию fade компонента
		FVector CurrentLocation = FadeComponent->GetRelativeLocation();
		FadeComponent->SetRelativeLocation(CurrentLocation + FVector(0.f, -1.f, 0.f));
		
		// Fade компонент начинает с полной видимости и будет исчезать
		FadeComponent->SetVisibility(true, true);
		SetAnimationAlpha(FadeComponent, 1.0f);
		SetTargetAlpha(FadeComponent, 0.0f);
		FadingOutComponents.Add(FadeComponent);
		
		// Устанавливаем новый контент в main компонент
		ValidateAndSetupSkeletalComponentSilent(MainComponent, NewMesh);
		SetAnimationAlpha(MainComponent, 0.0f);
		
		// Сбрасываем attachment для правильной настройки
		ResetComponentAttachmentToDefault(MainComponent);
		
		// Определяем целевую альфу для main компонента
		FLinearColor TargetColor = GetTargetColorForComponent(MainComponent);
		SetTargetAlpha(MainComponent, TargetColor.A);
		FadingInComponents.Add(MainComponent);
		
		return;
	}
	
	// === СЛУЧАЙ 4: ПУСТОЕ → ПУСТОЕ (Ничего не делаем) ===
	VN_LOG_DEBUG(TEXT("PrepareSkeletalTransition: %s - Empty to Empty (no action)"), *MainComponent->GetName());
}

void AVNCharacter::PrepareSpriteTransition(UPaperSpriteComponent* MainComponent, UPaperSpriteComponent* FadeComponent, TSoftObjectPtr<UPaperSprite> NewSprite)
{
	if (!MainComponent || !FadeComponent) 
	{
		VN_LOG_ERROR(TEXT("PrepareSpriteTransition: Invalid components passed"));
		return;
	}
	
	VN_LOG_DEBUG(TEXT("PrepareSpriteTransition: Starting transition for %s"), *MainComponent->GetName());
	
	// === АНАЛИЗ СОСТОЯНИЯ ===
	bool bCurrentlyHasSprite = (MainComponent->GetSprite() != nullptr && MainComponent->IsVisible());
	bool bWillHaveSprite = !NewSprite.IsNull();
	
	VN_LOG_DEBUG(TEXT("PrepareSpriteTransition: %s - Current: %s, Target: %s"), 
		*MainComponent->GetName(),
		bCurrentlyHasSprite ? TEXT("HasSprite") : TEXT("Empty"),
		bWillHaveSprite ? TEXT("HasSprite") : TEXT("Empty"));
	
	// === СЛУЧАЙ 1: КОНТЕНТ → ПУСТОЕ (Перенос в fade + исчезновение) ===
	if (bCurrentlyHasSprite && !bWillHaveSprite)
	{
		VN_LOG_DEBUG(TEXT("PrepareSpriteTransition: %s - Content to Empty transition"), *MainComponent->GetName());
		
		// Переносим текущий контент в fade компонент
		CopySpriteComponentSettings(MainComponent, FadeComponent);
		
		// Настраиваем позицию fade компонента
		FVector CurrentLocation = FadeComponent->GetRelativeLocation();
		FadeComponent->SetRelativeLocation(CurrentLocation + FVector(0.f, -1.f, 0.f));
		
		// Fade компонент начинает с полной видимости и будет исчезать
		FadeComponent->SetVisibility(true, true);
		SetAnimationAlpha(FadeComponent, 1.0f);
		SetTargetAlpha(FadeComponent, 0.0f);
		FadingOutComponents.Add(FadeComponent);
		
		// Очищаем главный компонент
		MainComponent->SetSprite(nullptr);
		MainComponent->SetVisibility(false);
		
		return;
	}
	
	// === СЛУЧАЙ 2: ПУСТОЕ → КОНТЕНТ (Простое появление) ===
	if (!bCurrentlyHasSprite && bWillHaveSprite)
	{
		VN_LOG_DEBUG(TEXT("PrepareSpriteTransition: %s - Empty to Content transition"), *MainComponent->GetName());
		
		// Устанавливаем новый sprite
		ValidateAndSetupSpriteComponentSilent(MainComponent, NewSprite);
		
		// Устанавливаем альфу 0 ПОСЛЕ установки sprite
		SetAnimationAlpha(MainComponent, 0.0f);
		
		// Настраиваем для появления
		FLinearColor TargetColor = GetTargetColorForComponent(MainComponent);
		SetTargetAlpha(MainComponent, TargetColor.A);
		FadingInComponents.Add(MainComponent);
		
		return;
	}
	
	// === СЛУЧАЙ 3: КОНТЕНТ → ДРУГОЙ КОНТЕНТ (Классический нахлест) ===
	if (bCurrentlyHasSprite && bWillHaveSprite)
	{
		VN_LOG_DEBUG(TEXT("PrepareSpriteTransition: %s - Content to Content crossfade"), *MainComponent->GetName());
		
		// Переносим старый контент в fade компонент
		CopySpriteComponentSettings(MainComponent, FadeComponent);
		
		// Настраиваем позицию fade компонента
		FVector CurrentLocation = FadeComponent->GetRelativeLocation();
		FadeComponent->SetRelativeLocation(CurrentLocation + FVector(0.f, -1.f, 0.f));
		
		// Fade компонент начинает с полной видимости и будет исчезать
		FadeComponent->SetVisibility(true, true);
		SetAnimationAlpha(FadeComponent, 1.0f);
		SetTargetAlpha(FadeComponent, 0.0f);
		FadingOutComponents.Add(FadeComponent);
		
		// Устанавливаем новый контент в main компонент
		ValidateAndSetupSpriteComponentSilent(MainComponent, NewSprite);
		SetAnimationAlpha(MainComponent, 0.0f);
		
		// Сбрасываем attachment для правильной настройки
		ResetComponentAttachmentToDefault(MainComponent);
		
		// Определяем целевую альфу для main компонента
		FLinearColor TargetColor = GetTargetColorForComponent(MainComponent);
		SetTargetAlpha(MainComponent, TargetColor.A);
		FadingInComponents.Add(MainComponent);
		
		return;
	}
	
	// === СЛУЧАЙ 4: ПУСТОЕ → ПУСТОЕ (Ничего не делаем) ===
	VN_LOG_DEBUG(TEXT("PrepareSpriteTransition: %s - Empty to Empty (no action)"), *MainComponent->GetName());
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
	
	// Убеждаемся, что все main компоненты имеют правильную альфу
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
		VN_LOG_ERROR(TEXT("ValidateAndSetupSkeletalComponentSilent: Component is null"));
		return;
	}
	
	if (!SkeletalMesh.IsNull())
	{
		USkeletalMesh* LoadedMesh = SkeletalMesh.LoadSynchronous();
		if (LoadedMesh)
		{
			Component->SetSkeletalMesh(LoadedMesh);
			Component->SetVisibility(true);
			// НЕ применяем цвет автоматически - это будет сделано через систему анимации
			VN_LOG_DEBUG(TEXT("ValidateAndSetupSkeletalComponentSilent: Set mesh for %s"), *Component->GetName());
		}
		else
		{
			Component->SetSkeletalMesh(nullptr);
			Component->SetAnimInstanceClass(nullptr);
			Component->SetVisibility(false);
			VN_LOG_WARNING(TEXT("ValidateAndSetupSkeletalComponentSilent: Failed to load mesh for %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSkeletalMesh(nullptr);
		Component->SetAnimInstanceClass(nullptr);
		Component->SetVisibility(false);
		VN_LOG_DEBUG(TEXT("ValidateAndSetupSkeletalComponentSilent: Cleared mesh for %s"), *Component->GetName());
	}
}

void AVNCharacter::ValidateAndSetupSpriteComponentSilent(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite> Sprite)
{
	if (!Component) 
	{
		VN_LOG_ERROR(TEXT("ValidateAndSetupSpriteComponentSilent: Component is null"));
		return;
	}
	
	if (!Sprite.IsNull())
	{
		UPaperSprite* LoadedSprite = Sprite.LoadSynchronous();
		if (LoadedSprite)
		{
			Component->SetSprite(LoadedSprite);
			Component->SetVisibility(true);
			// НЕ применяем цвет автоматически - это будет сделано через систему анимации
			VN_LOG_DEBUG(TEXT("ValidateAndSetupSpriteComponentSilent: Set sprite for %s"), *Component->GetName());
		}
		else
		{
			Component->SetSprite(nullptr);
			Component->SetVisibility(false);
			VN_LOG_WARNING(TEXT("ValidateAndSetupSpriteComponentSilent: Failed to load sprite for %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSprite(nullptr);
		Component->SetVisibility(false);
		VN_LOG_DEBUG(TEXT("ValidateAndSetupSpriteComponentSilent: Cleared sprite for %s"), *Component->GetName());
	}
}