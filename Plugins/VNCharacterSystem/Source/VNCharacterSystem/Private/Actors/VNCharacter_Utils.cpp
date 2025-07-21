#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

// =====================================================
// ПРИМЕНЕНИЕ ГЛОБАЛЬНЫХ ТРАНСФОРМАЦИЙ
// =====================================================

void AVNCharacter::ApplyGlobalTransforms()
{
	VN_LOG_DEBUG(TEXT("Applying global transforms to individual components"));
	
	// Применяем глобальные настройки к каждому Skeletal Mesh компоненту индивидуально
	ApplyGlobalSkeletalTransforms();
	
	// Применяем глобальные настройки к каждому спрайту индивидуально
	ApplyGlobalSpriteTransforms();

	VN_LOG_DEBUG(TEXT("Global transforms applied: SkeletalOffset=%s, SkeletalScale=%.2f, SpriteOffset=%s, SpriteScale=%.2f"),
		*GlobalSkeletalOffset.ToString(), GlobalSkeletalScale,
		*GlobalSpriteOffset.ToString(), GlobalSpriteScale);
}

void AVNCharacter::ApplyGlobalSkeletalTransforms()
{
	// Получаем все Skeletal Mesh компоненты (основные + fade)
	TArray<USkeletalMeshComponent*> AllSkeletalMeshes;
	
	// Основные компоненты
	if (Body_Skeletal) AllSkeletalMeshes.Add(Body_Skeletal);
	if (Arms_Skeletal) AllSkeletalMeshes.Add(Arms_Skeletal);
	if (Head_Skeletal) AllSkeletalMeshes.Add(Head_Skeletal);
	if (Custom01_Skeletal) AllSkeletalMeshes.Add(Custom01_Skeletal);
	if (Custom02_Skeletal) AllSkeletalMeshes.Add(Custom02_Skeletal);
	if (Custom03_Skeletal) AllSkeletalMeshes.Add(Custom03_Skeletal);
	
	// Fade компоненты
	if (Body_Skeletal_Fade) AllSkeletalMeshes.Add(Body_Skeletal_Fade);
	if (Arms_Skeletal_Fade) AllSkeletalMeshes.Add(Arms_Skeletal_Fade);
	if (Head_Skeletal_Fade) AllSkeletalMeshes.Add(Head_Skeletal_Fade);
	if (Custom01_Skeletal_Fade) AllSkeletalMeshes.Add(Custom01_Skeletal_Fade);
	if (Custom02_Skeletal_Fade) AllSkeletalMeshes.Add(Custom02_Skeletal_Fade);
	if (Custom03_Skeletal_Fade) AllSkeletalMeshes.Add(Custom03_Skeletal_Fade);
	
	VN_LOG_DEBUG(TEXT("Applying global skeletal transforms to %d components"), AllSkeletalMeshes.Num());
	
	for (USkeletalMeshComponent* SkeletalMesh : AllSkeletalMeshes)
	{
		if (!SkeletalMesh) continue;
		
		// Создаем трансформ с глобальными настройками
		FTransform ComponentTransform = FTransform::Identity;
		ComponentTransform.SetLocation(GlobalSkeletalOffset);
		ComponentTransform.SetScale3D(FVector(GlobalSkeletalScale));
		
		// Применяем к компоненту
		SkeletalMesh->SetRelativeTransform(ComponentTransform);
		
		VN_LOG_DEBUG(TEXT("Applied global skeletal transform to %s: Offset=%s, Scale=%.2f"), 
			*SkeletalMesh->GetName(), *GlobalSkeletalOffset.ToString(), GlobalSkeletalScale);
	}
}

void AVNCharacter::ApplyGlobalSpriteTransforms()
{
	// Получаем все спрайт компоненты (основные + fade)
	TArray<UPaperSpriteComponent*> AllSprites;
	
	// Основные спрайты тела (прикреплены к GlobalSpriteTransform)
	if (Body_Sprite) AllSprites.Add(Body_Sprite);
	if (Arms_Sprite) AllSprites.Add(Arms_Sprite);
	if (BodyShadow_Sprite) AllSprites.Add(BodyShadow_Sprite);
	if (EmotionBodyEffect01_Sprite) AllSprites.Add(EmotionBodyEffect01_Sprite);
	if (EmotionBodyEffect02_Sprite) AllSprites.Add(EmotionBodyEffect02_Sprite);
	if (EmotionBodyEffect03_Sprite) AllSprites.Add(EmotionBodyEffect03_Sprite);
	
	// Fade спрайты тела
	if (Body_Sprite_Fade) AllSprites.Add(Body_Sprite_Fade);
	if (Arms_Sprite_Fade) AllSprites.Add(Arms_Sprite_Fade);
	if (BodyShadow_Sprite_Fade) AllSprites.Add(BodyShadow_Sprite_Fade);
	if (EmotionBodyEffect01_Sprite_Fade) AllSprites.Add(EmotionBodyEffect01_Sprite_Fade);
	if (EmotionBodyEffect02_Sprite_Fade) AllSprites.Add(EmotionBodyEffect02_Sprite_Fade);
	if (EmotionBodyEffect03_Sprite_Fade) AllSprites.Add(EmotionBodyEffect03_Sprite_Fade);
	
	// Head_Sprite (получает глобальные трансформации)
	if (Head_Sprite) AllSprites.Add(Head_Sprite);
	
	VN_LOG_DEBUG(TEXT("Applying global sprite transforms to %d components"), AllSprites.Num());
	
	for (UPaperSpriteComponent* Sprite : AllSprites)
	{
		if (!Sprite) continue;
		
		// Создаем трансформ с глобальными настройками
		FTransform ComponentTransform = FTransform::Identity;
		ComponentTransform.SetLocation(GlobalSpriteOffset);
		ComponentTransform.SetScale3D(FVector(GlobalSpriteScale));
		
		// Применяем к компоненту
		Sprite->SetRelativeTransform(ComponentTransform);
		
		VN_LOG_DEBUG(TEXT("Applied global sprite transform to %s: Offset=%s, Scale=%.2f"), 
			*Sprite->GetName(), *GlobalSpriteOffset.ToString(), GlobalSpriteScale);
	}
	
	// ВАЖНО: Дочерние спрайты головы НЕ получают глобальные трансформации напрямую
	// Они наследуют их через Head_Sprite, поэтому оставляем их трансформ как Identity
	ApplyChildHeadSpriteTransforms();
}

void AVNCharacter::ApplyChildHeadSpriteTransforms()
{
	// Дочерние спрайты головы - оставляем их трансформы как Identity
	// Они получат глобальные трансформации через наследование от Head_Sprite
	TArray<UPaperSpriteComponent*> ChildHeadSprites;
	
	// Основные дочерние спрайты
	if (Eyebrow_Sprite) ChildHeadSprites.Add(Eyebrow_Sprite);
	if (Eyes_Sprite) ChildHeadSprites.Add(Eyes_Sprite);
	if (Eyelids_Sprite) ChildHeadSprites.Add(Eyelids_Sprite);
	if (Wink_Sprite) ChildHeadSprites.Add(Wink_Sprite);
	if (Mouth_Sprite) ChildHeadSprites.Add(Mouth_Sprite);
	if (EmotionHeadEffect01_Sprite) ChildHeadSprites.Add(EmotionHeadEffect01_Sprite);
	if (EmotionHeadEffect02_Sprite) ChildHeadSprites.Add(EmotionHeadEffect02_Sprite);
	if (EmotionHeadEffect03_Sprite) ChildHeadSprites.Add(EmotionHeadEffect03_Sprite);
	
	// Fade дочерние спрайты
	if (Eyebrow_Sprite_Fade) ChildHeadSprites.Add(Eyebrow_Sprite_Fade);
	if (Eyes_Sprite_Fade) ChildHeadSprites.Add(Eyes_Sprite_Fade);
	if (Eyelids_Sprite_Fade) ChildHeadSprites.Add(Eyelids_Sprite_Fade);
	if (Wink_Sprite_Fade) ChildHeadSprites.Add(Wink_Sprite_Fade);
	if (Mouth_Sprite_Fade) ChildHeadSprites.Add(Mouth_Sprite_Fade);
	if (EmotionHeadEffect01_Sprite_Fade) ChildHeadSprites.Add(EmotionHeadEffect01_Sprite_Fade);
	if (EmotionHeadEffect02_Sprite_Fade) ChildHeadSprites.Add(EmotionHeadEffect02_Sprite_Fade);
	if (EmotionHeadEffect03_Sprite_Fade) ChildHeadSprites.Add(EmotionHeadEffect03_Sprite_Fade);
	
	VN_LOG_DEBUG(TEXT("Setting identity transforms for %d child head sprites"), ChildHeadSprites.Num());
	
	for (UPaperSpriteComponent* ChildSprite : ChildHeadSprites)
	{
		if (!ChildSprite) continue;
		
		// Устанавливаем Identity трансформ - они получат глобальные настройки через Head_Sprite
		FTransform IdentityTransform = FTransform::Identity;
		ChildSprite->SetRelativeTransform(IdentityTransform);
		
		VN_LOG_DEBUG(TEXT("Set identity transform for child sprite: %s"), *ChildSprite->GetName());
	}
}

void AVNCharacter::ApplyIndividualSpriteTransform(UPaperSpriteComponent* SpriteComponent, E_VN_ComponentID_Sprite ComponentID)
{
	if (!SpriteComponent)
	{
		return;
	}

	// Дочерние элементы Head_Sprite получают только Identity трансформ
	// Глобальные трансформации они наследуют через Head_Sprite
	if (IsChildOfHeadSprite(ComponentID))
	{
		FTransform IdentityTransform = FTransform::Identity;
		SpriteComponent->SetRelativeTransform(IdentityTransform);
		VN_LOG_DEBUG(TEXT("Applied identity transform to child sprite %s"), *SpriteComponent->GetName());
		return;
	}

	// Остальные спрайты получают глобальные трансформации
	FTransform ComponentTransform = FTransform::Identity;
	ComponentTransform.SetLocation(GlobalSpriteOffset);
	ComponentTransform.SetScale3D(FVector(GlobalSpriteScale));
	SpriteComponent->SetRelativeTransform(ComponentTransform);
	
	VN_LOG_DEBUG(TEXT("Applied global sprite transform to %s: Offset=%s, Scale=%.2f"), 
		*SpriteComponent->GetName(), *GlobalSpriteOffset.ToString(), GlobalSpriteScale);
}

bool AVNCharacter::IsChildOfHeadSprite(E_VN_ComponentID_Sprite ComponentID) const
{
	// Проверяем, является ли спрайт одним из лицевых элементов
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

// =====================================================
// ВАЛИДАЦИЯ И НАСТРОЙКА КОМПОНЕНТОВ
// =====================================================

void AVNCharacter::ValidateAndSetupSkeletalComponent(USkeletalMeshComponent* Component, TSoftObjectPtr<USkeletalMesh> SkeletalMesh)
{
	if (!Component)
	{
		VN_LOG_WARNING(TEXT("ValidateAndSetupSkeletalComponent: Component is null"));
		return;
	}

	// УПРОЩЕННАЯ ВАЛИДАЦИЯ: Если есть меш - показываем, если нет - скрываем
	if (!SkeletalMesh.IsNull())
	{
		// Загружаем и устанавливаем меш
		USkeletalMesh* LoadedMesh = SkeletalMesh.LoadSynchronous();
		if (LoadedMesh)
		{
			Component->SetSkeletalMesh(LoadedMesh);
			Component->SetVisibility(true);
			
			// Устанавливаем цвет с учетом фокуса
			FLinearColor TargetColor = GetTargetColorForComponent(Component);
			SetComponentColor(Component, TargetColor);
			
			VN_LOG_DEBUG(TEXT("Skeletal component %s: Mesh set and visible"), *Component->GetName());
		}
		else
		{
			VN_LOG_WARNING(TEXT("Failed to load SkeletalMesh for component %s"), *Component->GetName());
			Component->SetVisibility(false);
		}
	}
	else
	{
		// Нет меша - скрываем компонент
		Component->SetSkeletalMesh(nullptr);
		Component->SetVisibility(false);
		VN_LOG_DEBUG(TEXT("Skeletal component %s: Hidden (no mesh)"), *Component->GetName());
	}
}

void AVNCharacter::ValidateAndSetupSpriteComponent(UPaperSpriteComponent* Component, TSoftObjectPtr<UPaperSprite> Sprite)
{
	if (!Component)
	{
		VN_LOG_WARNING(TEXT("ValidateAndSetupSpriteComponent: Component is null"));
		return;
	}

	// УПРОЩЕННАЯ ВАЛИДАЦИЯ: Если есть спрайт - показываем, если нет - скрываем
	if (!Sprite.IsNull())
	{
		// Загружаем и устанавливаем спрайт
		UPaperSprite* LoadedSprite = Sprite.LoadSynchronous();
		if (LoadedSprite)
		{
			Component->SetSprite(LoadedSprite);
			Component->SetVisibility(true);
			
			// Устанавливаем цвет с учетом фокуса
			FLinearColor TargetColor = GetTargetColorForComponent(Component);
			SetComponentColor(Component, TargetColor);
			
			VN_LOG_DEBUG(TEXT("Sprite component %s: Sprite set and visible"), *Component->GetName());
		}
		else
		{
			VN_LOG_WARNING(TEXT("Failed to load Sprite for component %s"), *Component->GetName());
			Component->SetVisibility(false);
		}
	}
	else
	{
		// Нет спрайта - скрываем компонент
		Component->SetSprite(nullptr);
		Component->SetVisibility(false);
		VN_LOG_DEBUG(TEXT("Sprite component %s: Hidden (no sprite)"), *Component->GetName());
	}
}

// =====================================================
// УТИЛИТЫ ДЛЯ РАБОТЫ С МАТЕРИАЛАМИ И ЦВЕТАМИ
// =====================================================

void AVNCharacter::SetComponentAlpha(USceneComponent* Component, float Alpha)
{
	if (!Component)
	{
		return;
	}

	Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

	// Для Skeletal Mesh компонентов
	if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(Component))
	{
		// Создаем или получаем динамический материал
		for (int32 i = 0; i < SkeletalMesh->GetNumMaterials(); ++i)
		{
			UMaterialInterface* BaseMaterial = SkeletalMesh->GetMaterial(i);
			if (BaseMaterial)
			{
				UMaterialInstanceDynamic* DynamicMaterial = SkeletalMesh->CreateAndSetMaterialInstanceDynamic(i);
				if (DynamicMaterial)
				{
					DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), Alpha);
				}
			}
		}
	}
	// Для Sprite компонентов
	else if (UPaperSpriteComponent* SpriteComponent = Cast<UPaperSpriteComponent>(Component))
	{
		// Получаем текущий цвет и изменяем альфа
		FLinearColor CurrentColor = SpriteComponent->GetSpriteColor();
		CurrentColor.A = Alpha;
		SpriteComponent->SetSpriteColor(CurrentColor);
	}
}

void AVNCharacter::SetComponentColor(USceneComponent* Component, const FLinearColor& Color)
{
	if (!Component)
	{
		return;
	}

	// Для Skeletal Mesh компонентов
	if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(Component))
	{
		// Создаем или получаем динамический материал
		for (int32 i = 0; i < SkeletalMesh->GetNumMaterials(); ++i)
		{
			UMaterialInterface* BaseMaterial = SkeletalMesh->GetMaterial(i);
			if (BaseMaterial)
			{
				UMaterialInstanceDynamic* DynamicMaterial = SkeletalMesh->CreateAndSetMaterialInstanceDynamic(i);
				if (DynamicMaterial)
				{
					DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
					DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), Color.A);
				}
			}
		}
	}
	// Для Sprite компонентов
	else if (UPaperSpriteComponent* SpriteComponent = Cast<UPaperSpriteComponent>(Component))
	{
		SpriteComponent->SetSpriteColor(Color);
	}
}

// =====================================================
// КОПИРОВАНИЕ НАСТРОЕК МЕЖДУ КОМПОНЕНТАМИ
// =====================================================

void AVNCharacter::CopySkeletalComponentSettings(USkeletalMeshComponent* Source, USkeletalMeshComponent* Target)
{
	if (!Source || !Target)
	{
		return;
	}

	// Копируем основные настройки
	Target->SetSkeletalMesh(Source->GetSkeletalMeshAsset());
	Target->SetAnimInstanceClass(Source->GetAnimClass());
	Target->SetRelativeTransform(Source->GetRelativeTransform());
	Target->SetVisibility(Source->IsVisible());

	// Копируем материалы
	for (int32 i = 0; i < Source->GetNumMaterials(); ++i)
	{
		Target->SetMaterial(i, Source->GetMaterial(i));
	}

	VN_LOG_DEBUG(TEXT("Copied settings from %s to %s"), *Source->GetName(), *Target->GetName());
}

void AVNCharacter::CopySpriteComponentSettings(UPaperSpriteComponent* Source, UPaperSpriteComponent* Target)
{
	if (!Source || !Target)
	{
		return;
	}

	// Копируем основные настройки
	Target->SetSprite(Source->GetSprite());
	Target->SetSpriteColor(Source->GetSpriteColor());
	Target->SetRelativeTransform(Source->GetRelativeTransform());
	Target->SetVisibility(Source->IsVisible());

	VN_LOG_DEBUG(TEXT("Copied settings from %s to %s"), *Source->GetName(), *Target->GetName());
}

// =====================================================
// ПОДГОТОВКА ПЕРЕХОДОВ ДЛЯ АНИМАЦИИ
// =====================================================

void AVNCharacter::PrepareSkeletalTransition(USkeletalMeshComponent* MainComponent, USkeletalMeshComponent* FadeComponent, TSoftObjectPtr<USkeletalMesh> NewMesh)
{
	if (!MainComponent || !FadeComponent)
	{
		VN_LOG_WARNING(TEXT("PrepareSkeletalTransition: Invalid components"));
		return;
	}

	VN_LOG_DEBUG(TEXT("PrepareSkeletalTransition: %s -> new mesh"), *MainComponent->GetName());

	// Копируем текущие настройки главного компонента в fade компонент
	CopySkeletalComponentSettings(MainComponent, FadeComponent);
	
	// Fade компонент будет исчезать (alpha от 1 до 0)
	SetComponentAlpha(FadeComponent, 1.0f);
	FadeComponent->SetVisibility(true);

	// Настраиваем главный компонент с новым мешем
	ValidateAndSetupSkeletalComponent(MainComponent, NewMesh);
	
	// Главный компонент будет появляться (alpha от 0 до 1)
	SetComponentAlpha(MainComponent, 0.0f);
}

void AVNCharacter::PrepareSpriteTransition(UPaperSpriteComponent* MainComponent, UPaperSpriteComponent* FadeComponent, TSoftObjectPtr<UPaperSprite> NewSprite)
{
	if (!MainComponent || !FadeComponent)
	{
		VN_LOG_WARNING(TEXT("PrepareSpriteTransition: Invalid components"));
		return;
	}

	VN_LOG_DEBUG(TEXT("PrepareSpriteTransition: %s -> new sprite"), *MainComponent->GetName());

	// Копируем текущие настройки главного компонента в fade компонент
	CopySpriteComponentSettings(MainComponent, FadeComponent);
	
	// Fade компонент будет исчезать (alpha от 1 до 0)
	SetComponentAlpha(FadeComponent, 1.0f);
	FadeComponent->SetVisibility(true);

	// Настраиваем главный компонент с новым спрайтом
	ValidateAndSetupSpriteComponent(MainComponent, NewSprite);
	
	// Определяем ComponentID для применения правильных трансформаций
	E_VN_ComponentID_Sprite ComponentID = E_VN_ComponentID_Sprite::Body; // Значение по умолчанию
	
	// Находим ComponentID по указателю на компонент
	if (MainComponent == Eyes_Sprite) ComponentID = E_VN_ComponentID_Sprite::Eyes;
	else if (MainComponent == Mouth_Sprite) ComponentID = E_VN_ComponentID_Sprite::Mouth;
	else if (MainComponent == Eyebrow_Sprite) ComponentID = E_VN_ComponentID_Sprite::Eyebrow;
	else if (MainComponent == Body_Sprite) ComponentID = E_VN_ComponentID_Sprite::Body;
	else if (MainComponent == Arms_Sprite) ComponentID = E_VN_ComponentID_Sprite::Arms;
	// ... добавить остальные компоненты при необходимости
	
	// Применяем правильные трансформации
	ApplyIndividualSpriteTransform(MainComponent, ComponentID);
	
	// Главный компонент будет появляться (alpha от 0 до 1)
	SetComponentAlpha(MainComponent, 0.0f);
}

void AVNCharacter::FinishTransition(USceneComponent* MainComponent, USceneComponent* FadeComponent)
{
	if (!MainComponent || !FadeComponent)
	{
		return;
	}

	VN_LOG_DEBUG(TEXT("FinishTransition: %s"), *MainComponent->GetName());

	// Устанавливаем полную непрозрачность для главного компонента
	SetComponentAlpha(MainComponent, 1.0f);

	// Скрываем и очищаем fade компонент
	FadeComponent->SetVisibility(false);
	SetComponentAlpha(FadeComponent, 0.0f);

	// Очищаем содержимое fade компонента
	if (USkeletalMeshComponent* SkeletalFade = Cast<USkeletalMeshComponent>(FadeComponent))
	{
		SkeletalFade->SetSkeletalMesh(nullptr);
	}
	else if (UPaperSpriteComponent* SpriteFade = Cast<UPaperSpriteComponent>(FadeComponent))
	{
		SpriteFade->SetSprite(nullptr);
	}

	VN_LOG_DEBUG(TEXT("Transition finished for %s"), *MainComponent->GetName());
}