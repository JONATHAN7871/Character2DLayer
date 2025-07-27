#include "Actors/VNCharacter.h"
#include "Components/VNCharacterAnimationManager.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"

// ===============================================
// ПРИНЦИП 2: Группировка индивидуальных изменений
// ===============================================

void AVNCharacter::SetSkeletalMesh(E_VN_ComponentID_Skeletal ComponentID, TSoftObjectPtr<USkeletalMesh> SkeletalMesh, bool bAnimate, float Duration)
{
	USkeletalMeshComponent* MainComponent = GetSkeletalComponent(ComponentID);
	if (!MainComponent)
	{
		VN_LOG_WARNING(TEXT("SetSkeletalMesh: Component not found for ID %d"), (int32)ComponentID);
		return;
	}

	// --- КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Завершаем текущие анимации если быстро меняем ---
	if (AnimationManager && AnimationManager->IsAnimating() && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::Transition)
	{
		VN_LOG_WARNING(TEXT("SetSkeletalMesh: Forcing completion of ongoing transition"));
		AnimationManager->ClearAnimationQueue();
		FinalizeCurrentTransition();
	}

	// --- КЛЮЧЕВАЯ ПРОВЕРКА: Изменился ли ассет ---
	bool bAssetChanged = false;
	const USkeletalMesh* CurrentMesh = MainComponent->GetSkeletalMeshAsset();
	
	if (!CurrentMesh && !SkeletalMesh.IsNull())
	{
		bAssetChanged = true; // Был пустым, стал непустым
	}
	else if (CurrentMesh && SkeletalMesh.IsNull())
	{
		bAssetChanged = true; // Был непустым, стал пустым
	}
	else if (CurrentMesh && !SkeletalMesh.IsNull())
	{
		bAssetChanged = (CurrentMesh->GetPathName() != SkeletalMesh.ToString()); // Сравниваем пути
	}

	if (bAnimate && bAssetChanged && Duration > 0.0f && AnimationManager)
	{
		VN_LOG_DEBUG(TEXT("SetSkeletalMesh: Preparing transition for component: %s. From [%s] to [%s]"), 
			*MainComponent->GetName(),
			CurrentMesh ? *CurrentMesh->GetName() : TEXT("None"),
			SkeletalMesh.IsNull() ? TEXT("None") : *SkeletalMesh.ToString());
		
		if (USkeletalMeshComponent* FadeComponent = GetSkeletalFadeComponent(ComponentID))
		{
			PrepareSkeletalTransition(MainComponent, FadeComponent, SkeletalMesh);
			RequestTransitionCommit(Duration);
		}
		else
		{
			VN_LOG_WARNING(TEXT("SetSkeletalMesh: Fade component not found for ID %d"), (int32)ComponentID);
			ValidateAndSetupSkeletalComponent(MainComponent, SkeletalMesh);
		}
	}
	else
	{
		// Мгновенное применение
		if (!bAssetChanged)
		{
			VN_LOG_DEBUG(TEXT("SetSkeletalMesh: Asset unchanged for component: %s, skipping animation"), *MainComponent->GetName());
		}
		ValidateAndSetupSkeletalComponent(MainComponent, SkeletalMesh);
	}
}

void AVNCharacter::SetSprite(E_VN_ComponentID_Sprite ComponentID, TSoftObjectPtr<UPaperSprite> Sprite, bool bAnimate, float Duration)
{
	UPaperSpriteComponent* MainComponent = GetSpriteComponent(ComponentID);
	if (!MainComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("SetSprite: Component not found for ID %d"), (int32)ComponentID);
		return;
	}

	// --- КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Завершаем текущие анимации если быстро меняем ---
	if (AnimationManager && AnimationManager->IsAnimating() && AnimationManager->GetCurrentAnimationType() == EVNAnimationType::Transition)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetSprite: Forcing completion of ongoing transition"));
		AnimationManager->ClearAnimationQueue();
		FinalizeCurrentTransition();
	}

	// --- КЛЮЧЕВАЯ ПРОВЕРКА: Изменился ли ассет ---
	bool bAssetChanged = false;
	const UPaperSprite* CurrentSprite = MainComponent->GetSprite();
	
	if (!CurrentSprite && !Sprite.IsNull())
	{
		bAssetChanged = true; // Был пустым, стал непустым
	}
	else if (CurrentSprite && Sprite.IsNull())
	{
		bAssetChanged = true; // Был непустым, стал пустым
	}
	else if (CurrentSprite && !Sprite.IsNull())
	{
		bAssetChanged = (CurrentSprite->GetPathName() != Sprite.ToString()); // Сравниваем пути
	}

	if (bAnimate && bAssetChanged && Duration > 0.0f && AnimationManager)
	{
		if (UPaperSpriteComponent* FadeComponent = GetSpriteFadeComponent(ComponentID))
		{
			PrepareSpriteTransition(MainComponent, FadeComponent, Sprite);
			RequestTransitionCommit(Duration);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("SetSprite: Fade component not found for ID %d"), (int32)ComponentID);
			ValidateAndSetupSpriteComponent(MainComponent, Sprite);
		}
	}
	else
	{
		// Мгновенное применение
		if (!bAssetChanged)
		{
			UE_LOG(LogTemp, Warning, TEXT("SetSprite: Asset unchanged, skipping"));
		}
		ValidateAndSetupSpriteComponent(MainComponent, Sprite);
	}

	OnCharacterComponentChanged.Broadcast(ComponentID);
}

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
    VN_LOG_DEBUG(TEXT("SetFace: Setting multiple face components with animate=%s, duration=%.2f"), bAnimate ? TEXT("true") : TEXT("false"), Duration);

    // НОВАЯ ЛОГИКА (ОШИБКА #3): Убраны лишние вызовы и дублирование логики.
    // Теперь мы только подготавливаем переходы и запрашиваем одну общую анимацию.
    if (bAnimate && Duration > 0.0f && AnimationManager)
    {
        bool bAnyComponentChanged = false;

        // --- Проверяем и подготавливаем ГЛАЗА ---
        UPaperSpriteComponent* EyesComp = GetSpriteComponent(E_VN_ComponentID_Sprite::Eyes);
        if (EyesComp)
        {
            const UPaperSprite* CurrentSprite = EyesComp->GetSprite();
            const bool bAssetChanged = (!CurrentSprite && !EyesSprite.IsNull()) ||
                                       (CurrentSprite && EyesSprite.IsNull()) ||
                                       (CurrentSprite && !EyesSprite.IsNull() && CurrentSprite->GetPathName() != EyesSprite.ToString());
            if (bAssetChanged)
            {
                if (UPaperSpriteComponent* FadeComp = GetSpriteFadeComponent(E_VN_ComponentID_Sprite::Eyes))
                {
                    PrepareSpriteTransition(EyesComp, FadeComp, EyesSprite);
                    bAnyComponentChanged = true;
                }
            }
        }

        // --- Проверяем и подготавливаем РОТ ---
        UPaperSpriteComponent* MouthComp = GetSpriteComponent(E_VN_ComponentID_Sprite::Mouth);
        if (MouthComp)
        {
            const UPaperSprite* CurrentSprite = MouthComp->GetSprite();
            const bool bAssetChanged = (!CurrentSprite && !MouthSprite.IsNull()) ||
                                       (CurrentSprite && MouthSprite.IsNull()) ||
                                       (CurrentSprite && !MouthSprite.IsNull() && CurrentSprite->GetPathName() != MouthSprite.ToString());
            if (bAssetChanged)
            {
                if (UPaperSpriteComponent* FadeComp = GetSpriteFadeComponent(E_VN_ComponentID_Sprite::Mouth))
                {
                    PrepareSpriteTransition(MouthComp, FadeComp, MouthSprite);
                    bAnyComponentChanged = true;
                }
            }
        }

        // --- Проверяем и подготавливаем БРОВИ ---
        UPaperSpriteComponent* EyebrowComp = GetSpriteComponent(E_VN_ComponentID_Sprite::Eyebrow);
        if (EyebrowComp)
        {
            const UPaperSprite* CurrentSprite = EyebrowComp->GetSprite();
            const bool bAssetChanged = (!CurrentSprite && !EyebrowSprite.IsNull()) ||
                                       (CurrentSprite && EyebrowSprite.IsNull()) ||
                                       (CurrentSprite && !EyebrowSprite.IsNull() && CurrentSprite->GetPathName() != EyebrowSprite.ToString());
            if (bAssetChanged)
            {
                if (UPaperSpriteComponent* FadeComp = GetSpriteFadeComponent(E_VN_ComponentID_Sprite::Eyebrow))
                {
                    PrepareSpriteTransition(EyebrowComp, FadeComp, EyebrowSprite);
                    bAnyComponentChanged = true;
                }
            }
        }

        // Если хоть что-то изменилось, запрашиваем групповую анимацию
        if (bAnyComponentChanged)
        {
            RequestTransitionCommit(Duration);
        }
    }
    else
    {
        // Мгновенное применение (этот блок был правильным)
        SetSprite(E_VN_ComponentID_Sprite::Eyes, EyesSprite, false, 0.0f);
        SetSprite(E_VN_ComponentID_Sprite::Mouth, MouthSprite, false, 0.0f);
        SetSprite(E_VN_ComponentID_Sprite::Eyebrow, EyebrowSprite, false, 0.0f);
    }
}

bool AVNCharacter::IsAnimating() const
{
	return AnimationManager && AnimationManager->IsAnimating();
}

FLinearColor AVNCharacter::GetTargetColorForComponent(USceneComponent* Component) const
{
	FLinearColor BaseColor = GetBaseColorForComponent(Component);
	return bIsInFocus ? BaseColor : BaseColor * DimColorMultiplier;
}

FLinearColor AVNCharacter::GetBaseColorForComponent(USceneComponent* Component) const
{
	return FLinearColor::White;
}