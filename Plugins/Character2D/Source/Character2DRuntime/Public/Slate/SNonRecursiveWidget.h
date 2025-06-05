#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"

/**
 * Custom widget that overrides Prepass without recursive loops.
 * Useful when engine versions suffer from Prepass recursion bugs.
 */
class SNonRecursiveWidget : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SNonRecursiveWidget) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // Override Prepass using the same signature as SWidget
    virtual void SlatePrepass(float LayoutScaleMultiplier) override;

protected:
    void NonRecursivePrepass(float LayoutScaleMultiplier);
};

