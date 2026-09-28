export module ClaFi.Icons.BrowseForwardIcon;

import ClaFi.Icons.BrowseUpIcon;

import ClaFi.Core.Context.PaintIconEvent;

namespace ClaFi::Icons::BrowseForwardIcon
{
    export void paint(PaintIconEvent& event)
    {
        BrowseUpIcon::paintHeading(event, BrowseUpIcon::ArrowHeading::Right);
    }

}
