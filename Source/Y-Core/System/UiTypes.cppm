export module ClaFi.Core.System.UiTypes;

export import :Keys;
export import :Point;
export import :Rect;
export import :Color;
export import :TextTypes;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    // The size of a cursor image and the point in it that does the pointing.
    export struct CursorInfo
    {
        IntPoint size;
        IntPoint hotSpot;
    };


    // The shape the pointer wears over a control.
    export enum class CursorShape {
        Arrow,
        IBeam,
        Hand,
        Wait
    };

    // The cursor as the platform holds it - a handle on Win32, a shape number on Wayland -
    // answered by Platform::cursor and taken back by Platform::setCursor. Opaque above the
    // platform layer, and never made by hand.
    export struct Cursor
    {
        std::uintptr_t value{ 0 };
    };

    // What the user did that led here, as the display server numbers it. See UI-Types
    export struct InputStamp
    {
        // The server's number for the event. Zero is no event at all.
        std::uint32_t serial{ 0 };
        [[nodiscard]] bool empty() const { return serial == 0; }
    };

    // What a control does with the mouse input the pointer position hands it. See UI-Types
    export enum class HitTest {
        Client,       // the control receives mouse input
        Title,        // dragging it moves the window
        MaxButton,    // it opens the Windows 11 snap menu
        Transparent   // input passes through to the controls or windows underneath
    };

    // How far a control takes part in input - focus, selection, hover. See UI-Types
    export enum class Interactivity {
        None,           // a static label or container, taking no input
        MouseOnly,      // hovered and clicked with the mouse, and never focused
        Focusable,      // a button, a text box, a list item: focus, click and selection
        ActiveContainer // a container managing focus traversal and child selection
    };

    // What the platform layer switches on - window class, styles, activation. See UI-Types
    export enum class WindowRole {
        Dialog,
        Menu,       // menus, dropdowns, date pickers
        Tooltip
    };

    // The platform, as a service standing below the context sees it: a name and nothing else.
    // Each platform layer states what stands behind it, and the service's half for that
    // platform casts to it. See UI-Types
    export class IPlatformServices
    {
    public:
        virtual ~IPlatformServices() = default;
    };

    // What a control does for an action it presents. See UI-Types
    export enum class PresenterRole
    {
        Button,   // delivers the click and shows nothing of the command but its face
        Line,     // delivers the click and writes the name and the key itself
        Display   // shows the command without delivering it
    };

    // Whether arrow navigation runs off one edge of a container and back on at the other.
    export struct NavigationWrap
    {
        bool horizontal{};
        bool vertical{};
    };

    export using ScaleFactor = float;

    template<typename Value, typename Tag>
    struct NumericValueWrapper
    {
        Value value;
        bool operator<(NumericValueWrapper other) const { return value < other.value; }
        bool operator>(NumericValueWrapper other) const { return value > other.value; }
        bool operator<=(NumericValueWrapper other) const { return value <= other.value; }
        bool operator>=(NumericValueWrapper other) const { return value >= other.value; }
        bool operator==(NumericValueWrapper other) const { return value == other.value; }
        bool operator<(Value other) const { return value < other; }
        bool operator>(Value other) const { return value > other; }
        bool operator<=(Value other) const { return value <= other; }
        bool operator>=(Value other) const { return value >= other; }
        bool operator==(Value other) const { return value == other; }
    };

    // Frequency and MilliSeconds can be directly passed to WinApi's ::Beep, ::SetTimer, because bit-wise compatible with uint32_t
    export struct Frequency : public NumericValueWrapper<std::uint32_t, Frequency> {};
    export struct MilliSeconds : public NumericValueWrapper<std::uint32_t, MilliSeconds> {};


    // Which way along an axis a scroll goes.
    export enum class ScrollDirection {
        ToBegin,
        ToEnd
    };

    // Which of a scroll box's two axes is meant.
    export enum class ScrollAxis {
        Vertical, Horizontal
    };

    // What the data says about a control - may it be acted on, is it chosen. See UI-Types
    export struct ActionState
    {
        bool enabled{ true };
        bool selected{ false };
        bool isStateDefault() const { return enabled && !selected; }
    };

    // The states a control is drawn in, indexed for the factor array beside them. See UI-Types
    export enum class VisualStateIndex : std::size_t {
        Hovered,
        Pressed,
        Focused,
        Selected,
        Enabled,
        TextHovered,
        Current,   // the item a container hands the focus back to when it is entered again
        Count
    };

    // What a control is drawn as, one flag per state.
    export struct VisualState
    {
        bool* asArray() { return &hovered; }
        bool hovered{};
        bool pressed{};
        bool focused{};
        bool selected{};
        bool enabled{ true };
        bool textHovered{};
        bool current{};
    };

    // The same states as animated factors, each a byte standing for zero to one.
    export struct StateFactors
    {
        StateFactors() = default;
        StateFactors(const VisualState&);
        float hoveredOrSelected(float kSel = 1.0f) const { return compose(hovered(), selected() * kSel); }
        float hoveredOrSelected(float kHover, float kSel) const { return compose(hovered() * kHover, selected() * kSel); }
        static float compose(std::initializer_list<float> factors) {
            float inverse_product = 1.0f;
            for (float f : factors) {
                inverse_product *= (1.0f - std::clamp(f, 0.0f, 1.0f));
            }
            return 1.0f - inverse_product;
        }
        static float compose(const float f1, const float f2) { return compose({ f1, f2 }); }
        static float compose(const float f1, const float f2, const float f3) { return compose({ f1, f2, f3 }); }
        //static float composeZeros(const float f1, const float f2) { return 1.0f - compose(1.0f - f1, 1.0f - f2); }
        float operator[](VisualStateIndex index) const { return (&m_hovered)[static_cast<std::size_t>(index)] / 255.0f; }
        float operator[](std::size_t index) const { return (&m_hovered)[index] / 255.0f; }
        void set(std::size_t index, float value) { (&m_hovered)[index] = toByte(value); }
    public:
        float hovered() const { return m_hovered / 255.0f; }
        float pressed() const { return m_pressed / 255.0f; }
        float focused() const { return m_focused / 255.0f; }
        float selected() const { return m_selected / 255.0f; }
        float enabled() const { return m_enabled / 255.0f; }
        float textHovered() const { return m_textHovered / 255.0f; }
        float current() const { return m_current / 255.0f; }
        void setHovered(float value) { m_hovered = toByte(value); }
        void setPressed(float value) { m_pressed = toByte(value); }
        void setFocused(float value) { m_focused = toByte(value); }
        void setSelected(float value) { m_selected = toByte(value); }
        void setEnabled(float value) { m_enabled = toByte(value); }
        void setTextHovered(float value) { m_textHovered = toByte(value); }
        void setCurrent(float value) { m_current = toByte(value); }
    private:
        static inline std::uint8_t toByte(float value) noexcept {
            // Clamping ensures values staying slightly out of bounds don't wrap integers
            float clamped = std::clamp(value, 0.0f, 1.0f);
            return static_cast<std::uint8_t>(clamped * 255.0f + 0.5f);
        }
    private:
        // Optimizing memory footprint
        std::uint8_t m_hovered{};
        std::uint8_t m_pressed{};
        std::uint8_t m_focused{};
        std::uint8_t m_selected{};
        std::uint8_t m_enabled{ 255 };
        std::uint8_t m_textHovered{};
        std::uint8_t m_current{};
    };

    StateFactors::StateFactors(const VisualState& state)
        :
        m_hovered{ toByte(static_cast<float>(state.hovered)) },
        m_pressed{ toByte(static_cast<float>(state.pressed)) },
        m_focused{ toByte(static_cast<float>(state.focused)) },
        m_selected{ toByte(static_cast<float>(state.selected)) },
        m_enabled{ toByte(static_cast<float>(state.enabled)) },
        m_textHovered{ toByte(static_cast<float>(state.textHovered)) },
        m_current{ toByte(static_cast<float>(state.current)) }
    {
    }

    // Which device the user last acted with. See UI-Types
    export enum class InputDevice {
        Mouse = 0,
        Keyboard
    };

    // ControlFlags are declared here but there are no public members of this type exist.
    // Only one out there declared in the BaseControl class and it's private.
    // But the type is here because it's used in the Horizontal/VericalAlign enums,
    // and they are public
    export using ControlFlag = FlagByte ;
    export using ControlFlags = FlagByte ;
    export using ControlFlags2 = FlagByte ;
    //
    export constexpr ControlFlag cfHorizontalAlignBit1 = 1;
    export constexpr ControlFlag cfHorizontalAlignBit2 = 2;
    export constexpr ControlFlag cfHorizontalAlignMask = cfHorizontalAlignBit1 | cfHorizontalAlignBit2;
    export constexpr ControlFlag cfVerticalAlignBit1 = 4;
    export constexpr ControlFlag cfVerticalAlignBit2 = 8;
    export constexpr ControlFlag cfVerticalAlignMask = cfVerticalAlignBit1 | cfVerticalAlignBit2;
    export constexpr ControlFlag cfHidden = 16;
    export constexpr ControlFlag cfTextTrimmed = 32;
    // Set on the CONTAINER, not on the child: what it says is that the collection is no
    // longer ordered by geometry - see Control::firstChildInViewport.
    export constexpr ControlFlag cfChildHidden = 64;

    // Second flagset of ControlBase
    export constexpr ControlFlag cfPainted      = 1;
    export constexpr ControlFlag cfTextDrawn    = 2;
    export constexpr ControlFlag cfVerticalTextAnchorBit1 = 4;
    export constexpr ControlFlag cfVerticalTextAnchorBit2 = 8;
    export constexpr ControlFlag cfVerticalTextAnchorMask = cfVerticalTextAnchorBit1 | cfVerticalTextAnchorBit2;
    export constexpr ControlFlag cfHorizontalTextAnchorBit1 = 16;
    export constexpr ControlFlag cfHorizontalTextAnchorBit2 = 32;
    export constexpr ControlFlag cfHorizontalTextAnchorMask = cfHorizontalTextAnchorBit1 | cfHorizontalTextAnchorBit2;
    export constexpr ControlFlag cfNoWordWrap   = 64;
    export constexpr ControlFlag cfReserved7    = 128;

    // What the second flagset holds at rest: text anchored Top and Left, the rest clear.
    export constexpr ControlFlags2 cfDefaultFlags2 = cfVerticalTextAnchorBit1 | cfHorizontalTextAnchorBit1;

    // Where a control sits across the space its parent gives it.
    export enum class HorizontalAlign : FlagByte {
        Fill = 0,
        Left = cfHorizontalAlignBit1,                           /* 1U */
        Center = cfHorizontalAlignBit2,                         /* 2U */
        Right = cfHorizontalAlignBit1 | cfHorizontalAlignBit2   /* 3U */
    };

    // Where a control sits down the space its parent gives it.
    export enum class VerticalAlign : FlagByte {
        Fill = 0,
        Top = cfVerticalAlignBit1,                          /*  4U */
        Center = cfVerticalAlignBit2,                       /*  8U */
        Bottom = cfVerticalAlignBit1 | cfVerticalAlignBit2  /* 12U */
    };

    // Where the measured text block sits down its zone, moving as a whole and drawing alone.
    export enum class VerticalTextAnchor : FlagByte
    {
        Top = cfVerticalTextAnchorBit1,
        Center = cfVerticalTextAnchorBit2,
        Bottom = cfVerticalTextAnchorBit1 | cfVerticalTextAnchorBit2
    };

    // Where the text block sits across its zone, and the box TextAlign works in. See UI-Types
    export enum class HorizontalTextAnchor : FlagByte
    {
        Left = cfHorizontalTextAnchorBit1,
        Center = cfHorizontalTextAnchorBit2,
        Right = cfHorizontalTextAnchorBit1 | cfHorizontalTextAnchorBit2
    };

    // Whether a text is broken to the box it is drawn in. See UI-Types
    export enum class WordWrap : FlagByte
    {
        Yes = 0,
        No = cfNoWordWrap
    };

    // Both text anchors together, which is how every mapping reads them. See UI-Types
    export struct TextAnchor
    {
        VerticalTextAnchor vertical{ VerticalTextAnchor::Top };
        HorizontalTextAnchor horizontal{ HorizontalTextAnchor::Left };

        bool operator==(const TextAnchor&) const = default;
    };

    export using TagValue = std::uintptr_t;

    template<typename T>
    concept IntegralOrEnum = std::is_integral_v<T> || std::is_enum_v<T>;
    /*

    Tag Struct
        A memory - efficient container that packs pointers, enums, or multiple integers into a single uintptr_t.It maintains a size exactly equal to sizeof(void*).

    Usage Reference
    Operation   Syntax  Description
    Store Pointer   Tag t(ptr); Stores address in value.
    Store Multi Tag t(v1, v2);  Packs values into bit-segments.
    Access Full t.get<T*>();    Reinterprets the entire value.
    Access Segment  t.get<0, 2, T>();   Extracts the first of two segments.
    Step    ++t Increments internal value by 1.
    Post-Step   t++ Increments internal value by 1 (returns old copy).
    Offset  t += n  Adds n bytes/units to the internal value.
    Binary  t + n   Returns a new Tag with value + n.

    Constraints
    Alignment: Multi-value packing assumes the input values fit within the calculated segment bit-width (e.g., 32 bits for halves on a 64-bit system).
    All arithmetic operations on Tag are byte-aligned. If Tag holds a pointer to a double, t += 1 moves the address by 1 byte, not 8.
    Portability: Segment sizes scale automatically based on the architecture's uintptr_t width.

    Technical Notes
    Pointer Arithmetic: If the Tag holds a pointer (e.g., int*), ++tag increments by 1 byte, not sizeof(int). To increment by the type size, you must cast it back: t = t.get<int*>() + 1;.
    Packed Data: Do not use ++ if the Tag contains packed segments (halves/quarters), as the carry bit from the low segment will bleed into the high segment, corrupting the data.

    */
    export struct Tag {
        std::uintptr_t value;

        constexpr Tag() : value{ 0 } {}

        // --- Constructors ---

        // Pointer storage (void* handles any pointer type)
        Tag(const void* ptr) : value{ reinterpret_cast<std::uintptr_t>(ptr) } {}

        // Single integer/enum storage
        template<IntegralOrEnum T>
        constexpr Tag(T val) : value{ static_cast<std::uintptr_t>(val) } {}

        // Multi-value storage (packs 2, 4, or 8 values)
        template<IntegralOrEnum... Ts>
            requires (sizeof...(Ts) > 1)
        constexpr Tag(Ts... args) : value(0) {
            constexpr std::size_t count = sizeof...(Ts);
            constexpr std::size_t bits = (sizeof(std::uintptr_t) * 8) / count;
            constexpr std::uintptr_t mask = (std::uintptr_t(1) << bits) - 1;

            std::size_t shift = 0;
            // Fold expression: packs args from right to left (or left to right)
            ((value |= (static_cast<std::uintptr_t>(args) & mask) << (shift++ * bits)), ...);
        }

        // --- Getters ---

        // Reinterpret full value as pointer or specific type
        template<typename T>
        T get() const {
            if constexpr (std::is_pointer_v<T>)
                return reinterpret_cast<T>(value);
            else
#pragma warning(push)
#pragma warning(disable : 4244) // 'argument': conversion from 'const uintptr_t' to 'ClaFi::ColorAsUint', possible loss of data
                return static_cast<T>(value);
#pragma warning(pop)
        }

        /**
         * Optimized Indexed Getter
         * @tparam N The index (e.g., 0-1 for halves, 0-3 for quarters)
         * @tparam Total The total number of segments (2, 4, or 8)
         * @tparam T The return type
         */
        template<std::size_t N, std::size_t Total, typename T>
            requires (Total > 0 && N < Total)
        T get() const {
            constexpr std::size_t bits = (sizeof(std::uintptr_t) * 8) / Total;
            constexpr std::uintptr_t mask = (std::uintptr_t(1) << bits) - 1;
            return static_cast<T>((value >> (N * bits)) & mask);
        }

        // Comparing two of the SAME derived type: hidden friends, so that both operands
        // are explicit parameters.
        //
        // As a member this is ambiguous. The implicit object argument needs a
        // derived-to-base conversion while the other operand is an exact match, and C++20
        // also synthesises the reversed candidate `b == a`, which has exactly the same
        // conversions - leaving nothing to break the tie (MSVC C2666). Two explicit
        // parameters make both candidates symmetric, so the "not a rewritten candidate"
        // tie-break applies and the normal one wins.
        friend bool operator==(const Tag& a, const Tag& b) { return a.value == b.value; }
        friend bool operator>(const Tag& a, const Tag& b) { return a.value > b.value; }

        // Comparison against enums, integers and pointers. Constrained to exclude Derived
        // so it never competes with the friends above - and so that comparing two
        // DIFFERENT derivations does not compile, which is the point of deriving rather
        // than aliasing: the else branch cannot static_cast another TagBase to uintptr_t.
        template<typename T>
            requires (!std::is_same_v<std::decay_t<T>, Tag>)
        bool operator==(const T& other) const {
            if constexpr (std::is_pointer_v<T>) return value == reinterpret_cast<std::uintptr_t>(other);
            else return value == static_cast<std::uintptr_t>(other);
        }
        template<typename T>
            requires (!std::is_same_v<std::decay_t<T>, Tag>)
        bool operator>(const T& other) const {
            if constexpr (std::is_pointer_v<T>) return value > reinterpret_cast<std::uintptr_t>(other);
            else return value > static_cast<std::uintptr_t>(other);
        }

        // Pre-increment: ++t
        Tag& operator++() {
            ++value;
            return static_cast<Tag&>(*this);
        }

        // Post-increment: t++
        Tag operator++(int) {
            Tag temp = static_cast<Tag&>(*this);
            ++value;
            return temp;
        }

        // Pre-decrement: --t
        Tag& operator--() {
            --value;
            return static_cast<Tag&>(*this);
        }

        // Post-decrement: t--
        Tag operator--(int) {
            Tag temp = static_cast<Tag&>(*this);
            --value;
            return temp;
        }
    };
    static_assert(sizeof(Tag) == sizeof(void*), "Tag overhead detected!");

    // The Tag a grid group's span row is given.
    export struct SpanTag
    {
        Tag value;
    };

    // Where a form is put when it is shown. See UI-Types
    export enum class FormPlacement {
        Default,
        ScreenRight,
        OverText,
        Bottom,
        Top,
        ContextMenu,
        Mouse
    };
    export using TooltipPlacement = FormPlacement;

    // Which way a form's size and its content settle against each other. See UI-Types
    export enum class AutoFit
    {
        No,    // the content is laid out into the window
        Yes    // the window is placed again around whatever the content came out as
    };

    export struct KeyModifiers
    {
        bool shift{};
        bool ctrl{};
        bool alt{};
        // No modifier is held - none of the three, not merely not all of them. `!(shift && ctrl)`
        // would be true whenever either was up, counting a plain Shift press as unmodified.
        [[nodiscard]] bool empty() const { return !(shift || ctrl || alt); }
    };

    // TODO: move BakedPathCommand and BakedPathPoint to the CPU modules - nothing else uses them.
    // Whether a baked path point begins a run or continues one.
    export enum class BakedPathCommand { Move, Line };
    export struct BakedPathPoint {
        BakedPathPoint(FloatPoint p, BakedPathCommand c) : coord(p), command(c) {}
        BakedPathPoint(float x, float y, BakedPathCommand c) : coord{ x,y }, command(c) {}
        FloatPoint coord;
        BakedPathCommand command;
    };

    export struct CustomFloatPoint : public FloatPoint
    {
    public:
        // using FloatPoint::FloatPoint;
        // redefine only to fix intellisense

        constexpr CustomFloatPoint(float x, float y)
            :
            FloatPoint{ x, y }
        {
        }
        constexpr CustomFloatPoint(float xy)
            :
            FloatPoint{ xy }
        {
        }

        constexpr CustomFloatPoint(const FloatPoint& value)
            :
            FloatPoint{ value }
        {
        }
        constexpr CustomFloatPoint()
            :
            FloatPoint{}
        {
        }
    };
    export struct Padding : public CustomFloatPoint { using CustomFloatPoint::CustomFloatPoint; };
    export struct Spacing : public CustomFloatPoint { using CustomFloatPoint::CustomFloatPoint; };
    export struct MinSize : public CustomFloatPoint { using CustomFloatPoint::CustomFloatPoint; };
    export struct PreferredSize : public CustomFloatPoint { using CustomFloatPoint::CustomFloatPoint; };
    export struct MaxSize : public CustomFloatPoint { using CustomFloatPoint::CustomFloatPoint; };
    export struct FixedSize : public CustomFloatPoint { using CustomFloatPoint::CustomFloatPoint; };
    export struct ContentSize : public CustomFloatPoint { using CustomFloatPoint::CustomFloatPoint; };
    // How heavy a stroke is, from no stroke up. Each grade's design width is strokeWidth's answer.
    export enum class Thickness
    {
        None,
        Hairline,
        Thin,
        Regular,
        Bold,
        Heavy
    };

    // A grade's design width in points. Scaler::scaledStrokeWidth turns it into device pixels.
    export [[nodiscard]] constexpr float strokeWidth(Thickness thickness)
    {
        switch (thickness)
        {
            case Thickness::None:
                return 0.0f;
            case Thickness::Hairline:
                return 0.51f;
            case Thickness::Thin:
                return 0.83f;
            case Thickness::Regular:
                return 1.5f;
            case Thickness::Bold:
                return 2.0f;
            case Thickness::Heavy:
                return 4.0f;
        }
        unreachable("strokeWidth: a Thickness with no width");
    }

    export struct Border { Thickness value; };
    export struct Radius { float value; };

    export struct DesignDimensions : public CustomFloatPoint { using CustomFloatPoint::CustomFloatPoint; };

    // Which pass of the layout and paint cycle an event belongs to.
    export enum class EventPhase {
        Calculate,
        Paint
    };

    // Whether what an overlay control paints is held to its own bounds.
    export enum class ClippingMode
    {
        Standard,
        Unbounded
    };

    export struct OverlayEntry
    {
        // we use control only to compare with an actual control pointer,
        // received in the traverse event, so there's no need for a real type here
        const void* control;
        ClippingMode clippingMode{ ClippingMode::Standard };
    };

    export struct TabColors
    {
        Color surface{};
        Color lineCaps{};
        Color tabLine{};
        Color indicator{};
        float indicatorFactor{};
    };

    // Which edge of the box a tab strip runs along.
    export enum class TabsOrientation
    {
        HorizontalTop,
        HorizontalBottom,
        VerticalLeft,
        VerticalRight
    };

    export struct TabGeometry
    {
        FloatPoint origin;
        TabsOrientation orientation;
        float lineStart;
        float lineEnd;
        float tabStart;
        float tabEnd;
        float tabProtrusion;
        float cornerRadius;
        float lineWidth;
        float indicatorHeight;
    };

    // Whether a two-state control is checked.
    export enum class Checked
    {
        No,
        Yes
    };


    // Where a mark points, as a fraction of a full turn clockwise from pointing down. See UI-Types
    export namespace ChevronTurn
    {
        constexpr float down = 0.0f;
        constexpr float left = 0.25f;
        constexpr float up = 0.5f;
        constexpr float right = -0.25f;
    }

    // Below: the vocabulary the controls under Controls/Base take as construction properties.
    // It lives here so that using Button costs no import of the base it is built on.

    // Whether a button shows its selection indicator, and when.
    export enum class IndicatorVisibility
    {
        None,
        Always,
        Hover
    };

    // What a button's selection indicator is drawn as.
    export enum class IndicatorStyle
    {
        Check,    // a checkbox
        Radio,    // a radio button
        Custom    // the derived class paints its own
    };

    // Whether a button's selection is shown on its surface.
    export enum class ShowSelectionOnSurface
    {
        No,
        Yes
    };

    // Whether a button carries a surface when nothing is happening to it. See UI-Types
    export enum class ShowSurfaceAtRest
    {
        No,
        Yes
    };

    // Where a button's selection indicator sits.
    export enum class IndicatorPlacement
    {
        LeftCenter,
        TopLeft,
        TopLeftIn
    };

    // Design size of the icon a control draws.
    export struct IconSize : public DesignDimensions
    {
        using DesignDimensions::DesignDimensions;
    };

    // How a button lays its icon out against its text.
    export enum class ButtonViewMode
    {
        TextLabel,
        LeftIcon,
        TopLeftIcon,
        TopCenterIcon,
        BottomIcon,
        IconOnly
    };

    // The modes that put the icon above the text, whatever its own horizontal placement.
    export constexpr bool hasTopIcon(ButtonViewMode value)
    {
        return value == ButtonViewMode::TopLeftIcon || value == ButtonViewMode::TopCenterIcon;
    }

    // Whether the command a button stands for opens a window of its own. See Controls-Base
    export enum class OpensWindow
    {
        No,
        Yes
    };

    // Which side of the control a secondary part sits on.
    export enum class SecondaryEdge
    {
        Right,
        Bottom
    };

    // Where the dropdown mark sits, and whether the control has a strip of its own. See UI-Types
    export enum class ArrowPlacement
    {
        Auto,        // resolved from the view mode: a top icon gives Bottom, anything else Right
        InText,      // the mark rides in the control's own text; there is no separate strip
        Right,       // a strip down the right hand side
        Bottom       // a strip across the bottom
    };

    // Where the dropdown mark points with the popup closed and with it open. See UI-Types
    export struct DropdownMarkTurn
    {
        float closed{ ChevronTurn::down };
        float open{ ChevronTurn::up };
    };

    // Design width of the dropdown strip, which only ArrowPlacement::Right reads. See UI-Types
    export struct DropdownWidth
    {
        float value{ 18.0f };
    };

    // What an expander draws of itself, and what its header is built from. See UI-Types
    export enum class ExpanderViewMode
    {
        Section,
        Divider,
        TreeNode
    };

    // Where a panel puts its own text.
    export enum class TextPlacement
    {
        Body,
        Top,
        Left
    };

    // A width to hold a panel's text to, or nothing to let it size itself.
    export struct FixedTextWidth
    {
        const std::optional<float> value;
    };

    // Where a bar sits in a panel, in the order the panel is read. See UI-Types
    export enum class PanelSlot
    {
        Top,
        Left,
        Body,
        Right,
        Bottom,
        Count
    };

    // Whether a slider carries the stepping buttons at its ends.
    export enum class ScrollButtons
    {
        Yes,
        No
    };

    // What a scrollable control reports about its range: the page it shows, and the whole.
    export struct ScrollInfo
    {
        float page;
        float max;
        float maxPos() const { return (std::max)(max - page, 0.0f); }
    };

    // Whether the control reads as a slider or as a scroll bar.
    export enum class SliderViewMode
    {
        Slider,
        ScrollBar
    };

    // The picture a dialog leads with, one name per kind of thing a dialog is about. See UI-Types
    export enum class MessageIcon
    {
        None,
        Warning,
        Error,
        Question,
        Information,
        Ok
    };

    // One answer a dialog offers.
    export enum class DialogButton
    {
        Ok,
        Cancel,
        Yes,
        No,
        Save,
        Discard
    };

    // What an in-place editor is for, and whether one opens. See UI-Types
    export enum class EditorMode
    {
        None,     // nothing opens, and the gesture goes on meaning whatever else it meant
        ReadOnly, // the value is shown whole, selected and copied; nothing is written back
        Editable  // the value is typed, and offered back to its source
    };

    // ItemIndex

    export using ItemIndexValue = std::optional<std::size_t>;

    export struct ItemIndex
    {
        ItemIndexValue value{};
    };

    // The size the items of a list draw their icons at.
    export struct ItemsIconSize : public DesignDimensions
    {
        using DesignDimensions::DesignDimensions;
    };

    // How those items lay an icon out against their text.
    export struct ItemsViewMode
    {
        ButtonViewMode value;
    };

}
