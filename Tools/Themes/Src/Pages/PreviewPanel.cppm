export module Themes_App.PreviewPanel;

import Themes_App.PreviewPage;
import Themes_App.PreviewPageBase;

import ClaFi.Controls.Label;
import ClaFi.Controls.MessageBar;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;

import ClaFi.Icons.SideBar;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // The preview beside a page: one page of controls at a time, each built when first shown.
    export class PreviewPanel : public Panel
    {
    public:
        template<typename... Args>
        explicit PreviewPanel(const CreateParams&, Args&&...);
    public:
        void showPage(PreviewPage);
        // Shows or hides the note that the sizes edited show only with Preview in application on.
        void setSizesNoteVisible(bool);
    private:
        using Pages = std::array<PreviewPageBase*, k_previewPageCount>;
    private:
        // The panel's name, set as a page's title is, and under it what the page shows.
        [[nodiscard]] static Text titleOf(std::wstring_view whatPageShows);
    private:
        // A stated width: a page switch leaves the page beside the panel where it is.
        static constexpr float k_width = 240.0f;

        Pages m_pages{}; // a page is kept once built, and serves every node that names it

        StackPanel& m_head{ createTopBar<StackPanel>(
            Orientation::Vertical,
            Padding{ 12.0f, 8.0f },
            Spacing{ 8.0f }
        ) };

        Label& m_title{ m_head.add<Label>() };

        MessageBar& m_sizesNote{ m_head.add<MessageBar>() };

        // A page taller than the window scrolls, and loses the bar's width while it does.
        ScrollBox& m_pageBox{ createBody<ScrollBox>(
            ScrollBars::Auto
        ) };

        PageControl& m_pageControl{ m_pageBox.createBody<PageControl>() };
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    PreviewPanel::PreviewPanel(const CreateParams& params, Args&&... args)
        :
        Panel{
            params,
            // The preference states the width the bounds pin: a side bar's width is otherwise its
            // content's answer, and a text wrapped at a share of it is measured at the whole.
            // See Item-Containers
            PreferredSize{ k_width, 0.0f },
            MinSize{ k_width, 0.0f },
            MaxSize{ k_width, k_maxFloat },
            std::forward<Args>(args)...
        }
    {
        m_sizesNote.setMessage(MessageIcon::Information, Text{
            L"Sizes show with Preview in application ",
            InTextIcon{ 16.0f, 16.0f, Icons::SideBar::drawPanelIcon },
            L" on."
        });
        m_sizesNote.hide();
    }
}
