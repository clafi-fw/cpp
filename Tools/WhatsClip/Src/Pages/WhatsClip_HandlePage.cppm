export module ClaFi.Tools.WhatsClip.HandlePage;

import ClaFi.Tools.WhatsClip.Page;

import ClaFi.Controls.TextBox;
import ClaFi.Controls.Base.PanelBase;

import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.Foundation;

import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    using namespace Controls;

    using HandlePageBase = WithBody<RepresentationPage, TextBox>;

    // THE HANDLE A FORMAT CARRIES, where the clipboard's entry for it is a handle and not bytes.
    // Nothing travelled for such a format, so the number is the whole of the reading: what a
    // hex dump or a decoding would show is something this viewer made, not what is on the
    // clipboard, and neither is offered beside this one - see representationsOf.
    //
    // The number is written to a width rather than trimmed to its digits, being a place in a table
    // and not a quantity, and the page says what it is worth: the clipboard hands each reader its
    // own handle, so the value was live for the length of the read and is a dead number now.
    export class HandlePage : public HandlePageBase
    {
    public:
        explicit HandlePage(const CreateParams&);

        // Whether a format's entry is a handle, which is what settles that it has no bytes and no
        // text to show. The names are one platform's; on the other nothing carries a handle.
        [[nodiscard]] static bool carriesHandle(const Transfer::Format&);
    protected:
        void showContent(Transfer::Offer&, const Transfer::Format&) override;
    };
}
