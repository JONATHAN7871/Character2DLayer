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

    // Построение UI
    ChildSlot
    [
        SNew(SScrollBox)
        + SScrollBox::Slot().Padding(4) [ SNew(SExpandableArea).AreaTitle(LOCTEXT("AnimationTesting", "Animation Testing")).InitiallyCollapsed(true).BodyContent()[BuildAnimationTestingSection()] ]
        + SScrollBox::Slot().Padding(4) [ SNew(SExpandableArea).AreaTitle(LOCTEXT("VisibilityTest", "Visibility Testing")).InitiallyCollapsed(true).BodyContent()[BuildVisibilityTestSection()] ]
    ];
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
        SyncStateFromActor();
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
        Actor->GetWorldTimerManager().ClearTimer(BlinkTestHandle);
        Actor->GetWorldTimerManager().ClearTimer(TalkTestHandle);
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

#undef LOCTEXT_NAMESPACE