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
        float density{ 1.0f }; // the share of each effect's usual count drawn
        float fallPace{ 1.0f }; // the pace the rain, snow, leaves and petals fall at
        float windPace{ 1.0f }; // the pace the dust is blown at
        float seed{ 0.0f }; // shifts where every effect's pieces are
        Color color1{}; // the drops, the flakes, half the leaves and one end of the dust's colours
        Color color2{}; // the other half of the leaves and the other end of the dust's colours
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
