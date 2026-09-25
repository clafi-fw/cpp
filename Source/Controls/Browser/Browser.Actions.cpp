module ClaFi.Browser.Actions;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::Browser::Actions
{
    // THESE ARE CONSTRUCTED BEFORE MAIN, so nothing built here may touch the platform, the theme
    // or the text engine - none of them exists yet. Text is a string and its markers, and
    // EventComponent connects handlers into a map of its own, so both are safe; anything added
    // to an action below has to answer the same question. The same rule ClaFi::StdActions
    // states for the standard set.
    Action open{
        Text{ L"Open" }
        // ,Shortcut{ vkReturn } - it won't let you press any other button
    };
    Action openInNewTab{
        Text{ L"Open in new tab" }
    };

    void registerAll()
    {
        // The application's scope is one object for the whole process and outlives any single
        // browser, and Actions::add appends unconditionally, so a second browser would otherwise
        // register the same pair behind the first one. One registration is all there is.
        static bool registered = false;
        if (registered)
            return;
        registered = true;

        AppActions& actions = AppActions::get();
        // Neither carries a shortcut, so nothing in the scope can ever match one. Added anyway,
        // so the scope holds the pair whole and an application that gives one a key needs to do
        // no more than that.
        actions.add(open);
        actions.add(openInNewTab);
    }

}
