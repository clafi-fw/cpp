export module Themes_App.Previews;

import Themes_App.PreviewPage;
import Themes_App.PreviewPageBase;

import ClaFi.Controls.PageControl;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // Builds the page into the page control - the one place that names every page's class.
    export PreviewPageBase& addPreviewPage(PageControl&, PreviewPage);
}
