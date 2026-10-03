module ClaFi.App.Information;

import ClaFi.Controls.Button;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.TextBox;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.UpdateCheck;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    using namespace Controls;

    namespace
    {
        // The page's text - name, version and the check's answer, site, description, framework.
        [[nodiscard]] Text informationText(const AppContext&);
        // What the version line adds for the check's answer - nothing until there is one.
        [[nodiscard]] std::wstring_view versionRemark(UpdateState);
        // Appends a plain paragraph, every web address in it made a link.
        void appendLinked(Text&, std::wstring_view paragraph);
        // The address as the page shows it - the scheme left out.
        [[nodiscard]] std::wstring_view shownAddress(std::wstring_view address);

        // The scheme every address the page links starts with.
        constexpr std::wstring_view k_scheme{ L"https://" };
        // The room a page of options keeps around its column, so the name starts where options do.
        constexpr float k_pagePadding{ 12.0f };
        // Between the text and the button, as between the groups of a page of options.
        constexpr float k_pageSpacing{ 10.0f };
        // The longest a line runs - a description wraps at it rather than widening the backstage.
        constexpr float k_lineWidth{ 400.0f };
    }

    // InformationPage

    InformationPage::InformationPage(const CreateParams& params)
        :
        StackPanel{
            params,
            Orientation::Vertical,
            Padding{ k_pagePadding },
            Spacing{ k_pageSpacing }
        },
        m_text{ add<TextBox>(
            ReadOnly::Yes,
            Padding{ 0.0f },
            HorizontalAlign::Left,
            MaxSize{ k_lineWidth, k_maxFloat },
            informationText(params.appContext())
        ) },
        m_checkButton{ add<Button>(
            L"Check for updates",
            HorizontalAlign::Left
        ) },
        m_updateConnection{ appContext().updateCheck().onChange([this](UpdateCheckEvent&) {
            showUpdateState();
        }) }
    {
        // Greyed while the question is out, so a second press cannot ask it twice.
        m_checkButton.onGetState([this](GetStateEvent& event) {
            event.state.enabled = appContext().updateCheck().state() != UpdateState::Checking;
        });
        m_checkButton.onClick([this](ClickEvent&) {
            appContext().updateCheck().start();
        });
        stateCheckButton();
    }

    // THE ANSWER IS TEXT, so once there is one the button goes and the text says it. A check that
    // got no answer keeps the button, offering to ask again.
    void InformationPage::stateCheckButton()
    {
        const UpdateCheck& updates = appContext().updateCheck();
        const UpdateState state = updates.state();
        const bool answered = state == UpdateState::Latest || state == UpdateState::Newer;
        m_checkButton.setVisible(updates.available() && !answered);
        if (state == UpdateState::Failed)
            m_checkButton.text() = Text{ L"Try again" };
        m_checkButton.invalidateState();
    }

    void InformationPage::showUpdateState()
    {
        m_text.text() = informationText(appContext());
        stateCheckButton();
        invalidateFormAlign();
    }

    // informationText

    namespace
    {
        Text informationText(const AppContext& context)
        {
            const UpdateCheck& updates = context.updateCheck();
            Text text{
                TextStyleId::Title, context.appName(), PopTextStyle{},
                k_endLine
            };
            if (!context.version().empty())
            {
                text << InkGrade::Muted;
                text << L"Version ";
                text << context.version();
                text << versionRemark(updates.state());
                text << PopColor{};
                text << k_endLine;
            }
            if (updates.state() == UpdateState::Newer)
            {
                const std::wstring address = updates.releaseAddress();
                text << L"Version ";
                text << updates.newerVersion();
                text << L" is out: ";
                text << PushLink{ address };
                text << shownAddress(address);
                text << PopLink{};
                text << k_endLine;
            }
            text << InkGrade::Muted;
            text << context.publisher();
            text << PopColor{};
            text << k_endLine;
            if (!context.site().empty())
            {
                text << PushLink{ std::wstring{ context.site() } };
                text << shownAddress(context.site());
                text << PopLink{};
                text << k_endLine;
            }
            text << k_endLine;
            if (!context.description().empty())
            {
                text << context.description();
                text << k_endLine;
                text << k_endLine;
            }
            for (const std::wstring& paragraph : context.information())
            {
                appendLinked(text, paragraph);
                text << k_endLine;
                text << k_endLine;
            }
            text << L"Built with ClaFi - Clarity First the Framework";
            text << k_endLine;
            text << PushLink{ L"https://github.com/clafi-fw/cpp" };
            text << L"github.com/clafi-fw/cpp";
            text << PopLink{};
            return text;
        }

        std::wstring_view versionRemark(const UpdateState state)
        {
            if (state == UpdateState::Latest)
                return L" - you have the latest version";
            if (state == UpdateState::Failed)
                return L" - could not check for updates";
            return {};
        }

        // An address runs from its scheme to the next blank; punctuation closing the sentence
        // around it stays in the text.
        void appendLinked(Text& text, const std::wstring_view paragraph)
        {
            // Punctuation a sentence closes with, a closing guillemet last.
            constexpr std::wstring_view k_closers = L".,;:)»\"'";
            std::size_t pos = 0;
            while (pos < paragraph.size())
            {
                const std::size_t start = paragraph.find(k_scheme, pos);
                if (start == std::wstring_view::npos)
                    break;
                std::size_t end = paragraph.find(L' ', start);
                if (end == std::wstring_view::npos)
                    end = paragraph.size();
                while (end > start && k_closers.find(paragraph[end - 1]) != std::wstring_view::npos)
                    --end;
                const std::wstring_view address = paragraph.substr(start, end - start);
                text << paragraph.substr(pos, start - pos);
                text << PushLink{ std::wstring{ address } };
                text << address;
                text << PopLink{};
                pos = end;
            }
            text << paragraph.substr(pos);
        }

        std::wstring_view shownAddress(const std::wstring_view address)
        {
            if (address.starts_with(k_scheme))
                return address.substr(k_scheme.size());
            return address;
        }
    }
}
