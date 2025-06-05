#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

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

    // Custom Prepass function using the same name as SWidget's method.
    // Not marked override because SlatePrepass is not virtual in UE5.5.
    void SlatePrepass(float LayoutScaleMultiplier);

protected:
    void NonRecursivePrepass(float LayoutScaleMultiplier);
};

