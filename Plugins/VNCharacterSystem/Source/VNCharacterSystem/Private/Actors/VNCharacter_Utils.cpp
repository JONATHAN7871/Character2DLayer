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
	
	Target->SetSprite(Source->GetSprite());
	Target->SetSpriteColor(Source->GetSpriteColor());
	Target->SetVisibility(Source->IsVisible());
	
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
// НОВАЯ СИСТЕМА ПОДГОТОВКИ ПЕРЕХОДОВ
// =====================================================

void AVNCharacter::PrepareSkeletalTransition(USkeletalMeshComponent* MainComponent, USkeletalMeshComponent* FadeComponent, TSoftObjectPtr<USkeletalMesh> NewMesh)
{
	if (!MainComponent || !FadeComponent) return;
	
	UE_LOG(LogTemp, Warning, TEXT("PrepareSkeletalTransition: Starting transition for %s"), *MainComponent->GetName());
	
	FadingInComponents.Add(MainComponent);
	FadingOutComponents.Add(FadeComponent);

	CopySkeletalComponentSettings(MainComponent, FadeComponent);

	if (!NewMesh.IsNull())
	{
		FadeComponent->SetLeaderPoseComponent(MainComponent);
	}
	
	FVector CurrentLocation = FadeComponent->GetRelativeLocation();
	FadeComponent->SetRelativeLocation(CurrentLocation + FVector(0.f, -1.f, 0.f));
	
	// Устанавливаем fade компонент на полную видимость
	SetAnimationAlpha(FadeComponent, 1.0f);
	SetTargetAlpha(FadeComponent, 0.0f); // Будет исчезать
	FadeComponent->SetVisibility(true, true);

	ResetComponentAttachmentToDefault(MainComponent);
	
	// Настраиваем новый компонент
	ValidateAndSetupSkeletalComponent(MainComponent, NewMesh);
	
	// КЛЮЧЕВОЕ ИЗМЕНЕНИЕ: Запоминаем целевую альфу из цвета компонента
	FLinearColor TargetColor = GetTargetColorForComponent(MainComponent);
	SetTargetAlpha(MainComponent, TargetColor.A);
	
	// Начинаем с нулевой альфы для плавного появления
	SetAnimationAlpha(MainComponent, 0.0f);
	
	UE_LOG(LogTemp, Warning, TEXT("PrepareSkeletalTransition: %s will fade IN (target alpha: %.2f), %s will fade OUT"), 
	    *MainComponent->GetName(), TargetColor.A, *FadeComponent->GetName());
}

void AVNCharacter::PrepareSpriteTransition(UPaperSpriteComponent* MainComponent, UPaperSpriteComponent* FadeComponent, TSoftObjectPtr<UPaperSprite> NewSprite)
{
	if (!MainComponent || !FadeComponent) return;
	
	UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: Starting transition for %s"), *MainComponent->GetName());
	
	FadingInComponents.Add(MainComponent);
	FadingOutComponents.Add(FadeComponent);

	CopySpriteComponentSettings(MainComponent, FadeComponent);

	FVector CurrentLocation = FadeComponent->GetRelativeLocation();
	FadeComponent->SetRelativeLocation(CurrentLocation + FVector(0.f, -1.f, 0.f));
	
	// Устанавливаем fade компонент на полную видимость
	SetAnimationAlpha(FadeComponent, 1.0f);
	SetTargetAlpha(FadeComponent, 0.0f); // Будет исчезать
	FadeComponent->SetVisibility(true, true);
	
	ResetComponentAttachmentToDefault(MainComponent);
	
	// Настраиваем новый компонент
	ValidateAndSetupSpriteComponent(MainComponent, NewSprite);
	
	// КЛЮЧЕВОЕ ИЗМЕНЕНИЕ: Запоминаем целевую альфу из цвета компонента
	FLinearColor TargetColor = GetTargetColorForComponent(MainComponent);
	SetTargetAlpha(MainComponent, TargetColor.A);
	
	// Начинаем с нулевой альфы для плавного появления
	SetAnimationAlpha(MainComponent, 0.0f);
	
	UE_LOG(LogTemp, Warning, TEXT("PrepareSpriteTransition: %s will fade IN (target alpha: %.2f), %s will fade OUT"), 
	    *MainComponent->GetName(), TargetColor.A, *FadeComponent->GetName());
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
			ClearAnimationAlphas(Component);
			
			if (USkeletalMeshComponent* SkeletalFade = Cast<USkeletalMeshComponent>(Component))
			{
				SkeletalFade->SetSkeletalMesh(nullptr);
				SkeletalFade->SetAnimInstanceClass(nullptr);
			}
			else if (UPaperSpriteComponent* SpriteFade = Cast<UPaperSpriteComponent>(Component))
			{
				SpriteFade->SetSprite(nullptr);
			}
		}
	}
	
	// Очищаем все списки анимации и карты альфы
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