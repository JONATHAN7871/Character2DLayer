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
        + SScrollBox::Slot().Padding(4) [ SNew(SExpandableArea).AreaTitle(LOCTEXT("AutoAnimations", "Auto Animations")).InitiallyCollapsed(false).BodyContent()[BuildAutoAnimationsSection()] ]
        + SScrollBox::Slot().Padding(4) [ SNew(SExpandableArea).AreaTitle(LOCTEXT("EditorPreview", "Editor Preview Testing")).InitiallyCollapsed(true).BodyContent()[BuildAnimationTestingSection()] ]
        + SScrollBox::Slot().Padding(4) [ SNew(SExpandableArea).AreaTitle(LOCTEXT("VisibilityTest", "Visibility Testing")).InitiallyCollapsed(true).BodyContent()[BuildVisibilityTestSection()] ]
    ];
}

void SCharacter2DActionPanel::SyncStateFromActor()
{
    // ИСПРАВЛЕНО: Синхронизируем состояние с preview-specific флагами, а не с актором
    bBlinkingEnabled = bPreviewBlinkingEnabled;
    bTalkingEnabled = bPreviewTalkingEnabled;
    
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        bSpritesVisible = Actor->bSpritesVisible;
        bSkeletalVisible = Actor->bSkeletalVisible;
    }
    else
    {
        bSpritesVisible = true;
        bSkeletalVisible = true;
    }
}

// Секция Auto Animations для отображения состояния Auto Blink/Talk (только информативно)
TSharedRef<SWidget> SCharacter2DActionPanel::BuildAutoAnimationsSection()
{
    return SNew(SVerticalBox)
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) 
        [ 
            SAssignNew(AutoBlinkCheckBox, SCheckBox)
            .OnCheckStateChanged(this, &SCharacter2DActionPanel::OnAutoBlinkChanged)
            .IsChecked(this, &SCharacter2DActionPanel::GetAutoBlinkState)
        ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0) 
        [ 
            SAssignNew(AutoBlinkLabel, STextBlock)
            .Text(LOCTEXT("AutoBlink", "Auto Blink (Game Setting)"))
            .ColorAndOpacity(this, &SCharacter2DActionPanel::GetAutoBlinkColor)
            .ToolTipText(LOCTEXT("AutoBlinkTooltip", "Controls whether actors spawned in game will automatically blink"))
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
        ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0) 
        [ 
            SAssignNew(AutoTalkLabel, STextBlock)
            .Text(LOCTEXT("AutoTalk", "Auto Talk (Game Setting)"))
            .ColorAndOpacity(this, &SCharacter2DActionPanel::GetAutoTalkColor)
            .ToolTipText(LOCTEXT("AutoTalkTooltip", "Controls whether actors spawned in game will automatically talk"))
        ]
    ];
}

TSharedRef<SWidget> SCharacter2DActionPanel::BuildAnimationTestingSection()
{
    return SNew(SVerticalBox)
    
    // НОВОЕ: Пояснительный текст
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(STextBlock)
        .Text(LOCTEXT("EditorOnlyNote", "Note: These controls are for editor preview only and don't affect game behavior"))
        .ColorAndOpacity(FLinearColor(1.0f, 1.0f, 0.0f, 0.8f)) // Желтый цвет для предупреждения
        .AutoWrapText(true)
    ]
    
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) 
        [ 
            SNew(SCheckBox)
            .OnCheckStateChanged(this, &SCharacter2DActionPanel::OnBlinkChanged)
            .IsChecked_Lambda([this]() 
            { 
                SyncStateFromActor(); 
                return bBlinkingEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; 
            })
        ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0) 
        [ 
            SNew(STextBlock)
            .Text(LOCTEXT("EnableBlinking", "Enable Blinking (Preview Only)"))
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(8,0) 
        [ 
            SNew(SButton)
            .Text(LOCTEXT("TestBlink", "Test Blink"))
            .OnClicked(this, &SCharacter2DActionPanel::OnTestBlink)
            .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid) 
        ]
    ]
    + SVerticalBox::Slot().AutoHeight().Padding(2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center) 
        [ 
            SNew(SCheckBox)
            .OnCheckStateChanged(this, &SCharacter2DActionPanel::OnTalkChanged)
            .IsChecked_Lambda([this]() 
            { 
                SyncStateFromActor(); 
                return bTalkingEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; 
            })
        ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0, 0, 0) 
        [ 
            SNew(STextBlock)
            .Text(LOCTEXT("EnableTalking", "Enable Talking (Preview Only)"))
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(8,0) 
        [ 
            SNew(SButton)
            .Text(LOCTEXT("TestTalk", "Test Talk"))
            .OnClicked(this, &SCharacter2DActionPanel::OnTestTalk)
            .IsEnabled(this, &SCharacter2DActionPanel::IsPreviewActorValid) 
        ]
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
// === НОВЫЕ МЕТОДЫ: Для получения состояния Auto настроек ===
// =========================================

ECheckBoxState SCharacter2DActionPanel::GetAutoBlinkState() const
{
    if (CharacterAsset.IsValid())
    {
        return CharacterAsset->bAutoBlink ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return ECheckBoxState::Unchecked;
}

ECheckBoxState SCharacter2DActionPanel::GetAutoTalkState() const
{
    if (CharacterAsset.IsValid())
    {
        return CharacterAsset->bAutoTalk ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return ECheckBoxState::Unchecked;
}

FSlateColor SCharacter2DActionPanel::GetAutoBlinkColor() const
{
    if (CharacterAsset.IsValid() && CharacterAsset->bAutoBlink)
    {
        return FSlateColor(FLinearColor::Green);
    }
    return FSlateColor(FLinearColor::White);
}

FSlateColor SCharacter2DActionPanel::GetAutoTalkColor() const
{
    if (CharacterAsset.IsValid() && CharacterAsset->bAutoTalk)
    {
        return FSlateColor(FLinearColor::Green);
    }
    return FSlateColor(FLinearColor::White);
}

// =========================================
// === Обработчики событий (Event Handlers) ===
// =========================================

// Auto Blink/Talk обработчики (влияют на настройки ассета)
void SCharacter2DActionPanel::OnAutoBlinkChanged(ECheckBoxState NewState)
{
    if (CharacterAsset.IsValid())
    {
        CharacterAsset->bAutoBlink = (NewState == ECheckBoxState::Checked);
        CharacterAsset->MarkPackageDirty();
        
        UE_LOG(LogTemp, Log, TEXT("Auto Blink changed to: %s"), CharacterAsset->bAutoBlink ? TEXT("TRUE") : TEXT("FALSE"));
        
        // НЕ влияем на preview актора - он остается под контролем editor-only настроек
        
        // Принудительно обновляем UI
        if (AutoBlinkCheckBox.IsValid())
        {
            AutoBlinkCheckBox->Invalidate(EInvalidateWidget::LayoutAndVolatility);
        }
        if (AutoBlinkLabel.IsValid())
        {
            AutoBlinkLabel->Invalidate(EInvalidateWidget::LayoutAndVolatility);
        }
    }
}

void SCharacter2DActionPanel::OnAutoTalkChanged(ECheckBoxState NewState)
{
    if (CharacterAsset.IsValid())
    {
        CharacterAsset->bAutoTalk = (NewState == ECheckBoxState::Checked);
        CharacterAsset->MarkPackageDirty();
        
        UE_LOG(LogTemp, Log, TEXT("Auto Talk changed to: %s"), CharacterAsset->bAutoTalk ? TEXT("TRUE") : TEXT("FALSE"));
        
        // НЕ влияем на preview актора - он остается под контролем editor-only настроек
        
        // Принудительно обновляем UI
        if (AutoTalkCheckBox.IsValid())
        {
            AutoTalkCheckBox->Invalidate(EInvalidateWidget::LayoutAndVolatility);
        }
        if (AutoTalkLabel.IsValid())
        {
            AutoTalkLabel->Invalidate(EInvalidateWidget::LayoutAndVolatility);
        }
    }
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
        
        // НОВОЕ: Сбрасываем editor-only флаги
        bPreviewBlinkingEnabled = false;
        bPreviewTalkingEnabled = false;
        
        SyncStateFromActor();
    }
    return FReply::Handled();
}

// ИСПРАВЛЕНО: Preview-only обработчики анимаций
void SCharacter2DActionPanel::OnBlinkChanged(ECheckBoxState NewState)
{
    bPreviewBlinkingEnabled = (NewState == ECheckBoxState::Checked);
    bBlinkingEnabled = bPreviewBlinkingEnabled;
    
    if (ACharacter2DActor* Actor = PreviewActor.Get()) 
    { 
        // Используем специальный метод для preview
        SetPreviewBlinking(bPreviewBlinkingEnabled);
        EnsurePreviewVisible(); 
    }
}

void SCharacter2DActionPanel::OnTalkChanged(ECheckBoxState NewState)
{
    bPreviewTalkingEnabled = (NewState == ECheckBoxState::Checked);
    bTalkingEnabled = bPreviewTalkingEnabled;
    
    if (ACharacter2DActor* Actor = PreviewActor.Get()) 
    { 
        // Используем специальный метод для preview
        SetPreviewTalking(bPreviewTalkingEnabled);
        EnsurePreviewVisible(); 
    }
}

FReply SCharacter2DActionPanel::OnTestBlink()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        SetPreviewBlinking(true);
        EnsurePreviewVisible();
        FTimerDelegate TimerDel;
        TimerDel.BindLambda([this]()
        { 
            if (ACharacter2DActor* InnerActor = PreviewActor.Get())
            { 
                SetPreviewBlinking(false);
                EnsurePreviewVisible(); 
            }
        });
        Actor->GetWorldTimerManager().SetTimer(BlinkTestHandle, TimerDel, 2.0f, false);
    }
    return FReply::Handled();
}

FReply SCharacter2DActionPanel::OnTestTalk()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        SetPreviewTalking(true);
        EnsurePreviewVisible();
        FTimerDelegate TimerDel;
        TimerDel.BindLambda([this]()
        { 
            if (ACharacter2DActor* InnerActor = PreviewActor.Get())
            { 
                SetPreviewTalking(false);
                EnsurePreviewVisible(); 
            }
        });
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

void SCharacter2DActionPanel::StopAllPreviewAnimations()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        SetPreviewBlinking(false);
        SetPreviewTalking(false);
        Actor->GetWorldTimerManager().ClearTimer(BlinkTestHandle);
        Actor->GetWorldTimerManager().ClearTimer(TalkTestHandle);
    }
    
    bPreviewBlinkingEnabled = false;
    bPreviewTalkingEnabled = false;
}

void SCharacter2DActionPanel::EnsurePreviewVisible()
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        Actor->SetActorHiddenInGame(false);
        Actor->SetBothVisible(bSpritesVisible, bSkeletalVisible);
    }
}

// НОВЫЕ МЕТОДЫ: Специально для preview, не затрагивающие игровую логику
void SCharacter2DActionPanel::SetPreviewBlinking(bool bEnable)
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        // Напрямую вызываем методы анимации, не изменяя bBlinkingActive
        if (bEnable)
        {
            Actor->EnableBlinking(true);
        }
        else
        {
            Actor->EnableBlinking(false);
        }
        
        // ВАЖНО: Сбрасываем флаг чтобы при обновлении актора анимации не восстанавливались
        Actor->bBlinkingActive = false;
    }
}

void SCharacter2DActionPanel::SetPreviewTalking(bool bEnable)
{
    if (ACharacter2DActor* Actor = PreviewActor.Get())
    {
        // Напрямую вызываем методы анимации, не изменяя bTalkingActive
        if (bEnable)
        {
            Actor->EnableTalking(true);
        }
        else
        {
            Actor->EnableTalking(false);
        }
        
        // ВАЖНО: Сбрасываем флаг чтобы при обновлении актора анимации не восстанавливались
        Actor->bTalkingActive = false;
    }
}

#undef LOCTEXT_NAMESPACE