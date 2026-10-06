module ClaFi.App.Information;

import ClaFi.Controls.Button;
import ClaFi.Controls.Divider;
import ClaFi.Controls.Spacer;
import ClaFi.Controls.Stack;
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
        // The name, and the version at the far end of its line.
        [[nodiscard]] Text titleText(const AppContext&);
        // What the check found, and under it how long ago, or where the newer release is.
        [[nodiscard]] Text statusText(const UpdateCheck&, std::chrono::sys_seconds now);
        // How long ago a moment was, the way the line under the status says it.
        [[nodiscard]] std::wstring ageSpelling(std::chrono::sys_seconds moment,
            std::chrono::sys_seconds now);
        // A count of a unit, plural where it is not one, and how long ago.
        [[nodiscard]] std::wstring countAgo(std::int64_t count, std::wstring_view unit);
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
        Stack{
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
            addUpdateRow();
        add<TextBox>(
            ReadOnly::Yes,
            Padding{ 0.0f },
            HorizontalAlign::Left,
            MaxSize{ k_lineWidth, k_maxFloat },
            aboutText(params.appContext())
        );
    }

    // THE ROW SPANS THE PAGE, so the button stands at the end of the line the version ends. The
    // status is hidden until there is something to say - see stateUpdateRow.
    void InformationPage::addUpdateRow()
    {
        Stack& row = add<Stack>(
            Orientation::Horizontal,
            Spacing{ k_pageSpacing }
        );
        m_status = &row.add<TextBox>(
            ReadOnly::Yes,
            Padding{ 0.0f },
            VerticalAlign::Center
        );
        row.add<FlexSpacer>();
        m_checkButton = &row.add<Button>(
            L"Check for updates",
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
        m_updateConnection = ScopedEventConnection{ appContext().updateCheck().onChange(
            [this](UpdateCheckEvent&) {
                stateUpdateRow();
                invalidateFormAlign();
            }) };
        stateUpdateRow();
    }

    // THE AGE IS WORKED OUT HERE AND NOWHERE ELSE - when the page is built, which is every time
    // the menu opens, and when the check moves. It does not tick while the menu stays open.
    void InformationPage::stateUpdateRow()
    {
        const UpdateCheck& updates = appContext().updateCheck();
        const std::chrono::sys_seconds now =
            std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
        m_status->setVisible(updates.state() != UpdateState::Unchecked);
        m_status->text() = statusText(updates, now);
        m_checkButton->text() = Text{
            updates.state() == UpdateState::Failed ? L"Try again" : L"Check for updates"
        };
        m_checkButton->invalidateState();
    }

    // titleText, statusText, ageSpelling, countAgo, aboutText

    namespace
    {
        Text titleText(const AppContext& context)
        {
            Text text{ TextStyleId::Title, context.appName(), PopTextStyle{} };
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

        // A NEWER RELEASE NAMES ITS PAGE IN PLACE OF THE AGE. The address in full runs under the
        // button at the page's usual width, so the line links it under a name.
        Text statusText(const UpdateCheck& updates, const std::chrono::sys_seconds now)
        {
            Text text{ TextStyleId::SubHeading };
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
                    text << L"Version ";
                    text << updates.newerVersion();
                    text << L" is out";
                    break;
                case UpdateState::Failed:
                    text << L"Could not check for updates";
                    break;
            }
            text << PopTextStyle{};
            if (updates.state() == UpdateState::Newer)
            {
                text << k_endLine;
                text << TextStyleId::SubBody;
                text << PushLink{ updates.releaseAddress() };
                text << L"Release page on GitHub";
                text << PopLink{};
                text << PopTextStyle{};
            }
            else if (updates.answeredAt())
            {
                text << k_endLine;
                text << TextStyleId::SubBody;
                text << L"last check: ";
                text << ageSpelling(updates.answeredAt().value(), now);
                text << PopTextStyle{};
            }
            return text;
        }

        // A moment ahead of now - the clock was set back since - reads as just now.
        std::wstring ageSpelling(const std::chrono::sys_seconds moment,
            const std::chrono::sys_seconds now)
        {
            const std::chrono::minutes minutes =
                std::chrono::floor<std::chrono::minutes>(now - moment);
            if (minutes < std::chrono::minutes{ 1 })
                return L"just now";
            if (minutes < std::chrono::hours{ 1 })
                return countAgo(minutes.count(), L"minute");
            const std::chrono::hours hours = std::chrono::floor<std::chrono::hours>(minutes);
            if (hours < std::chrono::days{ 1 })
                return countAgo(hours.count(), L"hour");
            return countAgo(std::chrono::floor<std::chrono::days>(hours).count(), L"day");
        }

        std::wstring countAgo(const std::int64_t count, const std::wstring_view unit)
        {
            std::wstring spelling = std::format(L"{} {}", count, unit);
            if (count != 1)
                spelling += L's';
            spelling += L" ago";
            return spelling;
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
