export module ClaFi.PathArt.WeatherEffect;

import ClaFi.PathArt.FirefliesEffect;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.Types;

namespace ClaFi::PathArt
{
    // A theme's rain, snow, falling leaves or petals and blowing dust, over the frame.
    export void drawWeather(const SceneFrame&, float cycleTime, const SceneTheme&);
    // A theme's fireflies with no area yet, for the scene to place among its scenery.
    export [[nodiscard]] Fireflies firefliesOf(const SceneTheme&);
}
