module ClaFi.Tools.WhatsClip.TextPreviewPage;

import ClaFi.Tools.WhatsClip.Page;

import ClaFi.Controls.Checkbox;
import ClaFi.Controls.CodeBox;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.Base.PanelBase;

import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.TextEngine.Fmt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Props;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    using namespace Controls;

    TextPreviewPage::TextPreviewPage(const CreateParams& params, const PickLists& picks)
        :
        TextPageBase{
            params,
            picks,
            HostProps{ ScrollBars::Both },
            BodyProps{ ReadOnly::Yes, Padding{ 12.0f } }
        }
    {
    }

    // THE WORDS AS THEY WOULD BE DRAWN, out of whichever format the preview was read from. Both
    // readings are decoded before they arrive - the framework's own rich text as a Text with its
    // markers, a standard text as the string it is - so the encoding pick never appears here and
    // the language is the text's own. Which of the two came is what the answer holds, not what
    // the format was called.
    void TextPreviewPage::showContent(Transfer::Offer& offer, const Transfer::Format& format)
    {
        const Transfer::Payload payload = offer.read(format);
        const Text* marked = std::any_cast<Text>(&payload);
        const std::wstring* plain = std::any_cast<std::wstring>(&payload);

        Text status{};
        status << TextStyleId::SubBody;
        if (marked)
        {
            status << Fmt{ L"{} characters, {} markers",
                marked->plainText().size(), marked->markers().size() };
        }
        else if (plain)
        {
            status << Fmt{ L"{} characters", plain->size() };
        }
        else
        {
            status << k_noAnswer;
        }

        writeStatus(std::move(status));

        Text report{};
        if (marked)
            report << TextStyleId::Body << *marked;
        else if (plain)
            report << TextStyleId::Body << *plain;

        showText(std::move(report));
    }
}
