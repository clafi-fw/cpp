export module ClaFi.PathArt.HareSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // How the hare runs or sits at one moment.
    export struct HarePose
    {
        FloatPoint foot; // where its hind foot is
        float size; // pixels per unit of the hare, which is about 70 units long
        float facing; // 1 facing right, -1 facing left; between them it turns
        float air; // 0 on the ground, 1 at the top of a hop
        float stretch;
        float bodyTurn;
        float sitting;
        float earTurn;
        float earWiggle;
        float footLift;
        float holding; // 0 forepaws down, 1 raised to hold something at its chest
        float teeth; // 0 mouth shut, 1 its two front teeth bared full length
    };

    // The colours of a hare: its fur and tail, its ears inside, its nose and its mouth.
    export struct HareCoat
    {
        Color light; // its head, and its back
        Color fur; // its near foreleg and ear
        Color shade; // its haunch, hind foot, far foreleg and ear, and its flank
        Color belly;
        Color tail;
        Color earInside;
        Color nose;
        Color whisker;
        Color mouth;
        Color tooth;
        Color toothEdge;
    };

    export void drawHare(const SceneFrame&, const HarePose&, const HareCoat&);
}
