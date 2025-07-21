#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"

// =====================================================
// ОСНОВНОЕ API - ПРЯМОЕ УПРАВЛЕНИЕ КОМПОНЕНТАМИ
// =====================================================

void AVNCharacter::SetSkeletalMesh(E_VN_ComponentID_Skeletal ComponentID, TSoftObjectPtr<USkeletalMesh> SkeletalMesh, bool bAnimate, float Duration)
{
	VN_LOG_DEBUG(TEXT("SetSkeletalMesh: ComponentID %d, Animate: %s"), (int32)ComponentID, bAnimate ? TEXT("Yes") : TEXT("No"));

	USkeletalMeshComponent* MainComponent = GetSkeletalComponent(ComponentID);
	if (!MainComponent)
	{
		VN_LOG_WARNING(TEXT("SetSkeletalMesh: Component not found for ID %d"), (int32)ComponentID);
		return;
	}

	if (bAnimate && Duration > 0.0f && AnimationManager)
	{
		// Подготавливаем анимацию перехода
		USkeletalMeshComponent* FadeComponent = GetSkeletalFadeComponent(ComponentID);
		if (FadeComponent)
		{
			PrepareSkeletalTransition(MainComponent, FadeComponent, SkeletalMesh);
			AnimationManager->PlayTransition(Duration);
		}
		else
		{
			// Если нет fade компонента, применяем мгновенно
			ValidateAndSetupSkeletalComponent(MainComponent, SkeletalMesh);
		}
	}
	else
	{
		// Мгновенное применение
		ValidateAndSetupSkeletalComponent(MainComponent, SkeletalMesh);
	}
}

void AVNCharacter::SetSprite(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> Sprite, bool bAnimate, float Duration)
{
	VN_LOG_DEBUG(TEXT("SetSprite: ComponentID %d, Animate: %s"), (int32)ComponentID, bAnimate ? TEXT("Yes") : TEXT("No"));

	UPaperSpriteComponent* MainComponent = GetSpriteComponent(ComponentID);
	if (!MainComponent)
	{
		VN_LOG_WARNING(TEXT("SetSprite: Component not found for ID %d"), (int32)ComponentID);
		return;
	}

	if (bAnimate && Duration > 0.0f && AnimationManager)
	{
		// Подготавливаем анимацию перехода
		UPaperSpriteComponent* FadeComponent = GetSpriteFadeComponent(ComponentID);
		if (FadeComponent)
		{
			PrepareSpriteTransition(MainComponent, FadeComponent, Sprite);
			AnimationManager->PlayTransition(Duration);
		}
		else
		{
			// Если нет fade компонента, применяем мгновенно
			ValidateAndSetupSpriteComponent(MainComponent, Sprite);
			ApplyIndividualSpriteTransform(MainComponent, ComponentID);
		}
	}
	else
	{
		// Мгновенное применение
		ValidateAndSetupSpriteComponent(MainComponent, Sprite);
		ApplyIndividualSpriteTransform(MainComponent, ComponentID);
	}

	// Уведомляем о изменении
	OnCharacterComponentChanged.Broadcast(ComponentID);
}

// =====================================================
// УПРОЩЕННЫЕ МЕТОДЫ ДЛЯ БЫСТРОГО ДОСТУПА
// =====================================================

void AVNCharacter::SetEyes(TSoftObjectPtr<UPaperSprite> EyesSprite, bool bAnimate, float Duration)
{
	SetSprite(E_VN_ComponentID_Sprite::Eyes, EyesSprite, bAnimate, Duration);
}

void AVNCharacter::SetMouth(TSoftObjectPtr<UPaperSprite> MouthSprite, bool bAnimate, float Duration)
{
	SetSprite(E_VN_ComponentID_Sprite::Mouth, MouthSprite, bAnimate, Duration);
}

void AVNCharacter::SetEyebrows(TSoftObjectPtr<UPaperSprite> EyebrowSprite, bool bAnimate, float Duration)
{
	SetSprite(E_VN_ComponentID_Sprite::Eyebrow, EyebrowSprite, bAnimate, Duration);
}

void AVNCharacter::SetBody(TSoftObjectPtr<USkeletalMesh> BodyMesh, bool bAnimate, float Duration)
{
	SetSkeletalMesh(E_VN_ComponentID_Skeletal::Body, BodyMesh, bAnimate, Duration);
}

void AVNCharacter::SetArms(TSoftObjectPtr<USkeletalMesh> ArmsMesh, bool bAnimate, float Duration)
{
	SetSkeletalMesh(E_VN_ComponentID_Skeletal::Arms, ArmsMesh, bAnimate, Duration);
}

void AVNCharacter::SetFace(TSoftObjectPtr<UPaperSprite> EyesSprite, TSoftObjectPtr<UPaperSprite> MouthSprite, TSoftObjectPtr<UPaperSprite> EyebrowSprite, bool bAnimate, float Duration)
{
	VN_LOG_DEBUG(TEXT("SetFace: Setting multiple facial components"));

	// Устанавливаем все элементы лица
	// ВАЖНО: Используем bAnimate=false для индивидуальных вызовов, 
	// чтобы избежать множественных анимаций
	SetSprite(E_VN_ComponentID_Sprite::Eyes, EyesSprite, false, 0.0f);
	SetSprite(E_VN_ComponentID_Sprite::Mouth, MouthSprite, false, 0.0f);
	SetSprite(E_VN_ComponentID_Sprite::Eyebrow, EyebrowSprite, false, 0.0f);

	// Если нужна анимация, запускаем её один раз для всех изменений
	if (bAnimate && Duration > 0.0f && AnimationManager)
	{
		AnimationManager->PlayTransition(Duration);
	}
}

// =====================================================
// УТИЛИТЫ И ИНФОРМАЦИЯ
// =====================================================

bool AVNCharacter::IsAnimating() const
{
	return AnimationManager && AnimationManager->IsAnimating();
}

FLinearColor AVNCharacter::GetTargetColorForComponent(USceneComponent* Component) const
{
	if (!Component)
	{
		return FLinearColor::White;
	}

	// Получаем базовый цвет (пока всегда белый, так как нет системы состояний)
	FLinearColor BaseColor = FLinearColor::White;

	// Применяем модификатор фокуса
	if (bIsInFocus)
	{
		return BaseColor;
	}
	else
	{
		return BaseColor * DimColorMultiplier;
	}
}

FLinearColor AVNCharacter::GetBaseColorForComponent(USceneComponent* Component) const
{
	// Пока всегда возвращаем белый, так как система состояний убрана
	return FLinearColor::White;
}