export module ClaFi.PathArt.ThemeBlend;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.Themes;
import ClaFi.PathArt.Types;
import ClaFi.Core.System.Animation;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // The theme a scene wears: the one picked, and the blend crossfading to it. See PathArt
    export class ThemeBlend
    {
    public:
        ThemeBlend();
        ThemeBlend(const ThemeBlend&) = delete;
        ThemeBlend& operator=(const ThemeBlend&) = delete;
    public:
        [[nodiscard]] std::size_t pickedIndex() const { return m_pickedIndex; } // in allThemes
        // The theme picked, blend or no blend - what a caller taking colours or a name means.
        [[nodiscard]] const SceneTheme& picked() const { return Themes::allThemes[m_pickedIndex]; }
        // What the scene is drawn in, which between two picks is a blend of them.
        [[nodiscard]] const SceneTheme& worn() const { return m_worn; }
        // Moves on with every step of the blend, so a scene can tell what it kept has gone stale.
        [[nodiscard]] std::uint64_t wornVersion() const { return m_wornVersion; }
        // Crossfades from what is worn to the theme at `index` of Themes::allThemes.
        void pick(std::size_t index);
        // Moves the crossfade on between the timer's ticks.
        void tick();
    private:
        void crossFadeToPicked();
    private:
        std::size_t m_pickedIndex{ 0 };
        // **Null until the constructor fades the first theme in** - see ThemeBlend::ThemeBlend.
        SceneTheme m_worn{};
        SceneTheme m_crossFadeStart{}; // what was worn when the crossfade began
        const SceneTheme* m_crossFadeTarget{ nullptr }; // in allThemes
        std::uint64_t m_wornVersion{ 0 };
        AnimationController m_animator{};
    };

    // The colours a readout over a scene is drawn in, where the scene wears this theme. See PathArt
    export [[nodiscard]] SceneReadoutColors readoutColorsOf(const SceneTheme&);
}
