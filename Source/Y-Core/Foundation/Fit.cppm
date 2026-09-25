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
        void shrinkBy(float);
    };


    //-------------------------------------------------------------------------


    void IFittableList::shrinkBy(float remainToShrink)
    {
        IFittable* lastShrinkedCol = nullptr;
        while (remainToShrink > 0.0f)
        {
            float maxW = 0.0f;
            float maxWCount = 0.0f;
            float secondMaxW = 0.0f;

            traverseFittables([&maxW, &maxWCount, &secondMaxW](IFittable& column)
                {
                    FitData fitData = column.fitData();
                    if (fitData.width > fitData.minWidth)
                    {
                        if (maxW == fitData.width)
                            ++maxWCount;
                        else if (maxW < fitData.width)
                        {
                            maxWCount = 1;
                            secondMaxW = maxW;
                            maxW = fitData.width;
                        }
                        else if (secondMaxW < fitData.width)
                            secondMaxW = fitData.width;
                    }
                });

            if (!maxWCount)
                break;
            // ***
            float shrinkFromOneCol = std::min(remainToShrink / maxWCount, maxW - secondMaxW);
            if (shrinkFromOneCol < 1.0f)
                break;

            traverseFittables([&lastShrinkedCol, &shrinkFromOneCol, &remainToShrink, &maxW](IFittable& column)
                {
                    FitData fitData = column.fitData();
                    if (fitData.width > fitData.minWidth)
                        if (maxW == fitData.width)
                        {
                            lastShrinkedCol = &column;
                            float takeOffValue = std::min(shrinkFromOneCol, fitData.width - fitData.minWidth);
                            column.takeOffWidth(takeOffValue);
                            remainToShrink -= takeOffValue;
                        }
                });
        }

        // Division error compensation.
        // TODO: is this still needed now that the layout is computed in float space?
        // The loop exits once remainToShrink reaches zero, and one sweep over several
        // equal-width columns can overshoot past it. A negative value here would hand
        // width back instead of taking it off.
        if (lastShrinkedCol && remainToShrink > 0.0f)
        {
            lastShrinkedCol->takeOffWidth(remainToShrink);
        }
    }

}
