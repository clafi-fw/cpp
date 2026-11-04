module Themes_App.CalloutsPreview;

import ClaFi.Controls.Callout;
import ClaFi.Controls.Label;
import ClaFi.Controls.Stack;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    Callout& CalloutsPreview::addCallout(const MessageIcon kind, const std::wstring_view line)
    {
        Callout& callout = add<Callout>(kind);
        callout.blocks().add<Label>(line);
        return callout;
    }
}
