export module ClaFi.Icons.BrowseBackIcon;

import ClaFi.Icons.BrowseUpIcon;

import ClaFi.Core.Context.DrawIconEvent;

namespace ClaFi::Icons::BrowseBackIcon
{
    export void draw(DrawIconEvent& event)
    {
        BrowseUpIcon::drawHeading(event, BrowseUpIcon::ArrowHeading::Left);
    }

}
