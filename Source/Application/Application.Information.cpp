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
                TextOp::EndLine,
                InkGrade::Muted, context.publisher(), PopColor{},
                TextOp::EndLine,
                TextOp::EndLine
            };
            if (!context.description().empty())
            {
                text << context.description();
                text << TextOp::EndLine;
                text << TextOp::EndLine;
            }
            text << L"Built with ClaFi - Clarity First the Framework";
            text << TextOp::EndLine;
            text << PushLink{ L"https://github.com/clafi-fw/cpp" };
            text << L"github.com/clafi-fw/cpp";
            text << PopLink{};
            return text;
        }
    }
}
