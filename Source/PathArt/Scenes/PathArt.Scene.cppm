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

    // A picture that moves through a cycle and starts it again without a seam. See PathArt
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
        // The scene at `cycleTime` of its cycle, 0 to 1, into bounds. See PathArt
        virtual void draw(Graphics::Canvas&, const FloatRect& bounds, float cycleTime) = 0;
    private:
        // The cycle a scene runs through unless it states its own.
        static constexpr std::chrono::seconds k_cycle{ 40 };
    };
}
