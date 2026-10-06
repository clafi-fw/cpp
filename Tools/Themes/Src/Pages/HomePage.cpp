module Themes_App.HomePage;

import Themes_App.WithPreview;

import ClaFi.App.ThemesList;

import ClaFi.Documents.HomePage;

import ClaFi.Core.AppTheme_Theme;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;

    const AppTheme* HomePage::selectedTheme()
    {
        return list().previewedTheme();
    }

    // The rebuild took the previewed tile with the rest, so what the application is wearing is a
    // theme nothing on screen stands for any more. Asked for outright because no gesture is going
    // to ask: the pointer has not moved and the current item was set from the list.
    void HomePage::documentsRebuilt()
    {
        Base::documentsRebuilt();
        invalidatePreview();
    }
}
