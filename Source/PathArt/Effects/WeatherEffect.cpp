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
    void drawWeather(const SceneFrame& frame, const float cycleTime, const SceneTheme& theme)
    {
        drawRain(frame, cycleTime, Rain{
            .opacity = theme.hasRain,
            .color = theme.effectColor1
        });
        drawSnow(frame, cycleTime, Snow{
            .opacity = theme.hasSnow,
            .color = theme.effectColor1
        });
        drawFallingLeaves(frame, cycleTime, FallingLeaves{
            .opacity = StateFactors::compose(theme.hasLeaves, theme.hasPetals),
            .petals = theme.hasPetals,
            .color1 = theme.effectColor1,
            .color2 = theme.effectColor2
        });
        drawBlowingDust(frame, cycleTime, BlowingDust{
            .opacity = theme.hasDust,
            .color = theme.effectColor1
        });
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
