export module ClaFi.Tools.WhatsClip.FormatView;

import ClaFi.Tools.WhatsClip.Page;

import ClaFi.Controls.TabbedBox;
import ClaFi.Controls.TabStrip;
import ClaFi.Controls.Base.StackPanelBase;

import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    using namespace Controls;

    // EVERYTHING THE VIEWER KNOWS ABOUT ONE FORMAT, which is what a tab in the strip down the
    // left opens. Its own tabs are the representations - the same content read a different way -
    // and only the one on top is ever read.
    export class FormatView : public TabbedBox
    {
    public:
        template<typename... Args>
        explicit FormatView(const CreateParams&, Args&&...);
    public:
        // One tab per representation, in the order given, the preferred one open. Called once,
        // when the format is first seen. The lists go to the pages that pick.
        void setRepresentations(const RepresentationList&, const PickLists&);
        // THE OPEN PRESENTATION IS THE APPLICATION'S AND NOT ONE VIEW'S: picked on one format, it
        // is what the next format opens on, so walking the strip never changes what is being read.
        // The word is kept by whoever owns the views; a view reads it when it opens and writes it
        // when the user picks. Called before setRepresentations, which is the first read of it.
        void setPreference(std::wstring_view& kept) { m_preference = &kept; }
        // Opens the presentation being browsed in, or this format's first where it has no such
        // presentation - a handle format has none, and a view is never left showing nothing.
        void openPreferred();
        // THE CLIPBOARD HAS MOVED. Whatever is open reads it now; whatever is opened after this
        // reads it when its own tab is picked.
        void show(Transfer::Offer&, const Transfer::Format&);
    private:
        // A representation as this view holds it: the word on its tab, the tab, and the page it
        // opens.
        struct RepresentationTab
        {
            std::wstring_view name;
            Tab* tab;
            RepresentationPage* page;
        };

        using RepresentationTabList = std::vector<RepresentationTab>;
        using PageList = std::vector<RepresentationPage*>;
    private:
        // Reads the open page unless it has already read this clipboard.
        void showOpenPage();
        // Writes what is open as the presentation to browse in - see setPreference.
        void rememberOpen();
        // Every page in this control was put there by setRepresentations, so the only thing the
        // page control can be showing is one of them.
        [[nodiscard]] RepresentationPage* openPage();
    private:
        // Filled once, by setRepresentations.
        RepresentationTabList m_representations{};
        // Owned by whoever owns the views, and null until it says so.
        std::wstring_view* m_preference{ nullptr };
        // Empty until the first show. A tab selected while building the strip raises the change
        // event before there is a format to read for, and that is what says so.
        std::optional<Transfer::Format> m_format{};
        // The pages that have read the clipboard as it now stands. Cleared where show says it has
        // moved on, so a tab picked after that reads again.
        PageList m_readPages{};
    };


//-----------------------------------------------------------------------------


    // The tabs go across the TOP: the format strip already runs down the left, and a second
    // vertical strip beside it would read as one list of two kinds of thing.
    template<typename... Args>
    FormatView::FormatView(const CreateParams& params, Args&&... args)
        :
        TabbedBox{
            params,
            TabsOrientation::HorizontalTop,
            TabLineThickness{ Thickness::Bold },
            Padding{ 0.0f },
            std::forward<Args>(args)...
            }
    {
        // A tab picked reads what it shows. The connection needs no scope: the page control is
        // this control's own member and goes when it does.
        pageControl().connectEvent([this](CurrentItemChangeEvent&) {
            rememberOpen();
            showOpenPage();
        });
        pageControl().setColorRules(std::nullopt);
    }
}
