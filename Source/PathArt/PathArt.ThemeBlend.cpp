module ClaFi.PathArt.ThemeBlend;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.Themes;
import ClaFi.PathArt.Types;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace std::chrono_literals;

    namespace
    {
        AnimationSlot g_crossFadeSlot{
            .duration{ .rise{ 555ms } },
            .easingFactor{ EasingFactor::EaseInOut }
        };
    }

    // **Always a fade in, never a cut.** The scene stands on a null theme - every colour zero and
    // every effect off - until this, so the first theme rises out of nothing the way every later
    // one crossfades out of the one before it. A scene restored from a config fades into the
    // stored theme rather than into the first of the list: the pick lands before the first paint,
    // and a crossfade restarted from a blend is still one movement.
    ThemeBlend::ThemeBlend()
    {
        crossFadeToPicked();
    }

    void ThemeBlend::pick(const std::size_t index)
    {
        if (m_pickedIndex == index)
            return;

        m_pickedIndex = index;
        crossFadeToPicked();
    }

    void ThemeBlend::tick()
    {
        m_animator.externalTimerTick();
    }

    // **The target is read at every step, not captured.** A crossfade asked for while one runs is
    // that running animation - the controller leaves one heading to the same end value alone - so
    // the step it already has must find the new target and the new start.
    void ThemeBlend::crossFadeToPicked()
    {
        m_crossFadeStart = m_worn;
        m_crossFadeTarget = &picked();
        // The name is the destination's at once, like the index: what crossfades below is the
        // colours, and a blend answering the name it came from tells its caller the wrong theme.
        m_worn.name = m_crossFadeTarget->name;
        m_animator.start(this, g_crossFadeSlot, 0.0f, 1.0f, [this](AnimateParams& params) {
            for (std::size_t i = 0; i != SceneTheme::colorsNum(); ++i)
            {
                m_worn.asArray()[i] = Color{
                    m_crossFadeStart.asArray()[i],
                    m_crossFadeTarget->asArray()[i],
                    params.value
                };
            }
            for (std::size_t i = 0; i != SceneTheme::effectsNum(); ++i)
            {
                const float v1 = m_crossFadeStart.effectsArray()[i];
                const float v2 = m_crossFadeTarget->effectsArray()[i];
                m_worn.effectsArray()[i] = v1 * (1.0f - params.value) + v2 * params.value;
            }
            ++m_wornVersion;
        });
    }

    // The ink is the light one or the river's ground at half, whichever stands out more against
    // the top of the sky.
    SceneReadoutColors readoutColorsOf(const SceneTheme& theme)
    {
        SceneReadoutColors colors = {
            .field = theme.riverBottom,
            .series = theme.riverWave
        };
        const Color darkInk = colorBetween(theme.riverBottom, Color{ 0xFF000000 }, 0.5f);
        if (contrastBetween(darkInk, theme.skyTop) > contrastBetween(colors.ink, theme.skyTop))
            colors.ink = darkInk;
        return colors;
    }
}
