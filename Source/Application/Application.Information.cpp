module ClaFi.App.Information;

import ClaFi.App.Settings;

import ClaFi.Controls.Button;
import ClaFi.Controls.CheckBox;
import ClaFi.Controls.Divider;
import ClaFi.Controls.Spacer;
import ClaFi.Controls.Stack;
import ClaFi.Controls.TextBox;

import ClaFi.Core.AppTheme_Metrics;
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
        // The name, and the version at the far end of its line.
        [[nodiscard]] Text titleText(const AppContext&);
        // What the check found.
        [[nodiscard]] Text statusText(const UpdateCheck&);
        // How long ago the last answer arrived - empty until one has.
        [[nodiscard]] Text lastCheckText(const UpdateCheck&, std::chrono::sys_seconds now);
        // The rest of the page - publisher, site, description, framework.
        [[nodiscard]] Text aboutText(const AppContext&);
        // Appends a plain paragraph, every web address in it made a link.
        void appendLinked(Text&, std::wstring_view paragraph);
        // The address as the page shows it - the scheme left out.
        [[nodiscard]] std::wstring_view shownAddress(std::wstring_view address);

        // The scheme every address the page links starts with.
        constexpr std::wstring_view k_scheme{ L"https://" };
        // The room a page of options keeps around its column, so the name starts where options do.
        constexpr float k_pagePadding{ 12.0f };
        // Between the rows of the page, as between the groups of a page of options.
        constexpr float k_pageSpacing{ 10.0f };
        // The longest a line runs - a description wraps at it rather than widening the backstage.
        constexpr float k_lineWidth{ 400.0f };
        // The least room between the name and the version, on a line with nothing to spare.
        constexpr float k_versionGap{ 24.0f };
    }

    // InformationPage

    InformationPage::InformationPage(const CreateParams& params)
        :
        StackPanel{
            params,
            Orientation::Vertical,
            Padding{ k_pagePadding },
            Spacing{ k_pageSpacing }
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
            MaxSize{ k_lineWidth, k_maxFloat },
            aboutText(params.appContext())
        );
    }

    // **The rows span the page**, so the button and the age stand at the end of the line the
    // version ends. The status and the age are hidden until there is something to say - see
    // stateUpdateRows.
    void InformationPage::addUpdateRows()
    {
        StackPanel& statusRow = add<StackPanel>(
            Orientation::Horizontal,
            Spacing{ k_pageSpacing }
        );
        m_status = &statusRow.add<TextBox>(
            ReadOnly::Yes,
            Padding{ 0.0f },
            VerticalAlign::Center
        );
        statusRow.add<FlexSpacer>();
        m_checkButton = &statusRow.add<Button>(
            L"Check for updates",
            VerticalAlign::Center
        );
        StackPanel& automaticRow = add<StackPanel>(
            Orientation::Horizontal,
            Spacing{ k_pageSpacing }
        );
        // No padding at the sides, so the box lines up with the status text above it.
        m_checkAutomaticallyBox = &automaticRow.add<CheckBox>(
            L"Check automatically",
            TooltipText{ L"Checks for updates on start, at most once in 24 hours" },
            Padding{ 0.0f, themeMetrics().listItem.padding.y },
            VerticalAlign::Center
        );
        automaticRow.add<FlexSpacer>();
        m_lastCheckLine = &automaticRow.add<TextBox>(
            ReadOnly::Yes,
            Padding{ 0.0f },
            VerticalAlign::Center
        );
        add<Divider>();
        // Greyed while the question is out, so a second press cannot ask it twice.
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
        m_status->setVisible(updates.state() != UpdateState::Unchecked);
        m_status->text() = statusText(updates);
        m_lastCheckLine->setVisible(updates.answeredAt().has_value());
        m_lastCheckLine->text() = lastCheckText(updates, now);
        m_checkButton->text() = Text{
            updates.state() == UpdateState::Failed ? L"Try again" : L"Check for updates"
        };
        m_checkButton->invalidateState();
    }

    // titleText, statusText, lastCheckText, aboutText

    namespace
    {
        Text titleText(const AppContext& context)
        {
            Text text{ TextStyle::Title, context.appName(), PopTextStyle{} };
            if (!context.version().empty())
            {
                text << FlexSpace{ k_versionGap };
                text << InkGrade::Muted;
                text << L"Version ";
                text << context.version();
                text << PopColor{};
            }
            return text;
        }

        // **A newer release is linked from the status line** - the address in full would run under
        // the button at the page's usual width.
        Text statusText(const UpdateCheck& updates)
        {
            Text text{ TextStyle::SubHeading };
            switch (updates.state())
            {
                case UpdateState::Unchecked:
                    break;
                case UpdateState::Checking:
                    text << L"Checking for updates\u2026";
                    break;
                case UpdateState::Latest:
                    text << L"You're up to date";
                    break;
                case UpdateState::Newer:
                    text << PushLink{ updates.releaseAddress() };
                    text << L"Version ";
                    text << updates.newerVersion();
                    text << L" is available";
                    text << PopLink{};
                    break;
                case UpdateState::Failed:
                    text << L"Could not check for updates";
                    break;
            }
            text << PopTextStyle{};
            return text;
        }

        Text lastCheckText(const UpdateCheck& updates, const std::chrono::sys_seconds now)
        {
            Text text{ TextStyle::SubBody };
            if (updates.answeredAt())
            {
                text << L"last check: ";
                text << ageSpelling(updates.answeredAt().value(), now);
            }
            text << PopTextStyle{};
            return text;
        }

        Text aboutText(const AppContext& context)
        {
            Text text{};
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

        // An address runs from its scheme to the next blank; punctuation closing the sentence
        // around it stays in the text.
        void appendLinked(Text& text, const std::wstring_view paragraph)
        {
            // Punctuation a sentence closes with, a closing guillemet last.
            constexpr std::wstring_view k_closers = L".,;:)\u00BB\"'";
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
