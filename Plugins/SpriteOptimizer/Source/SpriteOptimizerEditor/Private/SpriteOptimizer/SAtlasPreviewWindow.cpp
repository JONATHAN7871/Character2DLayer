// Copyright 2025, CRAFTCODE, All Rights Reserved.

#include "SpriteOptimizer/SAtlasPreviewWindow.h"
#include "SpriteOptimizer/SpriteOptimizer.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "SAtlasPreviewWindow"

void SAtlasPreviewWindow::Construct(const FArguments& InArgs)
{
    // Save data
    CurrentSettings = InArgs._AtlasSettings;
    CurrentAnalysis = InArgs._AnalysisResult;
    AtlasSize = CurrentAnalysis.AtlasSize;
    
    // Initialize preview data
    InitializePreviewData();
    
    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(8)
        [
            SNew(SVerticalBox)
            
            // Compact header
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SNew(STextBlock)
                .Text(FText::Format(LOCTEXT("PreviewTitle", "🔍 Atlas Preview ({0}x{1})"), 
                                   AtlasSize.X, AtlasSize.Y))
                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                .Justification(ETextJustify::Center)
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 2)
            [
                SNew(SSeparator)
            ]
            
            // Compact information in one line
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SNew(STextBlock)
                .Text(GetCompactAtlasInfoText())
                .AutoWrapText(true)
                .Font(FAppStyle::GetFontStyle("SmallFont"))
                .Justification(ETextJustify::Center)
                .ColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)))
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 2)
            [
                SNew(SSeparator)
            ]
            
            // Preview
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(0, 5)
            [
                CreateCompactPreviewSection()
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 2)
            [
                SNew(SSeparator)
            ]
            
            // Compact actions
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SNew(SHorizontalBox)
                
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SAssignNew(CloseButton, SButton)
                    .Text(LOCTEXT("ClosePreview", "Close"))
                    .OnClicked(this, &SAtlasPreviewWindow::OnCloseWindow)
                    .HAlign(HAlign_Center)
                ]
                
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
            ]
        ]
    ];
}

TSharedRef<SWidget> SAtlasPreviewWindow::CreateCompactPreviewSection()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
        .Padding(5)
        [
            SNew(SVerticalBox)
            
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(LOCTEXT("PreviewCanvasTitle", "🖼️ Layout Preview"))
                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                .Justification(ETextJustify::Center)
            ]
            
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(0, 5)
            [
                SAssignNew(PreviewScrollBox, SScrollBox)
                .Orientation(Orient_Vertical)
                
                + SScrollBox::Slot()
                [
                    SNew(SHorizontalBox)
                    
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .HAlign(HAlign_Center)
                    [
                        CreateAtlasCanvas()
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> SAtlasPreviewWindow::CreateAtlasCanvas()
{
    float DisplayScale = CalculateDisplayScale();
    
    return SNew(SAtlasCanvas)
        .PreviewSprites(PreviewSprites)
        .AtlasSize(AtlasSize)
        .DisplayScale(DisplayScale);
}

void SAtlasPreviewWindow::ShowAtlasPreview(
    const TArray<UPaperSprite*>& Sprites,
    const FSpriteAtlasSettings& Settings,
    const FSpriteAtlasResult& Analysis)
{
    if (Sprites.Num() < 2 || !Analysis.bSuccess)
    {
        return;
    }
    
    TSharedRef<SWindow> PreviewWindow = SNew(SWindow)
        .Title(FText::Format(LOCTEXT("AtlasPreviewTitle", "Atlas Preview - {0}x{1}"), 
                           Analysis.AtlasSize.X, Analysis.AtlasSize.Y))
        .SizingRule(ESizingRule::UserSized)
        .ClientSize(FVector2D(600, 500))
        .SupportsMaximize(true)
        .SupportsMinimize(false)
        .Content()
        [
            SNew(SAtlasPreviewWindow)
            .Sprites(Sprites)
            .AtlasSettings(Settings)
            .AnalysisResult(Analysis)
        ];
    
    FSlateApplication::Get().AddWindow(PreviewWindow);
}

void SAtlasPreviewWindow::InitializePreviewData()
{
    PreviewSprites.Empty();
    
    // Create preview data based on analysis
    if (CurrentAnalysis.SpriteRegions.Num() > 0)
    {
        for (int32 i = 0; i < CurrentAnalysis.SpriteRegions.Num(); i++)
        {
            const FIntRect& Region = CurrentAnalysis.SpriteRegions[i];
            
            // Create preview data
            FAtlasPreviewSprite PreviewSprite;
            PreviewSprite.Region = Region;
            PreviewSprite.SpriteName = FString::Printf(TEXT("Sprite_%d"), i + 1);
            
            // Generate color for each sprite
            float Hue = (float(i) / float(FMath::Max(1, CurrentAnalysis.SpriteRegions.Num()))) * 360.0f;
            PreviewSprite.BorderColor = FLinearColor::MakeFromHSV8(
                uint8(Hue), 
                200,  // Saturation
                255   // Value
            );
            
            PreviewSprites.Add(PreviewSprite);
        }
    }
}

float SAtlasPreviewWindow::CalculateDisplayScale() const
{
    // Maximum canvas size in window
    const float MaxCanvasSize = 400.0f;
    
    // Calculate scale based on atlas size
    float ScaleX = MaxCanvasSize / float(AtlasSize.X);
    float ScaleY = MaxCanvasSize / float(AtlasSize.Y);
    
    // Use smaller scale so atlas fits
    float Scale = FMath::Min(ScaleX, ScaleY);
    
    // Limit scale to reasonable bounds
    return FMath::Clamp(Scale, 0.1f, 2.0f);
}

FText SAtlasPreviewWindow::GetCompactAtlasInfoText() const
{
    FString AlgorithmName;
    switch (CurrentSettings.PackingAlgorithm)
    {
    case EAtlasPackingAlgorithm::Simple:
        AlgorithmName = TEXT("Simple");
        break;
    case EAtlasPackingAlgorithm::BestFit:
        AlgorithmName = TEXT("BestFit");
        break;
    case EAtlasPackingAlgorithm::MaxRects:
        AlgorithmName = TEXT("MaxRects");
        break;
    default:
        AlgorithmName = TEXT("Unknown");
        break;
    }
    
    return FText::Format(LOCTEXT("CompactAtlasInfoFormat",
        "{0} sprites • {1}% efficiency • {2}% memory savings • {3} algorithm"),
        FText::AsNumber(CurrentAnalysis.TotalSprites),
        FText::AsNumber(CurrentAnalysis.PackingEfficiency),
        FText::AsNumber(CurrentAnalysis.MemorySavings),
        FText::FromString(AlgorithmName)
    );
}

FReply SAtlasPreviewWindow::OnCloseWindow()
{
    TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
    if (ParentWindow.IsValid())
    {
        ParentWindow->RequestDestroyWindow();
    }
    return FReply::Handled();
}

// === CUSTOM CANVAS IMPLEMENTATION ===

void SAtlasCanvas::Construct(const FArguments& InArgs)
{
    SpriteData = InArgs._PreviewSprites;
    AtlasResolution = InArgs._AtlasSize;
    Scale = InArgs._DisplayScale;
}

FVector2D SAtlasCanvas::ComputeDesiredSize(float) const
{
    return FVector2D(
        AtlasResolution.X * Scale,
        AtlasResolution.Y * Scale
    );
}

int32 SAtlasCanvas::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                           const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                           int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    // Draw atlas background
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        FAppStyle::GetBrush("WhiteBrush"),
        ESlateDrawEffect::None,
        FLinearColor(0.1f, 0.1f, 0.1f, 1.0f) // Dark gray background
    );
    
    LayerId++;
    
    // Draw each sprite as rectangle
    for (int32 i = 0; i < SpriteData.Num(); i++)
    {
        const FAtlasPreviewSprite& Sprite = SpriteData[i];
        
        // Calculate position and size in canvas pixels
        FVector2D Position(
            Sprite.Region.Min.X * Scale,
            Sprite.Region.Min.Y * Scale
        );
        
        FVector2D Size(
            Sprite.Region.Width() * Scale,
            Sprite.Region.Height() * Scale
        );
        
        // Draw filled sprite rectangle
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)),
            FAppStyle::GetBrush("WhiteBrush"),
            ESlateDrawEffect::None,
            Sprite.BorderColor.CopyWithNewOpacity(0.3f) // Semi-transparent fill
        );
        
        // Draw sprite border
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId + 1,
            AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)),
            FAppStyle::GetBrush("Border"),
            ESlateDrawEffect::None,
            Sprite.BorderColor // Bright border
        );
        
        // Draw sprite number in center (only for large enough sprites)
        if (Size.X > 30 && Size.Y > 20)
        {
            FVector2D TextPosition = Position + Size * 0.5f - FVector2D(5, 5); // Approximate center
            
            FSlateDrawElement::MakeText(
                OutDrawElements,
                LayerId + 2,
                AllottedGeometry.ToPaintGeometry(FVector2D(20, 20), FSlateLayoutTransform(TextPosition)),
                FString::Printf(TEXT("%d"), i + 1),
                FAppStyle::GetFontStyle("SmallFont"),
                ESlateDrawEffect::None,
                FLinearColor::White
            );
        }
    }
    
    return LayerId + 3;
}

#undef LOCTEXT_NAMESPACE