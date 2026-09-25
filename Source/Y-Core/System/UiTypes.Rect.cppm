export module ClaFi.Core.System.UiTypes :Rect;

import :Point;
import ClaFi.Core.System.Serialization;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    // One side of a rectangle.
    export enum class RectSide : std::size_t
    {
        Top,
        Right,
        Bottom,
        Left,
        Count
    };

    export constexpr std::size_t k_rectSideTop = static_cast<std::size_t>(RectSide::Top);
    export constexpr std::size_t k_rectSideRight = static_cast<std::size_t>(RectSide::Right);
    export constexpr std::size_t k_rectSideBottom = static_cast<std::size_t>(RectSide::Bottom);
    export constexpr std::size_t k_rectSideLeft = static_cast<std::size_t>(RectSide::Left);
    export constexpr std::size_t k_rectSideCount = static_cast<std::size_t>(RectSide::Count);
    export constexpr RectSide allEdges[k_rectSideCount] = { RectSide::Top, RectSide::Right, RectSide::Bottom, RectSide::Left };
    export using RectSidesBoolArray = std::array<bool, k_rectSideCount>;

    export constexpr RectSidesBoolArray k_allRectSidesTrue = { true, true, true, true };
    export constexpr RectSidesBoolArray k_allRectSidesFalse = { false, false, false, false };

    // One corner of a rectangle.
    export enum class Corner : std::size_t
    {
        TopLeft,
        TopRight,
        BottomRight,
        BottomLeft,
        None
    };
    export constexpr std::size_t k_cornersNum = 4u;
    export [[nodiscard]] constexpr std::size_t cornerIndex(Corner corner) { return static_cast<std::size_t>(corner); }

    // A radius per corner, in Corner order: top left, top right, bottom right, bottom left. A
    // corner is round while its radius is above zero.
    export using CornerRadii = std::array<float, k_cornersNum>;
    export constexpr CornerRadii k_squareCorners = {};
    export [[nodiscard]] constexpr CornerRadii uniformCorners(float radius) { return { radius, radius, radius, radius }; }
    export [[nodiscard]] constexpr bool isSquare(const CornerRadii& radii)
    {
        return radii[0] <= 0.0f && radii[1] <= 0.0f && radii[2] <= 0.0f && radii[3] <= 0.0f;
    }
    export [[nodiscard]] constexpr bool isUniform(const CornerRadii& radii)
    {
        return radii[0] == radii[1] && radii[1] == radii[2] && radii[2] == radii[3];
    }


    export template <ArithmeticType T>
    struct Rect
    {
        T left;
        T top;
        T right;
        T bottom;

        Rect();
        Rect(const T left, const T top, const T right, const T bottom);
        Rect(const Rect&, const T inflate);
        Rect(const Rect&, const T inflateX, const T inflateY);
        Rect(const Rect&, const Point<T> inflate);
        Rect(const Rect&);

        T width() const { return right - left; }
        T height() const { return bottom - top; }

        bool operator==(const Rect& other) const;
        bool operator!=(const Rect& other) const;

        Rect<double> toDouble() const;
        Rect<float> toFloat() const;
        void toFloat(Rect<float>&) const;
        Rect<int> toInt() const { return rounded(); }
        Rect<int> rounded() const;
        Rect<int> roundedOut() const;
        bool empty() const { return (right <= left) || (bottom <= top); }
        void clear() { *this = Rect{}; }
        Rect<float> centerRect(int w, int h) const { return centerRect(static_cast<float>(w), static_cast<float>(h)); }
        Rect<float> centerRect(float sz) const { return centerRect(sz, sz); }
        Rect<float> centerRect(float w, float h) const;
        Rect<float> fitRect(float wRatio, float hRatio) const;
        Rect<float> relativeRect(FloatPoint topLeft, FloatPoint bottomRight) const;
        const float centerX() const { return (left + right) / 2.0f; }
        const float centerY() const { return (top + bottom) / 2.0f; }
        const float relativeX(float x) const { return left + width() * x; }
        const float relativeY(float y) const { return top + height() * y; }
        const Point<float> relativePt(float x, float y) const { return { relativeX(x) , relativeY(y) }; }
        const Point<float> relativePt(FloatPoint pt) const { return relativePt(pt.x, pt.y); }
        const Point<float> center() const { return { centerX(), centerY() }; }
        const Point<float> topCenter() const { return { centerX(), static_cast<float>(top) }; }
        const Point<float> bottomCenter() const { return { centerX(), static_cast<float>(bottom) }; }
        // Corners come out by value. A reference into the four edges let a caller write one
        // corner and silently resize the rect, which reads like a move and is not one. To move a
        // rect use offset() or setTopLeft(); to build one from a corner and a size, fromDimensions().
        Point<T> topLeft() const { return { left, top }; }
        Point<T> bottomLeft() const { return { left, bottom }; }
        Point<T> topRight() const { return { right, top }; }
        Point<T> bottomRight() const { return { right, bottom }; }
        Rect<T> topLeftSquare(T sideSize) const;
        Rect<T> bottomLeftSquare(T sideSize) const;
        Rect<T> topRightSquare(T sideSize) const;
        Rect<T> bottomRightSquare(T sideSize) const;
        Point<T> dimensions() const {   return { width(), height() }; }
        void setDimensions(const Point<T> value);
        void setDimensions(const T w, const T h);
        // Moves the rect so its top-left lands on value. The dimensions are carried along.
        void setTopLeft(const Point<T> value);
        static Rect<T> fromDimensions(Point<T> position, Point<T> dimensions);
        void inflateX(T value);
        void inflateY(T value);
        void inflate(T dx, T dy);
        void inflate(const T value) { inflate(value, value); }
        Rect<T> inflated(T dx, T dy) const;
        Rect<T> inflated(T value) const;
        template<ArithmeticType T2>
        void inflate(const Point<T2> value) { inflate(value.x, value.y); }
        void offset(const T x, const T y);
        void offset(const Point<T>& value)  { offset(value.x, value.y); }
        void unionWith(const Rect&);
        bool intersectWith(const Rect&);
        static  Rect<T> intersection(Rect, const Rect&);
        bool intersects(const Rect&) const;
        bool contains(const T x, const T y) const;
        bool contains(const Point<T> pt) const { return contains(pt.x, pt.y); }
        void clampTo(Rect<T> boundary);
        Point<T> closestPoint(Point<T> value) const;
        Point<T> corner(Corner);
    };

    //static constexpr auto k_serializedFields = std::make_tuple
    //(
    //  SerializedField{ L"Left",   &Rect::left },
    //  SerializedField{ L"Top",    &Rect::top },
    //  SerializedField{ L"Right",  &Rect::right },
    //  SerializedField{ L"Bottom", &Rect::bottom }
    //);
    export template<ArithmeticType T>
    constexpr auto serializedFields(Rect<T>) {
        return std::make_tuple
        (
            SerializedField{ L"Left",   &Rect<T>::left },
            SerializedField{ L"Top",    &Rect<T>::top },
            SerializedField{ L"Right",  &Rect<T>::right },
            SerializedField{ L"Bottom", &Rect<T>::bottom }
        );
    };

    //export using DoubleRect = Rect<double>;
    export using FloatRect  = Rect<float>;
    export using IntRect    = Rect<std::int32_t>; // WinApi compatible
    export using IntMargins = Rect<std::int32_t>; // NOT WinApi compatible (suddenly members order differs)

    export struct CornerNook
    {
        Corner corner;
        FloatPoint pivot; // The center point of the arc's circle
        FloatPoint sharpPt;
        float radius;
        FloatPoint startPt;
        FloatPoint endPt;
        // Constructor 1: Direct assignment
        CornerNook(Corner corner, FloatPoint pivot, float radius);
        // Constructor 2: Calculate pivot from a bounding box
        CornerNook(Corner corner, const FloatRect& rect, float radius);
    private:
        void computeOtherPoints();
    };

    export struct RoundedRectangleParts
    {
        FloatRect bounds;
        CornerRadii radii{ k_squareCorners };
        RectSidesBoolArray sides{ k_allRectSidesTrue };
        bool operator==(const RoundedRectangleParts&) const = default;
    };


    //----------------------------------------------------------------------------


    template<ArithmeticType T>
    Rect<T>::Rect() :
        left{},
        top{},
        right{},
        bottom{}
    {
    }

    template<ArithmeticType T>
    Rect<T>::Rect(const T left, const T top, const T right, const T bottom) :
        left{ left },
        top{ top },
        right{ right },
        bottom{ bottom }
    {
    }

    template<ArithmeticType T>
    Rect<T>::Rect(const Rect& other, const T inflate) :
        left{ other.left - inflate },
        top{ other.top - inflate },
        right{ other.right + inflate },
        bottom{ other.bottom + inflate }
    {
    }

    template<ArithmeticType T>
    Rect<T>::Rect(const Rect& other, const T inflateX, const T inflateY) :
        left{ other.left - inflateX },
        top{ other.top - inflateY },
        right{ other.right + inflateX },
        bottom{ other.bottom + inflateY }
    {
    }

    template<ArithmeticType T>
    Rect<T>::Rect(const Rect& other, const Point<T> inflate) :
        left{ other.left - inflate.x },
        top{ other.top - inflate.y },
        right{ other.right + inflate.x },
        bottom{ other.bottom + inflate.y }
    {
    }

    template<ArithmeticType T>
    Rect<T>::Rect(const Rect& other) :
        left{ other.left },
        top{ other.top },
        right{ other.right },
        bottom{ other.bottom }
    {
    }

    template<ArithmeticType T>
    bool Rect<T>::operator==(const Rect& other) const
    {
        return left == other.left && top == other.top && right == other.right && bottom == other.bottom;
    }

    template<ArithmeticType T>
    bool Rect<T>::operator!=(const Rect& other) const
    {
        return left != other.left || top != other.top || right != other.right || bottom != other.bottom;
    }

    template<ArithmeticType T>
    Rect<double> Rect<T>::toDouble() const
    {
        return {
            static_cast<double>(left),
            static_cast<double>(top),
            static_cast<double>(right),
            static_cast<double>(bottom)
        };
    }

    template<ArithmeticType T>
    Rect<float> Rect<T>::toFloat() const
    {
        return {
            static_cast<float>(left),
            static_cast<float>(top),
            static_cast<float>(right),
            static_cast<float>(bottom)
        };
    }

    template<ArithmeticType T>
    void Rect<T>::toFloat(Rect<float>& outValue) const
    {
        outValue = {
            static_cast<float>(left),
            static_cast<float>(top),
            static_cast<float>(right),
            static_cast<float>(bottom)
        };
    }

    template<ArithmeticType T>
    Rect<int> Rect<T>::rounded() const
    {
        return {
            static_cast<int>(std::round(left)),
            static_cast<int>(std::round(top)),
            static_cast<int>(std::round(right)),
            static_cast<int>(std::round(bottom))
        };
    }

    template<ArithmeticType T>
    Rect<int> Rect<T>::roundedOut() const
    {
        return {
            static_cast<int>(std::floor(left)),
            static_cast<int>(std::floor(top)),
            static_cast<int>(std::ceil(right)),
            static_cast<int>(std::ceil(bottom))
        };
    }

    template<ArithmeticType T>
    Rect<float> Rect<T>::centerRect(float w, float h) const
    {
        Rect<float> result;
        result.left = left + (right - left - w) / 2.0f;
        result.top = top + (bottom - top - h) / 2.0f;
        result.right = result.left + w;
        result.bottom = result.top + h;
        return result;
    }

    template<ArithmeticType T>
    Rect<float> Rect<T>::fitRect(float wRatio, float hRatio) const
    {
        float k = std::max(wRatio / width(), hRatio / height());
        if (k > 0.0)
        {
            wRatio = wRatio / k;
            hRatio = hRatio / k;
        };
        return centerRect(wRatio, hRatio);
    }

    template <ArithmeticType T>
    Rect<float> Rect<T>::relativeRect(FloatPoint topLeft, FloatPoint bottomRight) const
    {
        const FloatPoint dims = dimensions();
        const FloatPoint origin = this->topLeft().toFloat();
        const FloatPoint lt = origin + topLeft * dims;
        const FloatPoint rb = origin + bottomRight * dims;
        return { lt.x, lt.y, rb.x, rb.y };
    }

    template<ArithmeticType T>
    Rect<T> Rect<T>::topLeftSquare(T sideSize) const
    {
        return { left, top, left + sideSize, top + sideSize };
    }

    template<ArithmeticType T>
    Rect<T> Rect<T>::bottomLeftSquare(T sideSize) const
    {
        return { left, bottom - sideSize, left + sideSize, bottom };
    }

    template<ArithmeticType T>
    Rect<T> Rect<T>::topRightSquare(T sideSize) const
    {
        return { right - sideSize, top, right, top + sideSize };
    }

    template<ArithmeticType T>
    Rect<T> Rect<T>::bottomRightSquare(T sideSize) const
    {
        return { right - sideSize, bottom - sideSize, right, bottom };
    }

    template<ArithmeticType T>
    void Rect<T>::setDimensions(const Point<T> value)
    {
        right = left + value.x;
        bottom = top + value.y;
    }

    template<ArithmeticType T>
    void Rect<T>::setDimensions(const T w, const T h)
    {
        right = left + w;
        bottom = top + h;
    }

    template<ArithmeticType T>
    void Rect<T>::setTopLeft(const Point<T> value)
    {
        offset(value.x - left, value.y - top);
    }

    template<ArithmeticType T>
    Rect<T> Rect<T>::fromDimensions(Point<T> position, Point<T> dimensions)
    {
        return { position.x, position.y, position.x + dimensions.x, position.y + dimensions.y };
    }

    template<ArithmeticType T>
    void Rect<T>::inflateX(T value)
    {
        left -= value;
        right += value;
    }

    template<ArithmeticType T>
    void Rect<T>::inflateY(T value)
    {
        top -= value;
        bottom += value;
    }

    template<ArithmeticType T>
    void Rect<T>::inflate(T dx, T dy)
    {
        left -= dx;
        top -= dy;
        right += dx;
        bottom += dy;
    }

    template <ArithmeticType T>
    Rect<T> Rect<T>::inflated(T dx, T dy) const
    {
        return Rect<T>{ *this, dx, dy };
    }

    template <ArithmeticType T>
    Rect<T> Rect<T>::inflated(T value) const
    {
        return Rect<T>{ *this, value };
    }

    template<ArithmeticType T>
    void Rect<T>::offset(const T x, const T y)
    {
        left += x;
        top += y;
        right += x;
        bottom += y;
    }

    template<ArithmeticType T>
    void Rect<T>::unionWith(const Rect& red)
    {
        if (red.empty())
            return;
        if (empty())
        {
            *this = red;
            return;
        }
        left = std::min(left, red.left);
        top = std::min(top, red.top);
        right = std::max(right, red.right);
        bottom = std::max(bottom, red.bottom);
    }

    template<ArithmeticType T>
    bool Rect<T>::intersectWith(const Rect& red)
    {
        left = std::max(left, red.left);
        right = std::min(right, red.right);
        top = std::max(top, red.top);
        bottom = std::min(bottom, red.bottom);
        return !empty();
    }

    template<ArithmeticType T>
    Rect<T> Rect<T>::intersection(Rect rect1, const Rect& rect2)
    {
        rect1.intersectWith(rect2);
        return rect1;
    }

    template<ArithmeticType T>
    bool Rect<T>::intersects(const Rect& red) const
    {
        if (red.empty() || empty())
            return false;
        return !(red.left > right
            || red.right < left
            || red.top > bottom
            || red.bottom < top
            );
    }

    template<ArithmeticType T>
    bool Rect<T>::contains(const T x, const T y) const
    {
        return x >= left && x < right && y >= top && y < bottom;
    }

    template<ArithmeticType T>
    void Rect<T>::clampTo(Rect<T> boundary)
    {
        static auto clampAxis = [](T& posMin, T& posMax, T bMin, T bMax) {
            // 1. If boundary is already fully covered/swallowed, do nothing.
            if (posMin <= bMin && posMax >= bMax) return;

            T size = posMax - posMin;
            T bSize = bMax - bMin;

            if (size <= bSize) {
                // 2. Fits inside: Move minimal distance to stay within edges.
                if (posMin < bMin) { posMin = bMin; posMax = bMin + size; return; }
                if (posMax > bMax) { posMax = bMax; posMin = bMax - size; return; }
            }
            else {
                // 3. Oversized: Move to cover the 'gap' left/top or right/bottom.
                if (posMin > bMin) { posMin = bMin; posMax = bMin + size; return; }
                if (posMax < bMax) { posMax = bMax; posMin = bMax - size; return; }
            }
            };

        clampAxis(left, right, boundary.left, boundary.right);
        clampAxis(top, bottom, boundary.top, boundary.bottom);
    }

    template<ArithmeticType T>
    Point<T> Rect<T>::closestPoint(Point<T> value) const
    {
        return {
            std::clamp(value.x, left, right),
            std::clamp(value.y, top, bottom)
        };
    }

    template<ArithmeticType T>
    Point<T> Rect<T>::corner(Corner value)
    {
        switch (value)
        {
        case Corner::TopLeft: return topLeft(); break;
        case Corner::TopRight: return topRight(); break;
        case Corner::BottomRight: return bottomRight(); break;
        case Corner::BottomLeft: return bottomLeft(); break;
        case Corner::None:
            break;
        }
        unreachable();
    }

    CornerNook::CornerNook(Corner corner, FloatPoint pivot, float radius)
        :
        corner{ corner },
        pivot{ pivot },
        radius{ radius }
    {
        computeOtherPoints();
    }

    CornerNook::CornerNook(Corner corner, const FloatRect& rect, float radius)
        :
        corner{ corner },
        radius{ radius }
    {
        switch (corner) {
        case Corner::TopLeft:
            pivot = { rect.left + radius, rect.top + radius };
            break;
        case Corner::TopRight:
            pivot = { rect.right - radius, rect.top + radius };
            break;
        case Corner::BottomRight:
            pivot = { rect.right - radius, rect.bottom - radius };
            break;
        case Corner::BottomLeft:
            pivot = { rect.left + radius, rect.bottom - radius };
            break;
        case Corner::None:
            break;
        }
        computeOtherPoints();
    }

    void CornerNook::computeOtherPoints()
    {
        switch (corner) {
        case Corner::TopLeft:
            startPt = { pivot.x - radius, pivot.y };
            endPt = { pivot.x, pivot.y - radius };
            sharpPt = { startPt.x, endPt.y };
            break;
        case Corner::TopRight:
            startPt = { pivot.x, pivot.y - radius };
            endPt = { pivot.x + radius, pivot.y };
            sharpPt = { endPt.x, startPt.y };
            break;
        case Corner::BottomRight:
            startPt = { pivot.x + radius, pivot.y };
            endPt = { pivot.x, pivot.y + radius };
            sharpPt = { startPt.x, endPt.y };
            break;
        case Corner::BottomLeft:
            startPt = { pivot.x, pivot.y + radius };
            endPt = { pivot.x - radius, pivot.y };
            sharpPt = { endPt.x, startPt.y };
            break;
        case Corner::None:
            break;
        }
    }

}
