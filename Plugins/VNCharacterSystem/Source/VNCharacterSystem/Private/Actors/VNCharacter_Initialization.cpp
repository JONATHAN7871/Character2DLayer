// --- START OF FILE VNCharacter_Initialization.cpp ---

#include "Actors/VNCharacter.h"
#include "Data/VNCharacterDataAsset.h"
#include "Data/VNCharacterIdleAnimationDataAsset.h"
#include "Components/VNCharacterAnimationManager.h"
#include "Components/VNCharacterIdleAnimationManager.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "VNCharacterSystemModule.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

void AVNCharacter::RequestSpawn(const FString& NewName, bool bIsNarrator, TSoftObjectPtr<UVNCharacterDataAsset> InCharacterData, TSoftObjectPtr<UVNCharacterIdleAnimationDataAsset> InIdleData, 
                                bool bAnimateAsset, float AssetDuration, bool bShouldAppear, float AppearDuration)
{
    // 1. Сохраняем все параметры запроса в структуру
    CurrentSpawnRequest.CharacterName = NewName;
    CurrentSpawnRequest.bIsNarrator = bIsNarrator;
    CurrentSpawnRequest.CharacterDataPtr = InCharacterData;
    CurrentSpawnRequest.IdleDataPtr = InIdleData;
    CurrentSpawnRequest.bAnimateAsset = bAnimateAsset;
    CurrentSpawnRequest.AssetDuration = AssetDuration;
    CurrentSpawnRequest.bShouldAppear = bShouldAppear;
    CurrentSpawnRequest.AppearDuration = AppearDuration;

    // 2. Собираем список ассетов, которые нужно загрузить
    TArray<FSoftObjectPath> AssetsToLoad;
    if (!CurrentSpawnRequest.CharacterDataPtr.IsNull())
    {
        AssetsToLoad.Add(CurrentSpawnRequest.CharacterDataPtr.ToSoftObjectPath());
    }
    if (!CurrentSpawnRequest.IdleDataPtr.IsNull())
    {
        AssetsToLoad.Add(CurrentSpawnRequest.IdleDataPtr.ToSoftObjectPath());
    }

    // 3. Если ассеты есть, запускаем асинхронную загрузку. Если нет - сразу выполняем логику.
    if (AssetsToLoad.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("RequestSpawn: Starting async load for character '%s'"), *NewName);
        FStreamableManager& StreamableManager = UAssetManager::Get().GetStreamableManager();
        StreamableManager.RequestAsyncLoad(AssetsToLoad, FStreamableDelegate::CreateUObject(this, &AVNCharacter::OnAssetsLoadedForSpawn));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("RequestSpawn: No assets to load for '%s'. Executing spawn logic immediately."), *NewName);
        OnAssetsLoadedForSpawn();
    }
}

void AVNCharacter::OnAssetsLoadedForSpawn()
{
    // Используем данные из сохраненного запроса
    const FString& NewName = CurrentSpawnRequest.CharacterName;
    const bool bIsNarrator = CurrentSpawnRequest.bIsNarrator;
    UVNCharacterDataAsset* InCharacterData = CurrentSpawnRequest.CharacterDataPtr.Get();
    UVNCharacterIdleAnimationDataAsset* InIdleData = CurrentSpawnRequest.IdleDataPtr.Get();
    const bool bAnimateAsset = CurrentSpawnRequest.bAnimateAsset;
    const float AssetDuration = CurrentSpawnRequest.AssetDuration;
    const bool bShouldAppear = CurrentSpawnRequest.bShouldAppear;
    const float AppearDuration = CurrentSpawnRequest.AppearDuration;

    SetCharacterName(NewName);
    VN_LOG_DEBUG(TEXT("OnAssetsLoadedForSpawn: Assets loaded. Initializing character '%s'."), *NewName);

    if (AnimationManager) AnimationManager->ClearAnimationQueue();
    if (IdleAnimationManager) IdleAnimationManager->StopAllIdleAnimations();
    SetActorHiddenInGame(false);

    if (bIsNarrator)
    {
        ClearAllSettings();
        SetActorHiddenInGame(true);
        PrimaryActorTick.SetTickFunctionEnable(false);
        return;
    }

    PrimaryActorTick.SetTickFunctionEnable(true);
    SetActorHiddenInGame(true);

    if (InCharacterData)
    {
        ApplyDataAsset(InCharacterData, bAnimateAsset, AssetDuration);
    }
    else
    {
        ClearAllSettings();
    }

    if (InIdleData)
    {
        float IdleApplyDelay = (bAnimateAsset && AssetDuration > 0.f) ? AssetDuration + 0.1f : 0.f;
        if (IdleApplyDelay > 0.f)
        {
            FTimerHandle IdleApplyTimer;
            GetWorld()->GetTimerManager().SetTimer(IdleApplyTimer, [this, InIdleData]() {
                if (this && InIdleData) this->ApplyIdleAnimationDataAsset(InIdleData, true);
            }, IdleApplyDelay, false);
        }
        else
        {
            ApplyIdleAnimationDataAsset(InIdleData, true);
        }
    }

    if (bShouldAppear)
    {
        float AppearDelay = (bAnimateAsset && AssetDuration > 0.f) ? AssetDuration : 0.f;
        if (AppearDelay > 0.f)
        {
            FTimerHandle AppearTimer;
            GetWorld()->GetTimerManager().SetTimer(AppearTimer, [this, AppearDuration]() {
                if (this) this->Appear(AppearDuration);
            }, AppearDelay, false);
        }
        else
        {
            this->Appear(AppearDuration);
        }
    }
    else
    {
        SetActorHiddenInGame(false);
        ApplyVisibilityStateImmediate(true);
    }
}