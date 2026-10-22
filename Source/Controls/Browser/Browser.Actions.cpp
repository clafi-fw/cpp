module ClaFi.Browser.Actions;

import ClaFi.Icons.BrowseBackIcon;
import ClaFi.Icons.BrowseForwardIcon;
import ClaFi.Icons.PlusMark;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::Browser::Actions
{
    // **These are constructed before main**, so nothing built here may touch the platform, the
    // theme or the text engine - none of them exists yet. Text is a string and its markers, and
    // EventComponent connects handlers into a map of its own, so both are safe; anything added
    // to an action below has to answer the same question. The same rule ClaFi::StdActions
    // states for the standard set.
    Action Open{
        Text{ L"Open" }
        // ,Shortcut{ vkReturn } - it won't let you press any other button
    };
    Action OpenInNewTab{
        Text{ L"Open in new tab" }
    };
    Action back{
        Text{ L"Back" },
        Shortcut{ Keys::Left, { .alt = true } },
        Action::OnPaintIcon{ Icons::BrowseBackIcon::paint }
    };
    Action forward{
        Text{ L"Forward" },
        Shortcut{ Keys::Right, { .alt = true } },
        Action::OnPaintIcon{ Icons::BrowseForwardIcon::paint }
    };
    Action newTab{
        Text{ L"New tab" },
        Shortcut{ L'T', { .ctrl = true } },
        Action::OnPaintIcon{ Icons::PlusMark::paint }
    };
    Action closeTab{
        Text{ L"Close tab" },
        Shortcut{ L'W', { .ctrl = true } },
        Shortcut{ Keys::F4, { .ctrl = true } }
    };

    void registerAll()
    {
        // The application's scope is one object for the whole process and outlives any single
        // browser, and Actions::add appends unconditionally, so a second browser would otherwise
        // register the same set behind the first one. One registration is all there is.
        static bool registered = false;
        if (registered)
            return;
        registered = true;

        AppActions& actions = AppActions::get();
        // Open and Open in new tab carry no shortcut. They are added anyway, so an application
        // that gives one a key needs to do no more than that.
        actions.add(Open);
        actions.add(OpenInNewTab);
        actions.add(back);
        actions.add(forward);
        actions.add(newTab);
        actions.add(closeTab);
    }

}
