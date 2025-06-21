#include "Character2DAssetEditorToolkit/Slate/SCharacter2DActionPanel.h"

#include "Character2DActor.h"
#include "Character2DAsset.h"
#include "TimerManager.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SVectorInputBox.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "SCharacter2DActionPanel"

void SCharacter2DActionPanel::Construct(const FArguments& InArgs)
{
    CharacterAsset = InArgs._CharacterAsset;
    PreviewActor = InArgs._PreviewActor;

    SyncStateFromActor();

    // Инициализация опций эмоций
    EmotionOptions.Empty();
    for (int32 i = 0; i <= static_cast<int32>(ECharacter2DEmotionEffect::Flash); ++i)
    {
        ECharacter2DEmotionEffect Effect = static_cast<ECharacter2DEmotionEffect>(i);
        EmotionOptions.Add(MakeShared<ECharacter2DEmotionEffect>(Effect));
    }
    CurrentEmotion = EmotionOptions[0];

    // Инициализация значений по умолчанию
    MovementDuration = 1.0f;
    EmotionDuration = 2.0f;
    EmotionIntensity = 0.5f;
    TargetLocation = FVector::ZeroVector;
    bTeleportInstant = false;
    
    UpdateLocationFromActor();

    // Построение UI
    ChildSlot
    [
        SNew(SScrollBox)
        + SScrollBox::Slot().Padding(4) [ SNew(SExpandableArea).AreaTitle(LOCTEXT("QuickActions", "Quick Actions")).InitiallyCollapsed(false).BodyContent()[BuildQuickActionsSection()] ]
        + SScrollBox::Slot().Padding(4) [ SNew(SExpandableArea).AreaTitle(LOCTEXT("SpriteManipulation", "Sprite Manipulation")).InitiallyCollapsed(false).BodyContent()[BuildSpriteManipulationSection()] ]
        + SScrollBox::Slot().Padding(4) [ SNew(SExpandableArea).AreaTitle(LOCTEXT("EffectLayers", "Effect Layers")).InitiallyCollapsed(false).BodyContent()[BuildEffectLayersSection()] ]
        + SScrollBox::Slot().Padding(4) [ SNew(SExpandableArea).AreaTitle(LOCTEXT("AnimationTesting", "Animation Testing")).InitiallyCollapsed(true).BodyContent()[BuildAnimationTestingSection()] ]
        + SScrollBox::Slot().Padding(4) [ SNew(SExpandableArea).AreaTitle(LOCTEXT("MovementTesting", "Movement Testing")).InitiallyCollapsed(true).BodyContent()[BuildMovementTestingSection()] ]
        + SScrollBox::Slot().Padding(4) [ SNew(SExpandableArea).AreaTitle(LOCTEXT("EmotionTesting", "Emotion Testing")).InitiallyCollapsed(true).BodyContent()[BuildEmotionTestSection()] ]
        + SScrollBox::Slot().Padding(4) [ SNew(SExpandableArea).AreaTitle(LOCTEXT("VisibilityTest", "Visibility Testing")).InitiallyCollapsed(true).BodyContent()[BuildVisibilityTestSection()] ]
    ];

    // Запуск таймера для синхронизации позиции
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->GetWorldTimerManager().SetTimer(LocationSyncHandle, FTimerDelegate::CreateSP(this, &SCharacter2DActionPanel::UpdateLocationFromActor), 0.1f, true);
    }
}

void SCharacter2DActionPanel::SyncStateFromActor()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        bBlinkingEnabled = Actor->IsBlinkingEnabled();
        bTalkingEnabled = Actor->IsTalkingEnabled();
        bSpritesVisible = Actor->bSpritesVisible;
        bSkeletalVisible = Actor->bSkeletalVisible;
    }
    else
    {
        bBlinkingEnabled = false;
        bTalkingEnabled = false;
        bSpritesVisible = true;
        bSkeletalVisible = true;
    }
}

TSharedRef<SWidget> SCharacter2DActionPanel::BuildQuickActionsSection()
{
    return SNew(SVerticalBox)
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(STextBlock)
        .Text(LOCTEXT("QuickActionsDesc", "Quick visibility and reset controls"))
        .Font(FCoreStyle::GetDefaultFontStyle("Italic", 8))
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
        [
            SNew(SButton)
            .Text(LOCTEXT("ShowCharacter", "Show Character"))
            .OnClicked(this, &SCharacter2DActionPanel::OnShowCharacter)
            .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
        ]
        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
        [
            SNew(SButton)
            .Text(LOCTEXT("HideCharacter", "Hide Character"))
            .OnClicked(this, &SCharacter2DActionPanel::OnHideCharacter)
            .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
        ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
        [
            SNew(SButton)
            .Text(LOCTEXT("FadeIn", "Fade In"))
            .OnClicked(this, &SCharacter2DActionPanel::OnFadeIn)
            .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
        ]
        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
        [
            SNew(SButton)
            .Text(LOCTEXT("FadeOut", "Fade Out"))
            .OnClicked(this, &SCharacter2DActionPanel::OnFadeOut)
            .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
        ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SButton)
        .Text(LOCTEXT("ResetCharacter", "Reset Character"))
        .ToolTipText(LOCTEXT("ResetCharacterTooltip", "Reset character to original position and state"))
        .OnClicked(this, &SCharacter2DActionPanel::OnResetCharacter)
        .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
    ];
}

TSharedRef<SWidget> SCharacter2DActionPanel::BuildSpriteManipulationSection()
{
    return SNew(SVerticalBox)
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(STextBlock)
        .Text(LOCTEXT("SpriteManipDesc", "Change individual sprite elements"))
        .Font(FCoreStyle::GetDefaultFontStyle("Italic", 8))
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SButton)
        .Text(LOCTEXT("TestChangeEyebrows", "Test Change Eyebrows"))
        .ToolTipText(LOCTEXT("TestChangeEyebrowsTooltip", "Temporarily changes eyebrows to test sprite manipulation"))
        .OnClicked(this, &SCharacter2DActionPanel::OnTestChangeEyebrows)
        .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SButton)
        .Text(LOCTEXT("TestChangeMouth", "Test Change Mouth"))
        .ToolTipText(LOCTEXT("TestChangeMouthTooltip", "Temporarily changes mouth to test sprite manipulation"))
        .OnClicked(this, &SCharacter2DActionPanel::OnTestChangeMouth)
        .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SButton)
        .Text(LOCTEXT("RestoreFromAsset", "Restore From Asset"))
        .ToolTipText(LOCTEXT("RestoreFromAssetTooltip", "Restore all sprites from the character asset"))
        .OnClicked(this, &SCharacter2DActionPanel::OnRestoreFromAsset)
        .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
    ];
}

TSharedRef<SWidget> SCharacter2DActionPanel::BuildEffectLayersSection()
{
    return SNew(SVerticalBox)
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(STextBlock)
        .Text(LOCTEXT("EffectLayersDesc", "Control effect layers on the face"))
        .Font(FCoreStyle::GetDefaultFontStyle("Italic", 8))
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ SNew(SCheckBox).OnCheckStateChanged(this, &SCharacter2DActionPanel::OnEffect1Changed).IsChecked_Lambda([this]() { return bEffect1Enabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }) ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0) [ SNew(STextBlock).Text(LOCTEXT("Effect1", "Effect Layer 1 (e.g. Blush)")) ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ SNew(SCheckBox).OnCheckStateChanged(this, &SCharacter2DActionPanel::OnEffect2Changed).IsChecked_Lambda([this]() { return bEffect2Enabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }) ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0) [ SNew(STextBlock).Text(LOCTEXT("Effect2", "Effect Layer 2 (e.g. Tears)")) ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ SNew(SCheckBox).OnCheckStateChanged(this, &SCharacter2DActionPanel::OnEffect3Changed).IsChecked_Lambda([this]() { return bEffect3Enabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }) ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0) [ SNew(STextBlock).Text(LOCTEXT("Effect3", "Effect Layer 3 (e.g. Anger)")) ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SButton)
        .Text(LOCTEXT("ClearAllEffects", "Clear All Effects"))
        .OnClicked(this, &SCharacter2DActionPanel::OnClearAllEffects)
        .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
    ];
}

TSharedRef<SWidget> SCharacter2DActionPanel::BuildAnimationTestingSection()
{
    return SNew(SVerticalBox)
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ SNew(SCheckBox).OnCheckStateChanged(this, &SCharacter2DActionPanel::OnBlinkChanged).IsChecked_Lambda([this]() { SyncStateFromActor(); return bBlinkingEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }) ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0) [ SNew(STextBlock).Text(LOCTEXT("EnableBlinking", "Enable Blinking")) ]
        + SHorizontalBox::Slot().AutoWidth().Padding(8,0) [ SNew(SButton).Text(LOCTEXT("TestBlink", "Test Blink")).OnClicked(this, &SCharacter2DActionPanel::OnTestBlink).IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid) ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ SNew(SCheckBox).OnCheckStateChanged(this, &SCharacter2DActionPanel::OnTalkChanged).IsChecked_Lambda([this]() { SyncStateFromActor(); return bTalkingEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }) ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0) [ SNew(STextBlock).Text(LOCTEXT("EnableTalking", "Enable Talking")) ]
        + SHorizontalBox::Slot().AutoWidth().Padding(8,0) [ SNew(SButton).Text(LOCTEXT("TestTalk", "Test Talk")).OnClicked(this, &SCharacter2DActionPanel::OnTestTalk).IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid) ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SButton)
        .Text(LOCTEXT("BlinkOnce", "Blink Once"))
        .OnClicked(this, &SCharacter2DActionPanel::OnBlinkOnce)
        .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
    ];
}

TSharedRef<SWidget> SCharacter2DActionPanel::BuildMovementTestingSection()
{
    return SNew(SVerticalBox)
    + SVerticalBox::Slot().AutoHeight().Padding(4)
    [
        SNew(SBorder).BorderBackgroundColor(FLinearColor(0.2f, 0.2f, 0.2f, 0.5f)).Padding(4)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight() [ SNew(STextBlock).Text(LOCTEXT("CurrentPositionLabel", "Current Position:")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10)) ]
            + SVerticalBox::Slot().AutoHeight().Padding(2, 2, 2, 0) [ SNew(STextBlock).Text(this, &SCharacter2DActionPanel::GetCurrentPositionText).Font(FCoreStyle::GetDefaultFontStyle("Regular", 9)) ]
        ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(4)
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4) [ SNew(STextBlock).Text(LOCTEXT("TargetPositionLabel", "Target Position:")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10)) ]
        + SVerticalBox::Slot().AutoHeight() [ SNew(SVectorInputBox).bColorAxisLabels(true).AllowSpin(true).X(this, &SCharacter2DActionPanel::GetTargetX).Y(this, &SCharacter2DActionPanel::GetTargetY).Z(this, &SCharacter2DActionPanel::GetTargetZ).OnXCommitted(this, &SCharacter2DActionPanel::OnTargetXChanged).OnYCommitted(this, &SCharacter2DActionPanel::OnTargetYChanged).OnZCommitted(this, &SCharacter2DActionPanel::OnTargetZChanged) ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(4)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(0.5f).VAlign(VAlign_Center) [ SNew(STextBlock).Text(LOCTEXT("DurationLabel", "Duration (s):")) ]
        + SHorizontalBox::Slot().FillWidth(0.5f).Padding(4, 0, 0, 0) [ SNew(SNumericEntryBox<float>).Value_Lambda([this]() { return MovementDuration; }).OnValueChanged(this, &SCharacter2DActionPanel::OnMovementDurationChanged).MinValue(0.0f).MaxValue(10.0f) ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(4)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ SNew(SCheckBox).OnCheckStateChanged(this, &SCharacter2DActionPanel::OnTeleportInstantChanged).IsChecked_Lambda([this]() { return bTeleportInstant ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }) ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8,0,0,0) [ SNew(STextBlock).Text(LOCTEXT("TeleportInstant", "Teleport Instantly")) ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(4)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2) [ SNew(SButton).Text(LOCTEXT("MoveToTarget", "Move to Target")).OnClicked(this, &SCharacter2DActionPanel::OnMoveToTarget).IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid) ]
        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2) [ SNew(SButton).Text(LOCTEXT("SetFromCurrent", "Set Target from Current")).ToolTipText(LOCTEXT("SetFromCurrentTooltip", "Copy current position to target fields")).OnClicked(this, &SCharacter2DActionPanel::OnSetTargetFromCurrent).IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid) ]
    ];
}

TSharedRef<SWidget> SCharacter2DActionPanel::BuildEmotionTestSection()
{
    return SNew(SVerticalBox)
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ SNew(STextBlock).Text(LOCTEXT("EmotionTypeLabel", "Emotion Type:")) ]
        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0, 0, 0)
        [
            SNew(SComboBox<TSharedPtr<ECharacter2DEmotionEffect>>)
            .OptionsSource(&EmotionOptions)
            .OnSelectionChanged(this, &SCharacter2DActionPanel::OnEmotionChanged)
            .OnGenerateWidget_Lambda([](TSharedPtr<ECharacter2DEmotionEffect> Item) { return SNew(STextBlock).Text(EmotionTypeToText(*Item)); })
            [ SNew(STextBlock).Text(this, &SCharacter2DActionPanel::GetEmotionTypeText) ]
        ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ SNew(STextBlock).Text(LOCTEXT("EmotionDurationLabel", "Duration (s):")) ]
        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0, 0, 0) [ SNew(SNumericEntryBox<float>).Value_Lambda([this]() { return EmotionDuration; }).OnValueChanged(this, &SCharacter2DActionPanel::OnEmotionDurationChanged).MinValue(0.1f).MaxValue(10.0f) ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ SNew(STextBlock).Text(LOCTEXT("EmotionIntensityLabel", "Intensity:")) ]
        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0, 0, 0) [ SNew(SNumericEntryBox<float>).Value_Lambda([this]() { return EmotionIntensity; }).OnValueChanged(this, &SCharacter2DActionPanel::OnEmotionIntensityChanged).MinValue(0.0f).MaxValue(1.0f) ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2) [ SNew(SButton).Text(LOCTEXT("TestEmotion", "Test Emotion")).OnClicked(this, &SCharacter2DActionPanel::OnTestEmotion).IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid) ]
        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2) [ SNew(SButton).Text(LOCTEXT("StopEmotion", "Stop Emotion")).OnClicked(this, &SCharacter2DActionPanel::OnStopEmotion).IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid) ]
    ];
}

TSharedRef<SWidget> SCharacter2DActionPanel::BuildVisibilityTestSection()
{
    return SNew(SVerticalBox)
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ SNew(SCheckBox).OnCheckStateChanged(this, &SCharacter2DActionPanel::OnToggleSprites).IsChecked(this, &SCharacter2DActionPanel::GetSpritesVisibleState) ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0) [ SNew(STextBlock).Text(LOCTEXT("SpritesVisible", "Sprites Visible")) ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) [ SNew(SCheckBox).OnCheckStateChanged(this, &SCharacter2DActionPanel::OnToggleSkeletal).IsChecked(this, &SCharacter2DActionPanel::GetSkeletalVisibleState) ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0) [ SNew(STextBlock).Text(LOCTEXT("SkeletalVisible", "Skeletal Visible")) ]
    ];
}

// =========================================
// === Обработчики событий (Event Handlers) ===
// =========================================

FReply SCharacter2DActionPanel::OnShowCharacter()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->SetActorHiddenInGame(false);
        Actor->SetBothVisible(true, true);
        UpdateLocationFromActor();
        SyncStateFromActor();
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnHideCharacter()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->SetActorHiddenInGame(true);
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnFadeIn()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->PlayFadeIn();
        FTimerDelegate Del;
        Del.BindLambda([this]() { if (ACharacter2DActor* InnerActor = PreviewActor.Get()){ InnerActor->SetActorHiddenInGame(false); InnerActor->SetBothVisible(true, true); } });
        Actor->GetWorldTimerManager().SetTimer(TransitionTestHandle, Del, 1.1f, false);
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnFadeOut()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->PlayFadeOut();
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnResetCharacter()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        StopAllPreviewAnimations();
        Actor->SetActorLocation(FVector::ZeroVector);
        Actor->SetActorRotation(FRotator::ZeroRotator);
        Actor->SetActorScale3D(FVector(1.0f));
        Actor->SetActorHiddenInGame(false);
        Actor->SetBothVisible(true, true);
        Actor->ClearAllEffects();
        bEffect1Enabled = false;
        bEffect2Enabled = false;
        bEffect3Enabled = false;
        UpdateLocationFromActor();
        SyncStateFromActor();
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnTestChangeEyebrows()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->SetEyebrowSprite(nullptr);
        FTimerDelegate Del;
        Del.BindLambda([this]() { if (ACharacter2DActor* InnerActor = PreviewActor.Get()) { InnerActor->RefreshFromAsset(); } });
        Actor->GetWorldTimerManager().SetTimer(TransitionTestHandle, Del, 2.0f, false);
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnTestChangeMouth()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->SetMouthSprite(nullptr);
        FTimerDelegate Del;
        Del.BindLambda([this]() { if (ACharacter2DActor* InnerActor = PreviewActor.Get()) { InnerActor->RefreshFromAsset(); } });
        Actor->GetWorldTimerManager().SetTimer(TransitionTestHandle, Del, 2.0f, false);
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnRestoreFromAsset()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->UpdateFromAssetPreserveState();
    }
    return FReply::Handled();
}

void SCharacter2DActionPanel::OnEffect1Changed(ECheckBoxState NewState)
{
    bEffect1Enabled = (NewState == ECheckBoxState::Checked);
    if (ACharacter2DActor* Actor = PreviewActor.Get()) { Actor->ShowEffectLayer(1, bEffect1Enabled); }
}

void SCharacter2DActionPanel::OnEffect2Changed(ECheckBoxState NewState)
{
    bEffect2Enabled = (NewState == ECheckBoxState::Checked);
    if (ACharacter2DActor* Actor = PreviewActor.Get()) { Actor->ShowEffectLayer(2, bEffect2Enabled); }
}

void SCharacter2DActionPanel::OnEffect3Changed(ECheckBoxState NewState)
{
    bEffect3Enabled = (NewState == ECheckBoxState::Checked);
    if (ACharacter2DActor* Actor = PreviewActor.Get()) { Actor->ShowEffectLayer(3, bEffect3Enabled); }
}

FReply SCharacter2DActionPanel::OnClearAllEffects()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->ClearAllEffects();
        bEffect1Enabled = false;
        bEffect2Enabled = false;
        bEffect3Enabled = false;
    }
    return FReply::Handled();
}

void SCharacter2DActionPanel::OnBlinkChanged(ECheckBoxState NewState)
{
    bBlinkingEnabled = (NewState == ECheckBoxState::Checked);
    if (ACharacter2DActor* Actor = PreviewActor.Get()) { Actor->EnableBlinking(bBlinkingEnabled); EnsurePreviewVisible(); }
}

void SCharacter2DActionPanel::OnTalkChanged(ECheckBoxState NewState)
{
    bTalkingEnabled = (NewState == ECheckBoxState::Checked);
    if (ACharacter2DActor* Actor = PreviewActor.Get()) { Actor->EnableTalking(bTalkingEnabled); EnsurePreviewVisible(); }
}

FReply SCharacter2DActionPanel::OnTestBlink()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->EnableBlinking(true);
        EnsurePreviewVisible();
        FTimerDelegate TimerDel;
        TimerDel.BindLambda([this](){ if (ACharacter2DActor* InnerActor = PreviewActor.Get()){ InnerActor->EnableBlinking(false); EnsurePreviewVisible(); }});
        Actor->GetWorldTimerManager().SetTimer(BlinkTestHandle, TimerDel, 2.0f, false);
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnTestTalk()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->EnableTalking(true);
        EnsurePreviewVisible();
        FTimerDelegate TimerDel;
        TimerDel.BindLambda([this](){ if (ACharacter2DActor* InnerActor = PreviewActor.Get()){ InnerActor->EnableTalking(false); EnsurePreviewVisible(); }});
        Actor->GetWorldTimerManager().SetTimer(TalkTestHandle, TimerDel, 2.0f, false);
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnBlinkOnce()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->BlinkOnce();
        EnsurePreviewVisible();
    }
    return FReply::Handled();
}

FText SCharacter2DActionPanel::GetCurrentPositionText() const
{
    if (PreviewActor.IsValid())
    {
        FVector Pos = CurrentLocation;
        return FText::Format(LOCTEXT("PositionFormat", "X: {0}  Y: {1}  Z: {2}"), FText::AsNumber(FMath::RoundToInt(Pos.X)), FText::AsNumber(FMath::RoundToInt(Pos.Y)), FText::AsNumber(FMath::RoundToInt(Pos.Z)));
    }
    return LOCTEXT("NoActor", "No Actor");
}

TOptional<float> SCharacter2DActionPanel::GetTargetX() const { return TargetLocation.X; }
TOptional<float> SCharacter2DActionPanel::GetTargetY() const { return TargetLocation.Y; }
TOptional<float> SCharacter2DActionPanel::GetTargetZ() const { return TargetLocation.Z; }
void SCharacter2DActionPanel::OnTargetXChanged(float NewValue, ETextCommit::Type) { TargetLocation.X = NewValue; }
void SCharacter2DActionPanel::OnTargetYChanged(float NewValue, ETextCommit::Type) { TargetLocation.Y = NewValue; }
void SCharacter2DActionPanel::OnTargetZChanged(float NewValue, ETextCommit::Type) { TargetLocation.Z = NewValue; }
void SCharacter2DActionPanel::OnMovementDurationChanged(float NewValue) { MovementDuration = FMath::Clamp(NewValue, 0.0f, 10.0f); }
void SCharacter2DActionPanel::OnTeleportInstantChanged(ECheckBoxState NewState) { bTeleportInstant = (NewState == ECheckBoxState::Checked); }

FReply SCharacter2DActionPanel::OnMoveToTarget()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        if (bTeleportInstant || MovementDuration <= 0.0f) { Actor->SetActorLocation(TargetLocation); }
        else { Actor->MoveToLocation(TargetLocation, MovementDuration); }
        EnsurePreviewVisible();
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnSetTargetFromCurrent()
{
    UpdateLocationFromActor();
    TargetLocation = CurrentLocation;
    return FReply::Handled();
}

void SCharacter2DActionPanel::UpdateLocationFromActor()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get()) { CurrentLocation = Actor->GetActorLocation(); }
}

void SCharacter2DActionPanel::OnEmotionChanged(TSharedPtr<ECharacter2DEmotionEffect> NewSelection, ESelectInfo::Type SelectInfo)
{
    if (NewSelection.IsValid()) { CurrentEmotion = NewSelection; }
}

void SCharacter2DActionPanel::OnEmotionDurationChanged(float NewValue) { EmotionDuration = FMath::Clamp(NewValue, 0.1f, 10.0f); }
void SCharacter2DActionPanel::OnEmotionIntensityChanged(float NewValue) { EmotionIntensity = FMath::Clamp(NewValue, 0.0f, 1.0f); }
FText SCharacter2DActionPanel::GetEmotionTypeText() const { return EmotionTypeToText(*CurrentEmotion); }

FCharacter2DEmotionSettings SCharacter2DActionPanel::GetCurrentEmotionSettings() const
{
    FCharacter2DEmotionSettings Settings;
    Settings.Duration = EmotionDuration;
    Settings.Intensity = EmotionIntensity;
    if (UCharacter2DAsset* Asset = CharacterAsset.Get())
    {
        Settings.ShakeFrequency = Asset->VisualNovelSettings.DefaultEmotionSettings.ShakeFrequency;
        Settings.TargetColor = Asset->VisualNovelSettings.DefaultEmotionSettings.TargetColor;
        Settings.bLoop = Asset->VisualNovelSettings.DefaultEmotionSettings.bLoop;
        Settings.AnimationCurve = Asset->VisualNovelSettings.DefaultEmotionSettings.AnimationCurve;
    }
    return Settings;
}

FReply SCharacter2DActionPanel::OnTestEmotion()
{
    if (!IsPreviewActorValid()) return FReply::Handled();
    ACharacter2DActor* Actor = PreviewActor.Get();
    FCharacter2DEmotionSettings Settings = GetCurrentEmotionSettings();
    Actor->PlayEmotion(*CurrentEmotion, Settings);
    EnsurePreviewVisible();
    Actor->GetWorldTimerManager().ClearTimer(TransitionTestHandle);
    FTimerDelegate Del;
    Del.BindLambda([this]() { if (ACharacter2DActor* Inner = PreviewActor.Get()) { Inner->StopCurrentEmotion(); EnsurePreviewVisible(); } });
    Actor->GetWorldTimerManager().SetTimer(TransitionTestHandle, Del, Settings.Duration, false);
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnStopEmotion()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get()) { Actor->StopCurrentEmotion(); EnsurePreviewVisible(); }
    return FReply::Handled();
}

void SCharacter2DActionPanel::OnToggleSprites(ECheckBoxState NewState)
{
    bSpritesVisible = (NewState == ECheckBoxState::Checked);
    if (ACharacter2DActor* Actor = PreviewActor.Get()) { Actor->SetSpritesVisible(bSpritesVisible); EnsurePreviewVisible(); }
}

void SCharacter2DActionPanel::OnToggleSkeletal(ECheckBoxState NewState)
{
    bSkeletalVisible = (NewState == ECheckBoxState::Checked);
    if (ACharacter2DActor* Actor = PreviewActor.Get()) { Actor->SetSkeletalVisible(bSkeletalVisible); EnsurePreviewVisible(); }
}

ECheckBoxState SCharacter2DActionPanel::GetSpritesVisibleState() const
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        // ИСПРАВЛЕНО: Используем публичную переменную bSpritesVisible
        return Actor->bSpritesVisible ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return bSpritesVisible ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState SCharacter2DActionPanel::GetSkeletalVisibleState() const
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        // ИСПРАВЛЕНО: Используем публичную переменную bSkeletalVisible
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
    }
}

void SCharacter2DActionPanel::EnsurePreviewVisible()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->SetActorHiddenInGame(false);
        Actor->SetBothVisible(bSpritesVisible, bSkeletalVisible);
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
        default:                                    return LOCTEXT("Emo_None", "None");
    }
}

#undef LOCTEXT_NAMESPACE