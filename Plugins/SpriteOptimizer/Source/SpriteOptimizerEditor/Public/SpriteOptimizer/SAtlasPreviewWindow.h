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
        
        // Generate random color for each sprite border
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
    
    static void ShowAtlasPreview(
        const TArray<UPaperSprite*>& Sprites,
        const FSpriteAtlasSettings& Settings,
        const FSpriteAtlasResult& Analysis
    );

private:
    // Preview data
    TArray<FAtlasPreviewSprite> PreviewSprites;
    FIntPoint AtlasSize = FIntPoint::ZeroValue;
    FSpriteAtlasSettings CurrentSettings;
    FSpriteAtlasResult CurrentAnalysis;
    
    // UI widgets
    TSharedPtr<SScrollBox> PreviewScrollBox;
    TSharedPtr<SButton> CloseButton;
    
    // UI creation methods
    TSharedRef<SWidget> CreateCompactPreviewSection();
    TSharedRef<SWidget> CreateAtlasCanvas();
    
    // Event handlers
    FReply OnCloseWindow();
    
    // Helper methods
    void InitializePreviewData();
    float CalculateDisplayScale() const;
    
    // Text getters
    FText GetCompactAtlasInfoText() const;
};

// Custom widget for drawing atlas
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
    FIntPoint AtlasResolution = FIntPoint::ZeroValue;
    float Scale = 1.0f;
};