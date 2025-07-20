/**
 * VNCharacter_ComponentSetup.cpp
 * 
 * Модуль для настройки и применения конфигураций к компонентам
 * Содержит методы для настройки Skeletal Mesh и Sprite компонентов
 * из различных типов конфигураций.
 */

#include "Actors/VNCharacter.h"
#include "VNCharacterSystemModule.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"

// =====================================================
// ВНУТРЕННИЕ МЕТОДЫ - ПРИМЕНЕНИЕ СОСТОЯНИЯ
// =====================================================

void AVNCharacter::ApplyCharacterState(const F_VN_CharacterState& State)
{
	VN_LOG_DEBUG(TEXT("ApplyCharacterState: Applying state '%s'"), *State.StateID.ToString());

	// Применяем конфигурации Skeletal Mesh компонентов
	SetupComponentFromConfig(Body_Skeletal, State.BodyConfig);
	SetupComponentFromConfig(Arms_Skeletal, State.ArmsConfig);
	SetupComponentFromConfig(Head_Skeletal, State.HeadConfig);
	SetupComponentFromConfig(Custom01_Skeletal, State.Custom01Config);
	SetupComponentFromConfig(Custom02_Skeletal, State.Custom02Config);
	SetupComponentFromConfig(Custom03_Skeletal, State.Custom03Config);

	// Применяем конфигурации Sprite компонентов (Attachment)
	SetupComponentFromConfig(Body_Sprite, State.BodySpriteConfig);
	SetupComponentFromConfig(Arms_Sprite, State.ArmsSpriteConfig);
	SetupComponentFromConfig(BodyShadow_Sprite, State.BodyShadowSpriteConfig);
	SetupComponentFromConfig(EmotionBody01_Sprite, State.EmotionBodyEffect01SpriteConfig);
	SetupComponentFromConfig(EmotionBody02_Sprite, State.EmotionBodyEffect02SpriteConfig);
	SetupComponentFromConfig(EmotionBody03_Sprite, State.EmotionBodyEffect03SpriteConfig);

	// Head_Sprite ТЕПЕРЬ использует Attachment конфигурацию
	SetupComponentFromConfig(Head_Sprite, State.HeadSpriteConfig);

	// Применяем конфигурации Sprite компонентов (Simple)
	SetupComponentFromConfig(Eyebrow_Sprite, State.EyebrowSpriteConfig);
	SetupComponentFromConfig(Eyes_Sprite, State.EyesSpriteConfig);
	SetupComponentFromConfig(Eyelids_Sprite, State.EyelidsSpriteConfig);
	SetupComponentFromConfig(Wink_Sprite, State.WinkSpriteConfig);
	SetupComponentFromConfig(Mouth_Sprite, State.MouthSpriteConfig);
	SetupComponentFromConfig(EmotionHead01_Sprite, State.EmotionHeadEffect01SpriteConfig);
	SetupComponentFromConfig(EmotionHead02_Sprite, State.EmotionHeadEffect02SpriteConfig);
	SetupComponentFromConfig(EmotionHead03_Sprite, State.EmotionHeadEffect03SpriteConfig);

	// Применяем глобальные настройки трансформации
	ApplyGlobalTransforms();

	VN_LOG_DEBUG(TEXT("Character state applied successfully"));
}

void AVNCharacter::ApplyGlobalTransforms()
{
	// НЕ применяем глобальные настройки к корневым трансформам!
	// GlobalSkeletalMeshTransform и GlobalSpriteTransform используются только для организации иерархии
	
	VN_LOG_DEBUG(TEXT("Applying global transforms individually to components"));
	
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
	// Получаем все Skeletal Mesh компоненты
	TArray<USkeletalMeshComponent*> AllSkeletalMeshes = GetAllSkeletalComponents();
	
	VN_LOG_DEBUG(TEXT("Applying global skeletal transforms to %d components"), AllSkeletalMeshes.Num());
	
	for (USkeletalMeshComponent* SkeletalMesh : AllSkeletalMeshes)
	{
		if (!SkeletalMesh) continue;
		
		// Получаем текущий relative transform компонента
		FTransform CurrentTransform = SkeletalMesh->GetRelativeTransform();
		
		// Применяем глобальные настройки ПОВЕРХ индивидуальных настроек
		FVector NewLocation = CurrentTransform.GetLocation() + GlobalSkeletalOffset;
		FVector NewScale = CurrentTransform.GetScale3D() * GlobalSkeletalScale;
		
		// Создаем новый трансформ
		FTransform NewTransform = CurrentTransform;
		NewTransform.SetLocation(NewLocation);
		NewTransform.SetScale3D(NewScale);
		
		// Применяем
		SkeletalMesh->SetRelativeTransform(NewTransform);
		
		VN_LOG_DEBUG(TEXT("Applied global skeletal transform to %s: Offset=%s, Scale=%.2f"), 
			*SkeletalMesh->GetName(), *GlobalSkeletalOffset.ToString(), GlobalSkeletalScale);
	}
}

void AVNCharacter::ApplyGlobalSpriteTransforms()
{
	// Получаем все спрайт компоненты
	TArray<UPaperSpriteComponent*> AllSprites = GetAllSpriteComponents();
	
	VN_LOG_DEBUG(TEXT("Applying global sprite transforms to %d components"), AllSprites.Num());
	
	for (UPaperSpriteComponent* Sprite : AllSprites)
	{
		if (!Sprite) continue;
		
		// ИСКЛЮЧАЕМ дочерние элементы Head_Sprite из глобальных трансформаций
		// Они получат трансформации через наследование от Head_Sprite
		if (IsChildOfHeadSprite(Sprite))
		{
			VN_LOG_DEBUG(TEXT("Skipping global transform for %s (child of Head_Sprite)"), *Sprite->GetName());
			continue;
		}
		
		// Получаем текущий relative transform спрайта
		FTransform CurrentTransform = Sprite->GetRelativeTransform();
		
		// Применяем глобальные настройки ПОВЕРХ индивидуальных настроек
		FVector NewLocation = CurrentTransform.GetLocation() + GlobalSpriteOffset;
		FVector NewScale = CurrentTransform.GetScale3D() * GlobalSpriteScale;
		
		// Создаем новый трансформ
		FTransform NewTransform = CurrentTransform;
		NewTransform.SetLocation(NewLocation);
		NewTransform.SetScale3D(NewScale);
		
		// Применяем
		Sprite->SetRelativeTransform(NewTransform);
		
		VN_LOG_DEBUG(TEXT("Applied global sprite transform to %s: Offset=%s, Scale=%.2f"), 
			*Sprite->GetName(), *GlobalSpriteOffset.ToString(), GlobalSpriteScale);
	}
}

// =====================================================
// МЕТОДЫ НАСТРОЙКИ КОМПОНЕНТОВ
// =====================================================

void AVNCharacter::SetupComponentFromConfig(USkeletalMeshComponent* Component, const F_VN_SkeletalConfig_Body& Config)
{
	if (!Component)
	{
		VN_LOG_WARNING(TEXT("SetupComponentFromConfig: SkeletalMeshComponent is null"));
		return;
	}

	// Устанавливаем видимость
	Component->SetVisibility(Config.bVisible);
	
	if (!Config.bVisible)
	{
		return; // Если компонент невидим, дальше не настраиваем
	}

	// Загружаем и устанавливаем Skeletal Mesh
	if (!Config.SkeletalMesh.IsNull())
	{
		USkeletalMesh* LoadedMesh = Config.SkeletalMesh.LoadSynchronous();
		if (LoadedMesh)
		{
			Component->SetSkeletalMesh(LoadedMesh);
		}
		else
		{
			VN_LOG_WARNING(TEXT("Failed to load SkeletalMesh for component %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSkeletalMesh(nullptr);
	}

	// Устанавливаем AnimInstance класс
	if (Config.AnimInstanceClass)
	{
		Component->SetAnimInstanceClass(Config.AnimInstanceClass);
	}

	// Применяем переопределения материалов
	for (const auto& MaterialPair : Config.MaterialOverrides)
	{
		int32 MaterialIndex = MaterialPair.Key;
		TSoftObjectPtr<UMaterialInterface> MaterialPtr = MaterialPair.Value;

		if (!MaterialPtr.IsNull())
		{
			UMaterialInterface* LoadedMaterial = MaterialPtr.LoadSynchronous();
			if (LoadedMaterial)
			{
				Component->SetMaterial(MaterialIndex, LoadedMaterial);
			}
		}
	}

	// Устанавливаем трансформ
	FTransform ComponentTransform = Component->GetRelativeTransform();
	ComponentTransform.SetLocation(Config.Offset);
	ComponentTransform.SetScale3D(FVector(Config.Scale));
	Component->SetRelativeTransform(ComponentTransform);

	// Устанавливаем цвет
	FLinearColor FinalColor = GetTargetColorForComponent(Component);
	SetComponentColor(Component, FinalColor);

	VN_LOG_DEBUG(TEXT("SkeletalMesh component %s configured from Attachment config"), *Component->GetName());
}

void AVNCharacter::SetupComponentFromConfig(UPaperSpriteComponent* Component, const F_VN_SpriteConfig_Attachment& Config)
{
	if (!Component)
	{
		VN_LOG_WARNING(TEXT("SetupComponentFromConfig: PaperSpriteComponent is null"));
		return;
	}

	// Устанавливаем видимость
	Component->SetVisibility(Config.bVisible);
	
	if (!Config.bVisible)
	{
		return; // Если компонент невидим, дальше не настраиваем
	}

	// Загружаем и устанавливаем Sprite
	if (!Config.Sprite.IsNull())
	{
		if (UPaperSprite* LoadedSprite = Config.Sprite.LoadSynchronous())
		{
			Component->SetSprite(LoadedSprite);
		}
		else
		{
			VN_LOG_WARNING(TEXT("Failed to load Sprite for component %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSprite(nullptr);
	}

	// Настраиваем прикрепление к Skeletal Mesh компонентам
	if (Config.AttachTo != E_SpriteAttachmentTarget::None)
	{
		USkeletalMeshComponent* AttachTarget = nullptr;
		
		switch (Config.AttachTo)
		{
			case E_SpriteAttachmentTarget::Body_Skeletal:
				AttachTarget = Body_Skeletal;
				break;
			case E_SpriteAttachmentTarget::Arms_Skeletal:
				AttachTarget = Arms_Skeletal;
				break;
			case E_SpriteAttachmentTarget::Head_Skeletal:
				AttachTarget = Head_Skeletal;
				break;
			case E_SpriteAttachmentTarget::Custom01_Skeletal:
				AttachTarget = Custom01_Skeletal;
				break;
			case E_SpriteAttachmentTarget::Custom02_Skeletal:
				AttachTarget = Custom02_Skeletal;
				break;
			case E_SpriteAttachmentTarget::Custom03_Skeletal:
				AttachTarget = Custom03_Skeletal;
				break;
			default:
				VN_LOG_WARNING(TEXT("Unknown sprite attachment target for component %s"), *Component->GetName());
				break;
		}

		if (AttachTarget && Config.bUseSocketTransform && !Config.SocketName.IsNone())
		{
			// Прикрепляем к сокету
			Component->AttachToComponent(AttachTarget, 
				FAttachmentTransformRules::KeepWorldTransform, Config.SocketName);
		}
		else if (AttachTarget)
		{
			// Прикрепляем к компоненту без сокета
			Component->AttachToComponent(AttachTarget, 
				FAttachmentTransformRules::KeepRelativeTransform);
		}
	}

	// Устанавливаем трансформ
	FTransform ComponentTransform = Component->GetRelativeTransform();
	ComponentTransform.SetLocation(Config.Offset);
	ComponentTransform.SetScale3D(FVector(Config.Scale));
	Component->SetRelativeTransform(ComponentTransform);

	// Устанавливаем цвет
	FLinearColor FinalColor = GetTargetColorForComponent(Component);
	SetComponentColor(Component, FinalColor);

	VN_LOG_DEBUG(TEXT("Sprite component %s configured from Attachment config"), *Component->GetName());
}

void AVNCharacter::SetupComponentFromConfig(UPaperSpriteComponent* Component, const F_VN_SpriteConfig_Simple& Config)
{
	if (!Component)
	{
		VN_LOG_WARNING(TEXT("SetupComponentFromConfig: PaperSpriteComponent is null"));
		return;
	}

	// Устанавливаем видимость
	Component->SetVisibility(Config.bVisible);
	
	if (!Config.bVisible)
	{
		return; // Если компонент невидим, дальше не настраиваем
	}

	// Загружаем и устанавливаем Sprite
	if (!Config.Sprite.IsNull())
	{
		if (UPaperSprite* LoadedSprite = Config.Sprite.LoadSynchronous())
		{
			Component->SetSprite(LoadedSprite);
		}
		else
		{
			VN_LOG_WARNING(TEXT("Failed to load Sprite for component %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSprite(nullptr);
	}

	// Устанавливаем трансформ
	FTransform ComponentTransform = Component->GetRelativeTransform();
	ComponentTransform.SetLocation(Config.Offset);
	ComponentTransform.SetScale3D(FVector(Config.Scale));
	Component->SetRelativeTransform(ComponentTransform);

	// Устанавливаем цвет
	FLinearColor FinalColor = GetTargetColorForComponent(Component);
	SetComponentColor(Component, FinalColor);

	VN_LOG_DEBUG(TEXT("Sprite component %s configured from Simple config"), *Component->GetName());
}

// =====================================================
// УТИЛИТЫ ДЛЯ РАБОТЫ С КОМПОНЕНТАМИ
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
// ПОДГОТОВКА КОМПОНЕНТОВ ДЛЯ АНИМАЦИИ ПЕРЕХОДОВ
// =====================================================

void AVNCharacter::PrepareTransitionComponents(const F_VN_CharacterState& NewState)
{
	VN_LOG_DEBUG(TEXT("PrepareTransitionComponents: Preparing transition components"));

	// Очищаем массивы компонентов для анимации
	SkeletalMeshesToFadeOut.Empty();
	SpritesToFadeOut.Empty();
	SkeletalMeshesToFadeIn.Empty();
	SpritesToFadeIn.Empty();

	// Сравниваем Skeletal Mesh конфигурации и подготавливаем компоненты для анимации
	
	// Body
	if (CurrentState.BodyConfig != NewState.BodyConfig)
	{
		USkeletalMeshComponent* TempComponent = DuplicateObject<USkeletalMeshComponent>(Body_Skeletal, this);
		if (TempComponent)
		{
			SkeletalMeshesToFadeOut.Add(TempComponent);
			SetupComponentFromConfig(Body_Skeletal, NewState.BodyConfig);
			SetComponentAlpha(Body_Skeletal, 0.0f);
			SkeletalMeshesToFadeIn.Add(Body_Skeletal);
		}
	}

	// Arms
	if (CurrentState.ArmsConfig != NewState.ArmsConfig)
	{
		USkeletalMeshComponent* TempComponent = DuplicateObject<USkeletalMeshComponent>(Arms_Skeletal, this);
		if (TempComponent)
		{
			SkeletalMeshesToFadeOut.Add(TempComponent);
			SetupComponentFromConfig(Arms_Skeletal, NewState.ArmsConfig);
			SetComponentAlpha(Arms_Skeletal, 0.0f);
			SkeletalMeshesToFadeIn.Add(Arms_Skeletal);
		}
	}

	// Head
	if (CurrentState.HeadConfig != NewState.HeadConfig)
	{
		USkeletalMeshComponent* TempComponent = DuplicateObject<USkeletalMeshComponent>(Head_Skeletal, this);
		if (TempComponent)
		{
			SkeletalMeshesToFadeOut.Add(TempComponent);
			SetupComponentFromConfig(Head_Skeletal, NewState.HeadConfig);
			SetComponentAlpha(Head_Skeletal, 0.0f);
			SkeletalMeshesToFadeIn.Add(Head_Skeletal);
		}
	}

	// Сравниваем Sprite конфигурации и подготавливаем компоненты для анимации
	
	// Eyes
	if (CurrentState.EyesSpriteConfig != NewState.EyesSpriteConfig)
	{
		UPaperSpriteComponent* TempComponent = DuplicateObject<UPaperSpriteComponent>(Eyes_Sprite, this);
		if (TempComponent)
		{
			SpritesToFadeOut.Add(TempComponent);
			SetupComponentFromConfig(Eyes_Sprite, NewState.EyesSpriteConfig);
			SetComponentAlpha(Eyes_Sprite, 0.0f);
			SpritesToFadeIn.Add(Eyes_Sprite);
		}
	}

	// Eyebrow
	if (CurrentState.EyebrowSpriteConfig != NewState.EyebrowSpriteConfig)
	{
		UPaperSpriteComponent* TempComponent = DuplicateObject<UPaperSpriteComponent>(Eyebrow_Sprite, this);
		if (TempComponent)
		{
			SpritesToFadeOut.Add(TempComponent);
			SetupComponentFromConfig(Eyebrow_Sprite, NewState.EyebrowSpriteConfig);
			SetComponentAlpha(Eyebrow_Sprite, 0.0f);
			SpritesToFadeIn.Add(Eyebrow_Sprite);
		}
	}

	// Mouth
	if (CurrentState.MouthSpriteConfig != NewState.MouthSpriteConfig)
	{
		UPaperSpriteComponent* TempComponent = DuplicateObject<UPaperSpriteComponent>(Mouth_Sprite, this);
		if (TempComponent)
		{
			SpritesToFadeOut.Add(TempComponent);
			SetupComponentFromConfig(Mouth_Sprite, NewState.MouthSpriteConfig);
			SetComponentAlpha(Mouth_Sprite, 0.0f);
			SpritesToFadeIn.Add(Mouth_Sprite);
		}
	}

	// Можно добавить остальные компоненты по аналогии...

	VN_LOG_DEBUG(TEXT("Transition components prepared: %d SkeletalMesh FadeOut, %d Sprite FadeOut"), 
		SkeletalMeshesToFadeOut.Num(), SpritesToFadeOut.Num());
}

void AVNCharacter::FinishAndCleanupTransition()
{
	VN_LOG_DEBUG(TEXT("FinishAndCleanupTransition: Cleaning up transition components"));

	// Устанавливаем полную непрозрачность для fade-in компонентов
	for (USkeletalMeshComponent* Component : SkeletalMeshesToFadeIn)
	{
		if (Component)
		{
			SetComponentAlpha(Component, 1.0f);
		}
	}

	for (UPaperSpriteComponent* Component : SpritesToFadeIn)
	{
		if (Component)
		{
			SetComponentAlpha(Component, 1.0f);
		}
	}

	// Удаляем fade-out компоненты
	for (USkeletalMeshComponent* Component : SkeletalMeshesToFadeOut)
	{
		if (Component && IsValid(Component))
		{
			Component->DestroyComponent();
		}
	}

	for (UPaperSpriteComponent* Component : SpritesToFadeOut)
	{
		if (Component && IsValid(Component))
		{
			Component->DestroyComponent();
		}
	}

	// Очищаем массивы
	SkeletalMeshesToFadeOut.Empty();
	SpritesToFadeOut.Empty();
	SkeletalMeshesToFadeIn.Empty();
	SpritesToFadeIn.Empty();

	VN_LOG_DEBUG(TEXT("Transition cleanup completed"));
}

void AVNCharacter::SetupComponentFromConfig(USkeletalMeshComponent* Component, const F_VN_SkeletalConfig_Attachment& Config)
{
	if (!Component)
	{
		VN_LOG_WARNING(TEXT("SetupComponentFromConfig: SkeletalMeshComponent is null"));
		return;
	}

	// Устанавливаем видимость
	Component->SetVisibility(Config.bVisible);
	
	if (!Config.bVisible)
	{
		return; // Если компонент невидим, дальше не настраиваем
	}

	// Загружаем и устанавливаем Skeletal Mesh
	if (!Config.SkeletalMesh.IsNull())
	{
		USkeletalMesh* LoadedMesh= Config.SkeletalMesh.LoadSynchronous();
		if (LoadedMesh)
		{
			Component->SetSkeletalMesh(LoadedMesh);
		}
		else
		{
			VN_LOG_WARNING(TEXT("Failed to load SkeletalMesh for component %s"), *Component->GetName());
		}
	}
	else
	{
		Component->SetSkeletalMesh(nullptr);
	}

	// Устанавливаем AnimInstance класс
	if (Config.AnimInstanceClass)
	{
		Component->SetAnimInstanceClass(Config.AnimInstanceClass);
	}

	// Применяем переопределения материалов
	for (const auto& MaterialPair : Config.MaterialOverrides)
	{
		int32 MaterialIndex = MaterialPair.Key;
		TSoftObjectPtr<UMaterialInterface> MaterialPtr = MaterialPair.Value;

		if (!MaterialPtr.IsNull())
		{
			UMaterialInterface* LoadedMaterial = MaterialPtr.LoadSynchronous();
			if (LoadedMaterial)
			{
				Component->SetMaterial(MaterialIndex, LoadedMaterial);
			}
		}
	}

	// Устанавливаем трансформ
	FTransform ComponentTransform = Component->GetRelativeTransform();
	ComponentTransform.SetLocation(Config.Offset);
	ComponentTransform.SetScale3D(FVector(Config.Scale));
	Component->SetRelativeTransform(ComponentTransform);

	// Устанавливаем цвет
	FLinearColor FinalColor = GetTargetColorForComponent(Component);
	SetComponentColor(Component, FinalColor);

	VN_LOG_DEBUG(TEXT("SkeletalMesh component %s configured from Attachment config"), *Component->GetName());
}