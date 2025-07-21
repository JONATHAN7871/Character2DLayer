#include "Actors/VNCharacter.h"
#include "Data/VNCharacterDataAsset.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Materials/MaterialInterface.h"

// =====================================================
// РАБОТА С DATA ASSET
// =====================================================

void AVNCharacter::ApplyDataAsset(UVNCharacterDataAsset* CharacterData, bool bAnimate, float Duration)
{
	if (!CharacterData)
	{
		VN_LOG_WARNING(TEXT("ApplyDataAsset: CharacterData is null"));
		return;
	}

	VN_LOG_DEBUG(TEXT("Applying DataAsset with full configurations"));

	// Применяем Skeletal Mesh конфигурации
	ApplySkeletalBodyConfig(Body_Skeletal, CharacterData->BodyConfig);
	ApplySkeletalAttachmentConfig(Arms_Skeletal, CharacterData->ArmsConfig);
	ApplySkeletalAttachmentConfig(Head_Skeletal, CharacterData->HeadConfig);
	ApplySkeletalAttachmentConfig(Custom01_Skeletal, CharacterData->Custom01Config);
	ApplySkeletalAttachmentConfig(Custom02_Skeletal, CharacterData->Custom02Config);
	ApplySkeletalAttachmentConfig(Custom03_Skeletal, CharacterData->Custom03Config);

	// Применяем Sprite конфигурации (Attachment)
	ApplySpriteAttachmentConfig(Body_Sprite, CharacterData->BodySpriteConfig);
	ApplySpriteAttachmentConfig(Arms_Sprite, CharacterData->ArmsSpriteConfig);
	ApplySpriteAttachmentConfig(BodyShadow_Sprite, CharacterData->BodyShadowSpriteConfig);
	ApplySpriteAttachmentConfig(Head_Sprite, CharacterData->HeadSpriteConfig);
	ApplySpriteAttachmentConfig(EmotionBodyEffect01_Sprite, CharacterData->EmotionBodyEffect01SpriteConfig);
	ApplySpriteAttachmentConfig(EmotionBodyEffect02_Sprite, CharacterData->EmotionBodyEffect02SpriteConfig);
	ApplySpriteAttachmentConfig(EmotionBodyEffect03_Sprite, CharacterData->EmotionBodyEffect03SpriteConfig);

	// Применяем Sprite конфигурации (Simple)
	ApplySpriteSimpleConfig(Eyebrow_Sprite, CharacterData->EyebrowSpriteConfig);
	ApplySpriteSimpleConfig(Eyes_Sprite, CharacterData->EyesSpriteConfig);
	ApplySpriteSimpleConfig(Eyelids_Sprite, CharacterData->EyelidsSpriteConfig);
	ApplySpriteSimpleConfig(Wink_Sprite, CharacterData->WinkSpriteConfig);
	ApplySpriteSimpleConfig(Mouth_Sprite, CharacterData->MouthSpriteConfig);
	ApplySpriteSimpleConfig(EmotionHeadEffect01_Sprite, CharacterData->EmotionHeadEffect01SpriteConfig);
	ApplySpriteSimpleConfig(EmotionHeadEffect02_Sprite, CharacterData->EmotionHeadEffect02SpriteConfig);
	ApplySpriteSimpleConfig(EmotionHeadEffect03_Sprite, CharacterData->EmotionHeadEffect03SpriteConfig);

	// Если нужна анимация, запускаем её один раз для всех изменений
	if (bAnimate && Duration > 0.0f && AnimationManager)
	{
		AnimationManager->PlayTransition(Duration);
	}

	VN_LOG_DEBUG(TEXT("DataAsset applied successfully"));
}

void AVNCharacter::ApplySkeletalBodyConfig(USkeletalMeshComponent* Component, const F_VN_SkeletalConfig_Body& Config)
{
	if (!Component)
	{
		VN_LOG_WARNING(TEXT("ApplySkeletalBodyConfig: Component is null"));
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
	FTransform ComponentTransform = FTransform::Identity;
	ComponentTransform.SetLocation(Config.Offset);
	ComponentTransform.SetScale3D(FVector(Config.Scale));
	Component->SetRelativeTransform(ComponentTransform);

	// Устанавливаем цвет
	SetComponentColor(Component, Config.Color);

	VN_LOG_DEBUG(TEXT("Applied Skeletal Body config to %s"), *Component->GetName());
}

void AVNCharacter::ApplySkeletalAttachmentConfig(USkeletalMeshComponent* Component, const F_VN_SkeletalConfig_Attachment& Config)
{
	if (!Component)
	{
		VN_LOG_WARNING(TEXT("ApplySkeletalAttachmentConfig: Component is null"));
		return;
	}

	// Устанавливаем видимость
	Component->SetVisibility(Config.bVisible);
	
	if (!Config.bVisible)
	{
		return;
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

	// Настраиваем прикрепление
	if (Config.AttachTo != E_SkeletalAttachmentTarget::None)
	{
		USkeletalMeshComponent* AttachTarget = nullptr;
		
		switch (Config.AttachTo)
		{
			case E_SkeletalAttachmentTarget::Body:
				AttachTarget = Body_Skeletal;
				break;
			default:
				VN_LOG_WARNING(TEXT("Unknown attachment target for component %s"), *Component->GetName());
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
	FTransform ComponentTransform = FTransform::Identity;
	ComponentTransform.SetLocation(Config.Offset);
	ComponentTransform.SetScale3D(FVector(Config.Scale));
	Component->SetRelativeTransform(ComponentTransform);

	// Устанавливаем цвет
	SetComponentColor(Component, Config.Color);

	VN_LOG_DEBUG(TEXT("Applied Skeletal Attachment config to %s"), *Component->GetName());
}

void AVNCharacter::ApplySpriteAttachmentConfig(UPaperSpriteComponent* Component, const F_VN_SpriteConfig_Attachment& Config)
{
	if (!Component)
	{
		VN_LOG_WARNING(TEXT("ApplySpriteAttachmentConfig: Component is null"));
		return;
	}

	// Устанавливаем видимость
	Component->SetVisibility(Config.bVisible);
	
	if (!Config.bVisible)
	{
		return;
	}

	// Загружаем и устанавливаем Sprite
	if (!Config.Sprite.IsNull())
	{
		UPaperSprite* LoadedSprite = Config.Sprite.LoadSynchronous();
		if (LoadedSprite)
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
	FTransform ComponentTransform = FTransform::Identity;
	ComponentTransform.SetLocation(Config.Offset);
	ComponentTransform.SetScale3D(FVector(Config.Scale));
	Component->SetRelativeTransform(ComponentTransform);

	// Устанавливаем цвет
	SetComponentColor(Component, Config.Color);

	VN_LOG_DEBUG(TEXT("Applied Sprite Attachment config to %s"), *Component->GetName());
}

void AVNCharacter::ApplySpriteSimpleConfig(UPaperSpriteComponent* Component, const F_VN_SpriteConfig_Simple& Config)
{
	if (!Component)
	{
		VN_LOG_WARNING(TEXT("ApplySpriteSimpleConfig: Component is null"));
		return;
	}

	// Устанавливаем видимость
	Component->SetVisibility(Config.bVisible);
	
	if (!Config.bVisible)
	{
		return;
	}

	// Загружаем и устанавливаем Sprite
	if (!Config.Sprite.IsNull())
	{
		UPaperSprite* LoadedSprite = Config.Sprite.LoadSynchronous();
		if (LoadedSprite)
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
	FTransform ComponentTransform = FTransform::Identity;
	ComponentTransform.SetLocation(Config.Offset);
	ComponentTransform.SetScale3D(FVector(Config.Scale));
	Component->SetRelativeTransform(ComponentTransform);

	// Устанавливаем цвет
	SetComponentColor(Component, Config.Color);

	VN_LOG_DEBUG(TEXT("Applied Sprite Simple config to %s"), *Component->GetName());
}