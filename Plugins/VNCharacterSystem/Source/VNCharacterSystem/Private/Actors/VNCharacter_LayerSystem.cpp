#include "Actors/VNCharacter.h"
#include "VNCharacterSystemModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "PaperSpriteComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

void AVNCharacter::SetCharacterLayer(int32 CharacterLayer)
{
    CharacterBaseLayer = CharacterLayer;
    
    VN_LOG_DEBUG(TEXT("SetCharacterLayer: Character '%s' set to layer %d"), 
        *GetCharacterName(), CharacterBaseLayer);
    
    // Обновляем все слои компонентов
    SetupAllComponentLayers();
}

int32 AVNCharacter::GetCharacterLayer() const
{
    return CharacterBaseLayer;
}

void AVNCharacter::SetupAllComponentLayers()
{
    // Константы для слоев компонентов внутри персонажа
    const int32 LayersPerCharacter = 50; // УВЕЛИЧИЛИ с 30 до 50 для большего запаса
    const int32 BaseOffset = CharacterBaseLayer * LayersPerCharacter;
    
    // ОСНОВНЫЕ СЛОИ (низкие значения = дальше от камеры)
    const int32 BodyLayer = BaseOffset + 0;
    const int32 ArmsLayer = BaseOffset + 2;
    const int32 HeadLayer = BaseOffset + 5;
    
    // СЛОИ ЛИЦА (выше головы, но НЕ выше других персонажей)
    const int32 MouthLayer = BaseOffset + 6;    // Сразу поверх головы
    const int32 EyesLayer = BaseOffset + 7;     // Поверх рта
    const int32 EyelidsLayer = BaseOffset + 8;  // Поверх глаз
    const int32 WinkLayer = BaseOffset + 9;     // Поверх век
    const int32 EyebrowLayer = BaseOffset + 10; // Поверх всего лица
    
    // КАСТОМНЫЕ СЛОИ
    const int32 CustomLayer01 = BaseOffset + 15;
    const int32 CustomLayer02 = BaseOffset + 16;
    const int32 CustomLayer03 = BaseOffset + 17;
    
    // ЭФФЕКТЫ (поверх персонажа, но не поверх других персонажей)
    const int32 EffectsLayerStart = BaseOffset + 25;
    
    // ТЕНЬ (поверх всего для этого персонажа)
    const int32 BodyShadowLayer = BaseOffset + 45;

    VN_LOG_DEBUG(TEXT("SetupAllComponentLayers: Character '%s' layer %d, range: %d-%d"), 
        *GetCharacterName(), CharacterBaseLayer, BaseOffset, BaseOffset + 49);

    // === ОСНОВНЫЕ КОМПОНЕНТЫ ===
    
    // Тело
    if (Body_Skeletal) Body_Skeletal->SetTranslucentSortPriority(BodyLayer);
    if (Body_Skeletal_Fade) Body_Skeletal_Fade->SetTranslucentSortPriority(BodyLayer);
    if (Body_Sprite) Body_Sprite->SetTranslucentSortPriority(BodyLayer);
    if (Body_Sprite_Fade) Body_Sprite_Fade->SetTranslucentSortPriority(BodyLayer);

    // Руки
    if (Arms_Skeletal) Arms_Skeletal->SetTranslucentSortPriority(ArmsLayer);
    if (Arms_Skeletal_Fade) Arms_Skeletal_Fade->SetTranslucentSortPriority(ArmsLayer);
    if (Arms_Sprite) Arms_Sprite->SetTranslucentSortPriority(ArmsLayer);
    if (Arms_Sprite_Fade) Arms_Sprite_Fade->SetTranslucentSortPriority(ArmsLayer);

    // Голова
    if (Head_Skeletal) Head_Skeletal->SetTranslucentSortPriority(HeadLayer);
    if (Head_Skeletal_Fade) Head_Skeletal_Fade->SetTranslucentSortPriority(HeadLayer);
    if (Head_Sprite) Head_Sprite->SetTranslucentSortPriority(HeadLayer);
    if (Head_Sprite_Fade) Head_Sprite_Fade->SetTranslucentSortPriority(HeadLayer);

    // === КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: ЭЛЕМЕНТЫ ЛИЦА ===
    // Каждый элемент лица получает СВОЙ ИНДИВИДУАЛЬНЫЙ слой
    
    // Рот
    if (Mouth_Sprite) {
        Mouth_Sprite->SetTranslucentSortPriority(MouthLayer);
        VN_LOG_DEBUG(TEXT("Mouth layer: %d"), MouthLayer);
    }
    if (Mouth_Sprite_Fade) Mouth_Sprite_Fade->SetTranslucentSortPriority(MouthLayer);

    // Глаза
    if (Eyes_Sprite) {
        Eyes_Sprite->SetTranslucentSortPriority(EyesLayer);
        VN_LOG_DEBUG(TEXT("Eyes layer: %d"), EyesLayer);
    }
    if (Eyes_Sprite_Fade) Eyes_Sprite_Fade->SetTranslucentSortPriority(EyesLayer);

    // Веки
    if (Eyelids_Sprite) {
        Eyelids_Sprite->SetTranslucentSortPriority(EyelidsLayer);
        VN_LOG_DEBUG(TEXT("Eyelids layer: %d"), EyelidsLayer);
    }
    if (Eyelids_Sprite_Fade) Eyelids_Sprite_Fade->SetTranslucentSortPriority(EyelidsLayer);

    // Подмигивание
    if (Wink_Sprite) {
        Wink_Sprite->SetTranslucentSortPriority(WinkLayer);
        VN_LOG_DEBUG(TEXT("Wink layer: %d"), WinkLayer);
    }
    if (Wink_Sprite_Fade) Wink_Sprite_Fade->SetTranslucentSortPriority(WinkLayer);

    // Брови
    if (Eyebrow_Sprite) {
        Eyebrow_Sprite->SetTranslucentSortPriority(EyebrowLayer);
        VN_LOG_DEBUG(TEXT("Eyebrow layer: %d"), EyebrowLayer);
    }
    if (Eyebrow_Sprite_Fade) Eyebrow_Sprite_Fade->SetTranslucentSortPriority(EyebrowLayer);

    // === КАСТОМНЫЕ КОМПОНЕНТЫ ===
    if (Custom01_Skeletal) Custom01_Skeletal->SetTranslucentSortPriority(CustomLayer01);
    if (Custom01_Skeletal_Fade) Custom01_Skeletal_Fade->SetTranslucentSortPriority(CustomLayer01);

    if (Custom02_Skeletal) Custom02_Skeletal->SetTranslucentSortPriority(CustomLayer02);
    if (Custom02_Skeletal_Fade) Custom02_Skeletal_Fade->SetTranslucentSortPriority(CustomLayer02);

    if (Custom03_Skeletal) Custom03_Skeletal->SetTranslucentSortPriority(CustomLayer03);
    if (Custom03_Skeletal_Fade) Custom03_Skeletal_Fade->SetTranslucentSortPriority(CustomLayer03);

    // === ЭФФЕКТЫ ===
    if (EmotionBodyEffect01_Sprite) EmotionBodyEffect01_Sprite->SetTranslucentSortPriority(EffectsLayerStart + 0);
    if (EmotionBodyEffect01_Sprite_Fade) EmotionBodyEffect01_Sprite_Fade->SetTranslucentSortPriority(EffectsLayerStart + 0);

    if (EmotionBodyEffect02_Sprite) EmotionBodyEffect02_Sprite->SetTranslucentSortPriority(EffectsLayerStart + 1);
    if (EmotionBodyEffect02_Sprite_Fade) EmotionBodyEffect02_Sprite_Fade->SetTranslucentSortPriority(EffectsLayerStart + 1);

    if (EmotionBodyEffect03_Sprite) EmotionBodyEffect03_Sprite->SetTranslucentSortPriority(EffectsLayerStart + 2);
    if (EmotionBodyEffect03_Sprite_Fade) EmotionBodyEffect03_Sprite_Fade->SetTranslucentSortPriority(EffectsLayerStart + 2);

    if (EmotionHeadEffect01_Sprite) EmotionHeadEffect01_Sprite->SetTranslucentSortPriority(EffectsLayerStart + 3);
    if (EmotionHeadEffect01_Sprite_Fade) EmotionHeadEffect01_Sprite_Fade->SetTranslucentSortPriority(EffectsLayerStart + 3);

    if (EmotionHeadEffect02_Sprite) EmotionHeadEffect02_Sprite->SetTranslucentSortPriority(EffectsLayerStart + 4);
    if (EmotionHeadEffect02_Sprite_Fade) EmotionHeadEffect02_Sprite_Fade->SetTranslucentSortPriority(EffectsLayerStart + 4);

    if (EmotionHeadEffect03_Sprite) EmotionHeadEffect03_Sprite->SetTranslucentSortPriority(EffectsLayerStart + 5);
    if (EmotionHeadEffect03_Sprite_Fade) EmotionHeadEffect03_Sprite_Fade->SetTranslucentSortPriority(EffectsLayerStart + 5);

    // === ТЕНЬ (поверх всего для этого персонажа) ===
    if (BodyShadow_Sprite) BodyShadow_Sprite->SetTranslucentSortPriority(BodyShadowLayer);
    if (BodyShadow_Sprite_Fade) BodyShadow_Sprite_Fade->SetTranslucentSortPriority(BodyShadowLayer);

    VN_LOG_DEBUG(TEXT("SetupAllComponentLayers: Layer setup completed for character '%s' at base layer %d"), 
        *GetCharacterName(), CharacterBaseLayer);
}