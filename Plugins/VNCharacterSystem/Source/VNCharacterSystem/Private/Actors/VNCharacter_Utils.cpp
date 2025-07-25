#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

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

void AVNCharacter::ApplyIndividualSpriteTransform(UPaperSpriteComponent* SpriteComponent, E_VN_ComponentID_Sprite ComponentID)
{
	if (!SpriteComponent) return;
	UpdateComponentTransform(SpriteComponent, FVector::ZeroVector, 1.0f);
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
// НОВАЯ СИСТЕМА УПРАВЛЕНИЯ АЛЬФОЙ ДЛЯ АНИМАЦИИ
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
    
    UE_LOG(LogTemp, Verbose, TEXT("SetAnimationAlpha: %s alpha set to %.2f"), *Component->GetName(), Alpha);
}

void AVNCharacter::SetTargetAlpha(USceneComponent* Component, float TargetAlpha)
{
    if (!Component) return;
    TargetAlpha = FMath::Clamp(TargetAlpha, 0.0f, 1.0f);
    ComponentTargetAlphas.Add(Component, TargetAlpha);
    UE_LOG(LogTemp, Log, TEXT("SetTargetAlpha: %s target alpha set to %.2f"), *Component->GetName(), TargetAlpha);
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
    UE_LOG(LogTemp, Log, TEXT("ClearAnimationAlphas: Cleared animation alphas for %s"), *Component->GetName());
}

// =====================================================
// УПРОЩЕННЫЕ ФУНКЦИИ БЕЗ InitialAlpha
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
			UE_LOG(LogTemp, Log, TEXT("ValidateAndSetupSkeletalComponent: Set mesh for %s"), *Component->GetName());
		}
		else
		{
			Component->SetSkeletalMesh(nullptr);
			Component->SetAnimInstanceClass(nullptr);
			Component->SetVisibility(false);
			UE_LOG(LogTemp, Warning, TEXT("ValidateAndSetupSkeletalComponent: Failed to load mesh for %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSkeletalMesh(nullptr);
		Component->SetAnimInstanceClass(nullptr);
		Component->SetVisibility(false);
		UE_LOG(LogTemp, Log, TEXT("ValidateAndSetupSkeletalComponent: Cleared mesh for %s"), *Component->GetName());
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
			UE_LOG(LogTemp, Log, TEXT("ValidateAndSetupSpriteComponent: Set sprite for %s"), *Component->GetName());
		}
		else
		{
			Component->SetSprite(nullptr);
			Component->SetVisibility(false);
			UE_LOG(LogTemp, Warning, TEXT("ValidateAndSetupSpriteComponent: Failed to load sprite for %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSprite(nullptr);
		Component->SetVisibility(false);
		UE_LOG(LogTemp, Log, TEXT("ValidateAndSetupSpriteComponent: Cleared sprite for %s"), *Component->GetName());
	}
}

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
	
	UE_LOG(LogTemp, Log, TEXT("CopySkeletalComponentSettings: Copying from %s to %s"), *Source->GetName(), *Target->GetName());
	
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
	
	UE_LOG(LogTemp, Log, TEXT("CopySkeletalComponentSettings: %s settings copied to %s"), *Source->GetName(), *Target->GetName());
}

void AVNCharacter::CopySpriteComponentSettings(UPaperSpriteComponent* Source, UPaperSpriteComponent* Target)
{
	if (!Source || !Target) return;
	
	UE_LOG(LogTemp, Log, TEXT("CopySpriteComponentSettings: Copying from %s to %s"), *Source->GetName(), *Target->GetName());
	
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
	
	UE_LOG(LogTemp, Log, TEXT("CopySpriteComponentSettings: %s settings copied to %s with alpha 1.0"), *Source->GetName(), *Target->GetName());
}

// =====================================================
// УНИВЕРСАЛЬНАЯ КЛАССИЧЕСКАЯ СИСТЕМА ПЕРЕХОДОВ ДЛЯ ВСЕХ КОМПОНЕНТОВ
// =====================================================

void AVNCharacter::PrepareSkeletalTransition(USkeletalMeshComponent* MainComponent, USkeletalMeshComponent* FadeComponent, TSoftObjectPtr<USkeletalMesh> NewMesh)
{
	if (!MainComponent || !FadeComponent) 
	{
		UE_LOG(LogTemp, Error, TEXT("PrepareSkeletalTransition: Invalid components passed"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("PrepareSkeletalTransition: Starting universal transition for %s"), *MainComponent->GetName());
	
	// === АНАЛИЗ СОСТОЯНИЯ ===
	bool bCurrentlyHasMesh = (MainComponent->GetSkeletalMeshAsset() != nullptr && MainComponent->IsVisible());
	bool bWillHaveMesh = !NewMesh.IsNull();
	
	UE_LOG(LogTemp, Log, TEXT("PrepareSkeletalTransition: %s - Current: %s, Target: %s"), 
		*MainComponent->GetName(),
		bCurrentlyHasMesh ? TEXT("HasMesh") : TEXT("Empty"),
		bWillHaveMesh ? TEXT("HasMesh") : TEXT("Empty"));
	
	// === СЛУЧАЙ 1: КОНТЕНТ → ПУСТОЕ (Перенос в fade + исчезновение) ===
	if (bCurrentlyHasMesh && !bWillHaveMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("PrepareSkeletalTransition: %s - Content to Empty (transfer to fade and disappear)"), *MainComponent->GetName());
		
		// ШАГ 1: ПЕРЕНОСИМ текущий контент в fade компонент
		CopySkeletalComponentSettings(MainComponent, FadeComponent);
		
		// Настраиваем позицию fade компонента (чуть смещаем для слоя)
		FVector CurrentLocation = FadeComponent->GetRelativeLocation();
		FadeComponent->SetRelativeLocation(CurrentLocation + FVector(0.f, -1.f, 0.f));
		
		// ШАГ 2: Fade компонент начинает с полной видимости и будет исчезать
		FadeComponent->SetVisibility(true, true);
		SetAnimationAlpha(FadeComponent, 1.0f);  // НАЧИНАЕМ с 1.0!
		SetTargetAlpha(FadeComponent, 0.0f);     // ЦЕЛЬ - 0.0
		FadingOutComponents.Add(FadeComponent);
		
		// ШАГ 3: ОЧИЩАЕМ главный компонент (он станет пустым)
		MainComponent->SetSkeletalMesh(nullptr);
		MainComponent->SetAnimInstanceClass(nullptr);
		MainComponent->SetVisibility(false);
		
		UE_LOG(LogTemp, Warning, TEXT("PrepareSkeletalTransition: %s content transferred to %s, %s will fade OUT (1.0→0.0)"), 
			*MainComponent->GetName(), *FadeComponent->GetName(), *FadeComponent->GetName());
		return;
	}
	
	// === СЛУЧАЙ 2: ПУСТОЕ → КОНТЕНТ (Простое появление) ===
	if (!bCurrentlyHasMesh && bWillHaveMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("PrepareSkeletalTransition: %s - Empty to Content (fade in)"), *MainComponent->GetName());
		
		// Устанавливаем новый mesh
		ValidateAndSetupSkeletalComponentSilent(MainComponent, NewMesh);
		
		// ВАЖНО: Устанавливаем альфу 0 ПОСЛЕ установки mesh
		SetAnimationAlpha(MainComponent, 0.0f);
		
		// Настраиваем для появления
		FLinearColor TargetColor = GetTargetColorForComponent(MainComponent);
		SetTargetAlpha(MainComponent, TargetColor.A);
		FadingInComponents.Add(MainComponent);
		
		UE_LOG(LogTemp, Warning, TEXT("PrepareSkeletalTransition: %s will fade IN from 0.0 to %.2f"), *MainComponent->GetName(), TargetColor.A);
		return;
	}
	
	// === СЛУЧАЙ 3: КОНТЕНТ → ДРУГОЙ КОНТЕНТ (Классический нахлест) ===
	if (bCurrentlyHasMesh && bWillHaveMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("PrepareSkeletalTransition: %s - Content to Content (crossfade)"), *MainComponent->GetName());
		
		// ШАГ 1: ПЕРЕНОСИМ старый контент в fade компонент
		CopySkeletalComponentSettings(MainComponent, FadeComponent);
		
		// Настраиваем позицию fade компонента (чуть смещаем для слоя)
		FVector CurrentLocation = FadeComponent->GetRelativeLocation();
		FadeComponent->SetRelativeLocation(CurrentLocation + FVector(0.f, -1.f, 0.f));
		
		// Fade компонент начинает с полной видимости и будет исчезать
		FadeComponent->SetVisibility(true, true);
		SetAnimationAlpha(FadeComponent, 1.0f);  // НАЧИНАЕМ с 1.0!
		SetTargetAlpha(FadeComponent, 0.0f);     // ЦЕЛЬ - 0.0
		FadingOutComponents.Add(FadeComponent);
		
		// ШАГ 2: УСТАНАВЛИВАЕМ новый контент в main компонент
		ValidateAndSetupSkeletalComponentSilent(MainComponent, NewMesh);
		
		// ВАЖНО: Устанавливаем альфу 0 ПОСЛЕ установки mesh
		SetAnimationAlpha(MainComponent, 0.0f);  // НАЧИНАЕМ с 0.0!
		
		// Сбрасываем attachment для правильной настройки
		ResetComponentAttachmentToDefault(MainComponent);
		
		// Определяем целевую альфу для main компонента
		FLinearColor TargetColor = GetTargetColorForComponent(MainComponent);
		SetTargetAlpha(MainComponent, TargetColor.A);  // ЦЕЛЬ - 1.0
		FadingInComponents.Add(MainComponent);
		
		UE_LOG(LogTemp, Warning, TEXT("PrepareSkeletalTransition: %s crossfade - %s fades OUT (1.0→0.0), %s fades IN (0.0→%.2f)"), 
			*MainComponent->GetName(), *FadeComponent->GetName(), *MainComponent->GetName(), TargetColor.A);
		return;
	}
	
	// === СЛУЧАЙ 4: ПУСТОЕ → ПУСТОЕ (Ничего не делаем) ===
	UE_LOG(LogTemp, Log, TEXT("PrepareSkeletalTransition: %s - Empty to Empty (no action)"), *MainComponent->GetName());
}

void AVNCharacter::PrepareSpriteTransition(UPaperSpriteComponent* MainComponent, UPaperSpriteComponent* FadeComponent, TSoftObjectPtr<UPaperSprite> NewSprite)
{
	if (!MainComponent || !FadeComponent) 
	{
		UE_LOG(LogTemp, Error, TEXT("PrepareSpriteTransition: Invalid components passed"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: Starting universal transition for %s"), *MainComponent->GetName());
	
	// === АНАЛИЗ СОСТОЯНИЯ ===
	bool bCurrentlyHasSprite = (MainComponent->GetSprite() != nullptr && MainComponent->IsVisible());
	bool bWillHaveSprite = !NewSprite.IsNull();
	
	UE_LOG(LogTemp, Log, TEXT("PrepareSpriteTransition: %s - Current: %s, Target: %s"), 
		*MainComponent->GetName(),
		bCurrentlyHasSprite ? TEXT("HasSprite") : TEXT("Empty"),
		bWillHaveSprite ? TEXT("HasSprite") : TEXT("Empty"));
	
	// === СЛУЧАЙ 1: КОНТЕНТ → ПУСТОЕ (Перенос в fade + исчезновение) ===
	if (bCurrentlyHasSprite && !bWillHaveSprite)
	{
		UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: %s - Content to Empty (transfer to fade and disappear)"), *MainComponent->GetName());
		
		// ШАГ 1: ПЕРЕНОСИМ текущий контент в fade компонент
		CopySpriteComponentSettings(MainComponent, FadeComponent);
		
		// Настраиваем позицию fade компонента (чуть смещаем для слоя)
		FVector CurrentLocation = FadeComponent->GetRelativeLocation();
		FadeComponent->SetRelativeLocation(CurrentLocation + FVector(0.f, -1.f, 0.f));
		
		// ШАГ 2: Fade компонент начинает с полной видимости и будет исчезать
		FadeComponent->SetVisibility(true, true);
		SetAnimationAlpha(FadeComponent, 1.0f);  // НАЧИНАЕМ с 1.0!
		SetTargetAlpha(FadeComponent, 0.0f);     // ЦЕЛЬ - 0.0
		FadingOutComponents.Add(FadeComponent);
		
		// ШАГ 3: ОЧИЩАЕМ главный компонент (он станет пустым)
		MainComponent->SetSprite(nullptr);
		MainComponent->SetVisibility(false);
		
		UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: %s content transferred to %s, %s will fade OUT (1.0→0.0)"), 
			*MainComponent->GetName(), *FadeComponent->GetName(), *FadeComponent->GetName());
		return;
	}
	
	// === СЛУЧАЙ 2: ПУСТОЕ → КОНТЕНТ (Простое появление) ===
	if (!bCurrentlyHasSprite && bWillHaveSprite)
	{
		UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: %s - Empty to Content (fade in)"), *MainComponent->GetName());
		
		// Устанавливаем новый sprite
		ValidateAndSetupSpriteComponentSilent(MainComponent, NewSprite);
		
		// ВАЖНО: Устанавливаем альфу 0 ПОСЛЕ установки sprite
		SetAnimationAlpha(MainComponent, 0.0f);
		
		// Настраиваем для появления
		FLinearColor TargetColor = GetTargetColorForComponent(MainComponent);
		SetTargetAlpha(MainComponent, TargetColor.A);
		FadingInComponents.Add(MainComponent);
		
		UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: %s will fade IN from 0.0 to %.2f"), *MainComponent->GetName(), TargetColor.A);
		return;
	}
	
	// === СЛУЧАЙ 3: КОНТЕНТ → ДРУГОЙ КОНТЕНТ (Классический нахлест) ===
	if (bCurrentlyHasSprite && bWillHaveSprite)
	{
		UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: %s - Content to Content (crossfade)"), *MainComponent->GetName());
		
		// ШАГ 1: ПЕРЕНОСИМ старый контент в fade компонент
		CopySpriteComponentSettings(MainComponent, FadeComponent);
		
		// Настраиваем позицию fade компонента (чуть смещаем для слоя)
		FVector CurrentLocation = FadeComponent->GetRelativeLocation();
		FadeComponent->SetRelativeLocation(CurrentLocation + FVector(0.f, -1.f, 0.f));
		
		// Fade компонент начинает с полной видимости и будет исчезать
		FadeComponent->SetVisibility(true, true);
		SetAnimationAlpha(FadeComponent, 1.0f);  // НАЧИНАЕМ с 1.0!
		SetTargetAlpha(FadeComponent, 0.0f);     // ЦЕЛЬ - 0.0
		FadingOutComponents.Add(FadeComponent);
		
		// ШАГ 2: УСТАНАВЛИВАЕМ новый контент в main компонент
		ValidateAndSetupSpriteComponentSilent(MainComponent, NewSprite);
		
		// ВАЖНО: Устанавливаем альфу 0 ПОСЛЕ установки sprite
		SetAnimationAlpha(MainComponent, 0.0f);  // НАЧИНАЕМ с 0.0!
		
		// Сбрасываем attachment для правильной настройки
		ResetComponentAttachmentToDefault(MainComponent);
		
		// Определяем целевую альфу для main компонента
		FLinearColor TargetColor = GetTargetColorForComponent(MainComponent);
		SetTargetAlpha(MainComponent, TargetColor.A);  // ЦЕЛЬ - 1.0
		FadingInComponents.Add(MainComponent);
		
		UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: %s crossfade - %s fades OUT (1.0→0.0), %s fades IN (0.0→%.2f)"), 
			*MainComponent->GetName(), *FadeComponent->GetName(), *MainComponent->GetName(), TargetColor.A);
		return;
	}
	
	// === СЛУЧАЙ 4: ПУСТОЕ → ПУСТОЕ (Ничего не делаем) ===
	UE_LOG(LogTemp, Log, TEXT("PrepareSpriteTransition: %s - Empty to Empty (no action)"), *MainComponent->GetName());
}

void AVNCharacter::FinishTransition(USceneComponent* MainComponent, USceneComponent* FadeComponent)
{
	if (!MainComponent || !FadeComponent) return;
	
	UE_LOG(LogTemp, Warning, TEXT("FinishTransition: Finishing transition for %s and %s"), 
		*MainComponent->GetName(), *FadeComponent->GetName());
	
	// Применяем целевую альфу к main компоненту
	float TargetAlpha = GetTargetAlpha(MainComponent);
	SetAnimationAlpha(MainComponent, TargetAlpha);
	
	// Скрываем fade компонент
	FadeComponent->SetVisibility(false);
	SetAnimationAlpha(FadeComponent, 0.0f);
	
	if (USkeletalMeshComponent* SkeletalFade = Cast<USkeletalMeshComponent>(FadeComponent))
	{
		SkeletalFade->SetSkeletalMesh(nullptr);
		SkeletalFade->SetAnimInstanceClass(nullptr);
	}
	else if (UPaperSpriteComponent* SpriteFade = Cast<UPaperSpriteComponent>(FadeComponent))
	{
		SpriteFade->SetSprite(nullptr);
	}
	
	// Очищаем данные анимации
	ClearAnimationAlphas(MainComponent);
	ClearAnimationAlphas(FadeComponent);
	
	UE_LOG(LogTemp, Log, TEXT("FinishTransition: Applied target alpha %.2f to %s"), 
		TargetAlpha, *MainComponent->GetName());
}

void AVNCharacter::HideAllFadeComponents()
{
	UE_LOG(LogTemp, Warning, TEXT("HideAllFadeComponents: Hiding all fade components"));
	
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
	
	// КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Убеждаемся, что все main компоненты имеют правильную альфу
	for (USceneComponent* Component : FadingInComponents)
	{
		if (Component)
		{
			// Применяем целевую альфу, если она есть
			float TargetAlpha = GetTargetAlpha(Component);
			if (TargetAlpha != 1.0f || ComponentTargetAlphas.Contains(Component))
			{
				SetAnimationAlpha(Component, TargetAlpha);
				UE_LOG(LogTemp, Log, TEXT("HideAllFadeComponents: Applied target alpha %.2f to %s"), 
					TargetAlpha, *Component->GetName());
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
	
	UE_LOG(LogTemp, Log, TEXT("HideAllFadeComponents: Hidden %d fade components"), FadeComponents.Num());
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
	
	if (!SkeletalMesh.IsNull())
	{
		USkeletalMesh* LoadedMesh = SkeletalMesh.LoadSynchronous();
		if (LoadedMesh)
		{
			Component->SetSkeletalMesh(LoadedMesh);
			Component->SetVisibility(true);
			// НЕ применяем цвет автоматически - это будет сделано через систему анимации
			UE_LOG(LogTemp, Log, TEXT("ValidateAndSetupSkeletalComponentSilent: Set mesh for %s"), *Component->GetName());
		}
		else
		{
			Component->SetSkeletalMesh(nullptr);
			Component->SetAnimInstanceClass(nullptr);
			Component->SetVisibility(false);
			UE_LOG(LogTemp, Warning, TEXT("ValidateAndSetupSkeletalComponentSilent: Failed to load mesh for %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSkeletalMesh(nullptr);
		Component->SetAnimInstanceClass(nullptr);
		Component->SetVisibility(false);
		UE_LOG(LogTemp, Log, TEXT("ValidateAndSetupSkeletalComponentSilent: Cleared mesh for %s"), *Component->GetName());
	}
}

void AVNCharacter::ValidateAndSetupSpriteComponentSilent(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite> Sprite)
{
	if (!Component) 
	{
		UE_LOG(LogTemp, Error, TEXT("ValidateAndSetupSpriteComponentSilent: Component is null"));
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
			UE_LOG(LogTemp, Log, TEXT("ValidateAndSetupSpriteComponentSilent: Set sprite for %s"), *Component->GetName());
		}
		else
		{
			Component->SetSprite(nullptr);
			Component->SetVisibility(false);
			UE_LOG(LogTemp, Warning, TEXT("ValidateAndSetupSpriteComponentSilent: Failed to load sprite for %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSprite(nullptr);
		Component->SetVisibility(false);
		UE_LOG(LogTemp, Log, TEXT("ValidateAndSetupSpriteComponentSilent: Cleared sprite for %s"), *Component->GetName());
	}
}