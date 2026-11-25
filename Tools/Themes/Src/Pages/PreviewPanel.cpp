module Themes_App.PreviewPanel;

import Themes_App.PageKinds;
import Themes_App.PreviewPage;
import Themes_App.PreviewPageBase;
import Themes_App.Previews;
import Themes_App.StyleStatesPreview;

import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Controls.Label;
import ClaFi.Controls.MessageBar;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;

import ClaFi.Icons.Eye;

import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Styles;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    void PreviewPanel::showPage(const NodePreview& preview)
    {
        PreviewPageBase*& page = m_pages[static_cast<std::size_t>(preview.page)];
        if (!page)
        {
            page = &addPreviewPage(m_pageControl, preview.page);
        }
        m_pageControl.setCurrentItem(page);
        if (m_styleName != preview.styleName)
        {
            m_styleName = preview.styleName;
            updatePreviewTheme();
            invalidate();
        }
        m_title.text() = titleOf(m_styleName.empty() ? page->whatItShows() : m_styleName);
    }

    void PreviewPanel::setTheme(const AppTheme* theme, const ColorMode mode)
    {
        m_theme = theme;
        m_mode = mode;
        updatePreviewTheme();
    }

    void PreviewPanel::setSizesNoteVisible(const bool value)
    {
        m_sizesNote.setVisible(value);
    }

    void PreviewPanel::adjustPaint(AdjustPaintEvent& event)
    {
        Panel::adjustPaint(event);
        if (!m_theme)
        {
            return;
        }
        event.resetTheme(*m_theme, m_colors);
        event.setSurfaceHsl(restingSurface(*m_theme, UiElement::Dialog, m_mode));
        const std::wstring_view dialog = uiElementOf(UiElement::Dialog).token;
        const bool inksAtRest = std::ranges::any_of(m_theme->styles.resolvedRules(dialog),
            [](const ColorRule& rule) {
                return rule.atRest() && rule.output == PaintChannel::Text;
            });
        if (inksAtRest)
        {
            event.setTextHsl(restingInk(*m_theme, UiElement::Dialog, m_mode));
        }
    }

    void PreviewPanel::updatePreviewTheme()
    {
        if (!m_theme)
        {
            return;
        }
        const AppElementNames elements = appContext().appElements();
        if (m_styleName.empty())
        {
            m_colors = bake(*m_theme, m_mode, elements);
            return;
        }

        // The last element belongs to the sidebar. Its samples keep this slot as the style changes.
        std::vector<std::wstring_view> names{ elements.begin(), elements.end() };
        names.push_back(m_styleName);
        m_colors = bake(*m_theme, m_mode, names);
        auto& samples = static_cast<StyleStatesPreview&>(
            *m_pages[static_cast<std::size_t>(PreviewPage::StyleStates)]);
        samples.showStyle(*m_theme, m_styleName, m_mode);
    }

    // The second paragraph is indented past the icon, so every line of it stands under the name.
    Text PreviewPanel::titleOf(const std::wstring_view whatPageShows)
    {
        Text title{
            InTextIcon{ k_titleIconSize, k_titleIconSize, Icons::Eye::draw },
            Space{ k_titleIconGap },
            StyledText{ TextStyles::SubTitle, L"Live preview" }
        };
        title << k_endLine << TextIndent{ k_titleIconSize + k_titleIconGap };
        title << StyledText{ TextStyles::SubBody, ColoredText{ InkGrade::Muted, whatPageShows } };
        return title;
    }
}
