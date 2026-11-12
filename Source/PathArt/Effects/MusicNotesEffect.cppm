export module ClaFi.PathArt.MusicNotesEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Six notes drifting up from an instrument, swaying as they rise and fading near the top.
    export struct MusicNotes
    {
        FloatPoint origin; // where they start from
        float spread; // how far apart they start, in units
        float time; // in seconds; a note takes 2.4 seconds to rise its 170 units
        float seed{ 0.0f }; // tells one instrument's notes from another's
    };

    export void drawMusicNotes(const SceneFrame&, const MusicNotes&);
}
