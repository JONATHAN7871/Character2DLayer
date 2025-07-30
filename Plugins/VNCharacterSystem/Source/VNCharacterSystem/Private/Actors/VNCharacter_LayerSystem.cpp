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
    const int32 LayersPerCharacter = 30; // Каждый персонаж занимает ровно 30 слоев
    const int32 BaseOffset = CharacterBaseLayer * LayersPerCharacter;
    
    const int32 BodyLayer = BaseOffset + 0;
    const int32 ArmsLayer = BaseOffset + 1;
    const int32 HeadLayer = BaseOffset + 2;
    const int32 MouthLayer = BaseOffset + 3;
    const int32 EyesLayer = BaseOffset + 4;
    const int32 EyelidsLayer = BaseOffset + 5;
    const int32 WinkLayer = BaseOffset + 6;
    const int32 EyebrowLayer = BaseOffset + 7;
    const int32 CustomLayer01 = BaseOffset + 10;
    const int32 CustomLayer02 = BaseOffset + 11;
    const int32 CustomLayer03 = BaseOffset + 12;
    const int32 EffectsLayerStart = BaseOffset + 20;
    const int32 BodyShadowLayer = BaseOffset + 29; // Тень поверх всего

    // === Слой Тела (Priority: 0) ===
    if (Body_Skeletal) Body_Skeletal->SetTranslucentSortPriority(BodyLayer);
    if (Body_Skeletal_Fade) Body_Skeletal_Fade->SetTranslucentSortPriority(BodyLayer);
    if (Body_Sprite) Body_Sprite->SetTranslucentSortPriority(BodyLayer);
    if (Body_Sprite_Fade) Body_Sprite_Fade->SetTranslucentSortPriority(BodyLayer);

    // === Слой Рук (Priority: 1) ===
    if (Arms_Skeletal) Arms_Skeletal->SetTranslucentSortPriority(ArmsLayer);
    if (Arms_Skeletal_Fade) Arms_Skeletal_Fade->SetTranslucentSortPriority(ArmsLayer);
    if (Arms_Sprite) Arms_Sprite->SetTranslucentSortPriority(ArmsLayer);
    if (Arms_Sprite_Fade) Arms_Sprite_Fade->SetTranslucentSortPriority(ArmsLayer);

    // === Слой Головы (Priority: 2) ===
    if (Head_Skeletal) Head_Skeletal->SetTranslucentSortPriority(HeadLayer);
    if (Head_Skeletal_Fade) Head_Skeletal_Fade->SetTranslucentSortPriority(HeadLayer);
    if (Head_Sprite) Head_Sprite->SetTranslucentSortPriority(HeadLayer);
    if (Head_Sprite_Fade) Head_Sprite_Fade->SetTranslucentSortPriority(HeadLayer);

    // === Слои Лица (поверх Головы) ===
    if (Mouth_Sprite) Mouth_Sprite->SetTranslucentSortPriority(MouthLayer);
    if (Mouth_Sprite_Fade) Mouth_Sprite_Fade->SetTranslucentSortPriority(MouthLayer);

    if (Eyes_Sprite) Eyes_Sprite->SetTranslucentSortPriority(EyesLayer);
    if (Eyes_Sprite_Fade) Eyes_Sprite_Fade->SetTranslucentSortPriority(EyesLayer);

    if (Eyelids_Sprite) Eyelids_Sprite->SetTranslucentSortPriority(EyelidsLayer);
    if (Eyelids_Sprite_Fade) Eyelids_Sprite_Fade->SetTranslucentSortPriority(EyelidsLayer);

    if (Wink_Sprite) Wink_Sprite->SetTranslucentSortPriority(WinkLayer);
    if (Wink_Sprite_Fade) Wink_Sprite_Fade->SetTranslucentSortPriority(WinkLayer);

    if (Eyebrow_Sprite) Eyebrow_Sprite->SetTranslucentSortPriority(EyebrowLayer);
    if (Eyebrow_Sprite_Fade) Eyebrow_Sprite_Fade->SetTranslucentSortPriority(EyebrowLayer);

    // === Слои Кастомных компонентов ===
    if (Custom01_Skeletal) Custom01_Skeletal->SetTranslucentSortPriority(CustomLayer01);
    if (Custom01_Skeletal_Fade) Custom01_Skeletal_Fade->SetTranslucentSortPriority(CustomLayer01);

    if (Custom02_Skeletal) Custom02_Skeletal->SetTranslucentSortPriority(CustomLayer02);
    if (Custom02_Skeletal_Fade) Custom02_Skeletal_Fade->SetTranslucentSortPriority(CustomLayer02);

    if (Custom03_Skeletal) Custom03_Skeletal->SetTranslucentSortPriority(CustomLayer03);
    if (Custom03_Skeletal_Fade) Custom03_Skeletal_Fade->SetTranslucentSortPriority(CustomLayer03);

    // === Слои Эффектов ===
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

    // === Слой Тени (Priority: 29 - поверх всего) ===
    if (BodyShadow_Sprite) BodyShadow_Sprite->SetTranslucentSortPriority(BodyShadowLayer);
    if (BodyShadow_Sprite_Fade) BodyShadow_Sprite_Fade->SetTranslucentSortPriority(BodyShadowLayer);

    VN_LOG_DEBUG(TEXT("SetupAllComponentLayers: Layer setup completed for character '%s' at base layer %d (range: %d-%d)"), 
        *GetCharacterName(), CharacterBaseLayer, BaseOffset, BaseOffset + 29);
}