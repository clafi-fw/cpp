module Themes_App.PreviewPanel;

import Themes_App.PageKinds;
import Themes_App.PreviewPage;
import Themes_App.PreviewPageBase;
import Themes_App.Previews;

import ClaFi.Controls.Label;
import ClaFi.Controls.MessageBar;
import ClaFi.Controls.PageControl;

import ClaFi.Icons.SideBar;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    void PreviewPanel::showPage(const PreviewPage page)
    {
        PreviewPageBase*& shownPage = m_pages[static_cast<std::size_t>(page)];
        if (!shownPage)
            shownPage = &addPreviewPage(m_pageControl, page);
        m_pageControl.setCurrentItem(shownPage);
        m_title.text() = titleOf(shownPage->whatItShows());
    }

    void PreviewPanel::setSizesNoteVisible(const bool value)
    {
        m_sizesNote.setVisible(value);
    }

    // The icon is the one on the button that shows the panel. The second paragraph is indented
    // past the icon, so every line of it stands under the name.
    Text PreviewPanel::titleOf(const std::wstring_view whatPageShows)
    {
        Text title{
            InTextIcon{ k_titleIconSize, k_titleIconSize, Icons::SideBar::drawRightBarIcon },
            Space{ k_titleIconGap },
            StyledText{ TextStyles::SubTitle, L"Live preview" }
        };
        title << k_endLine << TextIndent{ k_titleIconSize + k_titleIconGap };
        title << StyledText{ TextStyles::SubBody, ColoredText{ InkGrade::Muted, whatPageShows } };
        return title;
    }
}
