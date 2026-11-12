module ClaFi.PathArt.MusicNotesEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        constexpr std::array<Color, 3> k_noteColors{ Color{ 0xFFFFD34D }, Color{ 0xFFFFF1C9 },
            Color{ 0xFFFFD166 } };

        // A note in its own units, its head at 0, 0: two heads under one beam, or one with a flag.
        void drawNote(const SceneFrame& note, const Color color, const bool pair)
        {
            const SolidColor ink{ color };
            PixelPath stems;
            for (const float headX : { 0.0f, 12.0f })
            {
                fillOval(note, { headX, 0.0f }, 4.6f, 3.3f, ink, -0.45f);
                stems.moveTo(headX + 4.0f, -1.0f);
                stems.lineTo(headX + 4.0f, -17.0f);
                if (!pair)
                    break;
            }
            strokeShape(note, stems, ink, 1.6f);
            if (pair)
            {
                PixelPath beam;
                beam.moveTo(4.0f, -17.0f);
                beam.lineTo(16.0f, -19.0f);
                beam.lineTo(16.0f, -15.0f);
                beam.lineTo(4.0f, -13.0f);
                beam.close();
                fillShape(note, beam, ink);
                return;
            }
            PixelPath flag;
            flag.moveTo(4.0f, -17.0f);
            flag.quadTo({ 12.0f, -12.0f }, { 10.0f, -5.0f });
            strokeShape(note, flag, ink, 1.8f);
        }
    }

    void drawMusicNotes(const SceneFrame& frame, const MusicNotes& notes)
    {
        const float unit = frame.unit;
        const float seed = notes.seed;
        for (int i = 0; i != 6; ++i)
        {
            const float life = fractionalPart(notes.time * 0.42f + static_cast<float>(i) / 6.0f
                + pseudoRandom(i, seed) * 0.05f);
            const float side = pseudoRandom(i, seed + 1.0f) - 0.5f;
            const float sway = 26.0f
                * std::sin(k_2Pi * (life * 0.8f + pseudoRandom(i, seed + 2.0f)));
            const FloatPoint at = {
                notes.origin.x + (side * notes.spread + sway) * unit,
                notes.origin.y - 170.0f * unit * life
            };
            const float shown = std::min(1.0f,
                1.6f * std::sin(std::numbers::pi_v<float> * std::min(1.0f, life * 1.3f)));
            const float opacity = shown * (1.0f - life * 0.3f);
            const float size = unit * (1.3f + 0.5f * pseudoRandom(i, seed + 3.0f));
            const Color color = k_noteColors[static_cast<std::size_t>(i % 3)];
            drawNote(frame.within(Matrix3x2::Translation(at) * Matrix3x2::Scale(size)),
                faded(color, opacity), i % 3 == 1);
        }
    }
}
