export module ClaFi.PathArt.Scene;

import ClaFi.Core.DomEngine;
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

    // A choice a scene offers beside its look: its name and its options' names. See PathArt
    export struct SceneChoice
    {
        std::wstring_view name;
        std::span<const std::wstring_view> options;
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
        // The choices the scene offers beside its look, none unless it states them. See PathArt
        [[nodiscard]] virtual std::span<const SceneChoice> choices() const { return {}; }
        // The option taken in a choice, both at their indices.
        [[nodiscard]] virtual std::size_t chosenOption(std::size_t) const { return 0; }
        // Takes an option in a choice, both at their indices.
        virtual void chooseOption(std::size_t, std::size_t) {}
        // Whether the scene answers a drag and the wheel by moving its view. See PathArt
        [[nodiscard]] virtual bool offersView() const { return false; }
        // Whether a drag from the point, in the coordinates of bounds, moves the view. See PathArt
        [[nodiscard]] virtual bool dragMovesViewFrom(FloatPoint, const FloatRect&) const
        {
            return offersView();
        }
        // Moves the view by a drag across bounds, in the coordinates of bounds.
        virtual void dragView(FloatPoint, const FloatRect&) {}
        // Moves the view by wheel notches, counted as MouseWheelEvent counts them.
        virtual void scrollView(float) {}
        // A press holds the view (true); letGoView or clickView ends the hold. See PathArt
        virtual void holdView(bool) {}
        // Lets go of the view at the pointer's speed, 0 where it stood still. See PathArt
        virtual void letGoView(FloatPoint, const FloatRect&) {}
        // Lets go of the view after a click, by default as a still let-go. See PathArt
        virtual void clickView(const FloatRect& bounds) { letGoView({ 0.0f, 0.0f }, bounds); }
        // Writes the view as the viewer left it into its own section. See PathArt#scene-storeview
        virtual void storeView(Dom::Section&) const {}
        // Takes the view back from the scene's section, as storeView wrote it or as it starts.
        virtual void restoreView(const Dom::Section&) {}
    private:
        // The cycle a scene runs through unless it states its own.
        static constexpr std::chrono::seconds k_cycle{ 40 };
        SceneLook m_look{ SceneLook::Stylized };
    };
}
