export module ClaFi.Core.System.Scaler;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    export class Scaler;
    export using ScaleFactorChangeEvent = EventOf<const Scaler>;

    export class Scaler : public EventComponent
    {
    public:
        explicit Scaler(ScaleFactor systemFactor);
        EventConnection onChange(auto&& callback) {
            return connectEvent<ScaleFactorChangeEvent>(std::forward<decltype(callback)>(callback));
        }
        int systemPercent() const;
        void setSystemPercent(float value);
        void setSystemPercent(int value);
        int appPercent() const;
        void setAppPercent(const int value);
        ScaleFactor factor() const { return m_factor; }
        ScaleFactor prevFactor() const { return m_prevFactor; }
        float scale(float) const;
        float scaleBorder(float) const;
        // Whole pixels, at least one; from Thin up each grade is a pixel wider than the one below.
        [[nodiscard]] float scaledStrokeWidth(Thickness) const;
        float scaleF(float) const;
        FloatPoint scaleF(FloatPoint) const;
        FloatPoint scale(FloatPoint) const;
    public:
        float scaled1;
        float scaled2;
        float scaled3;
        float scaled4;
        float scaled5;
        float scaled6;
        float scaled7;
        float scaled8;
        float scaled9;
        float scaled10;
        float scaled11;
        float scaled12;
        float scaled13;
        float scaled14;
        float scaled15;
        float scaled16;
        float scaled17;
        float scaled20;
        float scaled22;
        float scaled24;
        float scaled32;
        float scaled40;
        float scaled48;
        float scaled2F;
        float scaled3F;
        float scaled4F;
        float scaled5F;
        float scaled12F;
    private:
        void factorChanged();
    private:
        ScaleFactor m_systemFactor;
        ScaleFactor m_factor{ 1.0f };
        ScaleFactor m_prevFactor{ 1.0f };
        ScaleFactor m_appFactor{ 1.0f };
    };


    //-------------------------------------------------------------------------


    // Scaler

    Scaler::Scaler(ScaleFactor systemFactor)
        :
        m_systemFactor{ systemFactor }
    {
        factorChanged();
    }

    // THE PERCENT THAT WAS STATED, and rounded rather than truncated to get it back. A percent
    // becomes a factor by a division with no exact answer in a float - 4 of the 76 values
    // between 75 and 150 are such - so multiplying back lands just under and a truncation loses
    // the last unit. A percent that comes back one low makes setSystemPercent and setAppPercent
    // state a factor they already hold, which lays every window out again for nothing, and
    // leaves holdScale copying a scale one unit off the one it is standing in.
    int Scaler::systemPercent() const
    {
        return static_cast<int>(std::round(m_systemFactor * 100.0f));
    }

    void Scaler::setSystemPercent(float value)
    {
        setSystemPercent(static_cast<int>(value));
    }

    void Scaler::setSystemPercent(int value)
    {
        if (systemPercent() == value)
            return;
        m_prevFactor = m_factor;
        m_systemFactor = static_cast<float>(value) / 100.0f;
        factorChanged();
    }

    int Scaler::appPercent() const
    {
        return static_cast<int>(std::round(m_appFactor * 100.0f));
    }

    void Scaler::setAppPercent(int value)
    {
        if (appPercent() == value)
            return;
        m_prevFactor = m_factor;
        m_appFactor = static_cast<float>(value) / 100.0f;
        factorChanged();
    }

    float Scaler::scale(float value) const
    {
        if (value == k_maxFloat)
            return k_maxFloat;
        return std::round(value * m_factor);
    }

    float Scaler::scaleBorder(float value) const
    {
        if (!value)
            return 0.0f;
        if (value < 0.0f)
            return std::min(scale(value), -1.0f);
        return std::max(scale(value), 1.0f);
    }

    float Scaler::scaledStrokeWidth(Thickness thickness) const
    {
        if (thickness == Thickness::None)
            return 0.0f;
        float result = std::max(std::round(strokeWidth(thickness) * m_factor), 1.0f);
        if (thickness > Thickness::Thin)
        {
            const Thickness below = static_cast<Thickness>(static_cast<int>(thickness) - 1);
            result = std::max(result, scaledStrokeWidth(below) + 1.0f);
        }
        return result;
    }

    float Scaler::scaleF(float value) const
    {
        if (!value)
            return 0.0f;
        return value * m_factor;
    }

    FloatPoint Scaler::scaleF(FloatPoint value) const
    {
        return { scaleF(value.x), scaleF(value.y) };
    }

    FloatPoint Scaler::scale(FloatPoint value) const
    {
        return { scale(value.x), scale(value.y) };
    }

    void Scaler::factorChanged()
    {
        m_factor = m_systemFactor * m_appFactor;
        // int
        scaled1 = scale(1.0f);
        scaled2 = scale(2.0f);
        scaled3 = scale(3.0f);
        scaled4 = scale(4.0f);
        scaled5 = scale(5.0f);
        scaled6 = scale(6.0f);
        scaled7 = scale(7.0f);
        scaled8 = scale(8.0f);
        scaled9 = scale(9.0f);
        scaled10 = scale(10.0f);
        scaled11 = scale(11.0f);
        scaled12 = scale(12.0f);
        scaled13 = scale(13.0f);
        scaled14 = scale(14.0f);
        scaled15 = scale(15.0f);
        scaled16 = scale(16.0f);
        scaled17 = scale(17.0f);
        scaled20 = scale(20.0f);
        scaled22 = scale(22.0f);
        scaled24 = scale(24.0f);
        scaled32 = scale(32.0f);
        scaled40 = scale(40.0f);
        scaled48 = scale(48.0f);
        // float
        scaled2F = scaleF(2.0f);
        scaled3F = scaleF(3.0f);
        scaled4F = scaleF(4.0f);
        scaled5F = scaleF(5.0f);
        scaled12F = scaleF(12.0f);

        emitEvent<ScaleFactorChangeEvent>(*this);
    }

}
