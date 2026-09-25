module ClaFi.Tools.WhatsClip.BytesPage;

import ClaFi.Tools.WhatsClip.Page;

import ClaFi.Controls.HexView;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Base.PanelBase;

import ClaFi.Core.Foundation;

import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.TextEngine.Fmt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.Core.System.Props;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{

    BytesPage::BytesPage(const CreateParams& params)
        :
        Base{
            params,
            HostProps{ ScrollBars::Both },
            BodyProps{ Padding{ 12.0f, 0.0f } }
        }
    {
        setReadoutWidth(236.0f);
        body().onSelectionMove([this](SelectionMoveEvent&) { writeSelectionReadout(); });
    }

    void BytesPage::showContent(Transfer::Offer& offer, const Transfer::Format& format)
    {
        // THE BYTES AS THEY ARRIVED. A format this application knows would come back from read()
        // as its Content with the bytes gone, which is the whole reason readBytes exists. The
        // view borrows them, so they are held here before the span is made.
        m_bytes = offer.readBytes(format);

        body().setBytes({
            reinterpret_cast<const unsigned char*>(m_bytes.data()),
            m_bytes.size(),
        });

        // writeByteCount;
        {
            Text status{};
            status << TextStyleId::SubBody;
            if (m_bytes.empty())
                status << k_noAnswer;
            else
                status << Fmt{ L"{} bytes", m_bytes.size() };

            writeStatus(std::move(status));
        }
    }

    void BytesPage::writeSelectionReadout()
    {
        Text reading{};
        //reading << TextStyleId::Code;
        reading << PushFontSize{ k_readoutFontSize };

        // THE OFFSETS ARE THE VIEW'S OWN SPELLING - see HexView::offsetText - so the reading and
        // the left column cannot disagree about how wide an offset is. A single byte reads as
        // where it stands; a run reads as where it starts, where it ends, and how much of it
        // there is. The count is decimal against the offsets' hexadecimal, which is what its
        // unit is there to say.
        if (!body().bytes().empty())
        {
            const HexRange selection = body().selection();
            reading << body().offsetText(selection.start);
            if (selection.length > 1)
            {
                reading << L"-";
                reading << body().offsetText(selection.start + selection.length - 1);
                reading << L"  ";
                reading << Fmt{ L"{} bytes", selection.length };
            }
        }

        writeReadout(std::move(reading));
    }
}
