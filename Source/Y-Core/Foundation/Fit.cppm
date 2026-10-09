export module ClaFi.Core.Foundation.Fit;

import ClaFi.StdLib;

namespace ClaFi
{
    export struct FitData
    {
        float width;
        float minWidth{ 0.0f };
    };

    export class IFittable
    {
    public:
        virtual ~IFittable() {}
        virtual FitData fitData() const = 0;
        // Takes `value` off this element and off every container above it.
        // shrinkBy() charges the leaf only and relies on that propagation, so an
        // implementation that stops at itself leaves its ancestors overstated.
        virtual void takeOffWidth(float value) = 0;
    };

    export class IFittableListBase
    {
    public:
        virtual ~IFittableListBase() {}
        virtual FitData fitData() const = 0;
    };

    export class IFittableList : public IFittableListBase
    {
    public:
        using FittableFunc = std::function<void(IFittable&)>;
    public:
        virtual ~IFittableList() {}
    public:
        virtual void traverseFittables(const FittableFunc) = 0;
        void shrinkBy(float width);   // the widest come down to one common width
    };
}
