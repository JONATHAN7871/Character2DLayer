// Создайте новый файл: SAtlasPreviewWindow.cpp

#include "SpriteOptimizer/SAtlasPreviewWindow.h"
#include "SpriteOptimizer/SpriteOptimizer.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"

#define LOCTEXT_NAMESPACE "SAtlasPreviewWindow"

void SAtlasPreviewWindow::Construct(const FArguments& InArgs)
{
    // Сохраняем данные
    CurrentSettings = InArgs._AtlasSettings;
    CurrentAnalysis = InArgs._AnalysisResult;
    AtlasSize = CurrentAnalysis.AtlasSize;
    
    // Инициализируем данные для предпросмотра
    InitializePreviewData();
    
    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(10)
        [
            SNew(SVerticalBox)
            
            // Заголовок
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                CreateHeaderSection()
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 2)
            [
                SNew(SSeparator)
            ]
            
            // Информация об атласе
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                CreateInfoSection()
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 2)
            [
                SNew(SSeparator)
            ]
            
            // Предпросмотр атласа
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(0, 5)
            [
                CreatePreviewSection()
            ]
            
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 2)
            [
                SNew(SSeparator)
            ]
            
            // Действия
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                CreateActionsSection()
            ]
        ]
    ];
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
        .ClientSize(FVector2D(1000, 700))
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

TSharedRef<SWidget> SAtlasPreviewWindow::CreateHeaderSection()
{
    return SNew(SVerticalBox)
        
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(STextBlock)
            .Text(LOCTEXT("PreviewTitle", "🔍 Atlas Preview"))
            .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
            .Justification(ETextJustify::Center)
        ]
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("PreviewSubtitle", "Preview how sprites will be arranged in the atlas before creation"))
            .Justification(ETextJustify::Center)
            .AutoWrapText(true)
        ];
}

TSharedRef<SWidget> SAtlasPreviewWindow::CreateInfoSection()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
        .Padding(10)
        [
            SNew(SHorizontalBox)
            
            // Левая колонка - информация об атласе
            + SHorizontalBox::Slot()
            .FillWidth(0.5f)
            .Padding(5)
            [
                SNew(SVerticalBox)
                
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("AtlasInfoTitle", "📊 Atlas Information"))
                    .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                ]
                
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SAssignNew(InfoText, STextBlock)
                    .Text(GetAtlasInfoText())
                    .AutoWrapText(true)
                ]
            ]
            
            // Правая колонка - список спрайтов
            + SHorizontalBox::Slot()
            .FillWidth(0.5f)
            .Padding(5)
            [
                SNew(SVerticalBox)
                
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("SpriteListTitle", "🎨 Sprites in Atlas"))
                    .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
                ]
                
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(STextBlock)
                    .Text(GetSpriteListText())
                    .AutoWrapText(true)
                ]
            ]
        ];
}

TSharedRef<SWidget> SAtlasPreviewWindow::CreatePreviewSection()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
        .Padding(10)
        [
            SNew(SVerticalBox)
            
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(LOCTEXT("PreviewCanvasTitle", "🖼️ Atlas Layout Preview"))
                .Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
            ]
            
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(0, 10)
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

TSharedRef<SWidget> SAtlasPreviewWindow::CreateActionsSection()
{
    return SNew(SVerticalBox)
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10, 5)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("PreviewInstructions", 
                "📋 This preview shows how sprites will be arranged in the atlas.\n"
                "✅ If you're satisfied with the layout, close this window and click 'Create Atlas' in the main window.\n"
                "⚙️ If you want to adjust settings, close this window and modify parameters, then preview again."))
            .Justification(ETextJustify::Center)
            .AutoWrapText(true)
            .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
        ]
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        [
            SNew(SHorizontalBox)
            
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SAssignNew(CloseButton, SButton)
                .Text(LOCTEXT("ClosePreview", "✖️ Close Preview"))
                .OnClicked(this, &SAtlasPreviewWindow::OnCloseWindow)
                .HAlign(HAlign_Center)
                .ToolTipText(LOCTEXT("ClosePreviewTooltip", "Close preview and return to atlas creation window"))
            ]
            
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
        ];
}

void SAtlasPreviewWindow::InitializePreviewData()
{
    PreviewSprites.Empty();
    
    // Создаем данные для предпросмотра на основе анализа
    if (CurrentAnalysis.SpriteRegions.Num() > 0)
    {
        for (int32 i = 0; i < CurrentAnalysis.SpriteRegions.Num(); i++)
        {
            const FIntRect& Region = CurrentAnalysis.SpriteRegions[i];
            
            // Создаем данные для предпросмотра
            FAtlasPreviewSprite PreviewSprite;
            PreviewSprite.Region = Region;
            PreviewSprite.SpriteName = FString::Printf(TEXT("Sprite_%d"), i + 1);
            
            // Генерируем цвет для каждого спрайта
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
    // Максимальный размер canvas в окне
    const float MaxCanvasSize = 400.0f;
    
    // Вычисляем масштаб на основе размера атласа
    float ScaleX = MaxCanvasSize / float(AtlasSize.X);
    float ScaleY = MaxCanvasSize / float(AtlasSize.Y);
    
    // Используем меньший масштаб, чтобы атлас поместился
    float Scale = FMath::Min(ScaleX, ScaleY);
    
    // Ограничиваем масштаб разумными пределами
    return FMath::Clamp(Scale, 0.1f, 2.0f);
}

FText SAtlasPreviewWindow::GetAtlasInfoText() const
{
    FString AlgorithmName;
    switch (CurrentSettings.PackingAlgorithm)
    {
        case EAtlasPackingAlgorithm::Simple:
            AlgorithmName = TEXT("Simple Grid");
            break;
        case EAtlasPackingAlgorithm::BestFit:
            AlgorithmName = TEXT("Best Fit");
            break;
        case EAtlasPackingAlgorithm::MaxRects:
            AlgorithmName = TEXT("MaxRects");
            break;
        default:
            AlgorithmName = TEXT("Unknown");
            break;
    }
    
    return FText::Format(LOCTEXT("AtlasInfoFormat",
        "📐 Atlas size: {0}x{1} pixels\n"
        "📦 Packing efficiency: {2}%\n"
        "💾 Estimated memory: {3} MB\n"
        "💰 Memory savings: {4}%\n"
        "🎨 Total sprites: {5}\n"
        "⚙️ Algorithm: {6}"),
        FText::AsNumber(AtlasSize.X),
        FText::AsNumber(AtlasSize.Y),
        FText::AsNumber(CurrentAnalysis.PackingEfficiency),
        FText::AsNumber((AtlasSize.X * AtlasSize.Y * 4) / (1024.0f * 1024.0f)),
        FText::AsNumber(CurrentAnalysis.MemorySavings),
        FText::AsNumber(CurrentAnalysis.TotalSprites),
        FText::FromString(AlgorithmName)
    );
}

FText SAtlasPreviewWindow::GetSpriteListText() const
{
    FString SpriteList;
    
    for (int32 i = 0; i < PreviewSprites.Num(); i++)
    {
        const FAtlasPreviewSprite& Sprite = PreviewSprites[i];
        SpriteList += FString::Printf(TEXT("• %s: %dx%d at (%d,%d)\n"),
            *Sprite.SpriteName,
            Sprite.Region.Width(), Sprite.Region.Height(),
            Sprite.Region.Min.X, Sprite.Region.Min.Y
        );
    }
    
    return FText::FromString(SpriteList);
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

// === РЕАЛИЗАЦИЯ КАСТОМНОГО CANVAS ===

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
    // Рисуем фон атласа
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        FAppStyle::GetBrush("WhiteBrush"),
        ESlateDrawEffect::None,
        FLinearColor(0.1f, 0.1f, 0.1f, 1.0f) // Темно-серый фон
    );
    
    LayerId++;
    
    // Рисуем каждый спрайт как прямоугольник
    for (int32 i = 0; i < SpriteData.Num(); i++)
    {
        const FAtlasPreviewSprite& Sprite = SpriteData[i];
        
        // Вычисляем позицию и размер в пикселях canvas
        FVector2D Position(
            Sprite.Region.Min.X * Scale,
            Sprite.Region.Min.Y * Scale
        );
        
        FVector2D Size(
            Sprite.Region.Width() * Scale,
            Sprite.Region.Height() * Scale
        );
        
        // Рисуем заполненный прямоугольник спрайта
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)),
            FAppStyle::GetBrush("WhiteBrush"),
            ESlateDrawEffect::None,
            Sprite.BorderColor.CopyWithNewOpacity(0.3f) // Полупрозрачная заливка
        );
        
        // Рисуем рамку спрайта
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId + 1,
            AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)),
            FAppStyle::GetBrush("Border"),
            ESlateDrawEffect::None,
            Sprite.BorderColor // Яркая рамка
        );
        
        // Рисуем номер спрайта в центре (только для достаточно больших спрайтов)
        if (Size.X > 30 && Size.Y > 20)
        {
            FVector2D TextPosition = Position + Size * 0.5f - FVector2D(5, 5); // Примерный центр
            
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