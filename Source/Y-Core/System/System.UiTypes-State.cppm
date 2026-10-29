export module ClaFi.Core.System.UiTypes :State;

import ClaFi.StdLib;

namespace ClaFi
{
    // The states a control is drawn in, indexed for the factor array beside them. See UI-Types
    export enum class VisualStateIndex : std::size_t {
        Hot,       // the user is on it: the mouse over it, or the keys on it in a focused window
        Pressed,
        Focused,   // the control's own focus, by either device
        Selected,
        Enabled,
        Current,   // wears a ring: a container's current item, or the keys' focus
        Count
    };

    // What a control is drawn as, one flag per state.
    export struct VisualState
    {
        bool* asArray() { return &hot; }
        bool hot{};
        bool pressed{};
        bool focused{};
        bool selected{};
        bool enabled{ true };
        bool current{};
    };

    // The same states as animated factors, each a byte standing for zero to one.
    export struct StateFactors
    {
        StateFactors() = default;
        explicit StateFactors(const VisualState&);
        float hotOrSelected(float kSel = 1.0f) const { return compose(hot(), selected() * kSel); }
        float hotOrSelected(float kHot, float kSel) const { return compose(hot() * kHot, selected() * kSel); }
        static float compose(std::initializer_list<float> factors);
        static float compose(const float f1, const float f2) { return compose({ f1, f2 }); }
        static float compose(const float f1, const float f2, const float f3) { return compose({ f1, f2, f3 }); }
        //static float composeZeros(const float f1, const float f2) { return 1.0f - compose(1.0f - f1, 1.0f - f2); }
        float operator[](VisualStateIndex index) const { return (&m_hot)[static_cast<std::size_t>(index)] / 255.0f; }
        float operator[](std::size_t index) const { return (&m_hot)[index] / 255.0f; }
        void set(std::size_t index, float value) { (&m_hot)[index] = toByte(value); }
    public:
        float hot() const { return m_hot / 255.0f; }
        float pressed() const { return m_pressed / 255.0f; }
        float focused() const { return m_focused / 255.0f; }
        float selected() const { return m_selected / 255.0f; }
        float enabled() const { return m_enabled / 255.0f; }
        float current() const { return m_current / 255.0f; }
        void setHot(float value) { m_hot = toByte(value); }
        void setPressed(float value) { m_pressed = toByte(value); }
        void setFocused(float value) { m_focused = toByte(value); }
        void setSelected(float value) { m_selected = toByte(value); }
        void setEnabled(float value) { m_enabled = toByte(value); }
        void setCurrent(float value) { m_current = toByte(value); }
    private:
        static inline std::uint8_t toByte(float value) noexcept;
    private:
        // Optimizing memory footprint
        std::uint8_t m_hot{};
        std::uint8_t m_pressed{};
        std::uint8_t m_focused{};
        std::uint8_t m_selected{};
        std::uint8_t m_enabled{ 255 };
        std::uint8_t m_current{};
    };

    StateFactors::StateFactors(const VisualState& state)
        :
        m_hot{ toByte(static_cast<float>(state.hot)) },
        m_pressed{ toByte(static_cast<float>(state.pressed)) },
        m_focused{ toByte(static_cast<float>(state.focused)) },
        m_selected{ toByte(static_cast<float>(state.selected)) },
        m_enabled{ toByte(static_cast<float>(state.enabled)) },
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
