module ClaFi.Tools.WhatsClip.TextPreviewPage;

import ClaFi.Tools.WhatsClip.Page;

import ClaFi.StdActions.Transfer;

import ClaFi.Controls.CheckBox;
import ClaFi.Controls.CodeBox;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.Base.PanelBase;

import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.TextEngine.StyleSheet;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Props;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    using namespace Controls;

    namespace
    {
        // **The styles copies carried, over the application's sheet**, which every rich text shown
        // is built over. Built on first use and kept for the process, so a style keeps its address
        // while any text drawn from it lives.
        [[nodiscard]] StyleSheet& carriedStyles()
        {
            static StyleSheet sheet{ &StyleSheet::application() };
            return sheet;
        }
    }

    TextPreviewPage::TextPreviewPage(const CreateParams& params, const PickLists& picks)
        :
        TextPageBase{
            params,
            picks,
            HostProps{ ScrollBars::Both },
            BodyProps{ ReadOnly::Yes, Padding{ 12.0f } }
        },
        m_ownFormat{ body().textFormat() ? *body().textFormat() : TextFormat{} }
    {
    }

    // **The words as they would be drawn**, out of whichever format the preview was read from. Both
    // readings are decoded before they arrive - the framework's own rich text as a Text with its
    // runs, a standard text as the string it is - so the encoding pick never appears here and
    // the language is the text's own.
    //
    // **The framework's rich text draws as it did where it was copied**: over the styles carried
    // beside it, which stand over the application's of the same name, and in the format of the
    // control it was copied from - none where that control had none.
    void TextPreviewPage::showContent(Transfer::Offer& offer, const Transfer::Format& format)
    {
        std::optional<Text> taken;
        std::size_t carriedCount = 0;
        TextFormat shownFormat = m_ownFormat;
        Transfer::Payload payload{};
        if (format == Transfer::ClaFiText::format())
        {
            const std::optional<StyleSheet> carried =
                Transfer::take<Transfer::ClaFiStyleSheet>(offer);
            if (carried.has_value())
            {
                carriedCount = carried->styles().size();
                carriedStyles().takeStyles(carried.value());
            }
            taken = Transfer::takeText(offer, &carriedStyles());
            const std::optional<TextFormat> copiedFormat =
                Transfer::take<Transfer::ClaFiTextFormat>(offer);
            shownFormat = copiedFormat.value_or(TextFormat{});
        }
        else
        {
            payload = offer.read(format);
        }
        const Text* marked = taken.has_value() ? &taken.value() : std::any_cast<Text>(&payload);
        const std::wstring* plain = std::any_cast<std::wstring>(&payload);

        Text status{};
        if (marked)
        {
            std::size_t runCount = 0;
            marked->forEachRun([&runCount](const TextRunView&){
                ++runCount;
            });
            status << std::format(L"{} characters, {} runs", marked->plainText().size(), runCount);
            if (carriedCount)
                status << std::format(L", {} carried {}", carriedCount,
                    carriedCount == 1 ? L"style" : L"styles");
        }
        else if (plain)
        {
            status << std::format(L"{} characters", plain->size());
        }
        else
        {
            status << k_noAnswer;
        }

        writeStatus(std::move(status));

        // A copy keeps the sheet the text was taken over.
        Text report{};
        if (marked)
            report = *marked;
        else if (plain)
            report << *plain;

        body().setTextFormat(shownFormat);
        showText(std::move(report));
    }
}
