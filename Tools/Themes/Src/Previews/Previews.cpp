module Themes_App.Previews;

import Themes_App.CalloutsPreview;
import Themes_App.CodeBlockPreview;
import Themes_App.DoDontPreview;
import Themes_App.GridPreview;
import Themes_App.InputsPreview;
import Themes_App.OverviewPreview;
import Themes_App.PreviewPage;
import Themes_App.PreviewPageBase;
import Themes_App.ScrollingPreview;
import Themes_App.SelectionPreview;
import Themes_App.SurfacesPreview;
import Themes_App.TabsPreview;
import Themes_App.TestSubjectsPreview;
import Themes_App.WindowsPreview;

import ClaFi.Controls.PageControl;

import ClaFi.StdLib;

namespace Themes_App
{
    // Count names no page, and is answered as the Overview is.
    PreviewPageBase& addPreviewPage(PageControl& pages, const PreviewPage page)
    {
        switch (page)
        {
            case PreviewPage::Windows:
                return pages.add<WindowsPreview>();
            case PreviewPage::Surfaces:
                return pages.add<SurfacesPreview>();
            case PreviewPage::Grid:
                return pages.add<GridPreview>();
            case PreviewPage::Inputs:
                return pages.add<InputsPreview>();
            case PreviewPage::Selection:
                return pages.add<SelectionPreview>();
            case PreviewPage::Tabs:
                return pages.add<TabsPreview>();
            case PreviewPage::Scrolling:
                return pages.add<ScrollingPreview>();
            case PreviewPage::Callouts:
                return pages.add<CalloutsPreview>();
            case PreviewPage::DoDont:
                return pages.add<DoDontPreview>();
            case PreviewPage::CodeBlock:
                return pages.add<CodeBlockPreview>();
            case PreviewPage::TestSubjects:
                return pages.add<TestSubjectsPreview>();
            case PreviewPage::Overview:
            case PreviewPage::Count:
                break;
        }
        return pages.add<OverviewPreview>();
    }
}
