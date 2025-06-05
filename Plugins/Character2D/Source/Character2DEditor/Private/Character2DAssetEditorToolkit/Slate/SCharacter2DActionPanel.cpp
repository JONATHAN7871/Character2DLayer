#include "Character2DAssetEditorToolkit/Slate/SCharacter2DActionPanel.h"
#include "Character2DActor.h"
#include "Character2DAsset.h"
#include "Character2DEnums.h"

#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Text/STextBlock.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "SCharacter2DActionPanel"

void SCharacter2DActionPanel::Construct(const FArguments& InArgs)
{
    CharacterAsset = InArgs._CharacterAsset;
    PreviewActor   = InArgs._PreviewActor;

    // Initialize state from actor if valid
    SyncStateFromActor();

    // --- Заполняем опции Dropdown для Transition ---
    TransitionOptions.Empty();
    for (int32 i = 0; i < static_cast<int32>(ECharacter2DTransitionType::ScaleOut) + 1; ++i)
    {
        ECharacter2DTransitionType Type = static_cast<ECharacter2DTransitionType>(i);
        TransitionOptions.Add(MakeShared<ECharacter2DTransitionType>(Type));
    }
    CurrentTransition = TransitionOptions[0]; // по умолчанию «None»

    // --- Заполняем опции Dropdown для Emotion ---
    EmotionOptions.Empty();
    for (int32 i = 0; i < static_cast<int32>(ECharacter2DEmotionEffect::Flash) + 1; ++i)
    {
        ECharacter2DEmotionEffect Effect = static_cast<ECharacter2DEmotionEffect>(i);
        EmotionOptions.Add(MakeShared<ECharacter2DEmotionEffect>(Effect));
    }
    CurrentEmotion = EmotionOptions[0]; // по умолчанию «Shake»

    // Начальные значения для Duration / Intensity
    TransitionDuration = 1.0f;
    EmotionDuration    = 2.0f;
    EmotionIntensity   = 0.5f;

    // Собираем весь UI в SScrollBox
    ChildSlot
    [
        SNew(SScrollBox)

        // === Quick Actions ===
        + SScrollBox::Slot().Padding(4)
        [
            SNew(SExpandableArea)
            .AreaTitle(LOCTEXT("QuickActions", "Quick Actions"))
            .InitiallyCollapsed(false)
            .BodyContent()
            [
                BuildQuickActionsSection()
            ]
        ]

        // === Animation Testing (Blink / Talk) ===
        + SScrollBox::Slot().Padding(4)
        [
            SNew(SExpandableArea)
            .AreaTitle(LOCTEXT("AnimationTesting", "Animation Testing"))
            .InitiallyCollapsed(true)
            .BodyContent()
            [
                BuildAnimationTestingSection()
            ]
        ]

        // === Transition Testing ===
        + SScrollBox::Slot().Padding(4)
        [
            SNew(SExpandableArea)
            .AreaTitle(LOCTEXT("TransitionTesting", "Transition Testing"))
            .InitiallyCollapsed(true)
            .BodyContent()
            [
                BuildTransitionTestingSection()
            ]
        ]

        // === Emotion Testing ===
        + SScrollBox::Slot().Padding(4)
        [
            SNew(SExpandableArea)
            .AreaTitle(LOCTEXT("EmotionTesting", "Emotion Testing"))
            .InitiallyCollapsed(true)
            .BodyContent()
            [
                BuildEmotionTestSection()
            ]
        ]

        // === Visibility Testing ===
        + SScrollBox::Slot().Padding(4)
        [
            SNew(SExpandableArea)
            .AreaTitle(LOCTEXT("VisibilityTest", "Visibility Testing"))
            .InitiallyCollapsed(true)
            .BodyContent()
            [
                BuildVisibilityTestSection()
            ]
        ]
    ];
}

void SCharacter2DActionPanel::SyncStateFromActor()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        bBlinkingEnabled = Actor->bBlinkingActive;
        bTalkingEnabled  = Actor->bTalkingActive;
        bSpritesVisible  = Actor->bSpritesVisible;
        bSkeletalVisible = Actor->bSkeletalVisible;
    }
    else
    {
        // Default values if no actor
        bBlinkingEnabled = false;
        bTalkingEnabled  = false;
        bSpritesVisible  = true;
        bSkeletalVisible = true;
    }
}

// =========================================
// === Реализация Quick Actions (Appear / Disappear / Reset) ===
// =========================================

TSharedRef<SWidget> SCharacter2DActionPanel::BuildQuickActionsSection()
{
    return SNew(SVerticalBox)

    // Appear Instantly
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
        [
            SNew(SButton)
            .Text(LOCTEXT("AppearInstantly", "Appear Instantly"))
            .OnClicked(this, &SCharacter2DActionPanel::OnAppearInstantly)
            .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
        ]

        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
        [
            SNew(SButton)
            .Text(LOCTEXT("DisappearInstantly", "Disappear Instantly"))
            .OnClicked(this, &SCharacter2DActionPanel::OnDisappearInstantly)
            .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
        ]
    ]

    // Appear / Disappear (Default Transition из VisualNovelSettings)
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
        [
            SNew(SButton)
            .Text(LOCTEXT("AppearDefault", "Appear (Default)"))
            .OnClicked(this, &SCharacter2DActionPanel::OnAppearDefault)
            .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
        ]

        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
        [
            SNew(SButton)
            .Text(LOCTEXT("DisappearDefault", "Disappear (Default)"))
            .OnClicked(this, &SCharacter2DActionPanel::OnDisappearDefault)
            .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
        ]
    ]

    // Reset Character
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SButton)
        .Text(LOCTEXT("ResetCharacter", "Reset Character"))
        .OnClicked(this, &SCharacter2DActionPanel::OnResetCharacter)
        .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
    ];
}

FReply SCharacter2DActionPanel::OnAppearInstantly()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->AppearInstantly();
        SyncStateFromActor();
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnDisappearInstantly()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->DisappearInstantly();
        SyncStateFromActor();
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnAppearDefault()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->AppearWithDefaultTransition();
        SyncStateFromActor();
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnDisappearDefault()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->DisappearWithDefaultTransition();
        SyncStateFromActor();
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnResetCharacter()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        // Останавливаем эмоции, говорение и мигание через публичное API
        Actor->PlayEmotionWithDefaults(ECharacter2DEmotionEffect::None);
        Actor->EnableTalking(false);
        Actor->EnableBlinking(false);

        // Сбрасываем видимость и «появляем» мгновенно
        Actor->SetSpritesVisible(true);
        Actor->SetSkeletalVisible(true);
        Actor->AppearInstantly();

        // Обновляем флаги UI
        SyncStateFromActor();
    }
    return FReply::Handled();
}

// =========================================
// === Реализация Animation Testing (Blink / Talk) ===
// =========================================

TSharedRef<SWidget> SCharacter2DActionPanel::BuildAnimationTestingSection()
{
    return SNew(SVerticalBox)

    // Blink
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(SCheckBox)
            .OnCheckStateChanged(this, &SCharacter2DActionPanel::OnBlinkChanged)
            .IsChecked_Lambda([this]() {
                SyncStateFromActor();
                return bBlinkingEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
            })
        ]

        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("EnableBlinking", "Enable Blinking"))
        ]
    ]

    // Talk
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(SCheckBox)
            .OnCheckStateChanged(this, &SCharacter2DActionPanel::OnTalkChanged)
            .IsChecked_Lambda([this]() {
                SyncStateFromActor();
                return bTalkingEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
            })
        ]

        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("EnableTalking", "Enable Talking"))
        ]
    ];
}

void SCharacter2DActionPanel::OnBlinkChanged(ECheckBoxState NewState)
{
    bBlinkingEnabled = (NewState == ECheckBoxState::Checked);
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->EnableBlinking(bBlinkingEnabled);
    }
}

void SCharacter2DActionPanel::OnTalkChanged(ECheckBoxState NewState)
{
    bTalkingEnabled = (NewState == ECheckBoxState::Checked);
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->EnableTalking(bTalkingEnabled);
    }
}

// =========================================
// === Реализация Transition Testing ===
// =========================================

TSharedRef<SWidget> SCharacter2DActionPanel::BuildTransitionTestingSection()
{
    return SNew(SVerticalBox)

    // Dropdown: Transition Type
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("TransitionTypeLabel", "Transition Type:"))
        ]

        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0, 0, 0)
        [
            SNew(SComboBox<TSharedPtr<ECharacter2DTransitionType>>)
            .OptionsSource(&TransitionOptions)
            .OnSelectionChanged(this, &SCharacter2DActionPanel::OnTransitionChanged)
            .OnGenerateWidget_Lambda([](TSharedPtr<ECharacter2DTransitionType> Item)
            {
                return SNew(STextBlock).Text(TransitionTypeToText(*Item));
            })
            [
                SNew(STextBlock).Text(this, &SCharacter2DActionPanel::GetTransitionTypeText)
            ]
        ]
    ]

    // Duration Setting
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("DurationLabel", "Duration (s):"))
        ]

        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0, 0, 0)
        [
            SNew(SNumericEntryBox<float>)
            .Value_Lambda([this]() { return TransitionDuration; })
            .OnValueChanged(this, &SCharacter2DActionPanel::OnTransitionDurationChanged)
            .MinValue(0.1f)
            .MaxValue(10.0f)
        ]
    ]

    // Кнопка Test Transition
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SButton)
        .Text(LOCTEXT("TestTransition", "Test Transition"))
        .OnClicked(this, &SCharacter2DActionPanel::OnTestTransition)
        .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
    ];
}

void SCharacter2DActionPanel::OnTransitionChanged(TSharedPtr<ECharacter2DTransitionType> NewSelection, ESelectInfo::Type SelectInfo)
{
    if (NewSelection.IsValid())
    {
        CurrentTransition = NewSelection;
    }
}

void SCharacter2DActionPanel::OnTransitionDurationChanged(float NewValue)
{
    TransitionDuration = FMath::Clamp(NewValue, 0.1f, 10.0f);
}

FText SCharacter2DActionPanel::GetTransitionTypeText() const
{
    return TransitionTypeToText(*CurrentTransition);
}

FCharacter2DTransitionSettings SCharacter2DActionPanel::GetCurrentTransitionSettings() const
{
    FCharacter2DTransitionSettings Settings;
    Settings.Duration = TransitionDuration;

    if (CharacterAsset)
    {
        // Берём значения переходов из VisualNovelSettings
        Settings.AnimationCurve = CharacterAsset->VisualNovelSettings.DefaultAppearanceTransition.AnimationCurve;
        Settings.StartDelay     = CharacterAsset->VisualNovelSettings.DefaultAppearanceTransition.StartDelay;
        Settings.SlideDistance  = CharacterAsset->VisualNovelSettings.DefaultAppearanceTransition.SlideDistance;
    }
    else
    {
        // Default values if no asset
        Settings.AnimationCurve = nullptr;
        Settings.StartDelay = 0.0f;
        Settings.SlideDistance = 500.0f;
    }
    return Settings;
}

FReply SCharacter2DActionPanel::OnTestTransition()
{
    if (!IsPreviewActorValid())
    {
        return FReply::Handled();
    }

    ACharacter2DActor* Actor = PreviewActor.Get();
    FCharacter2DTransitionSettings Settings = GetCurrentTransitionSettings();

    switch (*CurrentTransition)
    {
        case ECharacter2DTransitionType::FadeIn:
            Actor->PlayTransition(ECharacter2DTransitionType::FadeIn, Settings);
            break;
        case ECharacter2DTransitionType::FadeOut:
            Actor->PlayTransition(ECharacter2DTransitionType::FadeOut, Settings);
            break;
        case ECharacter2DTransitionType::SlideInLeft:
            Actor->PlayTransition(ECharacter2DTransitionType::SlideInLeft, Settings);
            break;
        case ECharacter2DTransitionType::SlideInRight:
            Actor->PlayTransition(ECharacter2DTransitionType::SlideInRight, Settings);
            break;
        case ECharacter2DTransitionType::SlideOutLeft:
            Actor->PlayTransition(ECharacter2DTransitionType::SlideOutLeft, Settings);
            break;
        case ECharacter2DTransitionType::SlideOutRight:
            Actor->PlayTransition(ECharacter2DTransitionType::SlideOutRight, Settings);
            break;
        case ECharacter2DTransitionType::ScaleIn:
            Actor->PlayTransition(ECharacter2DTransitionType::ScaleIn, Settings);
            break;
        case ECharacter2DTransitionType::ScaleOut:
            Actor->PlayTransition(ECharacter2DTransitionType::ScaleOut, Settings);
            break;
        default:
            break;
    }

    return FReply::Handled();
}

// =========================================
// === Реализация Emotion Testing ===
// =========================================

TSharedRef<SWidget> SCharacter2DActionPanel::BuildEmotionTestSection()
{
    return SNew(SVerticalBox)

    // Dropdown: Emotion Type
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("EmotionTypeLabel", "Emotion Type:"))
        ]

        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0, 0, 0)
        [
            SNew(SComboBox<TSharedPtr<ECharacter2DEmotionEffect>>)
            .OptionsSource(&EmotionOptions)
            .OnSelectionChanged(this, &SCharacter2DActionPanel::OnEmotionChanged)
            .OnGenerateWidget_Lambda([](TSharedPtr<ECharacter2DEmotionEffect> Item)
            {
                return SNew(STextBlock).Text(EmotionTypeToText(*Item));
            })
            [
                SNew(STextBlock).Text(this, &SCharacter2DActionPanel::GetEmotionTypeText)
            ]
        ]
    ]

    // Duration Setting
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("EmotionDurationLabel", "Duration (s):"))
        ]

        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0, 0, 0)
        [
            SNew(SNumericEntryBox<float>)
            .Value_Lambda([this]() { return EmotionDuration; })
            .OnValueChanged(this, &SCharacter2DActionPanel::OnEmotionDurationChanged)
            .MinValue(0.1f)
            .MaxValue(10.0f)
        ]
    ]

    // Intensity Setting
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("EmotionIntensityLabel", "Intensity:"))
        ]

        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0, 0, 0)
        [
            SNew(SNumericEntryBox<float>)
            .Value_Lambda([this]() { return EmotionIntensity; })
            .OnValueChanged(this, &SCharacter2DActionPanel::OnEmotionIntensityChanged)
            .MinValue(0.0f)
            .MaxValue(1.0f)
        ]
    ]

    // Test Emotion + Stop Emotion
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
        [
            SNew(SButton)
            .Text(LOCTEXT("TestEmotion", "Test Emotion"))
            .OnClicked(this, &SCharacter2DActionPanel::OnTestEmotion)
            .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
        ]

        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
        [
            SNew(SButton)
            .Text(LOCTEXT("StopEmotion", "Stop Emotion"))
            .OnClicked(this, &SCharacter2DActionPanel::OnStopEmotion)
            .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
        ]
    ];
}

void SCharacter2DActionPanel::OnEmotionChanged(TSharedPtr<ECharacter2DEmotionEffect> NewSelection, ESelectInfo::Type SelectInfo)
{
    if (NewSelection.IsValid())
    {
        CurrentEmotion = NewSelection;
    }
}

void SCharacter2DActionPanel::OnEmotionDurationChanged(float NewValue)
{
    EmotionDuration = FMath::Clamp(NewValue, 0.1f, 10.0f);
}

void SCharacter2DActionPanel::OnEmotionIntensityChanged(float NewValue)
{
    EmotionIntensity = FMath::Clamp(NewValue, 0.0f, 1.0f);
}

FText SCharacter2DActionPanel::GetEmotionTypeText() const
{
    return EmotionTypeToText(*CurrentEmotion);
}

FCharacter2DEmotionSettings SCharacter2DActionPanel::GetCurrentEmotionSettings() const
{
    FCharacter2DEmotionSettings Settings;
    Settings.Duration  = EmotionDuration;
    Settings.Intensity = EmotionIntensity;

    if (CharacterAsset)
    {
        // Берём значения эмоций из VisualNovelSettings
        Settings.ShakeFrequency  = CharacterAsset->VisualNovelSettings.DefaultEmotionSettings.ShakeFrequency;
        Settings.TargetColor     = CharacterAsset->VisualNovelSettings.DefaultEmotionSettings.TargetColor;
        Settings.bLoop           = CharacterAsset->VisualNovelSettings.DefaultEmotionSettings.bLoop;
        Settings.AnimationCurve  = CharacterAsset->VisualNovelSettings.DefaultEmotionSettings.AnimationCurve;
    }
    else
    {
        // Default values if no asset
        Settings.ShakeFrequency = 10.0f;
        Settings.TargetColor = FLinearColor::Red;
        Settings.bLoop = false;
        Settings.AnimationCurve = nullptr;
    }
    return Settings;
}

FReply SCharacter2DActionPanel::OnTestEmotion()
{
    if (!IsPreviewActorValid())
    {
        return FReply::Handled();
    }

    ACharacter2DActor* Actor = PreviewActor.Get();
    FCharacter2DEmotionSettings Settings = GetCurrentEmotionSettings();

    switch (*CurrentEmotion)
    {
        case ECharacter2DEmotionEffect::Shake:
            Actor->PlayEmotion(ECharacter2DEmotionEffect::Shake, Settings);
            break;
        case ECharacter2DEmotionEffect::Pulse:
            Actor->PlayEmotion(ECharacter2DEmotionEffect::Pulse, Settings);
            break;
        case ECharacter2DEmotionEffect::ColorShift:
            Actor->PlayEmotion(ECharacter2DEmotionEffect::ColorShift, Settings);
            break;
        case ECharacter2DEmotionEffect::Bounce:
            Actor->PlayEmotion(ECharacter2DEmotionEffect::Bounce, Settings);
            break;
        case ECharacter2DEmotionEffect::Flash:
            Actor->PlayEmotion(ECharacter2DEmotionEffect::Flash, Settings);
            break;
        default:
            break;
    }

    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnStopEmotion()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        // Чтобы прервать текущую эмоцию, просто запустим «None»
        Actor->PlayEmotionWithDefaults(ECharacter2DEmotionEffect::None);
    }
    return FReply::Handled();
}

// =========================================
// === Реализация Visibility Testing ===
// =========================================

TSharedRef<SWidget> SCharacter2DActionPanel::BuildVisibilityTestSection()
{
    return SNew(SVerticalBox)

    // Sprites Visible
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(SCheckBox)
            .OnCheckStateChanged(this, &SCharacter2DActionPanel::OnToggleSprites)
            .IsChecked(this, &SCharacter2DActionPanel::GetSpritesVisibleState)
        ]

        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("SpritesVisible", "Sprites Visible"))
        ]
    ]

    // Skeletal Visible
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(SCheckBox)
            .OnCheckStateChanged(this, &SCharacter2DActionPanel::OnToggleSkeletal)
            .IsChecked(this, &SCharacter2DActionPanel::GetSkeletalVisibleState)
        ]

        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("SkeletalVisible", "Skeletal Visible"))
        ]
    ];
}

void SCharacter2DActionPanel::OnToggleSprites(ECheckBoxState NewState)
{
    bSpritesVisible = (NewState == ECheckBoxState::Checked);
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->SetSpritesVisible(bSpritesVisible);
    }
}

void SCharacter2DActionPanel::OnToggleSkeletal(ECheckBoxState NewState)
{
    bSkeletalVisible = (NewState == ECheckBoxState::Checked);
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->SetSkeletalVisible(bSkeletalVisible);
    }
}

ECheckBoxState SCharacter2DActionPanel::GetSpritesVisibleState() const
{
    // Always sync from actor for real-time updates
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        return Actor->bSpritesVisible ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return bSpritesVisible ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState SCharacter2DActionPanel::GetSkeletalVisibleState() const
{
    // Always sync from actor for real-time updates
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        return Actor->bSkeletalVisible ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return bSkeletalVisible ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

// =========================================
// === Конвертеры enum -> текст для Dropdown'ов ===
// =========================================

FText SCharacter2DActionPanel::TransitionTypeToText(ECharacter2DTransitionType Type)
{
    switch (Type)
    {
        case ECharacter2DTransitionType::None:            return LOCTEXT("Trans_None", "None");
        case ECharacter2DTransitionType::FadeIn:          return LOCTEXT("Trans_FadeIn", "Fade In");
        case ECharacter2DTransitionType::FadeOut:         return LOCTEXT("Trans_FadeOut", "Fade Out");
        case ECharacter2DTransitionType::SlideInLeft:     return LOCTEXT("Trans_SlideInLeft", "Slide In From Left");
        case ECharacter2DTransitionType::SlideInRight:    return LOCTEXT("Trans_SlideInRight", "Slide In From Right");
        case ECharacter2DTransitionType::SlideOutLeft:    return LOCTEXT("Trans_SlideOutLeft", "Slide Out To Left");
        case ECharacter2DTransitionType::SlideOutRight:   return LOCTEXT("Trans_SlideOutRight", "Slide Out To Right");
        case ECharacter2DTransitionType::ScaleIn:         return LOCTEXT("Trans_ScaleIn", "Scale In");
        case ECharacter2DTransitionType::ScaleOut:        return LOCTEXT("Trans_ScaleOut", "Scale Out");
        default:                                         return LOCTEXT("Trans_Unknown", "Unknown");
    }
}

FText SCharacter2DActionPanel::EmotionTypeToText(ECharacter2DEmotionEffect Type)
{
    switch (Type)
    {
        case ECharacter2DEmotionEffect::Shake:      return LOCTEXT("Emo_Shake", "Shake");
        case ECharacter2DEmotionEffect::Pulse:      return LOCTEXT("Emo_Pulse", "Pulse");
        case ECharacter2DEmotionEffect::ColorShift: return LOCTEXT("Emo_ColorShift", "Color Shift");
        case ECharacter2DEmotionEffect::Bounce:     return LOCTEXT("Emo_Bounce", "Bounce");
        case ECharacter2DEmotionEffect::Flash:      return LOCTEXT("Emo_Flash", "Flash");
        default:                                    return LOCTEXT("Emo_Unknown", "Unknown");
    }
}

#undef LOCTEXT_NAMESPACE