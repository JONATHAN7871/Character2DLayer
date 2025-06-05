// Stub Unreal Engine SWidget implementation
#include "Widgets/SWidget.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

void SWidget::Prepass_Internal(float LayoutScaleMultiplier)
{
    // Compute this widget's desired size before visiting children
    CacheDesiredSize(LayoutScaleMultiplier);

    // Visit all children once without recursing back into this widget
    Prepass_ChildLoop(LayoutScaleMultiplier);
}

void SWidget::Prepass_ChildLoop(float LayoutScaleMultiplier)
{
    FChildren* MyChildren = this->GetChildren();
    if (!MyChildren)
    {
        return;
    }

    const int32 NumChildren = MyChildren->Num();
    for (int32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
    {
        TSharedRef<SWidget> ChildWidget = MyChildren->GetChildAt(ChildIndex);
        ChildWidget->Prepass_Internal(LayoutScaleMultiplier);
    }
}
