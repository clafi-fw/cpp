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

        struct WeatherStyle
        {
            WeatherEffect effect{};
            float SceneTheme::* strength{};
            Color color1{};
            Color color2{};
        };

        constexpr std::array k_weatherStyles{
            WeatherStyle{ WeatherEffect::Rain, &SceneTheme::hasRain, 0xFFB4C8FF, 0xFFFFFFFF },
            WeatherStyle{ WeatherEffect::Snow, &SceneTheme::hasSnow, 0xFFFFFFFF, 0xFFFFFFFF },
            WeatherStyle{ WeatherEffect::FallingPetals, &SceneTheme::hasPetals },
            WeatherStyle{ WeatherEffect::BlowingDust, &SceneTheme::hasDust, 0xFF8B5A2B, 0xFFF9F4F0 },
            WeatherStyle{ WeatherEffect::Fireflies, &SceneTheme::hasFireflies, 0xFFFFFFB4, 0x64FFFF64 },
            WeatherStyle{ WeatherEffect::FallingLeaves, &SceneTheme::hasLeaves, 0xFFD08A30, 0xFFA5561A }
        };

        static_assert(std::ranges::all_of(Themes::allThemes, [](const SceneTheme& theme) {
            return std::ranges::count_if(k_weatherStyles, [&theme](const WeatherStyle& style) {
                return theme.*style.strength > 0.0f;
            }) <= 1;
        }));

        void applyWeather(SceneTheme& theme, const SceneWeather& weather)
        {
            if (weather.lightning)
            {
                theme.hasLightning = *weather.lightning ? 1.0f : 0.0f;
            }
            if (weather.effect == WeatherEffect::ThemeDefault)
            {
                return;
            }
            for (const WeatherStyle& style : k_weatherStyles)
            {
                if (style.effect != weather.effect)
                {
                    theme.*style.strength = 0.0f;
                }
                else if (theme.*style.strength == 0.0f)
                {
                    theme.*style.strength = 1.0f;
                    if (style.effect != WeatherEffect::FallingPetals)
                    {
                        theme.effectColor1 = style.color1;
                        theme.effectColor2 = style.color2;
                    }
                }
            }
        }
    }

    // **Always a fade in, never a cut.** The scene stands on a null theme - every color zero and
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

    void ThemeBlend::wear(const std::size_t index)
    {
        m_animator.stop(this);
        m_pickedIndex = index;
        m_worn = picked();
        applyWeather(m_worn, m_weather);
        m_crossFadeStart = m_worn;
        m_crossFadeTarget = m_worn;
        ++m_wornVersion;
    }

    void ThemeBlend::setWeather(const SceneWeather& weather)
    {
        if (m_weather == weather)
        {
            return;
        }
        m_weather = weather;
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
        m_crossFadeTarget = picked();
        applyWeather(m_crossFadeTarget, m_weather);
        // The name is the destination's at once, like the index: what crossfades below is the
        // colors, and a blend answering the name it came from tells its caller the wrong theme.
        m_worn.name = m_crossFadeTarget.name;
        m_animator.start(this, g_crossFadeSlot, 0.0f, 1.0f, [this](AnimateParams& params) {
            for (std::size_t i = 0; i != SceneTheme::colorsNum(); ++i)
            {
                m_worn.asArray()[i] = Color{
                    m_crossFadeStart.asArray()[i],
                    m_crossFadeTarget.asArray()[i],
                    params.value
                };
            }
            for (std::size_t i = 0; i != SceneTheme::effectsNum(); ++i)
            {
                const float v1 = m_crossFadeStart.effectsArray()[i];
                const float v2 = m_crossFadeTarget.effectsArray()[i];
                m_worn.effectsArray()[i] = v1 * (1.0f - params.value) + v2 * params.value;
            }
            if (activeWeatherEffect(m_crossFadeStart) != activeWeatherEffect(m_crossFadeTarget))
            {
                const float outgoing = std::max(0.0f, 1.0f - 2.0f * params.value);
                const float incoming = std::max(0.0f, 2.0f * params.value - 1.0f);
                for (const WeatherStyle& style : k_weatherStyles)
                {
                    m_worn.*style.strength = m_crossFadeStart.*style.strength * outgoing
                        + m_crossFadeTarget.*style.strength * incoming;
                }
            }
            ++m_wornVersion;
        });
    }

    WeatherEffect activeWeatherEffect(const SceneTheme& theme)
    {
        for (const WeatherStyle& style : k_weatherStyles)
        {
            if (theme.*style.strength > 0.0f)
            {
                return style.effect;
            }
        }
        return WeatherEffect::None;
    }

    // The ink is the light one or the river's ground at half, whichever stands out more against
    // the top of the sky.
    SceneReadoutColors readoutColorsOf(const SceneTheme& theme)
    {
        SceneReadoutColors colors = {
            .field = theme.riverBottom,
            .series = theme.riverWave
        };
        const Color darkInk = Color::between(theme.riverBottom, Color{ 0xFF000000 }, 0.5f);
        if (Color::contrastBetween(darkInk, theme.skyTop) > Color::contrastBetween(colors.ink, theme.skyTop))
            colors.ink = darkInk;
        return colors;
    }
}
