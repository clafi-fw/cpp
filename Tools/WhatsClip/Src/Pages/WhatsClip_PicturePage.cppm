export module ClaFi.Tools.WhatsClip.PicturePage;

import ClaFi.Tools.WhatsClip.Page;

import ClaFi.Controls.PictureView;
import ClaFi.Controls.SplitButton;
import ClaFi.Controls.Slider;
import ClaFi.Controls.Label;
import ClaFi.Controls.Base.PanelBase;

import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    using namespace Controls;

    // How the readout writes the pixel's colour, and what a copy takes.
    enum class ColorSpelling
    {
        HexRgb,       // 5F3929
        HexBgr,       // 29395F - the order a COLORREF is written in
        DecimalRgb,   // 95 57 41
        CssHex        // #5F3929
    };

    // THE PIXELS A FORMAT DECODES TO. The strip says how many; the corner reads the selected
    // pixel's place, then its colour as a swatch and a value whose digits wear their channel's
    // colour, with the spelling behind the strip of that button and the editor behind its face;
    // the zoom stands at the right end of the strip above. The pixel itself carries those same two
    // commands - a menu with Copy and Edit color, and a double click that opens the editor.
    using PicturePageBase = WithBody<RepresentationPage, PictureView>;
    export class PicturePage : public PicturePageBase
    {
    public:
        explicit PicturePage(const CreateParams&);
    protected:
        void showContent(Transfer::Offer&, const Transfer::Format&) override;
    private:
        // Both readings own their layout: each is a different text on every move.
        using ColorReadout = WithTextLayout<SplitButton>;
        using ZoomReadout = WithTextLayout<Label>;
    private:
        void writePixelReadout();
        void writeColorReadout();
        void writeZoomReadout();
        // The slider from the view, without the view hearing of it again.
        void followZoom();
        void zoomFromSlider(float position);
        void dropSpellings(DropdownEvent&);
        // The page volunteering as the subject of Copy, the way HexView does for its own commands.
        void connectCopyAction();
        void copyColor(InputStamp);
        void showPixelMenu(ContextPopupEvent&);
        // Owned by whatever raised it, so a menu's editor stands over the menu and not over the
        // corner the readout sits in.
        void openEditor(Control& owner);
        [[nodiscard]] Text colorText(std::optional<Color>) const;
        [[nodiscard]] static std::wstring spell(Color, ColorSpelling);
        [[nodiscard]] static std::wstring_view nameOf(ColorSpelling);
        [[nodiscard]] static float zoomOf(float position);
        [[nodiscard]] static float positionOf(float zoom);
    private:
        // The slider runs over the view's zoom range one notch a position, so a press of its
        // button is one notch and the thumb lands between them: 1.25 to the 31st is the first
        // power past the thousandfold from 5% to 5000%.
        static constexpr float k_zoomNotches = 31.0f;
        static constexpr float k_readoutWidth = 152.0f;
        static constexpr float k_zoomReadoutWidth = 44.0f;
        static constexpr float k_zoomSliderWidth = 250.0f;
        static constexpr float k_swatchSize = 12.0f;
        static constexpr float k_swatchGap = 6.0f;
        static constexpr float k_readoutPaddingX = 6.0f;
        static constexpr float k_readoutPaddingY = 1.0f;
    private:
        // Held here because the view borrows.
        Graphics::Bitmap m_picture{};
        ColorSpelling m_spelling{ ColorSpelling::HexRgb };
        // What the readout shows in place of the pixel's colour: the editor's, while it is open.
        std::optional<Color> m_editedColor{};
        ColorReadout& m_colorReadout;
        ZoomReadout& m_zoomReadout;
        Slider& m_zoomSlider;
    };
}
