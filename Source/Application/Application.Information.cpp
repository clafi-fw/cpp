module ClaFi.App.Information;

import ClaFi.Controls.TextBox;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.AppContext;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    using namespace Controls;

    namespace
    {
        // The name, its publisher, what the application does, and the framework with its address.
        [[nodiscard]] Text informationText(const AppContext&);
        // Appends a plain paragraph, every web address in it made a link.
        void appendLinked(Text&, std::wstring_view paragraph);

        // The room a page of options keeps around its column, so the name starts where options do.
        constexpr float k_pagePadding{ 12.0f };
        // The longest a line runs - a description wraps at it rather than widening the backstage.
        constexpr float k_lineWidth{ 400.0f };
    }

    // InformationPage

    InformationPage::InformationPage(const CreateParams& params)
        :
        TextBox{
            params,
            ReadOnly::Yes,
            Padding{ k_pagePadding },
            HorizontalAlign::Left,
            MaxSize{ k_lineWidth + k_pagePadding * 2.0f, k_maxFloat },
            informationText(params.appContext())
        }
    {
    }

    // informationText

    namespace
    {
        Text informationText(const AppContext& context)
        {
            Text text{
                TextStyleId::Title, context.appName(), PopTextStyle{},
                k_endLine,
                InkGrade::Muted, context.publisher(), PopColor{},
                k_endLine,
                k_endLine
            };
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
            constexpr std::wstring_view k_scheme = L"https://";
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
    }
}
