#include "Character2DAssetEditorToolkit/Slate/SCharacter2DActionPanel.h"
#include "Character2DActor.h"
#include "Character2DAsset.h"
#include "Character2DEnums.h"
#include "TimerManager.h"

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

    // Preview restoration is managed by timers instead of event bindings

    // Initialize state from actor if valid
    SyncStateFromActor();

    // --- Заполняем опции Dropdown для Interpolation ---
    InterpOptions.Empty();
    for (int32 i = 0; i <= static_cast<int32>(EInterpType::Cubic); ++i)
    {
        EInterpType Type = static_cast<EInterpType>(i);
        InterpOptions.Add(MakeShared<EInterpType>(Type));
    }
    CurrentInterp = InterpOptions[0];

    // --- Заполняем опции Dropdown для Emotion ---
    EmotionOptions.Empty();
    for (int32 i = 0; i < static_cast<int32>(ECharacter2DEmotionEffect::Flash) + 1; ++i)
    {
        ECharacter2DEmotionEffect Effect = static_cast<ECharacter2DEmotionEffect>(i);
        EmotionOptions.Add(MakeShared<ECharacter2DEmotionEffect>(Effect));
    }
    CurrentEmotion = EmotionOptions[0]; // по умолчанию «Shake»

    // Начальные значения для Duration / Intensity and target location
    TransitionDuration = 1.0f;
    EmotionDuration    = 2.0f;
    EmotionIntensity   = 0.5f;
    TargetX = TargetY = TargetZ = 0.0f;
    bTeleportInstant = false;

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

    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->GetWorldTimerManager().SetTimer(LocationSyncHandle, FTimerDelegate::CreateSP(this, &SCharacter2DActionPanel::UpdateTargetFromActor), 0.1f, true);
    }
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
        EnsurePreviewVisible(); // keep preview visible after action
        UpdateTargetFromActor();
        SyncStateFromActor();
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnDisappearInstantly()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->DisappearInstantly();
        // Reappear shortly so preview is not left hidden
        FTimerDelegate Del;
        Del.BindLambda([this]() { EnsurePreviewVisible(); UpdateTargetFromActor(); });
        Actor->GetWorldTimerManager().SetTimer(TransitionTestHandle, Del, 0.5f, false);
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnAppearDefault()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->AppearWithDefaultTransition();
        EnsurePreviewVisible();
        UpdateTargetFromActor();
        SyncStateFromActor();
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnDisappearDefault()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->DisappearWithDefaultTransition();
        // Restore visibility after the entire transition including any delay
        FCharacter2DTransitionSettings Settings = GetCurrentTransitionSettings();
        float TotalTime = Settings.StartDelay + Settings.Duration;
        FTimerDelegate Del;
        Del.BindLambda([this]() { EnsurePreviewVisible(); UpdateTargetFromActor(); });
        Actor->GetWorldTimerManager().ClearTimer(TransitionTestHandle);
        Actor->GetWorldTimerManager().SetTimer(TransitionTestHandle, Del, TotalTime, false);
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnResetCharacter()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        StopAllPreviewAnimations();
        Actor->AppearInstantly();
        EnsurePreviewVisible();
        UpdateTargetFromActor();

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

    // Blink Row
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

        + SHorizontalBox::Slot().AutoWidth().Padding(8,0)
        [
            SNew(SButton)
            .Text(LOCTEXT("TestBlink", "Test Blink"))
            .OnClicked(this, &SCharacter2DActionPanel::OnTestBlink)
            .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
        ]
    ]

    // Talk Row
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

        + SHorizontalBox::Slot().AutoWidth().Padding(8,0)
        [
            SNew(SButton)
            .Text(LOCTEXT("TestTalk", "Test Talk"))
            .OnClicked(this, &SCharacter2DActionPanel::OnTestTalk)
            .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
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

FReply SCharacter2DActionPanel::OnTestBlink()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->EnableBlinking(true);
        FTimerDelegate TimerDel;
        TimerDel.BindLambda([this]()
        {
            if (ACharacter2DActor* InnerActor = PreviewActor.Get())
            {
                InnerActor->EnableBlinking(false);
                EnsurePreviewVisible();
            }
        });
        Actor->GetWorldTimerManager().SetTimer(BlinkTestHandle, TimerDel, 1.0f, false);
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnTestTalk()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->EnableTalking(true);
        FTimerDelegate TimerDel;
        TimerDel.BindLambda([this]()
        {
            if (ACharacter2DActor* InnerActor = PreviewActor.Get())
            {
                InnerActor->EnableTalking(false);
                EnsurePreviewVisible();
            }
        });
        Actor->GetWorldTimerManager().SetTimer(TalkTestHandle, TimerDel, 1.0f, false);
    }
    return FReply::Handled();
}

// =========================================
// === Реализация Transition Testing ===
// =========================================

TSharedRef<SWidget> SCharacter2DActionPanel::BuildTransitionTestingSection()
{
    return SNew(SVerticalBox)

    // Dropdown: Interpolation Type
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("InterpTypeLabel", "Interpolation Type:"))
        ]

        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0, 0, 0)
        [
            SNew(SComboBox<TSharedPtr<EInterpType>>)
            .OptionsSource(&InterpOptions)
            .OnSelectionChanged(this, &SCharacter2DActionPanel::OnInterpChanged)
            .OnGenerateWidget_Lambda([](TSharedPtr<EInterpType> Item)
            {
                return SNew(STextBlock).Text(InterpTypeToText(*Item));
            })
            [
                SNew(STextBlock).Text(this, &SCharacter2DActionPanel::GetInterpTypeText)
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

    // Target X
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("TargetXLabel", "Target X:"))
        ]

        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0, 0, 0)
        [
            SNew(SNumericEntryBox<float>)
            .Value_Lambda([this]() { return TargetX; })
            .OnValueChanged(this, &SCharacter2DActionPanel::OnTargetXChanged)
        ]
    ]

    // Target Y
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("TargetYLabel", "Target Y:"))
        ]

        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0, 0, 0)
        [
            SNew(SNumericEntryBox<float>)
            .Value_Lambda([this]() { return TargetY; })
            .OnValueChanged(this, &SCharacter2DActionPanel::OnTargetYChanged)
        ]
    ]

    // Target Z
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("TargetZLabel", "Target Z:"))
        ]

        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0, 0, 0)
        [
            SNew(SNumericEntryBox<float>)
            .Value_Lambda([this]() { return TargetZ; })
            .OnValueChanged(this, &SCharacter2DActionPanel::OnTargetZChanged)
        ]
    ]

    // Teleport instantly checkbox
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(SCheckBox)
            .OnCheckStateChanged(this, &SCharacter2DActionPanel::OnTeleportInstantChanged)
            .IsChecked_Lambda([this]() { return bTeleportInstant ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
        ]

        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8,0,0,0)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("TeleportInstant", "Teleport Instantly"))
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

void SCharacter2DActionPanel::OnInterpChanged(TSharedPtr<EInterpType> NewSelection, ESelectInfo::Type SelectInfo)
{
    if (NewSelection.IsValid())
    {
        CurrentInterp = NewSelection;
    }
}

void SCharacter2DActionPanel::OnTransitionDurationChanged(float NewValue)
{
    TransitionDuration = FMath::Clamp(NewValue, 0.1f, 10.0f);
}

void SCharacter2DActionPanel::OnTargetXChanged(float NewValue)
{
    TargetX = NewValue;
}

void SCharacter2DActionPanel::OnTargetYChanged(float NewValue)
{
    TargetY = NewValue;
}

void SCharacter2DActionPanel::OnTargetZChanged(float NewValue)
{
    TargetZ = NewValue;
}

void SCharacter2DActionPanel::OnTeleportInstantChanged(ECheckBoxState NewState)
{
    bTeleportInstant = (NewState == ECheckBoxState::Checked);
}

FText SCharacter2DActionPanel::GetInterpTypeText() const
{
    return InterpTypeToText(*CurrentInterp);
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

    // Move actor to specified location using selected interpolation
    FVector TargetLocation(TargetX, TargetY, TargetZ);
    MovePreviewToLocation(TargetLocation, bTeleportInstant ? 0.0f : TransitionDuration, *CurrentInterp);
    UpdateTargetFromActor();
    // Ensure visibility in case any timeline hid the actor previously
    EnsurePreviewVisible();

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

    // Stop the emotion after the requested duration and make sure the actor stays visible
    Actor->GetWorldTimerManager().ClearTimer(TransitionTestHandle);
    FTimerDelegate Del;
    Del.BindLambda([this]()
    {
        if (ACharacter2DActor* Inner = PreviewActor.Get())
        {
            Inner->StopCurrentEmotion();
            EnsurePreviewVisible();
        }
    });
    Actor->GetWorldTimerManager().SetTimer(TransitionTestHandle, Del, Settings.Duration, false);

    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnStopEmotion()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        // Чтобы прервать текущую эмоцию, просто запустим «None»
        Actor->PlayEmotionWithDefaults(ECharacter2DEmotionEffect::None);
        EnsurePreviewVisible();
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

void SCharacter2DActionPanel::StopAllPreviewAnimations()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->EnableBlinking(false);
        Actor->EnableTalking(false);
        Actor->StopCurrentEmotion();
        Actor->GetWorldTimerManager().ClearTimer(BlinkTestHandle);
        Actor->GetWorldTimerManager().ClearTimer(TalkTestHandle);
        Actor->GetWorldTimerManager().ClearTimer(TransitionTestHandle);
        Actor->GetWorldTimerManager().ClearTimer(VisibilityHandle);
        Actor->GetWorldTimerManager().ClearTimer(LocationSyncHandle);
    }
}

void SCharacter2DActionPanel::MovePreviewToLocation(FVector NewLocation, float Duration, EInterpType InterpType)
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->GetWorldTimerManager().ClearTimer(TransitionTestHandle);
        if (Duration <= 0.0f)
        {
            Actor->SetActorLocation(NewLocation);
            EnsurePreviewVisible();
            return;
        }

        const FVector StartLocation = Actor->GetActorLocation();
        const float Step = 0.02f;
        TSharedRef<float> Elapsed = MakeShared<float>(0.0f);

        FTimerDelegate Tick;
        Tick.BindLambda([this, Actor, StartLocation, NewLocation, Duration, Elapsed, Step, InterpType]() mutable
        {
            *Elapsed += Step;
            float Alpha = FMath::Clamp(*Elapsed / Duration, 0.0f, 1.0f);
            FVector Pos;
            switch (InterpType)
            {
                case EInterpType::EaseIn:
                    Pos = FMath::InterpEaseIn(StartLocation, NewLocation, Alpha, 2.0f); break;
                case EInterpType::EaseOut:
                    Pos = FMath::InterpEaseOut(StartLocation, NewLocation, Alpha, 2.0f); break;
                case EInterpType::EaseInOut:
                    Pos = FMath::InterpEaseInOut(StartLocation, NewLocation, Alpha, 2.0f); break;
                case EInterpType::Cubic:
                    Pos = FMath::CubicInterp(StartLocation, FVector::ZeroVector, NewLocation, FVector::ZeroVector, Alpha); break;
                case EInterpType::Linear:
                default:
                    Pos = FMath::Lerp(StartLocation, NewLocation, Alpha); break;
            }
            Actor->SetActorLocation(Pos);
            if (Alpha >= 1.0f - KINDA_SMALL_NUMBER)
            {
                Actor->GetWorldTimerManager().ClearTimer(TransitionTestHandle);
                EnsurePreviewVisible();
            }
        });

        Actor->GetWorldTimerManager().SetTimer(TransitionTestHandle, Tick, Step, true);
    }
}

void SCharacter2DActionPanel::UpdateTargetFromActor()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        FVector Loc = Actor->GetActorLocation();
        TargetX = Loc.X;
        TargetY = Loc.Y;
        TargetZ = Loc.Z;
    }
}

void SCharacter2DActionPanel::EnsurePreviewVisible()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->SetActorHiddenInGame(false);
        Actor->SetBothVisible(true, true);
    }
}

// =========================================
// === Конвертеры enum -> текст для Dropdown'ов ===
// =========================================

FText SCharacter2DActionPanel::InterpTypeToText(EInterpType Type)
{
    switch (Type)
    {
        case EInterpType::Linear:     return LOCTEXT("Interp_Linear", "Linear");
        case EInterpType::EaseIn:     return LOCTEXT("Interp_EaseIn", "Ease In");
        case EInterpType::EaseOut:    return LOCTEXT("Interp_EaseOut", "Ease Out");
        case EInterpType::EaseInOut:  return LOCTEXT("Interp_EaseInOut", "Ease In Out");
        case EInterpType::Cubic:      return LOCTEXT("Interp_Cubic", "Cubic");
        default:                      return LOCTEXT("Interp_Unknown", "Unknown");
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

