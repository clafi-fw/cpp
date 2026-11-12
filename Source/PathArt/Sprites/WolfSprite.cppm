export module ClaFi.PathArt.WolfSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // How the wolf stands, runs or lies at one moment.
    export struct WolfPose
    {
        FloatPoint center;
        float size; // pixels per unit of the wolf, which is about 150 units from nose to tail
        float turn; // the whole wolf's turn about its middle, in radians
        float stride; // where the gallop is, in radians; the legs and the tail follow it
        float leap; // 0 on its feet, 1 stretched out in a leap, forelegs reaching
        float sprawl; // 0 on its feet, 1 sprawled on its belly
        float onBack; // 0 on its feet, 1 on its back with its legs in the air
        float dangle; // 0 as the rest has it, 1 its legs and tail hanging straight down
        float dazed; // past a half, its eyes are crosses
        float headTurn;
        float tongueTurn;
        float jawOpen; // 0 shut, 1 gaping, its teeth out
        float glance; // how far its eye looks down
        float cycleTime; // the time its legs twitch by on its back, and its tail wags by
        float wag{ 0.0f }; // 0 still to 1 held high and wagging fast, as a happy dog's tail
    };

    // The colours of a wolf: its fur, and its mouth with its tongue and teeth.
    export struct WolfCoat
    {
        Color light; // its head and snout, and its back
        Color shade; // its ears and lower jaw, and its flank
        Color coat; // its near legs and its tail
        Color far; // its far legs, and the pads of the near ones
        Color dark; // its tail's tip, and the pads of the far legs
        Color belly;
        Color tongue;
        Color mouth; // inside it, open
        Color tooth;
        Color toothEdge;
    };

    // Coats a wolf is drawn in, wherever it appears.
    export namespace WolfCoats
    {
        constexpr WolfCoat Grey{
            .light = Color{ 0xFF7D8592 },
            .shade = Color{ 0xFF59606B },
            .coat = Color{ 0xFF6C7480 },
            .far = Color{ 0xFF4F565F },
            .dark = Color{ 0xFF3D434C },
            .belly = Color{ 0xE6B3B9C2 },
            .tongue = Color{ 0xFFEF6F7F },
            .mouth = Color{ 0xFF5A1C28 },
            .tooth = Color{ 0xFFFBF6E8 },
            .toothEdge = Color{ 0xFF9C9488 }
        };
    }

    // A wolf running to the right, in the coat given.
    export void drawWolf(const SceneFrame&, const WolfPose&, const WolfCoat&);
}
