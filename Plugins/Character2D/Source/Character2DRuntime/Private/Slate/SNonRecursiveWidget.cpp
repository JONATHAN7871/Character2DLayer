#include "Slate/SNonRecursiveWidget.h"

void SNonRecursiveWidget::SlatePrepass(float LayoutScaleMultiplier)
{
    // Forward to leaf widget implementation and avoid recursive child loops
    SLeafWidget::SlatePrepass(LayoutScaleMultiplier);
}

void SNonRecursiveWidget::NonRecursivePrepass(float LayoutScaleMultiplier)
{
    // Cache desired size without recursing into children since SLeafWidget has none
    CacheDesiredSize(LayoutScaleMultiplier);
}
