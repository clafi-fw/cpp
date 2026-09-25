export module ClaFi.Tools.WhatsClip.TextPreviewPage;

import ClaFi.Tools.WhatsClip.Page;

import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.Foundation;

import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    // The clipboard read as text, with its markers on where what it was read from carries them.
    export class TextPreviewPage : public TextPageBase
    {
    public:
        TextPreviewPage(const CreateParams&, const PickLists&);
    protected:
        void showContent(Transfer::Offer&, const Transfer::Format&) override;
    };
}
