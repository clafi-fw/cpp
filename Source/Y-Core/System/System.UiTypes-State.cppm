export module ClaFi.Core.System.UiTypes :State;

import ClaFi.StdLib;

namespace ClaFi
{
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
        explicit StateFactors(const VisualState&);
        float hoveredOrSelected(float kSel = 1.0f) const { return compose(hovered(), selected() * kSel); }
        float hoveredOrSelected(float kHover, float kSel) const { return compose(hovered() * kHover, selected() * kSel); }
        static float compose(std::initializer_list<float> factors);
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
        static inline std::uint8_t toByte(float value) noexcept;
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

    float StateFactors::compose(std::initializer_list<float> factors)
    {
        float inverseProduct = 1.0f;
        for (float f : factors)
        {
            inverseProduct *= (1.0f - std::clamp(f, 0.0f, 1.0f));
        }
        return 1.0f - inverseProduct;
    }

    std::uint8_t StateFactors::toByte(float value) noexcept
    {
        // Clamping ensures values staying slightly out of bounds don't wrap integers
        const float clamped = std::clamp(value, 0.0f, 1.0f);
        return static_cast<std::uint8_t>(clamped * 255.0f + 0.5f);
    }
}
