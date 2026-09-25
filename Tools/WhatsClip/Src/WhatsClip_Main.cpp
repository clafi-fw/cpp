module ClaFi.Tools.WhatsClip.Main;

import ClaFi.Tools.WhatsClip.BytesPage;
import ClaFi.Tools.WhatsClip.FormatView;
import ClaFi.Tools.WhatsClip.HandlePage;
import ClaFi.Tools.WhatsClip.Icon;
import ClaFi.Tools.WhatsClip.Page;
import ClaFi.Tools.WhatsClip.PicturePage;
import ClaFi.Tools.WhatsClip.TextPage;
import ClaFi.Tools.WhatsClip.TextPreviewPage;

import ClaFi.StdActions.Transfer;

import ClaFi.Controls.AppButton;
import ClaFi.Controls.Divider;
import ClaFi.Controls.FormTitle;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Spacer;
import ClaFi.Controls.TabbedBox;
import ClaFi.Controls.TabStrip;
import ClaFi.Controls.Base.PanelBase;
import ClaFi.Controls.Base.StackPanelBase;

import ClaFi.App.Application;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Transfer.Clipboard;
import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.Core.System.Props;
import ClaFi.StdLib;
import ClaFi.Core.Syntax.Types;

namespace ClaFi::Tools::WhatsClip
{
    using namespace ::ClaFi::Controls;

    namespace
    {
        // STATED RATHER THAN FITTED. A format's name is whatever the application that copied
        // decided to call it, from a word to a MIME type, and a strip that took its width from the
        // longest of them would change width every time the clipboard did. The names wrap instead.
        // Room for the platform's own CF_ names on one line.
        constexpr float k_stripWidth = 200.0f;

        // The muted line under a tab's title, small against the title above it: what a preview
        // was read from, or that Windows derived the format.
        constexpr float k_tabNoteSize = 11.0f;
        constexpr std::wstring_view k_synthesizedNote = L"synthesized";
        constexpr std::wstring_view k_previewNote = L"from ";

        // What a tab keeps around its caption, and what the divider under the previews keeps
        // clear at the ends of its line.
        constexpr float k_tabPaddingX = 24.0f;
        constexpr float k_tabPaddingY = 12.0f;
        constexpr Padding k_dividerPadding = { 24.0f, 12.0f };

        // WHAT TO CALL A FORMAT: its platform name, in whatever spelling the platform handed
        // over. An offer lists platform formats alone - the framework's own are what a paste
        // asks for, and this viewer never does - so a nameless one cannot reach here.
        [[nodiscard]] std::wstring displayName(const Transfer::Format& format)
        {
            if (format.kind() == Transfer::Format::Kind::Custom)
                return format.name();

            return L"Unnamed framework format";
        }

        // WHICH FORMATS WINDOWS MAKES FROM WHICH. Each group is listed richest first: a source
        // places one and the system derives the rest, so a format is synthesized when a richer one
        // in its group is also on the clipboard. There is no flag for this - it is read from the
        // rules, so a format a source placed alongside its richer form reads as synthesized too.
        constexpr std::wstring_view k_textGroup[] = { L"CF_UNICODETEXT", L"CF_TEXT", L"CF_OEMTEXT" };
        constexpr std::wstring_view k_pictureGroup[] = { L"CF_DIBV5", L"CF_DIB" };
        constexpr std::wstring_view k_metafileGroup[] = { L"CF_ENHMETAFILE", L"CF_METAFILEPICT" };

        // The image formats a CF_BITMAP handle stands in for. CF_BITMAP is a GDI object, not a
        // wire format a source serializes, so where any of these is on the clipboard the bitmap is
        // the system's derived form of it.
        constexpr std::wstring_view k_imageFormats[] = { L"PNG", L"image/png", L"CF_DIBV5", L"CF_DIB" };

        [[nodiscard]] bool isPresent(const Transfer::FormatList& formats, const std::wstring_view name)
        {
            return std::ranges::any_of(formats, [name](const Transfer::Format& format) {
                return format.kind() == Transfer::Format::Kind::Custom && format.name() == name;
            });
        }

        // Whether a richer member of the group - one listed before this name - is on the clipboard.
        // A name the group does not hold answers false: the walk ends without ever meeting it.
        [[nodiscard]] bool derivedInGroup(const std::span<const std::wstring_view> group,
            const std::wstring_view name, const Transfer::FormatList& formats)
        {
            bool richerPresent = false;
            for (const std::wstring_view member : group)
            {
                if (member == name)
                    return richerPresent;
                if (isPresent(formats, member))
                    richerPresent = true;
            }

            return false;
        }

        // Whether Windows derived this format rather than the source placing it - see k_textGroup.
        // CF_LOCALE is the companion the system adds beside any text.
        [[nodiscard]] bool isSynthesized(const Transfer::Format& format,
            const Transfer::FormatList& formats)
        {
            if (format.kind() != Transfer::Format::Kind::Custom)
                return false;

            const std::wstring name = format.name();
            if (name == L"CF_LOCALE")
            {
                return isPresent(formats, L"CF_UNICODETEXT")
                    || isPresent(formats, L"CF_TEXT")
                    || isPresent(formats, L"CF_OEMTEXT");
            }

            // CF_BITMAP is the derived GDI handle whenever a real image format is present - see
            // k_imageFormats - not only when a richer DIB happens to be listed.
            if (name == L"CF_BITMAP")
            {
                return std::ranges::any_of(k_imageFormats, [&formats](const std::wstring_view image) {
                    return isPresent(formats, image);
                });
            }

            return derivedInGroup(k_textGroup, name, formats)
                || derivedInGroup(k_pictureGroup, name, formats)
                || derivedInGroup(k_metafileGroup, name, formats);
        }

        template<typename PageClass>
        [[nodiscard]] RepresentationPage& makeTextPage(PageControl& pages, const PickLists& picks)
        {
            return pages.add<PageClass>(picks);
        }

        [[nodiscard]] RepresentationPage& makeBytesPage(PageControl& pages, const PickLists&)
        {
            return pages.add<BytesPage>();
        }

        [[nodiscard]] RepresentationPage& makeHandlePage(PageControl& pages, const PickLists&)
        {
            return pages.add<HandlePage>();
        }

        [[nodiscard]] RepresentationPage& makePicturePage(PageControl& pages, const PickLists&)
        {
            return pages.add<PicturePage>();
        }

        // A PREVIEW IS REGISTERED AGAINST A STANDARD FORMAT KIND AND NEVER AGAINST A PLATFORM
        // SPELLING. Which spelling serves a kind is the platform's own ranking - a PNG ahead of a
        // DIB, the GDI bitmap last and never while a PNG is listed - and Offer::platformFormat is
        // where that is answered, so this viewer keeps no table of its own. The order here is the
        // order the tabs stand in, which is also the one that opens.
        struct Preview
        {
            Transfer::StandardFormat kind;
            std::wstring_view name;
            RepresentationPage& (*createPage)(PageControl&, const PickLists&);
        };

        constexpr Preview k_previews[] = {
            { Transfer::StandardFormat::Picture, L"Picture preview", &makePicturePage },
            { Transfer::StandardFormat::Text, L"Text preview", &makeTextPage<TextPreviewPage> }
        };

        // The preview tabs and the divider under them stand ahead of every format tab and are
        // never moved, so a format's place in the strip is its place in the clipboard's list past
        // these - see Viewer::orderTabs.
        constexpr std::size_t k_leadingStripControls = std::size(k_previews) + 1;

        // WHAT A PREVIEW OF ONE KIND READS, AND WHAT TO CALL WHERE IT CAME FROM, which are not the
        // same format: the pixels are asked for as the framework's own Picture, and the name is
        // the platform spelling that answered for it.
        struct PreviewSource
        {
            Transfer::Format read;
            std::wstring name;
        };

        // THE FRAMEWORK'S OWN RICH TEXT STANDS AHEAD OF PLAIN TEXT - the same words with their
        // markers on. It is the one preference stated here rather than below: ClaFi.Text is
        // declared above the Transfer core, so no platform spelling table can name it.
        //
        // Nothing where the clipboard cannot serve the kind at all, which is what takes its tab
        // down.
        [[nodiscard]] std::optional<PreviewSource> previewSourceOf(const Transfer::Offer& offer,
            const Transfer::StandardFormat kind)
        {
            if (kind == Transfer::StandardFormat::Text)
            {
                const Transfer::Format own = Transfer::ClaFiText::format();
                const Transfer::FormatList advertised = offer.advertisedFormats();
                if (std::ranges::find(advertised, own) != advertised.end())
                    return PreviewSource{ own, own.name() };
            }

            const Transfer::Format* served = offer.platformFormat(kind);
            if (!served)
                return std::nullopt;

            return PreviewSource{ Transfer::Format::of(kind), served->name() };
        }

        // THE REPRESENTATION REGISTRY. A format is not one thing to look at: it has bytes, and a
        // byte string meant to be read is text. WHAT THE CLIPBOARD IS WORTH SEEING AS - a picture,
        // a text with its markers on - is not one format's representation but a reading of the
        // whole clipboard, and stands on a tab of its own above the divider; see k_previews.
        //
        // A FORMAT WHOSE ENTRY IS A HANDLE HAS NEITHER OF THE TWO. Nothing travelled for it, so a
        // hex dump would be of bytes this viewer made and a reading as text would be of the same;
        // what it has is one number, and the Handle representation is the only one it is offered.
        //
        // The order is the order the tabs stand in, and the first one is the one that opens.
        [[nodiscard]] RepresentationList representationsOf(const Transfer::Format& format)
        {
            RepresentationList result{};
            if (HandlePage::carriesHandle(format))
            {
                result.push_back({ L"Handle", &makeHandlePage });
                return result;
            }

            result.push_back({ L"Bytes", &makeBytesPage });
            result.push_back({ L"As Text", &makeTextPage<TextPage> });

            return result;
        }

        // A tab whose caption can be read whole and copied out. A format's name is what a user
        // takes away from this viewer, and a tab draws its caption without letting it be
        // selected - so F2, or a click on the name of the tab already open, puts a reader over it.
        class ReaderTab : public WithInPlaceEdit<Tab>
        {
        public:
            using WithInPlaceEdit::WithInPlaceEdit;
        public:
            // The format name the reader copies, whatever else the caption shows.
            void setFormatName(std::wstring value) { m_formatName = std::move(value); }
            // The title, and the muted line under it where there is one to write.
            void setCaption(std::wstring_view title, std::wstring_view note);
        protected:
            [[nodiscard]] EditorMode editorMode() const override { return EditorMode::ReadOnly; }
            // The caption may carry the note; the reader takes the format name alone.
            void getEditorText(Text& text) const override { text << m_formatName; }
        private:
            std::wstring m_formatName{};
        };

        // WHAT IS ON SCREEN AGAINST WHAT IS ON THE CLIPBOARD. A tab per format ever seen, hidden
        // while its format is absent and shown again when it comes back - so the strip settles
        // rather than being rebuilt. WHICH PRESENTATION IS OPEN IS NOT THE VIEW'S TO KEEP: it is
        // held here and every view reads and writes it, so walking the formats reads them all the
        // same way. Above them stand the previews, made once and belonging to the clipboard
        // rather than to any one format.
        class Viewer
        {
        public:
            Viewer(TabbedBox&, const PickLists&);

            // Brings the strip in line with the clipboard, and reads whichever page is open.
            void refresh();
        private:
            struct FormatTab
            {
                Transfer::Format format;
                ReaderTab* tab;
                FormatView* view;
                // Whether the view has read the clipboard as it now stands. A view reads when it
                // is opened and not before: reading is a transfer out of another process on one of
                // the two platforms, and the formats nobody is looking at would pay for it too.
                // Which of its representations reads is the view's own affair.
                bool current;
            };

            struct PreviewTab
            {
                Transfer::StandardFormat kind;
                std::wstring_view name;
                ReaderTab* tab;
                RepresentationPage* page;
                // What this clipboard serves the kind from, and nothing where it cannot serve it
                // at all - which is what hides the tab.
                std::optional<PreviewSource> source;
                // Whether the page has read the clipboard as it now stands - see FormatTab.
                bool current;
            };

            using FormatTabList = std::vector<FormatTab>;
            using PreviewTabList = std::vector<PreviewTab>;

            [[nodiscard]] FormatTab* entryFor(const Control*);
            [[nodiscard]] PreviewTab* previewFor(const Control*);
            void addPreviewTab(const Preview&);
            void addTabFor(const Transfer::Format&);
            void showPreviews(const Transfer::Offer*);
            void orderTabs(const Transfer::FormatList&);
            void selectFirstVisible();
            void showCurrentPage();
        private:
            TabbedBox& m_box;
            const PickLists& m_picks;
            PreviewTabList m_previewTabs;
            FormatTabList m_formatTabs;
            // Between the readings of the clipboard and the formats it is made of, and down with
            // them where there is no reading to show. Set once, by the constructor.
            Divider* m_divider{ nullptr };
            // The presentation being browsed in - Bytes or As Text - which every format view both
            // reads and writes. Empty until the first view with a choice in it settles one.
            std::wstring_view m_openRepresentation{};
            // SCOPED, because the page control outlives this class. A handler left connected to a
            // live control after the object it captured is gone is called with a dangling this.
            ScopedEventConnection m_pageChanged;
            // The clipboard the strip was last brought in line with. A GENERATION RATHER THAN THE
            // OFFER: two copies made here in a row are one static object at one address, and an
            // offer this side does not own can be freed and its address handed to the next one.
            std::uint64_t m_generation{ 0 };
        };

        void ReaderTab::setCaption(const std::wstring_view title, const std::wstring_view note)
        {
            Text caption{};
            caption << title;
            if (!note.empty())
            {
                caption << L"\n";
                caption << InkGrade::Muted;
                caption << PushFontSize{ k_tabNoteSize };
                caption << note;
            }

            text() = std::move(caption);
            invalidateFormAlign();
        }

        Viewer::Viewer(TabbedBox& box, const PickLists& picks)
            :
            m_box{ box },
            m_picks{ picks }
        {
            m_box.pageControl().setPlaceHolderText(Text{ L"The clipboard is empty." });
            for (const Preview& preview : k_previews)
                addPreviewTab(preview);

            // A STRIP TAKES A CONTROL THAT IS NOT A TAB: its paint order draws those before the
            // tabs and its focus test answers for tabs alone, so the line is drawn and can never
            // become the current item. Added here, which puts it after every preview tab and
            // before every format tab - a container lays its children out in the order it holds
            // them.
            m_divider = &m_box.strip().add<Divider>(Thickness::Heavy, k_dividerPadding);

            m_pageChanged = m_box.pageControl().connectEvent([this](CurrentItemChangeEvent&) {
                showCurrentPage();
            });
            refresh();
        }

        void Viewer::refresh()
        {
            const std::uint64_t generation = m_box.formContext().clipboard().generation();
            if (generation == m_generation)
                return;

            m_generation = generation;
            const Transfer::Offer* offer = m_box.formContext().clipboard().offer();
            const Transfer::FormatList formats = offer
                ? offer->advertisedFormats()
                : Transfer::FormatList{};

            showPreviews(offer);

            // A TAB IS NEVER TAKEN AWAY, only hidden. One whose format comes back is the same tab
            // in the place it was, showing the page it always was.
            for (FormatTab& entry : m_formatTabs)
            {
                const bool onClipboard = std::ranges::find(formats, entry.format) != formats.end();
                entry.tab->setVisible(onClipboard);
                entry.current = false;
            }

            for (const Transfer::Format& format : formats)
            {
                const bool seen = std::ranges::any_of(m_formatTabs,
                    [&format](const FormatTab& entry) {
                        return entry.format == format;
                    });
                if (!seen)
                    addTabFor(format);
            }

            // The synthesized note follows the clipboard as it now stands - a format placed on one
            // copy can be derived on the next - so it is set on every refresh, not once when the
            // tab is made. A hidden tab stands for a format this clipboard does not carry.
            for (const FormatTab& entry : m_formatTabs)
            {
                if (!entry.tab->visible())
                    continue;

                const std::wstring name = displayName(entry.format);
                const bool derived = isSynthesized(entry.format, formats);
                entry.tab->setCaption(name, derived ? k_synthesizedNote : std::wstring_view{});
            }

            orderTabs(formats);

            // THE OPEN TAB CAN BE THE ONE THAT WENT. Leaving it selected would leave its page on
            // screen reading a clipboard that has moved on, so the first tab still standing is
            // opened instead, and with none left the strip is cleared. It is also what opens the
            // first tab of all.
            const Control* open = m_box.pageControl().currentItem();
            const PreviewTab* preview = previewFor(open);
            const FormatTab* entry = entryFor(open);
            const bool stands = (preview && preview->tab->visible())
                || (entry && entry->tab->visible());
            if (!stands)
                selectFirstVisible();

            showCurrentPage();
        }

        Viewer::FormatTab* Viewer::entryFor(const Control* page)
        {
            for (FormatTab& entry : m_formatTabs)
            {
                if (entry.view == page)
                    return &entry;
            }

            return nullptr;
        }

        Viewer::PreviewTab* Viewer::previewFor(const Control* page)
        {
            for (PreviewTab& entry : m_previewTabs)
            {
                if (entry.page == page)
                    return &entry;
            }

            return nullptr;
        }

        void Viewer::addPreviewTab(const Preview& preview)
        {
            RepresentationPage& page = preview.createPage(m_box.pageControl(), m_picks);
            ReaderTab& tab = m_box.strip().add<ReaderTab>(
                Text{ preview.name },
                WordWrap::Yes,
                Padding{ k_tabPaddingX, k_tabPaddingY },
                MinSize{ k_stripWidth, 0.0f },
                MaxSize{ k_stripWidth, k_maxFloat },
                Page{ page }
            );
            m_previewTabs.push_back(
                PreviewTab{ preview.kind, preview.name, &tab, &page, std::nullopt, false });
        }

        void Viewer::addTabFor(const Transfer::Format& format)
        {
            FormatView& view = m_box.pageControl().add<FormatView>();
            // BEFORE THE REPRESENTATIONS, which is what opens one of them.
            view.setPreference(m_openRepresentation);
            view.setRepresentations(representationsOf(format), m_picks);
            ReaderTab& tab = m_box.strip().add<ReaderTab>(
                Text{ displayName(format) },
                WordWrap::Yes,
                Padding{ k_tabPaddingX, k_tabPaddingY },
                MinSize{ k_stripWidth, 0.0f },
                MaxSize{ k_stripWidth, k_maxFloat },
                Page{ view }
            );
            // The reader copies this name alone; the caption may also carry the synthesized note.
            tab.setFormatName(displayName(format));
            m_formatTabs.push_back(FormatTab{ format, &tab, &view, false });
        }

        // WHAT THE CLIPBOARD IS WORTH LOOKING AT, asked of the offer rather than of its format
        // list: a kind is served by whichever spelling the platform ranks first, and the tab says
        // which that was. A kind this clipboard cannot serve takes its tab down, and with every
        // preview down the divider goes with them.
        void Viewer::showPreviews(const Transfer::Offer* offer)
        {
            bool anyShown = false;
            for (PreviewTab& entry : m_previewTabs)
            {
                entry.source = offer ? previewSourceOf(*offer, entry.kind) : std::nullopt;
                entry.current = false;
                entry.tab->setVisible(entry.source.has_value());
                if (!entry.source)
                    continue;

                anyShown = true;
                const std::wstring note = std::wstring{ k_previewNote } + entry.source->name;
                entry.tab->setFormatName(entry.source->name);
                entry.tab->setCaption(entry.name, note);
            }

            m_divider->setVisible(anyShown);
        }

        // THE ORDER THE PLATFORM GAVE THEM IN, which a strip made a tab at a time does not keep on
        // its own: a tab is added when its format is first seen and stays for good, so the strip
        // would stand in the order this viewer met the formats rather than the order this
        // clipboard lists them. The list is ordered, and an application that copies names its
        // best format first - so where a tab stands says something its name does not.
        //
        // A FORMAT THIS CLIPBOARD DOES NOT CARRY HAS NO PLACE IN THAT ORDER, its tab being hidden.
        // Those trail after the listed ones, keeping the order they had between them.
        //
        // THE PREVIEWS AND THEIR DIVIDER ARE NOT IN THIS WALK. They stand where they were made,
        // which is what k_leadingStripControls counts.
        void Viewer::orderTabs(const Transfer::FormatList& formats)
        {
            const auto placeOf = [&formats](const FormatTab& entry) {
                const Transfer::FormatList::const_iterator found
                    = std::ranges::find(formats, entry.format);
                return found - formats.begin();
            };

            std::ranges::stable_sort(m_formatTabs, std::ranges::less{}, placeOf);

            std::size_t place = k_leadingStripControls;
            for (const FormatTab& entry : m_formatTabs)
            {
                m_box.strip().moveControl(*entry.tab, place);
                ++place;
            }
        }

        // A preview opens where the clipboard has one, the picture ahead of the text, and the
        // clipboard's own first format where it has none.
        void Viewer::selectFirstVisible()
        {
            for (const PreviewTab& entry : m_previewTabs)
            {
                if (!entry.tab->visible())
                    continue;

                entry.tab->select();
                return;
            }

            for (const FormatTab& entry : m_formatTabs)
            {
                if (!entry.tab->visible())
                    continue;

                entry.tab->select();
                return;
            }

            // Nothing to open. Clearing the strip takes the page of the tab that went down with
            // it, and the page control says why in its place.
            m_box.strip().setCurrentItem(nullptr);
        }

        void Viewer::showCurrentPage()
        {
            const Control* open = m_box.pageControl().currentItem();
            PreviewTab* preview = previewFor(open);
            FormatTab* entry = entryFor(open);
            if (!preview && !entry)
                return;

            // A FORMAT OPENS ON THE PRESENTATION BEING BROWSED IN rather than on the one it was
            // last left showing, which may have been picked before the presentation moved on.
            if (entry)
                entry->view->openPreferred();

            if (preview && (preview->current || !preview->source))
                return;
            if (entry && entry->current)
                return;

            // ASKED FOR AGAIN RATHER THAN KEPT. An offer belongs to Transfer and is freed the
            // moment anything else reads a clipboard that has moved on - the paste action polls
            // for its state far more often than this page is opened.
            Transfer::Offer* offer = m_box.formContext().clipboard().offer();
            if (!offer)
                return;

            if (preview)
            {
                preview->page->show(*offer, preview->source->read);
                preview->current = true;
                return;
            }

            entry->view->show(*offer, entry->format);
            entry->current = true;
        }
    }

    AppParams applicationParams()
    {
        return {
            .name = Text{
            L"What",
            InkGrade::Muted, L"s", PopColor{},
            InkWell::spotInk(), L"Clip", PopColor{}
            },
            .publisher = L"ClaFi Framework",
            .description = Text{
                L"Shows what is on the clipboard, format by format, and follows it as it changes. "
                L"A format reads as a picture, as text in a chosen encoding, or as raw bytes."
            }
        };
    }

    namespace
    {
        class MainForm : public WithBody<Panel, TabbedBox>
        {
        public:
            explicit MainForm(const CreateParams& params);
        public:
        };

        // The formats down the left, whichever one is open on the right. A clipboard is a list of
        // formats and one of them at a time is worth looking at, which is what a tabbed box is.
        //class MainForm : public WithBody<Panel, TabbedBox>
        //{
        //public:
        //    using WithBody::WithBody;
        //};

        MainForm::MainForm(const CreateParams& params)
            :
            WithBody{
                params,
                HostProps{ // Root Panel
                    FormPlacement::Default,
                    params.themeMetrics().primaryWindow,
                    params.themeMetrics().primaryWindowShadow,

                    UiElement::Page, // Root window color, visible behind the formats bar
                    Border{ Thickness::Regular },
                    Padding{ 0.0f },
                    Spacing{ 0.0f },
                    MinSize{ 422.0f, 280.0f },
                    PreferredSize{ 760.0f, 560.0f },
                },
                BodyProps{ // TabbedBox
                    TabsOrientation::VerticalLeft,
                    TabLineThickness{ Thickness::Bold }
                }
            }
        {
            body().pageControl().setBorder(Thickness::None);
            body().pageControl().setColorRules(UiElement::Page);
        }
    }

    int runWhatsClip(ApplicationBase& application, const PickLists& picks)
    {
        const std::unique_ptr<Form<MainForm>> form = application.createDialog<MainForm>();
        form->setConfigName(AppContext::k_mainFormName);

        Viewer viewer{ form->body(), picks };

        // NOTHING POLLS. Transfer raises this when the display server says the selection moved
        // on. Under Wayland that is the clipboard manager's channel this application asks for at
        // startup, or, on a compositor offering none, the moment this window takes the keyboard
        // focus - see the Wayland entry point.
        //
        // Declared after the viewer so that it is destroyed before it: the dispatcher is the
        // clipboard's, which the platform keeps up past every window, so the connection is the
        // shorter-lived half and is the half that has to be scoped.
        const ScopedEventConnection watchClipboard = form->context().clipboard().events()
            .connect<Transfer::ClipboardChangeEvent>([&viewer](Transfer::ClipboardChangeEvent&) {
                viewer.refresh();
            });

        // The title is the app button's container, and its padding is what stands between the
        // mark and the corner of the window.
        FormTitle& title = form->createTopBar<FormTitle>(
            HorizontalTextAnchor::Left,
            application.name(),
            Padding{ 0.0f },
            Spacing{ 4.0f }
        );
        title.createLeftBar<AppButton>(
            VerticalAlign::Center,
            AppButton::OnPaintIcon{ AppIcon::paintIcon }
        );

        return form->execute();
    }
}
