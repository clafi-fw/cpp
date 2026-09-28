export module ClaFi.Icons.BrowseBackIcon;

import ClaFi.Icons.BrowseUpIcon;

import ClaFi.Core.Context.PaintIconEvent;

namespace ClaFi::Icons::BrowseBackIcon
{
    export void paint(PaintIconEvent& event)
    {
        BrowseUpIcon::paintHeading(event, BrowseUpIcon::ArrowHeading::Left);
    }

}
