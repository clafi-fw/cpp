module ClaFi.Showcase.TextEngine.Main;

import ClaFi.Showcase.TextEngine;
import ClaFi.Showcase.TextEngine.Code;
import ClaFi.Showcase.TextEngine.Rich;
import ClaFi.Showcase.TextEngine.Math;
import ClaFi.Showcase.TextEngine.Emoji;
import ClaFi.Showcase.TextEngine.Scripts;
import ClaFi.Showcase.TextEngine.Icon;

import ClaFi.Controls.AppButton;
import ClaFi.Controls.CodeBox;
import ClaFi.Controls.FormTitle;
import ClaFi.Controls.Label;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.TabbedBox;
import ClaFi.Controls.TabStrip;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.Divider;
import ClaFi.Controls.Base.PanelBase;
import ClaFi.Controls.Base.StackPanelBase;

import ClaFi.App.Application;
import ClaFi.App.Themes;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.Syntax.Languages;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Fmt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.System.Utils;
import ClaFi.Core.System.Props;
import ClaFi.StdLib;

namespace ClaFi::Showcase::TextEngine
{
    using namespace ::ClaFi::Controls;

    namespace
    {
        constexpr float k_windowWidth = 800.0f;
        constexpr float k_windowHeight = 500.0f;

        // How many lines each generated page is filled with. A page that is not the selected one
        // is not laid out, so a page's first shaping is paid on the first click of its tab rather
        // than at startup.
        //
        // Set to be FELT rather than to be reasonable: a re-shape is linear in paragraphs, at
        // roughly 5 microseconds each for a line of source, so this is about a quarter of a second
        // per key press on the code page. Both blocks are 50 lines, so a round number here divides
        // exactly. Raising it costs memory in proportion and once per page opened - one native
        // layout is held per paragraph for as long as the page exists.
        constexpr std::size_t k_generatedLines = 50000;

        // The caret readout in a page's corner. A label holding its OWN layout rather than a plain
        // one: the reading is rewritten on every caret move, and a text that changes takes a fresh
        // entry in TextEngine's cache for every value it passes through, evicting the static
        // layouts the rest of the form draws from.
        using CaretReadout = WithTextLayout<Label>;

        // Stated rather than left to the words, and wide enough for the longest reading these
        // pages produce plus the padding it stands behind. A corner that resized itself would
        // shorten the horizontal bar under the pointer every time the caret crossed a power of ten.
        constexpr float k_caretReadoutWidth = 124.0f;

        constexpr MinSize k_tabSize{ 0.0f, 32.0f };
        constexpr VerticalTextAnchor k_tabAnchor{ VerticalTextAnchor::Center };
        constexpr Padding k_tabPadding{ 12.0f, 12.0f };

        // How much of a case its ClaFi page states. A marker is four lines of ClaFi and a line of
        // the ledger carries six or seven of them, so the whole of a generated page is a document
        // of some hundred thousand lines, and the ClaFi page shapes every one of those on the
        // first click of its tab. This many paragraphs keeps that document near the length of the
        // code page itself; a case with no more than this is stated whole.
        constexpr std::size_t k_claFiParagraphs = 2000;

        // The readout stands in the corner - the square the two bars leave between them - so it
        // shows only while both of a page's bars are up, which is what every page states. The
        // body is named apart from its box because a page holds a TextBox or a CodeBox, and the
        // readout asks the same of either.
        void showCaretPosition(ScrollBox& page, TextBox& body)
        {
            CaretReadout* readout = &page.createCorner<CaretReadout>(
                Text{ TextStyleId::SubBody, L"Ln 1, Col 1" },
                MinSize{ k_caretReadoutWidth, 0.0f },
                MaxSize{ k_caretReadoutWidth, k_maxFloat },
                WordWrap::No,
                Padding{ 8.0f, 0.0f },
                // Anchored Left, which is what holds the reading still. Centred, a line number
                // gaining or losing a digit moves every character of both readings half a digit
                // sideways, and the caret crossing back and forth over ten shakes the whole of it.
                HorizontalTextAnchor::Left,
                VerticalTextAnchor::Center
            );
            body.onCaretMove([readout](CaretMoveEvent& event){
                const TextLineColumn caret = event.sender().caretLineColumn();
                readout->text() = Text{
                    TextStyleId::SubBody,
                    Fmt{ L"Ln {}, Col {}", caret.line, caret.column }
                };
                // The width is stated, so a new reading moves nothing on the page: a repaint of the
                // corner is the whole of what a caret move costs outside the box it moved in.
                readout->invalidate();
            });
        }

        // Where the excerpt a ClaFi page states ends: past the k_claFiParagraphs-th paragraph, or
        // past the whole text where it has no more than that.
        [[nodiscard]] std::size_t claFiExcerptEnd(const std::wstring& plainText)
        {
            std::size_t end = 0;
            for (std::size_t paragraph = 0; paragraph != k_claFiParagraphs; ++paragraph)
            {
                end = plainText.find(L'\n', end);
                if (end == std::wstring::npos)
                    return plainText.size();
                ++end;
            }
            return end;
        }

        [[nodiscard]] std::size_t paragraphCount(const std::wstring& plainText)
        {
            const std::size_t newlines = static_cast<std::size_t>(std::ranges::count(plainText, L'\n'));
            if (plainText.empty() || plainText.back() == L'\n')
                return newlines;
            return newlines + 1;
        }
    }

    AppParams textEngineAppParams()
    {
        return {
            .name = Text{ L"TextEngine" },
            .publisher = L"ClaFi Framework",
            .description = Text{
                L"Shows the ClaFi text engine at work: a long styled ledger, highlighted code, "
                L"formulas, emoji and fifty-six writing systems."
            }
        };
    }

    int runTextEngineShowcase(ApplicationBase& application)
    {
        // THE APPLICATION STATES ITS OWN DEFAULT, NOT THE USER'S CHOICE. Where a config
        // folder stands the mode has just been loaded out of it, and writing over it here
        // would undo the Settings page on every start.
        if (!application.configFolderExists())
            applyColorMode(application.context(), ColorModeSetting::Light);

        const auto form = application.createDialog<MainForm>(
            HostProps{
                FormPlacement::Default,
                MinSize{ k_windowWidth, k_windowHeight },
                PreferredSize{ k_windowWidth * 1.2, k_windowHeight * 1.5 },
                application.metrics().primaryWindow,
                application.metrics().primaryWindowShadow,
                UiElement::Section,
            },
            BodyProps{
                TabsOrientation::VerticalLeft,
            }
        );
        form->setConfigName(AppContext::k_mainFormName);

        TabbedBox& tabs = form->body();

        // Every case is a box of two pages: the text, and what a document would say of it.
        ScrollBoxWith<TextBox>& featuresPage = tabs.pageControl().add<ScrollBoxWith<TextBox>>(
            HostProps{
                ScrollBars::Both,
            },
            BodyProps{
                buildTextEngineShowcase(),
                ReadOnly::Yes,
                Padding{ 16.0f },
            }
        );

        ScrollBoxWith<CodeBox>& codePage = tabs.pageControl().add<ScrollBoxWith<CodeBox>>(
            HostProps{
                ScrollBars::Both,
            },
            BodyProps{
                buildCodeShowcase(k_generatedLines),
                Syntax::Languages::cpp,
                Padding{ 16.0f },
            }
        );

        ScrollBoxWith<TextBox>& ledgerPage = tabs.pageControl().add<ScrollBoxWith<TextBox>>(
            HostProps{
                ScrollBars::Both,
            },
            BodyProps{
                buildRichShowcase(k_generatedLines / 10),
                ReadOnly::Yes,
                Padding{ 16.0f },
            }
        );

        ScrollBoxWith<TextBox>& emojiPage = tabs.pageControl().add<ScrollBoxWith<TextBox>>(
            HostProps{
                ScrollBars::Both,
            },
            BodyProps{
                buildEmojiShowcase(),
                Padding{ 16.0f },
            }
        );

        // Fifty-six writing systems, read only: what is asked is whether the machine has a face
        // for each, and no key press changes the answer. Wrapping is left on, so a row too wide
        // for the box folds rather than running under the scroll bar.
        ScrollBoxWith<TextBox>& scriptsPage = tabs.pageControl().add<ScrollBoxWith<TextBox>>(
            HostProps{
                ScrollBars::Both,
            },
            BodyProps{
                buildScriptsShowcase(),
                ReadOnly::Yes,
                Padding{ 16.0f },
            }
        );

        // Notation instead of prose. Written once and read only, like the showcase page beside
        // it: what a formula costs to shape is the Rich page's question, and this one is about
        // what can be said at all.
        ScrollBoxWith<TextBox>& formulasPage = tabs.pageControl().add<ScrollBoxWith<TextBox>>(
            HostProps{
                ScrollBars::Both,
            },
            BodyProps{
                buildMathShowcase(),
                ReadOnly::Yes,
                Padding{ 16.0f },
            }
        );

        showCaretPosition(featuresPage, featuresPage.body());
        showCaretPosition(codePage, codePage.body());
        showCaretPosition(ledgerPage, ledgerPage.body());
        showCaretPosition(formulasPage, formulasPage.body());
        showCaretPosition(emojiPage, emojiPage.body());
        showCaretPosition(scriptsPage, scriptsPage.body());

        tabs.strip().addTab(
            Text{ PushFontFamily{ L"Gabriola" }, L"The Ledger", PopFontFamily{} },
            k_tabPadding, k_tabSize, k_tabAnchor,
            Page{ ledgerPage }
        )
        .select();

        tabs.strip().addTab(
            Text{ TextStyleId::Code, L"Syntax" },
            k_tabPadding, k_tabSize, k_tabAnchor,
            Page{ codePage }
        );

        tabs.strip().addTab(
            Text{ PushFontFamily{ L"Cambria" }, TextOp::PushItalic, L"Formulas", TextOp::PopItalic, PopFontFamily{} },
            k_tabPadding, k_tabSize, k_tabAnchor,
            Page{ formulasPage }
        );

        // The emoji stands INSIDE the word, so the label is one run broken into three by a face
        // change and closed up again. A tab measures and draws through the same path a page does,
        // over a MinSize of 32 and a centred anchor - so a label taller than the strip allows for,
        // or a gap where the faces meet, shows here and nowhere else on the form.
        tabs.strip().addTab(
            Text{ L"Em😮ji" },
            k_tabPadding, k_tabSize, k_tabAnchor,
            Page{ emojiPage }
        );

        tabs.strip().addTab(
            Text{ L"Scripts" },
            k_tabPadding, k_tabSize, k_tabAnchor,
            Page{ scriptsPage }
        );

        tabs.strip().add<Controls::Divider>(
            Thickness::Heavy,
            Padding{ 48.0f, 8.0f }
        );

        tabs.strip().addTab(
            Text{ L"Features reference" },
            k_tabPadding, k_tabSize, k_tabAnchor,
            Page{ featuresPage }
        );

        // The title is the app button's container, and its padding is what stands between
        // the mark and the corner of the window.
        FormTitle& title = form->createTopBar<FormTitle>(application.name(), Padding{ 0.0f });
        title.createLeftBar<AppButton>(
            VerticalAlign::Center,
            AppButton::OnPaintIcon{ AppIcon::paintIcon }
        );

        return form->execute();
    }
}
