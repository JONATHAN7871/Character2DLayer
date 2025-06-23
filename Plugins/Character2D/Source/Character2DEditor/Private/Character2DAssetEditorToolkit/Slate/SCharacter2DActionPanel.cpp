#include "Character2DAssetEditorToolkit/Slate/SCharacter2DActionPanel.h"
#include "Character2DActor.h"
#include "Character2DAsset.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "SCharacter2DActionPanel"

void SCharacter2DActionPanel::Construct(const FArguments& InArgs)
{
    CharacterAsset = InArgs._CharacterAsset;
    PreviewActor = InArgs._PreviewActor;

    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        bSpritesVisible = Actor->bSpritesVisible;
        bSkeletalVisible = Actor->bSkeletalVisible;
    }

    // Построение UI
    ChildSlot
    [
        SNew(SScrollBox)
        + SScrollBox::Slot()
        .Padding(4)
        [
            SNew(SExpandableArea)
            .AreaTitle(LOCTEXT("AutoAnimations", "In-Game Settings"))
            .InitiallyCollapsed(true)
            .BodyContent()
            [
                BuildAutoAnimationsSection()
            ]
        ]
        + SScrollBox::Slot()
        .Padding(4)
        [
            SNew(SExpandableArea)
            .AreaTitle(LOCTEXT("EditorPreview", "Editor Preview Controls"))
            .InitiallyCollapsed(false) // Раскрыто по умолчанию, т.к. это основная функция
            .BodyContent()
            [
                BuildAnimationTestingSection()
            ]
        ]
        + SScrollBox::Slot()
        .Padding(4)
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

TSharedRef<SWidget> SCharacter2DActionPanel::BuildAutoAnimationsSection()
{
    return SNew(SVerticalBox)
    + SVerticalBox::Slot().AutoHeight().Padding(2, 4)
    [
        SNew(STextBlock)
        .Text(LOCTEXT("AutoNote", "These settings affect actors spawned in the game."))
        .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
        .AutoWrapText(true)
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) 
        [ 
            SAssignNew(AutoBlinkCheckBox, SCheckBox)
            .OnCheckStateChanged(this, &SCharacter2DActionPanel::OnAutoBlinkChanged)
            .IsChecked(this, &SCharacter2DActionPanel::GetAutoBlinkState)
            .ToolTipText(LOCTEXT("AutoBlinkTooltip", "If checked, characters will automatically blink when spawned in the game world."))
        ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0) 
        [ 
            SNew(STextBlock)
            .Text(LOCTEXT("AutoBlink", "Auto Blink"))
        ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) 
        [ 
            SAssignNew(AutoTalkCheckBox, SCheckBox)
            .OnCheckStateChanged(this, &SCharacter2DActionPanel::OnAutoTalkChanged)
            .IsChecked(this, &SCharacter2DActionPanel::GetAutoTalkState)
            .ToolTipText(LOCTEXT("AutoTalkTooltip", "If checked, characters will automatically play talking animation when spawned in the game world."))
        ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0) 
        [ 
            SNew(STextBlock)
            .Text(LOCTEXT("AutoTalk", "Auto Talk"))
        ]
    ];
}

TSharedRef<SWidget> SCharacter2DActionPanel::BuildAnimationTestingSection()
{
    return SNew(SVerticalBox)
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) 
        [ 
            SNew(SCheckBox)
            .OnCheckStateChanged(this, &SCharacter2DActionPanel::OnBlinkChanged)
            .IsChecked(this, &SCharacter2DActionPanel::GetBlinkState)
        ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0) 
        [ 
            SNew(STextBlock)
            .Text(LOCTEXT("EnableBlinking", "Enable Blinking Preview"))
        ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) 
        [ 
            SNew(SCheckBox)
            .OnCheckStateChanged(this, &SCharacter2DActionPanel::OnTalkChanged)
            .IsChecked(this, &SCharacter2DActionPanel::GetTalkState)
        ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0) 
        [ 
            SNew(STextBlock)
            .Text(LOCTEXT("EnableTalking", "Enable Talking Preview"))
        ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SButton)
        .Text(LOCTEXT("BlinkOnce", "Blink Once"))
        .OnClicked(this, &SCharacter2DActionPanel::OnBlinkOnce)
        .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid)
        .HAlign(HAlign_Center)
    ];
}

TSharedRef<SWidget> SCharacter2DActionPanel::BuildVisibilityTestSection()
{
    return SNew(SVerticalBox)
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
            SNew(STextBlock).Text(LOCTEXT("SpritesVisible", "Sprites Visible")) 
        ]
    ]
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
            SNew(STextBlock).Text(LOCTEXT("SkeletalVisible", "Skeletal Visible")) 
        ]
    ];
}

// === ОБРАБОТЧИКИ ДЛЯ НАСТРОЕК АССЕТА ===

void SCharacter2DActionPanel::OnAutoBlinkChanged(ECheckBoxState NewState)
{
    if (UCharacter2DAsset* Asset = CharacterAsset.Get())
    {
        Asset->bAutoBlink = (NewState == ECheckBoxState::Checked);
        // Помечаем ассет как измененный, чтобы его можно было сохранить
        Asset->MarkPackageDirty();
    }
}

void SCharacter2DActionPanel::OnAutoTalkChanged(ECheckBoxState NewState)
{
    if (UCharacter2DAsset* Asset = CharacterAsset.Get())
    {
        Asset->bAutoTalk = (NewState == ECheckBoxState::Checked);
        Asset->MarkPackageDirty();
    }
}

ECheckBoxState SCharacter2DActionPanel::GetAutoBlinkState() const
{
    if (UCharacter2DAsset* Asset = CharacterAsset.Get())
    {
        return Asset->bAutoBlink ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return ECheckBoxState::Undetermined;
}

ECheckBoxState SCharacter2DActionPanel::GetAutoTalkState() const
{
    if (UCharacter2DAsset* Asset = CharacterAsset.Get())
    {
        return Asset->bAutoTalk ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return ECheckBoxState::Undetermined;
}


// === ОБРАБОТЧИКИ ДЛЯ ПРЕДПРОСМОТРА В РЕДАКТОРЕ ===

void SCharacter2DActionPanel::OnBlinkChanged(ECheckBoxState NewState)
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        // Просто отправляем команду актору. Его собственная логика (в RefreshFromAsset)
        // позаботится о сохранении этого состояния при обновлениях.
        const bool bEnable = (NewState == ECheckBoxState::Checked);
        Actor->EnableBlinking(bEnable);
    }
}

void SCharacter2DActionPanel::OnTalkChanged(ECheckBoxState NewState)
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        const bool bEnable = (NewState == ECheckBoxState::Checked);
        Actor->EnableTalking(bEnable);
    }
}

ECheckBoxState SCharacter2DActionPanel::GetBlinkState() const
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        // Напрямую запрашиваем состояние у актора. Он является "источником правды".
        return Actor->IsBlinkingEnabled() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return ECheckBoxState::Unchecked;
}

ECheckBoxState SCharacter2DActionPanel::GetTalkState() const
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        return Actor->IsTalkingEnabled() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return ECheckBoxState::Unchecked;
}

FReply SCharacter2DActionPanel::OnBlinkOnce()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->BlinkOnce();
    }
    return FReply::Handled();
}


// === ОБРАБОТЧИКИ ВИДИМОСТИ ===

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
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        return Actor->bSpritesVisible ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return bSpritesVisible ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState SCharacter2DActionPanel::GetSkeletalVisibleState() const
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        return Actor->bSkeletalVisible ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return bSkeletalVisible ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}


#undef LOCTEXT_NAMESPACE