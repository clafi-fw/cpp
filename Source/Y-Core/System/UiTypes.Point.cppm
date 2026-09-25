export module ClaFi.Core.System.UiTypes :Point;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    export template <ArithmeticType T>
    struct Point
    {
        T x;
        T y;
        constexpr Point<T>();
        constexpr Point<T>(const T oneValue);
        constexpr Point<T>(const T x, const T y);
        constexpr bool operator == (const Point other) const { return other.x == x && other.y == y; }

        Point<T> operator-() const { return Point<T>{ static_cast<T>(-x) , static_cast<T>(-y) }; }

        template <typename T2>
        Point<T> operator+(const Point<T2>& other) const { return { x + other.x, y + other.y }; }
        template <typename T2>
        Point<T> operator-(const Point<T2>& other) const { return { x - other.x, y - other.y }; }
        template <typename T2>
        Point<T> operator*(const Point<T2>& other) const { return { x * other.x, y * other.y }; }
        template <typename T2>
        Point<T> operator*(const T2 multiplier) const { return { x * multiplier, y * multiplier }; }
        template <typename T2>
        Point<T> operator*=(const T2 multiplier) { return { x *= multiplier, y *= multiplier }; }

        template <typename T2>
        Point<T> operator+=(const T2 other) {
            return { x += static_cast<T>(other), y += static_cast<T>(other) };
        }
        template <typename T2>
        Point<T> operator+=(const Point<T2> other) { return { x += other.x, y += other.y }; }
        template <typename T2>
        Point<T> operator-=(const T2 other) { return { x -= other, y -= other }; }
        template <typename T2>
        Point<T> operator-=(const Point<T2> other) { return { x -= other.x, y -= other.y }; }

        void offset(const Point<T> pt) { x += pt.x; y += pt.y; }
        void offset(const T ax, const T ay) { x += ax; y += ay; }
        const Point<double> toDouble() const { return { static_cast<double>(x), static_cast<double>(y) }; }
        const Point<float> toFloat() const { return { static_cast<float>(x), static_cast<float>(y) }; }
        const Point<int> toInt() const
        {
            // Though it may seem reasonable to the result here be rounded and not truncated as it is, there's some caveat.
            // There is no way to properly do it, because the round direction depends on the *canvas* axis.
            //
            // For example a point at x == 0.51, if you round it, becomes a 1
            // and that 1 falls outside of a range from 0 to 0.9, and even of a range from 0.0 to 1.0.
            // That's so because the end of the range typically belongs to the next range (from 1.0 to 2.0 in this case),
            // so by just rounding the original coord was moved out of its own range, which is obviously wrong.
            //
            // Note: Concept of such axis kind of introduced in the ShapePainter class.
            //
            // At the current moment toInt() is already used in fillRectF() to get the pixel's coords
            // from a float point, and if you change it here it will break things there
            //
            return { static_cast<int>(x), static_cast<int>(y) };
        }
        const Point<int> roundOut() const
        {
            return {
                static_cast<int>(std::ceil(x)),
                static_cast<int>(std::ceil(y))
            };
        }
    };

    template<ArithmeticType T>
    constexpr Point<T>::Point()
        :
        x{},
        y{}
    {
    }

    template<ArithmeticType T>
    constexpr Point<T>::Point(const T oneValue)
        :
        x{ oneValue },
        y{ oneValue }
    {
    }

    template<ArithmeticType T>
    constexpr Point<T>::Point(const T x, const T y)
        :
        x{ x },
        y{ y }
    {
    }

    export using FloatPoint = Point<float>;
    export using DoublePoint = Point<double>;
    export using IntPoint = Point<int>;

    export float distanceBetweenPoints(IntPoint pt1, IntPoint pt2)
    {
        float dx = static_cast<float>(pt1.x - pt2.x);
        float dy = static_cast<float>(pt1.y - pt2.y);
        return std::sqrt(dx * dx + dy * dy);
    }

    export float distanceBetweenPoints(FloatPoint pt1, FloatPoint pt2)
    {
        float dx = pt1.x - pt2.x;
        float dy = pt1.y - pt2.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    struct FormSpaceTag {};
    struct ControlSpaceTag {};
    template<typename Space>
    struct PointInSpace : public FloatPoint {
        using FloatPoint::FloatPoint;
        // temporary implicit convertor - remove it when the old code that uses IntPoints is cleared.
        PointInSpace(FloatPoint value) : PointInSpace{ value.x, value.y } {}
    };
    export using PointInForm = PointInSpace<FormSpaceTag>;
    export using PointInControl = PointInSpace<ControlSpaceTag>;

    export using IntSize = IntPoint;
    export using FloatSize = FloatPoint;

    export using ScaledPosition = FloatPoint;
    export using ScaledDimensions = FloatPoint;
    export using CalculatedDimensions = FloatPoint;
    export using ScaledMaxSize = FloatPoint;
    export using ScaledPadding = FloatPoint;
    export using ScaledSpacing = FloatPoint;



}
