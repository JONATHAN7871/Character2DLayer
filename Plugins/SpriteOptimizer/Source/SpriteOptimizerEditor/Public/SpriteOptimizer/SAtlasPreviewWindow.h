// Создайте новый файл: SAtlasPreviewWindow.h

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "SpriteOptimizer/SpriteOptimizer.h"
#include "PaperSprite.h"
#include "Framework/Application/SlateApplication.h"

class UPaperSprite;

struct FAtlasPreviewSprite
{
    TObjectPtr<UPaperSprite> Sprite = nullptr;
    FIntRect Region;
    FString SpriteName;
    FLinearColor BorderColor;
    
    FAtlasPreviewSprite()
    {
        Region = FIntRect(0, 0, 0, 0);
        BorderColor = FLinearColor::White;
    }
    
    FAtlasPreviewSprite(UPaperSprite* InSprite, const FIntRect& InRegion)
    {
        Sprite = InSprite;
        Region = InRegion;
        SpriteName = InSprite ? InSprite->GetName() : TEXT("Unknown");
        
        // Генерируем случайный цвет для рамки каждого спрайта
        BorderColor = FLinearColor(
            FMath::RandRange(0.3f, 1.0f),
            FMath::RandRange(0.3f, 1.0f), 
            FMath::RandRange(0.3f, 1.0f),
            1.0f
        );
    }
};

class SAtlasPreviewWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SAtlasPreviewWindow) {}
        SLATE_ARGUMENT(TArray<UPaperSprite*>, Sprites)
        SLATE_ARGUMENT(FSpriteAtlasSettings, AtlasSettings)
        SLATE_ARGUMENT(FSpriteAtlasResult, AnalysisResult)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    
    // Статический метод для показа окна предпросмотра
    static void ShowAtlasPreview(
        const TArray<UPaperSprite*>& Sprites,
        const FSpriteAtlasSettings& Settings,
        const FSpriteAtlasResult& Analysis
    );

private:
    // Данные для предпросмотра
    TArray<FAtlasPreviewSprite> PreviewSprites;
    FIntPoint AtlasSize;
    FSpriteAtlasSettings CurrentSettings;
    FSpriteAtlasResult CurrentAnalysis;
    
    // UI элементы
    TSharedPtr<SScrollBox> PreviewScrollBox;
    TSharedPtr<STextBlock> InfoText;
    TSharedPtr<SButton> CloseButton;
    TSharedPtr<SButton> CreateAtlasButton;
    
    // Создание UI секций
    TSharedRef<SWidget> CreateHeaderSection();
    TSharedRef<SWidget> CreatePreviewSection();
    TSharedRef<SWidget> CreateInfoSection();
    TSharedRef<SWidget> CreateActionsSection();
    
    // Кастомный виджет для рисования атласа
    TSharedRef<SWidget> CreateAtlasCanvas();
    
    // Обработчики событий
    FReply OnCloseWindow();
    FReply OnCreateAtlasFromPreview();
    
    // Вспомогательные методы
    void InitializePreviewData();
    FText GetAtlasInfoText() const;
    FText GetSpriteListText() const;
    
    // Расчет масштаба для отображения
    float CalculateDisplayScale() const;
};

// Кастомный виджет для рисования атласа
class SAtlasCanvas : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SAtlasCanvas) {}
        SLATE_ARGUMENT(TArray<FAtlasPreviewSprite>, PreviewSprites)
        SLATE_ARGUMENT(FIntPoint, AtlasSize)
        SLATE_ARGUMENT(float, DisplayScale)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    
    // SWidget interface
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, 
                         const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, 
                         int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    
    virtual FVector2D ComputeDesiredSize(float) const override;

private:
    TArray<FAtlasPreviewSprite> SpriteData;
    FIntPoint AtlasResolution;
    float Scale;
};