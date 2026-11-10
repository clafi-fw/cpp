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
        drawRain(frame, cycleTime, Rain{
            .opacity = weather.rain * weather.opacity,
            .color = weather.color1
        });
        drawSnow(frame, cycleTime, Snow{
            .opacity = weather.snow * weather.opacity,
            .color = weather.color1
        });
        drawFallingLeaves(frame, cycleTime, FallingLeaves{
            .opacity = StateFactors::compose(weather.leaves, weather.petals) * weather.opacity,
            .petals = weather.petals,
            .color1 = weather.color1,
            .color2 = weather.color2
        });
        drawBlowingDust(frame, cycleTime, BlowingDust{
            .opacity = weather.dust * weather.opacity,
            .color = weather.color1
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
            .color2 = theme.effectColor2
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
