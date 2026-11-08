export module ClaFi.PathArt.Scene;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // What a scene's frames depend on besides the time. See PathArt
    export struct SceneParams
    {
        FloatRect bounds{};
        Graphics::Matrix3x2 transform{}; // the canvas's, from the bounds' coordinates to pixels
        Graphics::BackendType backend{ Graphics::BackendType::Cpu };

        bool operator==(const SceneParams&) const = default;
    };

    // How a scene that offers a choice of looks is drawn. See PathArt#scenelook
    export enum class SceneLook
    {
        Stylized,   // flat shapes in a few tones
        Realistic   // soft shading, the way a photograph shows it
    };

    // When a frame falls: how far into its cycle, and how many cycles came before. See PathArt
    export struct SceneTime
    {
        float cycleTime{}; // 0 to 1
        std::int64_t cyclesPassed{}; // for a motion that never comes round
    };

    // A moving picture, drawn at a time into any bounds. See PathArt
    export class Scene
    {
    public:
        virtual ~Scene() = default;
    public:
        // What the scene is called where it is offered to pick.
        [[nodiscard]] virtual std::wstring_view name() const = 0;
        // How long one cycle lasts at speed 1.
        [[nodiscard]] virtual std::chrono::seconds cycleDuration() const { return k_cycle; }
        // Readies what the scene keeps between frames for the params it is drawn with. See PathArt
        virtual void prepare(const SceneParams&) {}
        // The scene at a time, into bounds. See PathArt
        virtual void draw(Graphics::Canvas&, const FloatRect& bounds, SceneTime) = 0;
        // Whether the scene draws differently in each look, so a choice of them is worth offering.
        [[nodiscard]] virtual bool offersLooks() const { return false; }
        // The look the scene is drawn in, where it offers looks. See PathArt#scenelook
        [[nodiscard]] SceneLook look() const { return m_look; }
        void setLook(SceneLook look) { m_look = look; }
    private:
        // The cycle a scene runs through unless it states its own.
        static constexpr std::chrono::seconds k_cycle{ 40 };
        SceneLook m_look{ SceneLook::Stylized };
    };
}
