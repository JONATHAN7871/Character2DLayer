#include "Slate/SNonRecursiveWidget.h"

void SNonRecursiveWidget::SlatePrepass(float LayoutScaleMultiplier)
{
    // Begin custom prepass without recursion
    NonRecursivePrepass(LayoutScaleMultiplier);
}

void SNonRecursiveWidget::NonRecursivePrepass(float LayoutScaleMultiplier)
{
    // Cache desired size first
    CacheDesiredSize(LayoutScaleMultiplier);

    // Iterate children manually without calling SlatePrepass again on this widget
    FChildren* MyChildren = this->GetChildren();
    if (!MyChildren)
    {
        return;
    }

    const int32 NumChildren = MyChildren->Num();
    for (int32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
    {
        TSharedRef<SWidget> ChildWidget = MyChildren->GetChildAt(ChildIndex);
        ChildWidget->SlatePrepass(LayoutScaleMultiplier);
    }
}
