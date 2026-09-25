export module ThisApp.FloorSlider;

import ClaFi.Controls.Slider;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // The colour a floor ramp takes its hue and saturation from, asked at paint time.
    export using OnGetFloorBase = std::function<Hsl()>;

    // A dark mode floor slider whose slot shows a surface at elevation 0 lifted to each floor.
    export class FloorSlider : public Slider
    {
    public:
        template <typename... Args>
        explicit FloorSlider(const CreateParams&, Args&&...);
        // Where the ramp takes its hue and saturation from, bound before the first paint.
        void bind(OnGetFloorBase);
    protected:
        void paintSlot(PaintEvent&, const FloatRect&, SlotSpan) override;
        Color thumbColor(PaintEvent&, FloatPoint&) override;
    private:
        // The base at elevation 0 in dark mode, on the floor a relative position stands for.
        [[nodiscard]] Color sampleAt(float position, const Hsl& base) const;
    private:
        // Enough to follow an HSL to RGB curve, as a rule ramp's stops do.
        static constexpr std::size_t k_stopCount{ 9ull };
        OnGetFloorBase m_base{};
    };


    //----------------------------------------------------------------------------


    template <typename... Args>
    FloorSlider::FloorSlider(const CreateParams& params, Args&&... args)
        :
        Slider{ params, std::forward<Args>(args)... }
    {
    }
}
