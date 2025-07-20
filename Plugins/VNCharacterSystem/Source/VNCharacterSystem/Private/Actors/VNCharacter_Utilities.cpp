/**
 * VNCharacter_Utilities.cpp
 * 
 * Модуль утилит и валидации для VN Character System
 * Содержит вспомогательные методы, валидацию, оптимизации
 * и методы для получения компонентов.
 */

#include "Actors/VNCharacter.h"
#include "VNCharacterSystemModule.h"

// =====================================================
// УТИЛИТЫ ДЛЯ РАБОТЫ С КОМПОНЕНТАМИ
// =====================================================

USkeletalMeshComponent* AVNCharacter::GetSkeletalComponent(E_VN_ComponentID_Skeletal ComponentID) const
{
	switch (ComponentID)
	{
		case E_VN_ComponentID_Skeletal::Body:
			return Body_Skeletal;
		case E_VN_ComponentID_Skeletal::Arms:
			return Arms_Skeletal;
		case E_VN_ComponentID_Skeletal::Head:
			return Head_Skeletal;
		case E_VN_ComponentID_Skeletal::Custom01:
			return Custom01_Skeletal;
		case E_VN_ComponentID_Skeletal::Custom02:
			return Custom02_Skeletal;
		case E_VN_ComponentID_Skeletal::Custom03:
			return Custom03_Skeletal;
		default:
			return nullptr;
	}
}

UPaperSpriteComponent* AVNCharacter::GetSpriteComponent(E_VN_ComponentID_Sprite ComponentID) const
{
	switch (ComponentID)
	{
		case E_VN_ComponentID_Sprite::Body:
			return Body_Sprite;
		case E_VN_ComponentID_Sprite::Arms:
			return Arms_Sprite;
		case E_VN_ComponentID_Sprite::Head:
			return Head_Sprite;
		case E_VN_ComponentID_Sprite::Eyebrow:
			return Eyebrow_Sprite;
		case E_VN_ComponentID_Sprite::Eyes:
			return Eyes_Sprite;
		case E_VN_ComponentID_Sprite::Eyelids:
			return Eyelids_Sprite;
		case E_VN_ComponentID_Sprite::Wink:
			return Wink_Sprite;
		case E_VN_ComponentID_Sprite::Mouth:
			return Mouth_Sprite;
		case E_VN_ComponentID_Sprite::BodyShadow:
			return BodyShadow_Sprite;
		case E_VN_ComponentID_Sprite::EmotionHead_01:
			return EmotionHead01_Sprite;
		case E_VN_ComponentID_Sprite::EmotionHead_02:
			return EmotionHead02_Sprite;
		case E_VN_ComponentID_Sprite::EmotionHead_03:
			return EmotionHead03_Sprite;
		case E_VN_ComponentID_Sprite::EmotionBody_01:
			return EmotionBody01_Sprite;
		case E_VN_ComponentID_Sprite::EmotionBody_02:
			return EmotionBody02_Sprite;
		case E_VN_ComponentID_Sprite::EmotionBody_03:
			return EmotionBody03_Sprite;
		default:
			return nullptr;
	}
}

TArray<USceneComponent*> AVNCharacter::GetAllRenderComponents() const
{
	TArray<USceneComponent*> Components;
	
	// Добавляем все Skeletal Mesh компоненты
	TArray<USkeletalMeshComponent*> SkeletalComponents = GetAllSkeletalComponents();
	for (USkeletalMeshComponent* Component : SkeletalComponents)
	{
		if (Component)
		{
			Components.Add(Component);
		}
	}
	
	// Добавляем все Sprite компоненты
	TArray<UPaperSpriteComponent*> SpriteComponents = GetAllSpriteComponents();
	for (UPaperSpriteComponent* Component : SpriteComponents)
	{
		if (Component)
		{
			Components.Add(Component);
		}
	}
	
	return Components;
}

TArray<USkeletalMeshComponent*> AVNCharacter::GetAllSkeletalComponents() const
{
	TArray<USkeletalMeshComponent*> Components;
	
	Components.Add(Body_Skeletal);
	Components.Add(Arms_Skeletal);
	Components.Add(Head_Skeletal);
	Components.Add(Custom01_Skeletal);
	Components.Add(Custom02_Skeletal);
	Components.Add(Custom03_Skeletal);
	
	// Удаляем null указатели
	Components.RemoveAll([](USkeletalMeshComponent* Component) { return Component == nullptr; });
	
	return Components;
}

TArray<UPaperSpriteComponent*> AVNCharacter::GetAllSpriteComponents() const
{
	TArray<UPaperSpriteComponent*> Components;
	
	Components.Add(Body_Sprite);
	Components.Add(Arms_Sprite);
	Components.Add(Head_Sprite);
	Components.Add(Eyebrow_Sprite);
	Components.Add(Eyes_Sprite);
	Components.Add(Eyelids_Sprite);
	Components.Add(Wink_Sprite);
	Components.Add(Mouth_Sprite);
	Components.Add(BodyShadow_Sprite);
	Components.Add(EmotionHead01_Sprite);
	Components.Add(EmotionHead02_Sprite);
	Components.Add(EmotionHead03_Sprite);
	Components.Add(EmotionBody01_Sprite);
	Components.Add(EmotionBody02_Sprite);
	Components.Add(EmotionBody03_Sprite);
	
	// Удаляем null указатели
	Components.RemoveAll([](UPaperSpriteComponent* Component) { return Component == nullptr; });
	
	return Components;
}

bool AVNCharacter::IsChildOfHeadSprite(UPaperSpriteComponent* Sprite) const
{
	if (!Sprite || !Head_Sprite) return false;
	
	// Проверяем, является ли спрайт одним из лицевых элементов
	return (Sprite == Eyebrow_Sprite ||
			Sprite == Eyes_Sprite ||
			Sprite == Eyelids_Sprite ||
			Sprite == Wink_Sprite ||
			Sprite == Mouth_Sprite ||
			Sprite == EmotionHead01_Sprite ||
			Sprite == EmotionHead02_Sprite ||
			Sprite == EmotionHead03_Sprite);
}

// =====================================================
// ВАЛИДАЦИЯ И ОБРАБОТКА ОШИБОК
// =====================================================

bool AVNCharacter::ValidateCharacterState(const F_VN_CharacterState& State) const
{
	if (!State.IsValid())
	{
		VN_LOG_WARNING(TEXT("Character state is invalid: StateID is None"));
		return false;
	}

	// =============== ВАЖНОЕ ИСПРАВЛЕНИЕ ===============
	// Для частичных состояний (создаваемых методами SetEyes, SetMouth и т.д.)
	// НЕ требуем, чтобы все компоненты были заполнены
    
	// Проверяем только, что есть хотя бы один видимый компонент ИЛИ
	// что это частичное обновление существующего состояния
	bool bHasVisibleComponents = State.HasVisibleComponents();
	bool bIsPartialUpdate = (State.StateID.ToString().Contains(TEXT("Change")) || 
							State.StateID.ToString().Contains(TEXT("Update")));
    
	if (!bHasVisibleComponents && !bIsPartialUpdate)
	{
		VN_LOG_WARNING(TEXT("Character state '%s' has no visible components"), *State.StateID.ToString());
		return false;
	}

	// Валидация ассетов только для заполненных конфигураций
	return ValidateAssetsSelectively(State);
}

bool AVNCharacter::ValidateAssetsSelectively(const F_VN_CharacterState& State) const
{
    bool bIsValid = true;
    TArray<FString> ValidationErrors;
    
    // Проверяем только заполненные Skeletal конфигурации
    if (IsSkeletalBodyConfigFilled(State.BodyConfig) && State.BodyConfig.bVisible && State.BodyConfig.SkeletalMesh.IsNull())
    {
        ValidationErrors.Add(TEXT("BodyConfig: SkeletalMesh is null but component is visible"));
        bIsValid = false;
    }
    
    if (IsSkeletalAttachmentConfigFilled(State.ArmsConfig) && State.ArmsConfig.bVisible && State.ArmsConfig.SkeletalMesh.IsNull())
    {
        ValidationErrors.Add(TEXT("ArmsConfig: SkeletalMesh is null but component is visible"));
        bIsValid = false;
    }
    
    if (IsSkeletalAttachmentConfigFilled(State.HeadConfig) && State.HeadConfig.bVisible && State.HeadConfig.SkeletalMesh.IsNull())
    {
        ValidationErrors.Add(TEXT("HeadConfig: SkeletalMesh is null but component is visible"));
        bIsValid = false;
    }
    
    // Проверяем только заполненные Sprite конфигурации
    if (IsSpriteSimpleConfigFilled(State.EyesSpriteConfig) && State.EyesSpriteConfig.bVisible && State.EyesSpriteConfig.Sprite.IsNull())
    {
        ValidationErrors.Add(TEXT("EyesSpriteConfig: Sprite is null but component is visible"));
        bIsValid = false;
    }
    
    if (IsSpriteSimpleConfigFilled(State.MouthSpriteConfig) && State.MouthSpriteConfig.bVisible && State.MouthSpriteConfig.Sprite.IsNull())
    {
        ValidationErrors.Add(TEXT("MouthSpriteConfig: Sprite is null but component is visible"));
        bIsValid = false;
    }
    
    // Выводим ошибки валидации только если они есть
    if (!bIsValid && ValidationErrors.Num() > 0)
    {
        FString ErrorMessage = FString::Printf(TEXT("Asset validation failed for state '%s':"), 
            *State.StateID.ToString());
        
        for (const FString& Error : ValidationErrors)
        {
            ErrorMessage += FString::Printf(TEXT("\n- %s"), *Error);
        }
        
        VN_LOG_WARNING(TEXT("%s"), *ErrorMessage);
    }
    
    return bIsValid;
}

bool AVNCharacter::ValidateAssets(const F_VN_CharacterState& State) const
{
	bool bIsValid = true;
	TArray<FString> ValidationErrors = State.GetDetailedValidationErrors();
	
	if (ValidationErrors.Num() > 0)
	{
		FString ErrorMessage = FString::Printf(TEXT("Asset validation failed for state '%s':"), 
			*State.StateID.ToString());
		
		for (const FString& Error : ValidationErrors)
		{
			ErrorMessage += FString::Printf(TEXT("\n- %s"), *Error);
		}
		
		VN_LOG_WARNING(TEXT("%s"), *ErrorMessage);
		bIsValid = false;
	}
	
	return bIsValid;
}

// =====================================================
// ОПТИМИЗАЦИИ И ПРОИЗВОДИТЕЛЬНОСТЬ
// =====================================================

void AVNCharacter::ApplyMobileOptimizations()
{
#if VN_CHARACTER_SYSTEM_MOBILE
	if (RenderSettings.bDisableShadowsOnMobile)
	{
		// Отключаем тени для всех Sprite компонентов
		TArray<UPaperSpriteComponent*> SpriteComponents = GetAllSpriteComponents();
		for (UPaperSpriteComponent* Sprite : SpriteComponents)
		{
			if (Sprite)
			{
				Sprite->SetCastShadow(false);
			}
		}
		
		VN_LOG_DEBUG(TEXT("Mobile optimization: Shadows disabled for sprites"));
	}
	
	if (RenderSettings.bUseSimplifiedMaterialsOnMobile)
	{
		// Здесь можно добавить логику замены материалов на упрощенные версии
		VN_LOG_DEBUG(TEXT("Mobile optimization: Simplified materials applied"));
	}
#endif
}

void AVNCharacter::UpdateLOD()
{
	if (!RenderSettings.bEnableLOD)
	{
		return;
	}

	// Получаем расстояние до камеры игрока
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (APawn* PlayerPawn = PC->GetPawn())
		{
			float Distance = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());
			bool bShouldUseHighLOD = Distance < RenderSettings.LODDistance;
			
			// Переключаем LOD для всех Skeletal Mesh компонентов
			TArray<USkeletalMeshComponent*> SkeletalComponents = GetAllSkeletalComponents();
			for (USkeletalMeshComponent* Component : SkeletalComponents)
			{
				if (Component)
				{
					Component->SetForcedLOD(bShouldUseHighLOD ? 0 : 1);
				}
			}
		}
	}
}

// =====================================================
// ДОПОЛНИТЕЛЬНЫЕ УТИЛИТЫ ДЛЯ ДИАЛОГОВОЙ СИСТЕМЫ
// =====================================================

bool AVNCharacter::HasMainPosePreset() const
{
	return !MainPosePreset.StateID.IsNone() && MainPosePreset.HasVisibleComponents();
}

bool AVNCharacter::IsComponentCurrentlyVisible(E_VN_ComponentID_Sprite ComponentID) const
{
	UPaperSpriteComponent* Component = GetSpriteComponent(ComponentID);
	if (!Component)
	{
		return false;
	}
	
	return Component->IsVisible() && Component->GetSprite() != nullptr;
}

bool AVNCharacter::IsComponentCurrentlyVisible(E_VN_ComponentID_Skeletal ComponentID) const
{
	USkeletalMeshComponent* Component = GetSkeletalComponent(ComponentID);
	if (!Component)
	{
		return false;
	}
	
	return Component->IsVisible() && Component->GetSkeletalMeshAsset() != nullptr;
}

FString AVNCharacter::GetCurrentStateDescription() const
{
	return FString::Printf(TEXT("Character '%s': %s"), 
		*CharacterName, *CurrentState.GetSummary());
}

FString AVNCharacter::GetMainPosePresetDescription() const
{
	return FString::Printf(TEXT("Main Pose Preset for '%s': %s"), 
		*CharacterName, *MainPosePreset.GetSummary());
}

// =====================================================
// МЕТОДЫ ДЛЯ РАБОТЫ С СОСТОЯНИЯМИ
// =====================================================

bool AVNCharacter::CanApplyPartialState(const F_VN_CharacterState& PartialState) const
{
	// Проверяем, есть ли хотя бы одно заполненное поле
	bool bHasFilledConfig = false;
	
	bHasFilledConfig |= IsSkeletalBodyConfigFilled(PartialState.BodyConfig);
	bHasFilledConfig |= IsSkeletalAttachmentConfigFilled(PartialState.ArmsConfig);
	bHasFilledConfig |= IsSkeletalAttachmentConfigFilled(PartialState.HeadConfig);
	bHasFilledConfig |= IsSkeletalAttachmentConfigFilled(PartialState.Custom01Config);
	bHasFilledConfig |= IsSkeletalAttachmentConfigFilled(PartialState.Custom02Config);
	bHasFilledConfig |= IsSkeletalAttachmentConfigFilled(PartialState.Custom03Config);
	
	bHasFilledConfig |= IsSpriteAttachmentConfigFilled(PartialState.BodySpriteConfig);
	bHasFilledConfig |= IsSpriteAttachmentConfigFilled(PartialState.ArmsSpriteConfig);
	bHasFilledConfig |= IsSpriteAttachmentConfigFilled(PartialState.HeadSpriteConfig);
	bHasFilledConfig |= IsSpriteAttachmentConfigFilled(PartialState.BodyShadowSpriteConfig);
	
	bHasFilledConfig |= IsSpriteSimpleConfigFilled(PartialState.EyesSpriteConfig);
	bHasFilledConfig |= IsSpriteSimpleConfigFilled(PartialState.MouthSpriteConfig);
	bHasFilledConfig |= IsSpriteSimpleConfigFilled(PartialState.EyebrowSpriteConfig);
	bHasFilledConfig |= IsSpriteSimpleConfigFilled(PartialState.EyelidsSpriteConfig);
	bHasFilledConfig |= IsSpriteSimpleConfigFilled(PartialState.WinkSpriteConfig);
	
	// Проверяем эмоциональные эффекты
	bHasFilledConfig |= IsSpriteSimpleConfigFilled(PartialState.EmotionHeadEffect01SpriteConfig);
	bHasFilledConfig |= IsSpriteSimpleConfigFilled(PartialState.EmotionHeadEffect02SpriteConfig);
	bHasFilledConfig |= IsSpriteSimpleConfigFilled(PartialState.EmotionHeadEffect03SpriteConfig);
	bHasFilledConfig |= IsSpriteAttachmentConfigFilled(PartialState.EmotionBodyEffect01SpriteConfig);
	bHasFilledConfig |= IsSpriteAttachmentConfigFilled(PartialState.EmotionBodyEffect02SpriteConfig);
	bHasFilledConfig |= IsSpriteAttachmentConfigFilled(PartialState.EmotionBodyEffect03SpriteConfig);
	
	return bHasFilledConfig;
}

bool AVNCharacter::IsStateEquivalentToCurrent(const F_VN_CharacterState& State) const
{
	return CurrentState == State;
}

bool AVNCharacter::IsStateEquivalentToMainPose(const F_VN_CharacterState& State) const
{
	return MainPosePreset == State;
}

void AVNCharacter::ResetToDefaultState()
{
	VN_LOG_DEBUG(TEXT("ResetToDefaultState: Resetting character to default state"));
	
	F_VN_CharacterState DefaultState = F_VN_CharacterState::CreateEmpty(TEXT("DefaultState"));
	SetCharacterState(DefaultState, 0.0f); // Мгновенный сброс
}

void AVNCharacter::CopyCurrentStateToMainPose()
{
	VN_LOG_DEBUG(TEXT("CopyCurrentStateToMainPose: Copying current state to main pose preset"));
	
	MainPosePreset = CurrentState;
	MainPosePreset.StateID = TEXT("MainPose");
	MainPosePreset.Description = FString::Printf(TEXT("Main pose for %s"), *CharacterName);
}

// =====================================================
// МЕТОДЫ ДЛЯ РАБОТЫ С КОНКРЕТНЫМИ КОМПОНЕНТАМИ
// =====================================================

bool AVNCharacter::SetEyesFromPreset()
{
	if (!MainPosePreset.EyesSpriteConfig.Sprite.IsNull())
	{
		SetEyes(MainPosePreset.EyesSpriteConfig.Sprite, true, 0.5f);
		return true;
	}
	
	VN_LOG_DEBUG(TEXT("SetEyesFromPreset: No eyes sprite in main pose preset"));
	return false;
}

bool AVNCharacter::SetMouthFromPreset()
{
	if (!MainPosePreset.MouthSpriteConfig.Sprite.IsNull())
	{
		SetMouth(MainPosePreset.MouthSpriteConfig.Sprite, true, 0.5f);
		return true;
	}
	
	VN_LOG_DEBUG(TEXT("SetMouthFromPreset: No mouth sprite in main pose preset"));
	return false;
}

bool AVNCharacter::SetEyebrowsFromPreset()
{
	if (!MainPosePreset.EyebrowSpriteConfig.Sprite.IsNull())
	{
		SetEyebrows(MainPosePreset.EyebrowSpriteConfig.Sprite, true, 0.5f);
		return true;
	}
	
	VN_LOG_DEBUG(TEXT("SetEyebrowsFromPreset: No eyebrow sprite in main pose preset"));
	return false;
}

bool AVNCharacter::SetBodyFromPreset()
{
	if (!MainPosePreset.BodyConfig.SkeletalMesh.IsNull())
	{
		SetBody(MainPosePreset.BodyConfig.SkeletalMesh, true, 1.0f);
		return true;
	}
	
	VN_LOG_DEBUG(TEXT("SetBodyFromPreset: No body mesh in main pose preset"));
	return false;
}

bool AVNCharacter::SetArmsFromPreset()
{
	if (!MainPosePreset.ArmsConfig.SkeletalMesh.IsNull())
	{
		SetArms(MainPosePreset.ArmsConfig.SkeletalMesh, true, 1.0f);
		return true;
	}
	
	VN_LOG_DEBUG(TEXT("SetArmsFromPreset: No arms mesh in main pose preset"));
	return false;
}

void AVNCharacter::HideAllFacialFeatures(bool bAnimate, float Duration)
{
	VN_LOG_DEBUG(TEXT("HideAllFacialFeatures: Hiding all facial features"));
	
	// Скрываем все элементы лица передачей null
	SetEyes(nullptr, bAnimate, Duration);
	SetMouth(nullptr, bAnimate, Duration);
	SetEyebrows(nullptr, bAnimate, Duration);
	
	// Можно добавить и другие элементы лица если нужно
}

void AVNCharacter::RestoreAllFacialFeaturesFromPreset(bool bAnimate, float Duration)
{
	VN_LOG_DEBUG(TEXT("RestoreAllFacialFeaturesFromPreset: Restoring facial features from preset"));
	
	// Восстанавливаем все элементы лица из пресета
	SetEyesFromPreset();
	SetMouthFromPreset();
	SetEyebrowsFromPreset();
}

// =====================================================
// ДОПОЛНИТЕЛЬНЫЕ УТИЛИТЫ ДЛЯ ОТЛАДКИ
// =====================================================

int32 AVNCharacter::GetVisibleSkeletalComponentsCount() const
{
	int32 Count = 0;
	TArray<USkeletalMeshComponent*> SkeletalComponents = GetAllSkeletalComponents();
	
	for (USkeletalMeshComponent* Component : SkeletalComponents)
	{
		if (Component && Component->IsVisible() && Component->GetSkeletalMeshAsset())
		{
			Count++;
		}
	}
	
	return Count;
}

int32 AVNCharacter::GetVisibleSpriteComponentsCount() const
{
	int32 Count = 0;
	TArray<UPaperSpriteComponent*> SpriteComponents = GetAllSpriteComponents();
	
	for (UPaperSpriteComponent* Component : SpriteComponents)
	{
		if (Component && Component->IsVisible() && Component->GetSprite())
		{
			Count++;
		}
	}
	
	return Count;
}

TArray<FString> AVNCharacter::GetVisibleComponentNames() const
{
	TArray<FString> VisibleNames;
	
	// Проверяем Skeletal Mesh компоненты
	TArray<USkeletalMeshComponent*> SkeletalComponents = GetAllSkeletalComponents();
	for (USkeletalMeshComponent* Component : SkeletalComponents)
	{
		if (Component && Component->IsVisible() && Component->GetSkeletalMeshAsset())
		{
			VisibleNames.Add(Component->GetName());
		}
	}
	
	// Проверяем Sprite компоненты
	TArray<UPaperSpriteComponent*> SpriteComponents = GetAllSpriteComponents();
	for (UPaperSpriteComponent* Component : SpriteComponents)
	{
		if (Component && Component->IsVisible() && Component->GetSprite())
		{
			VisibleNames.Add(Component->GetName());
		}
	}
	
	return VisibleNames;
}

FString AVNCharacter::GetDetailedStatusString() const
{
	FString StatusString = FString::Printf(TEXT("=== VN Character Status: %s ===\n"), *CharacterName);
	
	StatusString += FString::Printf(TEXT("Current State: %s\n"), *CurrentState.StateID.ToString());
	StatusString += FString::Printf(TEXT("Main Pose Preset: %s\n"), *MainPosePreset.StateID.ToString());
	StatusString += FString::Printf(TEXT("Is In Focus: %s\n"), bIsInFocus ? TEXT("Yes") : TEXT("No"));
	StatusString += FString::Printf(TEXT("Is Visible: %s\n"), IsVisible() ? TEXT("Yes") : TEXT("No"));
	StatusString += FString::Printf(TEXT("Is Animating: %s\n"), IsAnimating() ? TEXT("Yes") : TEXT("No"));
	
	StatusString += FString::Printf(TEXT("Visible Skeletal Components: %d\n"), GetVisibleSkeletalComponentsCount());
	StatusString += FString::Printf(TEXT("Visible Sprite Components: %d\n"), GetVisibleSpriteComponentsCount());
	
	TArray<FString> VisibleNames = GetVisibleComponentNames();
	if (VisibleNames.Num() > 0)
	{
		StatusString += TEXT("Visible Components: ");
		for (int32 i = 0; i < VisibleNames.Num(); ++i)
		{
			StatusString += VisibleNames[i];
			if (i < VisibleNames.Num() - 1)
			{
				StatusString += TEXT(", ");
			}
		}
		StatusString += TEXT("\n");
	}
	
	return StatusString;
}

// =====================================================
// УТИЛИТЫ ДЛЯ BLUEPRINT ИНТЕГРАЦИИ
// =====================================================

void AVNCharacter::SetCharacterNameSafe(const FString& NewName)
{
	if (NewName.IsEmpty())
	{
		VN_LOG_WARNING(TEXT("SetCharacterNameSafe: Attempted to set empty character name"));
		return;
	}
	
	CharacterName = NewName;
	VN_LOG_DEBUG(TEXT("Character name set to: %s"), *CharacterName);
}

FString AVNCharacter::GetCharacterNameSafe() const
{
	return CharacterName.IsEmpty() ? TEXT("Unnamed Character") : CharacterName;
}

bool AVNCharacter::HasValidCurrentState() const
{
	return CurrentState.IsValid() && CurrentState.HasVisibleComponents();
}

bool AVNCharacter::HasValidMainPosePreset() const
{
	return MainPosePreset.IsValid() && MainPosePreset.HasVisibleComponents();
}

// =====================================================
// МЕТОДЫ ДЛЯ РАБОТЫ С ЦВЕТАМИ И ЭФФЕКТАМИ
// =====================================================

void AVNCharacter::SetGlobalTint(const FLinearColor& TintColor, bool bAnimate, float Duration)
{
	VN_LOG_DEBUG(TEXT("SetGlobalTint: Setting global tint color"));
	
	// Применяем тинт ко всем видимым компонентам
	TArray<USceneComponent*> AllComponents = GetAllRenderComponents();
	for (USceneComponent* Component : AllComponents)
	{
		if (Component && Component->IsVisible())
		{
			FLinearColor CurrentColor = GetBaseColorForComponent(Component);
			FLinearColor NewColor = CurrentColor * TintColor;
			
			if (bAnimate && Duration > 0.0f)
			{
				// Здесь можно добавить логику анимации цвета
				// Пока применяем мгновенно
				SetComponentColor(Component, NewColor);
			}
			else
			{
				SetComponentColor(Component, NewColor);
			}
		}
	}
}

void AVNCharacter::ResetGlobalTint(bool bAnimate, float Duration)
{
	SetGlobalTint(FLinearColor::White, bAnimate, Duration);
}

void AVNCharacter::SetGlobalAlpha(float Alpha, bool bAnimate, float Duration)
{
	VN_LOG_DEBUG(TEXT("SetGlobalAlpha: Setting global alpha to %.2f"), Alpha);
	
	Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	
	// Применяем альфу ко всем видимым компонентам
	TArray<USceneComponent*> AllComponents = GetAllRenderComponents();
	for (USceneComponent* Component : AllComponents)
	{
		if (Component && Component->IsVisible())
		{
			if (bAnimate && Duration > 0.0f)
			{
				// Здесь можно добавить логику анимации альфы
				// Пока применяем мгновенно
				SetComponentAlpha(Component, Alpha);
			}
			else
			{
				SetComponentAlpha(Component, Alpha);
			}
		}
	}
}