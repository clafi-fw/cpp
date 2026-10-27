export module ClaFi.Icons.BrowseForwardIcon;

import ClaFi.Icons.BrowseUpIcon;

import ClaFi.Core.Context.DrawIconEvent;

namespace ClaFi::Icons::BrowseForwardIcon
{
    export void draw(DrawIconEvent& event)
    {
        BrowseUpIcon::drawHeading(event, BrowseUpIcon::ArrowHeading::Right);
    }

}
