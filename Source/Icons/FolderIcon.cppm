export module ClaFi.Icons.FolderIcon;

import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::FolderIcon
{
    using namespace ::ClaFi::Graphics;

    // A folder's outline filling `box`: a tab across the top left, a slant down to the body, and
    // the body, the corners rounded. Added to the path rather than drawn, so the caller strokes
    // it in its own ink and in its own company - the arrow OpenInExplorerIcon sets beside it, the
    // row a tree puts it at the head of.
    export void addOutline(PixelPath&, const FloatRect& box);


    //-------------------------------------------------------------------------


    // Where the tab ends and where its slant meets the body, as fractions of the box's width, and
    // how far the tab stands proud of the body as a fraction of its height. The slant is long
    // enough to be read as one: a shallower step reads as a nick in the top edge rather than as
    // a tab, and it is the first thing to close up as the icon shrinks.
    constexpr float k_tabEnd = 0.40f;
    constexpr float k_slantEnd = 0.53f;
    constexpr float k_tabHeight = 0.18f;
    // How far back each corner is cut, as a fraction of the box's width.
    constexpr float k_cornerRadius = 0.10f;

    void addOutline(PixelPath& path, const FloatRect& box)
    {
        const float width = box.width();
        const float bodyTop = box.top + box.height() * k_tabHeight;
        const std::array<FloatPoint, 6ull> folder{
            FloatPoint{ box.left, box.top },
            FloatPoint{ box.left + width * k_tabEnd, box.top },
            FloatPoint{ box.left + width * k_slantEnd, bodyTop },
            FloatPoint{ box.right, bodyTop },
            FloatPoint{ box.right, box.bottom },
            FloatPoint{ box.left, box.bottom }
        };
        path.addRoundedPolygon(folder, width * k_cornerRadius);
    }

}
