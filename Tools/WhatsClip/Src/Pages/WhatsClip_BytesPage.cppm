export module ClaFi.Tools.WhatsClip.BytesPage;

import ClaFi.Tools.WhatsClip.Page;

import ClaFi.Controls.HexView;
import ClaFi.Controls.Base.PanelBase;

import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.Foundation;

import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    using namespace Controls;

    using Base = WithBody<RepresentationPage, HexView>;
    
    // The representation that each format has.
    export class BytesPage : public Base
    {
    public:
        explicit BytesPage(const CreateParams&);
    protected:
        void showContent(Transfer::Offer&, const Transfer::Format&) override;
    private:
        void writeSelectionReadout();
    private:
        std::string m_bytes{};
    };
}
