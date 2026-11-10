export module ClaFi.PathArt.WeatherEffect;

import ClaFi.PathArt.FirefliesEffect;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.Types;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // A theme's rain, snow, falling leaves or petals and blowing dust, each 0 to 1 strong.
    export struct Weather
    {
        float rain{};
        float snow{};
        float leaves{};
        float petals{}; // the leaves falling as petals
        float dust{};
        float opacity{ 1.0f }; // the whole weather's, lower in the distance
        Color color1{}; // the drops, the flakes, the dust and half the leaves
        Color color2{}; // the other half of the leaves
        Color petal1{}; // half the petals
        Color petal2{}; // the other half of the petals
    };

    // The weather over the frame.
    export void drawWeather(const SceneFrame&, float cycleTime, const Weather&);
    // A theme's weather in its own effect colours.
    export [[nodiscard]] Weather weatherOf(const SceneTheme&);
    // A theme's fireflies with no area yet, for the scene to place among its scenery.
    export [[nodiscard]] Fireflies firefliesOf(const SceneTheme&);
}
