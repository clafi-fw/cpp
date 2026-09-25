module ClaFi.Tools.WhatsClip.FormatView;

import ClaFi.Tools.WhatsClip.Page;

import ClaFi.Controls.PageControl;
import ClaFi.Controls.TabbedBox;
import ClaFi.Controls.TabStrip;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Transfer.Clipboard;
import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    void FormatView::setRepresentations(const RepresentationList& representations,
        const PickLists& picks)
    {
        for (const Representation& representation : representations)
        {
            RepresentationPage& page = representation.createPage(pageControl(), picks);
            Tab& tab = strip().addTab(
                Text{ representation.name },
                Padding{ 12.0f, themeMetrics().tab.padding.y },
                Page{ page }
            );
            m_representations.push_back(RepresentationTab{ representation.name, &tab, &page });
        }

        openPreferred();
    }

    void FormatView::openPreferred()
    {
        if (m_representations.empty())
            return;

        for (const RepresentationTab& entry : m_representations)
        {
            if (!m_preference || entry.name != *m_preference)
                continue;

            entry.tab->select();
            return;
        }

        m_representations.front().tab->select();
    }

    void FormatView::show(Transfer::Offer& offer, const Transfer::Format& format)
    {
        m_format = format;
        m_readPages.clear();

        RepresentationPage* page = openPage();
        if (!page)
            return;

        page->show(offer, format);
        m_readPages.push_back(page);
    }

    void FormatView::showOpenPage()
    {
        if (!m_format)
            return;

        RepresentationPage* page = openPage();
        if (!page || std::ranges::find(m_readPages, page) != m_readPages.end())
            return;

        // ASKED FOR AGAIN RATHER THAN KEPT, for the reason the Viewer asks again: an offer
        // belongs to Transfer and is freed the moment anything else reads a clipboard that has
        // moved on, so the one show was handed is not a thing to hold on to.
        Transfer::Offer* offer = formContext().clipboard().offer();
        if (!offer)
            return;

        page->show(*offer, *m_format);
        m_readPages.push_back(page);
    }

    // A VIEW OFFERING ONE REPRESENTATION OFFERS NO CHOICE, so it writes none: a handle format
    // would otherwise make Handle the presentation to browse in, and every format after it would
    // be left showing whatever it already had. The same test covers the view while it is still
    // being built, its first page arriving before there is a second to pick between.
    void FormatView::rememberOpen()
    {
        if (!m_preference || m_representations.size() < 2)
            return;

        const RepresentationPage* open = openPage();
        for (const RepresentationTab& entry : m_representations)
        {
            if (entry.page != open)
                continue;

            *m_preference = entry.name;
            return;
        }
    }

    RepresentationPage* FormatView::openPage()
    {
        return static_cast<RepresentationPage*>(pageControl().currentItem());
    }
}
