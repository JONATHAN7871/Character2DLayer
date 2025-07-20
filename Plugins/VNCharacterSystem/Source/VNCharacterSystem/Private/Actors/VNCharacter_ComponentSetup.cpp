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

	// Head_Sprite использует Attachment конфигурацию
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

	VN_LOG_DEBUG(TEXT("SkeletalMesh component %s configured from Body config"), *Component->GetName());
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
// ИСПРАВЛЕННАЯ ПОДГОТОВКА КОМПОНЕНТОВ ДЛЯ АНИМАЦИИ ПЕРЕХОДОВ
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
	PrepareSkeletalTransition(Body_Skeletal, CurrentState.BodyConfig, NewState.BodyConfig);
	PrepareSkeletalAttachmentTransition(Arms_Skeletal, CurrentState.ArmsConfig, NewState.ArmsConfig);
	PrepareSkeletalAttachmentTransition(Head_Skeletal, CurrentState.HeadConfig, NewState.HeadConfig);
	PrepareSkeletalAttachmentTransition(Custom01_Skeletal, CurrentState.Custom01Config, NewState.Custom01Config);
	PrepareSkeletalAttachmentTransition(Custom02_Skeletal, CurrentState.Custom02Config, NewState.Custom02Config);
	PrepareSkeletalAttachmentTransition(Custom03_Skeletal, CurrentState.Custom03Config, NewState.Custom03Config);

	// Сравниваем Sprite конфигурации и подготавливаем компоненты для анимации
	PrepareSpriteAttachmentTransition(Body_Sprite, CurrentState.BodySpriteConfig, NewState.BodySpriteConfig);
	PrepareSpriteAttachmentTransition(Arms_Sprite, CurrentState.ArmsSpriteConfig, NewState.ArmsSpriteConfig);
	PrepareSpriteAttachmentTransition(Head_Sprite, CurrentState.HeadSpriteConfig, NewState.HeadSpriteConfig);
	PrepareSpriteAttachmentTransition(BodyShadow_Sprite, CurrentState.BodyShadowSpriteConfig, NewState.BodyShadowSpriteConfig);

	// Простые спрайты
	PrepareSpriteSimpleTransition(Eyes_Sprite, CurrentState.EyesSpriteConfig, NewState.EyesSpriteConfig);
	PrepareSpriteSimpleTransition(Mouth_Sprite, CurrentState.MouthSpriteConfig, NewState.MouthSpriteConfig);
	PrepareSpriteSimpleTransition(Eyebrow_Sprite, CurrentState.EyebrowSpriteConfig, NewState.EyebrowSpriteConfig);
	PrepareSpriteSimpleTransition(Eyelids_Sprite, CurrentState.EyelidsSpriteConfig, NewState.EyelidsSpriteConfig);
	PrepareSpriteSimpleTransition(Wink_Sprite, CurrentState.WinkSpriteConfig, NewState.WinkSpriteConfig);

	// Эмоциональные эффекты
	PrepareSpriteSimpleTransition(EmotionHead01_Sprite, CurrentState.EmotionHeadEffect01SpriteConfig, NewState.EmotionHeadEffect01SpriteConfig);
	PrepareSpriteSimpleTransition(EmotionHead02_Sprite, CurrentState.EmotionHeadEffect02SpriteConfig, NewState.EmotionHeadEffect02SpriteConfig);
	PrepareSpriteSimpleTransition(EmotionHead03_Sprite, CurrentState.EmotionHeadEffect03SpriteConfig, NewState.EmotionHeadEffect03SpriteConfig);
	PrepareSpriteAttachmentTransition(EmotionBody01_Sprite, CurrentState.EmotionBodyEffect01SpriteConfig, NewState.EmotionBodyEffect01SpriteConfig);
	PrepareSpriteAttachmentTransition(EmotionBody02_Sprite, CurrentState.EmotionBodyEffect02SpriteConfig, NewState.EmotionBodyEffect02SpriteConfig);
	PrepareSpriteAttachmentTransition(EmotionBody03_Sprite, CurrentState.EmotionBodyEffect03SpriteConfig, NewState.EmotionBodyEffect03SpriteConfig);

	VN_LOG_DEBUG(TEXT("Transition components prepared: %d SkeletalMesh FadeOut, %d Sprite FadeOut, %d SkeletalMesh FadeIn, %d Sprite FadeIn"), 
		SkeletalMeshesToFadeOut.Num(), SpritesToFadeOut.Num(), SkeletalMeshesToFadeIn.Num(), SpritesToFadeIn.Num());
}

void AVNCharacter::PrepareSkeletalTransition(USkeletalMeshComponent* Component, const F_VN_SkeletalConfig_Body& OldConfig, const F_VN_SkeletalConfig_Body& NewConfig)
{
	if (!Component || OldConfig == NewConfig)
	{
		return;
	}

	// Создаем временный компонент для fade out с текущей конфигурацией
	USkeletalMeshComponent* FadeOutComponent = DuplicateObject<USkeletalMeshComponent>(Component, this);
	if (FadeOutComponent)
	{
		// Настраиваем fade out компонент с текущими настройками
		FadeOutComponent->AttachToComponent(Component->GetAttachParent(), FAttachmentTransformRules::KeepWorldTransform);
		FadeOutComponent->SetRelativeTransform(Component->GetRelativeTransform());
		FadeOutComponent->SetSkeletalMesh(Component->GetSkeletalMeshAsset());
		
		// Копируем материалы
		for (int32 i = 0; i < Component->GetNumMaterials(); ++i)
		{
			FadeOutComponent->SetMaterial(i, Component->GetMaterial(i));
		}
		
		// Устанавливаем альфу на 1 (полная видимость)
		SetComponentAlpha(FadeOutComponent, 1.0f);
		
		SkeletalMeshesToFadeOut.Add(FadeOutComponent);
		VN_LOG_DEBUG(TEXT("Created fade out component for %s"), *Component->GetName());
	}

	// Настраиваем основной компонент с новой конфигурацией и альфой 0
	SetupComponentFromConfig(Component, NewConfig);
	SetComponentAlpha(Component, 0.0f);
	SkeletalMeshesToFadeIn.Add(Component);
	
	VN_LOG_DEBUG(TEXT("Prepared skeletal transition for %s"), *Component->GetName());
}

void AVNCharacter::PrepareSkeletalAttachmentTransition(USkeletalMeshComponent* Component, const F_VN_SkeletalConfig_Attachment& OldConfig, const F_VN_SkeletalConfig_Attachment& NewConfig)
{
	if (!Component || OldConfig == NewConfig)
	{
		return;
	}

	// Создаем временный компонент для fade out с текущей конфигурацией
	USkeletalMeshComponent* FadeOutComponent = DuplicateObject<USkeletalMeshComponent>(Component, this);
	if (FadeOutComponent)
	{
		// Настраиваем fade out компонент с текущими настройками
		FadeOutComponent->AttachToComponent(Component->GetAttachParent(), FAttachmentTransformRules::KeepWorldTransform);
		FadeOutComponent->SetRelativeTransform(Component->GetRelativeTransform());
		FadeOutComponent->SetSkeletalMesh(Component->GetSkeletalMeshAsset());
		
		// Копируем материалы
		for (int32 i = 0; i < Component->GetNumMaterials(); ++i)
		{
			FadeOutComponent->SetMaterial(i, Component->GetMaterial(i));
		}
		
		// Устанавливаем альфу на 1 (полная видимость)
		SetComponentAlpha(FadeOutComponent, 1.0f);
		
		SkeletalMeshesToFadeOut.Add(FadeOutComponent);
		VN_LOG_DEBUG(TEXT("Created fade out component for %s"), *Component->GetName());
	}

	// Настраиваем основной компонент с новой конфигурацией и альфой 0
	SetupComponentFromConfig(Component, NewConfig);
	SetComponentAlpha(Component, 0.0f);
	SkeletalMeshesToFadeIn.Add(Component);
	
	VN_LOG_DEBUG(TEXT("Prepared skeletal attachment transition for %s"), *Component->GetName());
}

void AVNCharacter::PrepareSpriteAttachmentTransition(UPaperSpriteComponent* Component, const F_VN_SpriteConfig_Attachment& OldConfig, const F_VN_SpriteConfig_Attachment& NewConfig)
{
	if (!Component || OldConfig == NewConfig)
	{
		return;
	}

	// Создаем временный компонент для fade out с текущей конфигурацией
	UPaperSpriteComponent* FadeOutComponent = DuplicateObject<UPaperSpriteComponent>(Component, this);
	if (FadeOutComponent)
	{
		// Настраиваем fade out компонент с текущими настройками
		FadeOutComponent->AttachToComponent(Component->GetAttachParent(), FAttachmentTransformRules::KeepWorldTransform);
		FadeOutComponent->SetRelativeTransform(Component->GetRelativeTransform());
		FadeOutComponent->SetSprite(Component->GetSprite());
		FadeOutComponent->SetSpriteColor(Component->GetSpriteColor());
		
		SpritesToFadeOut.Add(FadeOutComponent);
		VN_LOG_DEBUG(TEXT("Created fade out sprite component for %s"), *Component->GetName());
	}

	// Настраиваем основной компонент с новой конфигурацией и альфой 0
	SetupComponentFromConfig(Component, NewConfig);
	SetComponentAlpha(Component, 0.0f);
	SpritesToFadeIn.Add(Component);
	
	VN_LOG_DEBUG(TEXT("Prepared sprite attachment transition for %s"), *Component->GetName());
}

void AVNCharacter::PrepareSpriteSimpleTransition(UPaperSpriteComponent* Component, const F_VN_SpriteConfig_Simple& OldConfig, const F_VN_SpriteConfig_Simple& NewConfig)
{
	if (!Component || OldConfig == NewConfig)
	{
		return;
	}

	// Создаем временный компонент для fade out с текущей конфигурацией
	UPaperSpriteComponent* FadeOutComponent = DuplicateObject<UPaperSpriteComponent>(Component, this);
	if (FadeOutComponent)
	{
		// Настраиваем fade out компонент с текущими настройками
		FadeOutComponent->AttachToComponent(Component->GetAttachParent(), FAttachmentTransformRules::KeepWorldTransform);
		FadeOutComponent->SetRelativeTransform(Component->GetRelativeTransform());
		FadeOutComponent->SetSprite(Component->GetSprite());
		FadeOutComponent->SetSpriteColor(Component->GetSpriteColor());
		
		SpritesToFadeOut.Add(FadeOutComponent);
		VN_LOG_DEBUG(TEXT("Created fade out sprite component for %s"), *Component->GetName());
	}

	// Настраиваем основной компонент с новой конфигурацией и альфой 0
	SetupComponentFromConfig(Component, NewConfig);
	SetComponentAlpha(Component, 0.0f);
	SpritesToFadeIn.Add(Component);
	
	VN_LOG_DEBUG(TEXT("Prepared sprite simple transition for %s"), *Component->GetName());
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