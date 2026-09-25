export module ClaFi.Tools.WhatsClip.Page;

import ClaFi.Tools.WhatsClip.EncodingPick;
import ClaFi.Tools.WhatsClip.LanguagePick;
import ClaFi.Tools.WhatsClip.Encodings;
import ClaFi.Tools.WhatsClip.Languages;

import ClaFi.Controls.Checkbox;
import ClaFi.Controls.CodeBox;
import ClaFi.Controls.ComboBox;
import ClaFi.Controls.Label;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.Base.PanelBase;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.Core.System.Props;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    using namespace ::ClaFi::Controls;

    // WHAT A PAGE SAYS WHEN THE CLIPBOARD HELD ITS PEACE. One sentence rather than one per page:
    // they all report a failed read in the same place, and there was never a reason for them to
    // report it in different words.
    export auto k_noAnswer = Text{ InkGrade::Muted, L"The clipboard did not answer for this format." };

    // WHAT THE TEXT PAGES PICK FROM: the encodings a format's bytes are read in, and the
    // languages the text is coloured in. Built by the entry point, which is where an application
    // over this viewer adds its own.
    export struct PickLists
    {
        EncodingList encodings;
        LanguageList languages;
        // Resolves the ANSI and OEM code pages from the clipboard's CF_LOCALE before a text page
        // decodes, so a code-page reading follows the locale the text was copied in. Null where
        // the platform has no such notion, and those readings keep the system default.
        void (*readLocale)(Transfer::Offer&) { nullptr };
    };

    // ONE WAY OF SHOWING ONE FORMAT'S CONTENT. A format is not one thing to look at: it has
    // bytes, it can almost always be read as text, and some formats carry more than either says.
    //
    // A page reads WHEN IT IS OPENED and not before. Reading is a transfer out of another process
    // on one of the two platforms, so a format with four representations would pay four of them
    // to fill three pages nobody is looking at.
    //
    // THE PAGE IS THE SCROLL BOX AND NOT ITS BODY. What stands in the body is the page's own
    // choice: text for the representations that decode into text, the byte view for the bytes.
    //
    // THE VIEW GOES HOME WITH THE CONTENT. A read replaces the document the page is showing, so
    // the bars go back to the start and the caret with them - a place in the last document is not
    // a place in this one.
    //
    // ITS STATUS BAR IS IN TWO PIECES, because the two readings change on different clocks. THE
    // STRIP ABOVE says what the page is showing - how much of it there is, or why there is
    // nothing - and is written once, when a format is opened. THE CORNER between the two scroll
    // bars says where the point of interest stands, and is written on every move of a caret or a
    // selection. A sentence needs the strip, being a sentence; a reading that moves belongs in
    // the corner, whose width is stated so that nothing else on the page moves when it does.
    export class RepresentationPage : public ScrollBox
    {
    public:
        // THE READING OWNS ITS LAYOUT. It is a different text on every move, and a text the
        // engine's cache has not seen is an entry it keeps - see WithTextLayout, and the bucket
        // per text in the layout cache.
        using Readout = WithTextLayout<Label>;
    public:
        template<typename... Args>
        explicit RepresentationPage(const CreateParams&, Args&&...);

        // The clipboard's answer for one format, into this page.
        void show(Transfer::Offer&, const Transfer::Format&);
    protected:
        // What this page makes of the format - see show, which sends the view home after it.
        virtual void showContent(Transfer::Offer&, const Transfer::Format&) = 0;
        // What the page is showing, or why it is showing nothing.
        void writeStatus(Text&&);
        // Where the point of interest stands.
        void writeReadout(Text&&);
        // THE CORNER SHORTENS THE HORIZONTAL BAR BY ITS OWN WIDTH AT ALL TIMES, so a page whose
        // reading does not fit the default says so and the bar keeps what is left. Called from a
        // page's constructor, which is before there is a pass to invalidate.
        void setReadoutWidth(float);
        // A control standing after the reading in the corner, sized by itself - after whatever
        // an earlier call put there.
        template<IsControl ControlClass, typename... Args>
        ControlClass& createCornerBar(Args&&...);
        // A control standing at the right end of the strip above, after whatever an earlier
        // call put there. For what a page sets rather than reads: the status stays the strip's
        // body.
        template<IsControl ControlClass, typename... Args>
        ControlClass& createStripBar(Args&&...);
        // The size a reading is written at. Stated here so that two pages cannot disagree about
        // it, and small because the corner is a strip beside a scroll bar.
        static constexpr float k_readoutFontSize = 11.0f;
    private:
        // Where the corner bars stand: made on the first call, since a panel has one right bar
        // and a page that stands nothing after the reading keeps the corner it had.
        [[nodiscard]] StackPanel& cornerBars();
        // The same for the strip above.
        [[nodiscard]] StackPanel& stripBars();
    private:
        static constexpr float k_readoutPaddingX = 8.0f;
        // Between the controls a page stands in the strip, and what they keep clear of its end.
        static constexpr float k_stripBarSpacing = 8.0f;
        static constexpr float k_stripBarPaddingX = 8.0f;
        // What the corner comes to where a page does not say otherwise: enough for a caret's
        // place in a document longer than anything a clipboard carries.
        static constexpr float k_defaultReadoutWidth = 104.0f;
    private:
        Panel& m_topPanel{ createTopBar<Panel>(
            Padding{ 8.0f }
            ) };
        // The strip is one surface: the status as its body, and what a page stands at its right
        // end on the same header - see stripBars.
        Panel& m_strip{ m_topPanel.createBody<Panel>(
            UiElement::Header,
            themeMetrics().page
        ) };
        Label& m_status{ m_strip.createBody<Label>(
            WordWrap::No,
            Padding{ 12.0f, 4.0f },
            VerticalTextAnchor::Center
        ) };
        // The corner in two parts: the reading as its body, and what a page stands after it.
        Panel& m_corner;
        Readout& m_readout;
        StackPanel* m_cornerBars{ nullptr };   // null until a page asks - see cornerBars
        StackPanel* m_stripBars{ nullptr };    // null until a page asks - see stripBars
    };

    // Bases for both: TextPage and TextPreviewPage
    // TextPreviewPage involuntary shares Syntax abilities, though did not ask for them
    export class TextPageBase : public WithBody<RepresentationPage, CodeBox>
    {
    public:
        using Base = WithBody<RepresentationPage, CodeBox>;
    public:
        template<typename... HProps, typename... BProps>
        TextPageBase(const CreateParams&, const PickLists&, HostProps<HProps...>&&,
            BodyProps<BProps...>&&);
    protected:
        // THE TAIL EVERY TEXT PAGE SHARES. The box takes the text, the align pass a text of a new
        // length needs is asked for, and the caret goes back to the start - which is also what
        // writes the corner. THE FORMAT IS THE BYTES' CONTEXT: a page showing a format's bytes as
        // text names it, and the name is put beside the box's question about the language, since
        // a name can settle what a reading of the text would get wrong. A page showing a text
        // decoded from the bytes names nothing, and the text is judged by what it holds.
        void showText(Text&&, const Transfer::Format&);
        void showText(Text&&);
        // The bytes as text, in the encoding picked. A terminated-string format - CF_TEXT and its
        // like - is cut to its content before the pick, so the block's slack sways neither the
        // encoding nor the text; any other is read whole and drops only the run its block is
        // padded with. A NUL left in reads as its symbol, and ends the line where it closes a
        // string. THE PICK STANDS WHERE THE PAGE READS BYTES THROUGH IT: hidden from construction,
        // shown by the first call, never seen on a page whose text was decoded elsewhere.
        [[nodiscard]] std::wstring decode(std::string_view bytes, const Transfer::Format&);
        // The encoding pick has moved. A page that keeps its bytes reads them again.
        virtual void encodingPicked() {}
        // Resolves the code page a code-page reading uses from the offer, before the page decodes.
        // Called with the offer a read has in hand; a page keeps no offer of its own.
        void prepareLocale(Transfer::Offer&);
    private:
        // The rest of showText, once the name is settled.
        void placeText(Text&&);
        void writeCaretReadout();
        // What the pick says: Auto has the box ask, a language is stated to it.
        void languagePicked();
        // The box's question, put to the pick with the format's name and the text.
        void answerLanguage(DetectLanguageEvent&);
        // What the check shows: whether the box breaks its lines.
        void wrapState(GetStateEvent&) const;
        // The check was clicked. The box shapes its lines again, and the bars are re-ranged.
        void wrapClicked();
        [[nodiscard]] static Text wrapLabel();
        // A custom format's name, empty for a standard one, which carries none this side can
        // read.
        [[nodiscard]] static std::wstring nameOf(const Transfer::Format&);
        // Stated like the reading's, so the bar keeps its length whatever name a face reads:
        // room for a name of a dozen characters and the Auto note beside the mark.
        static constexpr float k_pickWidth = 124.0f;
        // Thin, to keep the picks near the reading's height.
        static constexpr float k_pickPaddingX = 6.0f;
        static constexpr float k_pickPaddingY = 1.0f;
    private:
        // The name of the format whose bytes the text is - empty for a standard format, which
        // carries none, and for a text decoded from the bytes rather than read as them.
        std::wstring m_formatName{};
        EncodingPick& m_encodingPick;
        LanguagePick& m_languagePick;
        Checkbox& m_wrapCheck;
        // The platform's locale resolver, or null where there is none - see PickLists::readLocale.
        void (*m_readLocale)(Transfer::Offer&) { nullptr };
    };

    // A REPRESENTATION IS A LINE IN A TABLE: the word on its tab, and how to make the page that
    // shows it. Some belong to every format - a format always has bytes - and some to one format
    // alone, as the text with its markers belongs to the framework's own rich text.
    //
    // The maker is a plain function pointer because nothing about a representation is
    // per-instance: the table is written once and read for every format the clipboard offers.
    // The lists are handed through for the pages that pick, and the rest ignore them.
    export struct Representation
    {
        std::wstring_view name;
        RepresentationPage& (*createPage)(PageControl&, const PickLists&);
    };

    export using RepresentationList = std::vector<Representation>;


//-----------------------------------------------------------------------------


    template<typename... Args>
    RepresentationPage::RepresentationPage(const CreateParams& params, Args&&... args)
        :
        ScrollBox{ params, std::forward<Args>(args)... },
        m_corner{ createCorner<Panel>() },
        m_readout{ m_corner.createBody<Readout>(
            MinSize{ k_defaultReadoutWidth, 0.0f },
            MaxSize{ k_defaultReadoutWidth, k_maxFloat },
            WordWrap::No,
            Padding{ k_readoutPaddingX, 0.0f },
            // Anchored Left, which is what holds a reading still. Centred, a number gaining a
            // digit moves every character of it half a digit sideways.
            HorizontalTextAnchor::Left,
            VerticalTextAnchor::Center
        ) }
    {
    }

    template<IsControl ControlClass, typename... Args>
    ControlClass& RepresentationPage::createCornerBar(Args&&... args)
    {
        return cornerBars().add<ControlClass>(std::forward<Args>(args)...);
    }

    template<IsControl ControlClass, typename... Args>
    ControlClass& RepresentationPage::createStripBar(Args&&... args)
    {
        return stripBars().add<ControlClass>(std::forward<Args>(args)...);
    }

    template<typename... HProps, typename... BProps>
    TextPageBase::TextPageBase(const CreateParams& params, const PickLists& picks,
        HostProps<HProps...>&& hostProps, BodyProps<BProps...>&& bodyProps)
        :
        Base{ params, std::move(hostProps), std::move(bodyProps) },
        m_encodingPick{ createCornerBar<EncodingPick>(
            picks.encodings,
            k_readoutFontSize,
            MinSize{ k_pickWidth, 0.0f },
            MaxSize{ k_pickWidth, k_maxFloat },
            Padding{ k_pickPaddingX, k_pickPaddingY },
            VerticalTextAnchor::Center,
            ShowSurfaceAtRest::Yes,
            ComboBox::OnChange{ [this](ComboboxChangeEvent&) {
                encodingPicked();
            } }
        ) },
        m_languagePick{ createCornerBar<LanguagePick>(
            picks.languages,
            k_readoutFontSize,
            MinSize{ k_pickWidth, 0.0f },
            MaxSize{ k_pickWidth, k_maxFloat },
            Padding{ k_pickPaddingX, k_pickPaddingY },
            VerticalTextAnchor::Center,
            ShowSurfaceAtRest::Yes,
            ComboBox::OnChange{ [this](ComboboxChangeEvent&) {
                languagePicked();
            } }
        ) },
        // THE MARK READS THE BOX AND THE CLICK WRITES IT: the box's own property is the
        // setting.
        m_wrapCheck{ createStripBar<Checkbox>(
            wrapLabel(),
            Padding{ k_pickPaddingX, 0.0f },
            VerticalTextAnchor::Center,
            Checkbox::OnGetState{ [this](GetStateEvent& event) {
                wrapState(event);
            } },
            Checkbox::OnClick{ [this](ClickEvent&) {
                wrapClicked();
            } }
        ) },
        m_readLocale{ picks.readLocale }
    {
        // Until a page reads bytes through it - see decode.
        m_encodingPick.setVisible(false);
        // The connections need no scope: the box and the picks are this page's own and go when
        // it does.
        body().onDetectLanguage([this](DetectLanguageEvent& event) {
            answerLanguage(event);
        });
        // Auto from the start. Safe before there is a form: the box holds no text yet, and
        // invalidate is a no-op until it stands in one.
        body().setDetectLanguage(DetectLanguage::Yes);
        // The box says when its caret came to rest and answers where it now stands; nothing polls
        // it.
        body().onCaretMove([this](CaretMoveEvent&) {
            writeCaretReadout();
        });
        writeCaretReadout();
    }
}
