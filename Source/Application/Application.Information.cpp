module ClaFi.App.Information;

import ClaFi.App.Settings;

import ClaFi.Controls.Button;
import ClaFi.Controls.CheckBox;
import ClaFi.Controls.Divider;
import ClaFi.Controls.Spacer;
import ClaFi.Controls.Stack;
import ClaFi.Controls.TextBox;

import ClaFi.Core.AppTheme_Colors;
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
        [[nodiscard]] Text titleText(const AppContext&);
        // What the check found.
        [[nodiscard]] Text statusText(const UpdateCheck&);
        [[nodiscard]] Text lastCheckText(const UpdateCheck&, std::chrono::sys_seconds now);
        // The rest of the page - publisher, site, description, framework.
        [[nodiscard]] Text aboutText(const AppContext&);
        // Appends a plain paragraph, every web address in it made a link.
        void appendLinked(Text&, std::wstring_view paragraph);
        // The address as the page shows it - the scheme left out.
        [[nodiscard]] std::wstring_view shownAddress(std::wstring_view address);

        constexpr std::wstring_view k_webLinkProtocolPrefix{ L"https://" };
        constexpr Padding k_pagePadding{ 12.0f };
        constexpr Spacing k_pageSpacing{ 10.0f };
        constexpr MaxSize k_maxDescriptionBoxSize{ 400.0f, k_maxFloat };
        constexpr float k_versionGap{ 8.0f };
        constexpr MinSize k_updateControlMinSize{ 136.0f, 0.0f };
    }

    // InformationPage

    InformationPage::InformationPage(const CreateParams& params)
        :
        StackPanel{
            params,
            Orientation::Vertical,
            k_pagePadding,
            k_pageSpacing
        }
    {
        add<TextBox>(
            ReadOnly::Yes,
            Padding{ 0.0f },
            titleText(params.appContext())
        );
        if (appContext().updateCheck().available())
            addUpdateRows();
        add<TextBox>(
            ReadOnly::Yes,
            Padding{ 0.0f },
            HorizontalAlign::Left,
            k_maxDescriptionBoxSize,
            aboutText(params.appContext())
        );
    }

    void InformationPage::addUpdateRows()
    {
        StackPanel& statusRow = add<StackPanel>(
            Orientation::Horizontal,
            k_pageSpacing
        );
        m_status = &statusRow.add<TextBox>(
            ReadOnly::Yes,
            Padding{ 0.0f },
            VerticalAlign::Center
        );
        statusRow.add<FlexSpacer>();
        m_checkButton = &statusRow.add<Button>(
            k_updateControlMinSize,
            VerticalAlign::Center
        );
        StackPanel& lastCheckRow = add<StackPanel>(
            Orientation::Horizontal,
            k_pageSpacing
        );
        m_lastCheckLine = &lastCheckRow.add<TextBox>(
            ReadOnly::Yes,
            Padding{ 0.0f },
            VerticalAlign::Center
        );
        lastCheckRow.add<FlexSpacer>();
        m_checkAutomaticallyBox = &lastCheckRow.add<CheckBox>(
            L"Check automatically",
            TooltipText{ L"Check for updates on start, at most once in 24 hours" },
            k_updateControlMinSize,
            VerticalAlign::Center
        );
        add<Divider>();
        // Grayed while the question is out, so a second press cannot ask it twice.
        m_checkButton->onGetState([this](GetStateEvent& event) {
            event.state.enabled = appContext().updateCheck().state() != UpdateState::Checking;
        });
        m_checkButton->onClick([this](ClickEvent&) {
            appContext().updateCheck().start();
        });
        m_checkAutomaticallyBox->onGetState([this](GetStateEvent& event) {
            event.state.selected = appContext().updateCheck().automatic();
        });
        // **The answer is kept in the config**, so turning it on where no settings folder exists
        // asks for one first - the question Keep settings on this PC asks.
        m_checkAutomaticallyBox->onClick([this](ClickEvent&) {
            UpdateCheck& updates = appContext().updateCheck();
            const bool turningOn = !updates.automatic();
            if (turningOn && !appContext().configFolderExists())
            {
                if (!allowSettingsStorage(*m_checkAutomaticallyBox))
                    return;
            }
            updates.setAutomatic(turningOn);
            m_checkAutomaticallyBox->invalidateState();
        });
        m_updateConnection = ScopedEventConnection{ appContext().updateCheck().onChange(
            [this](UpdateCheckEvent&) {
                stateUpdateRows();
                invalidateFormAlign();
            }) };
        stateUpdateRows();
    }

    // **The age is worked out here and nowhere else** - when the page is built, which is every time
    // the menu opens, and when the check moves. It does not tick while the menu stays open.
    void InformationPage::stateUpdateRows()
    {
        const UpdateCheck& updates = appContext().updateCheck();
        const std::chrono::sys_seconds now =
            std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
        m_status->text() = statusText(updates);
        m_lastCheckLine->text() = lastCheckText(updates, now);
        m_checkButton->text() = Text{
            TextAlign::Center,
            updates.state() == UpdateState::Failed ? L"Try again" : L"Check for updates"
        };
        m_checkButton->invalidateState();
    }

    // titleText, statusText, lastCheckText, aboutText

    namespace
    {
        Text titleText(const AppContext& context)
        {
            Text text{ StyledText{ TextStyles::Title, context.appName() } };
            if (!context.version().empty())
            {
                text << Space{ k_versionGap };
                text << ColoredText{ InkGrade::Muted, L"Version ", context.version() };
            }
            return text;
        }

        // **A newer release is linked from the status line** - the address in full would run under
        // the button at the page's usual width.
        Text statusText(const UpdateCheck& updates)
        {
            Text words{};
            switch (updates.state())
            {
                case UpdateState::Unchecked:
                    words << L"Updates haven't been checked";
                    break;
                case UpdateState::Checking:
                    words << L"Checking for updates\u2026";
                    break;
                case UpdateState::Latest:
                    words << L"You're up to date";
                    break;
                case UpdateState::Newer:
                    words << LinkText{
                        updates.releaseAddress(),
                        L"Version ",
                        updates.newerVersion(),
                        L" is available"
                    };
                    break;
                case UpdateState::Failed:
                    words << L"Could not check for updates";
                    break;
            }
            return Text{ StyledText{ TextStyles::SubHeading, words } };
        }

        Text lastCheckText(const UpdateCheck& updates, const std::chrono::sys_seconds now)
        {
            Text words{ L"last check: " };
            if (const UpdateCheck::AnswerTime answeredAt = updates.answeredAt())
                words << ageSpelling(answeredAt.value(), now);
            else
                words << L"never";
            return Text{ StyledText{ TextStyles::SubBody, words } };
        }

        Text aboutText(const AppContext& context)
        {
            Text text{};
            text << ColoredText{ InkGrade::Muted, context.publisher() };
            text << k_endLine;
            if (!context.site().empty())
            {
                text << LinkText{ std::wstring{ context.site() }, shownAddress(context.site()) };
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
            text << L"Built with " << LinkText{ L"https://github.com/clafi-fw/cpp", L"ClaFi Framework" };
            return text;
        }

        // An address runs from its scheme to the next blank; punctuation closing the sentence
        // around it stays in the text.
        void appendLinked(Text& text, const std::wstring_view paragraph)
        {
            // Punctuation a sentence closes with, a closing guillemet last.
            constexpr std::wstring_view k_closers = L".,;:)\u00BB\"'";
            std::size_t pos = 0;
            while (pos < paragraph.size())
            {
                const std::size_t start = paragraph.find(k_webLinkProtocolPrefix, pos);
                if (start == std::wstring_view::npos)
                    break;
                std::size_t end = paragraph.find(L' ', start);
                if (end == std::wstring_view::npos)
                    end = paragraph.size();
                while (end > start && k_closers.find(paragraph[end - 1]) != std::wstring_view::npos)
                    --end;
                const std::wstring_view address = paragraph.substr(start, end - start);
                text << paragraph.substr(pos, start - pos);
                text << LinkText{ std::wstring{ address }, address };
                pos = end;
            }
            text << paragraph.substr(pos);
        }

        std::wstring_view shownAddress(const std::wstring_view address)
        {
            if (address.starts_with(k_webLinkProtocolPrefix))
                return address.substr(k_webLinkProtocolPrefix.size());
            return address;
        }
    }
}
