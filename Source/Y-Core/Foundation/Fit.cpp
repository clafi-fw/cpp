module ClaFi.Core.Foundation.Fit;

import ClaFi.StdLib;

namespace ClaFi
{
    namespace
    {
        using FitDataList = std::vector<FitData>;

        [[nodiscard]] float leveledWidth(const FitData& data, const float level)
        {
            return std::clamp(level, std::min(data.minWidth, data.width), data.width);
        }

        [[nodiscard]] float leveledTotal(const FitDataList& fits, const float level)
        {
            float result = 0.0f;
            for (const FitData& data : fits)
                result += leveledWidth(data, level);
            return result;
        }

        // The total is linear between neighbouring breaks, so the level is solved for on the one
        // stretch that crosses the target, with no comparing of widths for equality.
        [[nodiscard]] float levelFor(const FitDataList& fits, const float target)
        {
            std::vector<float> breaks;
            for (const FitData& data : fits)
            {
                breaks.push_back(std::min(data.minWidth, data.width));
                breaks.push_back(data.width);
            }
            std::ranges::sort(breaks);
            float below = 0.0f;
            for (const float level : breaks)
            {
                const float total = leveledTotal(fits, level);
                if (total >= target)
                {
                    const float totalBelow = leveledTotal(fits, below);
                    if (total == totalBelow)
                        return level;
                    return below + (level - below) * (target - totalBelow) / (total - totalBelow);
                }
                below = level;
            }
            return below;
        }
    }

    // IFittableList

    void IFittableList::shrinkBy(const float width)
    {
        std::vector<IFittable*> fittables;
        FitDataList fits;
        traverseFittables([&fittables, &fits](IFittable& fittable){
            fittables.push_back(&fittable);
            fits.push_back(fittable.fitData());
        });
        float total = 0.0f;
        for (const FitData& data : fits)
            total += data.width;
        const float level = levelFor(fits, total - width);
        for (std::size_t i = 0; i != fits.size(); ++i)
        {
            const float excess = fits[i].width - leveledWidth(fits[i], level);
            if (excess > 0.0f)
                fittables[i]->takeOffWidth(excess);
        }
    }
}
