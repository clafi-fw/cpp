module ClaFi.PathArt.WeatherEffect;

import ClaFi.PathArt.BlowingDustEffect;
import ClaFi.PathArt.FallingLeavesEffect;
import ClaFi.PathArt.FirefliesEffect;
import ClaFi.PathArt.RainEffect;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SnowEffect;
import ClaFi.PathArt.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    void drawWeather(const SceneFrame& frame, const float cycleTime, const Weather& weather)
    {
        const auto countOf = [&](int usualCount) {
            return static_cast<int>(std::round(static_cast<float>(usualCount) * weather.density));
        };
        drawRain(frame, cycleTime, Rain{
            .opacity = weather.rain * weather.opacity,
            .color = weather.color1,
            .count = countOf(Rain{}.count),
            .pace = weather.fallPace,
            .seed = weather.seed
        });
        drawSnow(frame, cycleTime, Snow{
            .opacity = weather.snow * weather.opacity,
            .color = weather.color1,
            .count = countOf(Snow{}.count),
            .pace = weather.fallPace,
            .seed = weather.seed
        });
        drawFallingLeaves(frame, cycleTime, FallingLeaves{
            .opacity = StateFactors::compose(weather.leaves, weather.petals) * weather.opacity,
            .petals = weather.petals,
            .color1 = Color::between(weather.color1, weather.petal1, weather.petals),
            .color2 = Color::between(weather.color2, weather.petal2, weather.petals),
            .count = countOf(FallingLeaves{}.count),
            .pace = weather.fallPace,
            .seed = weather.seed
        });
        drawBlowingDust(frame, cycleTime, BlowingDust{
            .opacity = weather.dust * weather.opacity,
            .color1 = weather.color1,
            .color2 = weather.color2,
            .count = countOf(BlowingDust{}.count),
            .pace = weather.windPace,
            .seed = weather.seed
        });
    }

    Weather weatherOf(const SceneTheme& theme)
    {
        return {
            .rain = theme.hasRain,
            .snow = theme.hasSnow,
            .leaves = theme.hasLeaves,
            .petals = theme.hasPetals,
            .dust = theme.hasDust,
            .color1 = theme.effectColor1,
            .color2 = theme.effectColor2,
            .petal1 = theme.petal1,
            .petal2 = theme.petal2
        };
    }

    Fireflies firefliesOf(const SceneTheme& theme)
    {
        return {
            .opacity = theme.hasFireflies,
            .light = theme.effectColor1,
            .glow = theme.effectColor2
        };
    }
}
