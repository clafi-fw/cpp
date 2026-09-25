module ClaFi.Tools.WhatsClip.PicturePage;

import ClaFi.Tools.WhatsClip.Page;

import ClaFi.StdActions;

import ClaFi.Controls.ColorEditDialog;
import ClaFi.Controls.Menu;
import ClaFi.Controls.PictureView;
import ClaFi.Controls.SplitButton;
import ClaFi.Controls.Slider;
import ClaFi.Controls.Label;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Controls.Base.SliderBase;
import ClaFi.Controls.Base.PanelBase;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.FormContext;

import ClaFi.Core.Transfer.Clipboard;
import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.Transfer.Source;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.TextEngine.Fmt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.Core.System.Props;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    using namespace ::ClaFi::Controls;

    namespace
    {
        constexpr std::wstring_view k_hexDigits = L"0123456789ABCDEF";

        // Which channel a run of digits belongs to, and the ink it wears for it.
        enum class Channel
        {
            Red,
            Green,
            Blue
        };

        constexpr ColorSpelling k_spellings[] = {
            ColorSpelling::HexRgb,
            ColorSpelling::HexBgr,
            ColorSpelling::DecimalRgb,
            ColorSpelling::CssHex,
        };

        [[nodiscard]] ColorByte valueOf(const Color color, const Channel channel)
        {
            switch (channel)
            {
                case Channel::Red:
                    return color.red;
                case Channel::Green:
                    return color.green;
                default:
                    return color.blue;
            }
        }

        // The three inks, at a lightness that reads over a light surface and a dark one alike.
        [[nodiscard]] Color inkOf(const Channel channel)
        {
            switch (channel)
            {
                case Channel::Red:
                    return Hsl{ 29, 90, 50 }.toColor();
                case Channel::Green:
                    return Hsl{ 142, 90, 50 }.toColor();
                default:
                    return Hsl{ 264, 90, 50 }.toColor();
            }
        }

        void appendHex(Text& text, const ColorByte value)
        {
            text << k_hexDigits[value >> 4];
            text << k_hexDigits[value & 0x0f];
        }

        void appendChannel(Text& text, const Color color, const Channel channel, const bool hex)
        {
            const ColorByte value = valueOf(color, channel);
            text << inkOf(channel);
            if (hex)
                appendHex(text, value);
            else
                text << static_cast<int>(value);
            text << PopColor{};
        }

        // The digits with each run in its channel's ink; what spell writes without them.
        void appendSpelling(Text& text, const Color color, const ColorSpelling spelling)
        {
            switch (spelling)
            {
                case ColorSpelling::HexRgb:
                    appendChannel(text, color, Channel::Red, true);
                    appendChannel(text, color, Channel::Green, true);
                    appendChannel(text, color, Channel::Blue, true);
                    break;
                case ColorSpelling::HexBgr:
                    appendChannel(text, color, Channel::Blue, true);
                    appendChannel(text, color, Channel::Green, true);
                    appendChannel(text, color, Channel::Red, true);
                    break;
                case ColorSpelling::DecimalRgb:
                    appendChannel(text, color, Channel::Red, false);
                    text << L' ';
                    appendChannel(text, color, Channel::Green, false);
                    text << L' ';
                    appendChannel(text, color, Channel::Blue, false);
                    break;
                case ColorSpelling::CssHex:
                    text << InkGrade::Muted << L'#' << PopColor{};
                    appendChannel(text, color, Channel::Red, true);
                    appendChannel(text, color, Channel::Green, true);
                    appendChannel(text, color, Channel::Blue, true);
                    break;
            }
        }
    }

    PicturePage::PicturePage(const CreateParams& params)
        :
        PicturePageBase{
            params,
            HostProps{ ScrollBars::Both },
            BodyProps{}
        },
        // THE FACE OPENS THE EDITOR AND THE STRIP DROPS THE SPELLINGS. Its width is stated, like
        // the reading's, so the corner holds still whatever value it reads.
        m_colorReadout{ createCornerBar<ColorReadout>(
            MinSize{ k_readoutWidth, 0.0f },
            MaxSize{ k_readoutWidth, k_maxFloat },
            Padding{ k_readoutPaddingX, k_readoutPaddingY },
            WordWrap::No,
            HorizontalTextAnchor::Left,
            VerticalTextAnchor::Center,
            ShowSurfaceAtRest::Yes,
            OnEvent{ [this](ClickEvent&) {
                openEditor(m_colorReadout);
            } },
            SplitButton::OnDropdown{ [this](DropdownEvent& event) {
                dropSpellings(event);
            } }
        ) },
        m_zoomReadout{ createStripBar<ZoomReadout>(
            MinSize{ k_zoomReadoutWidth, 0.0f },
            MaxSize{ k_zoomReadoutWidth, k_maxFloat },
            WordWrap::No,
            HorizontalTextAnchor::Right,
            VerticalTextAnchor::Center
        ) },
        m_zoomSlider{ createStripBar<Slider>(
            MinSize{ k_zoomSliderWidth, 0.0f },
            MaxSize{ k_zoomSliderWidth, k_maxFloat },
            Slider::OnChange{ [this](SliderChangeEvent& event) {
                zoomFromSlider(event.newPosition);
            } }
        ) }
    {
        m_zoomSlider.setMaxPosition(k_zoomNotches);
        m_zoomSlider.setPaintTails(true);
        // The view says when its selection and its zoom move; nothing polls it. The connections
        // need no scope: the view is this page's own and goes when it does.
        body().onPixelSelect([this](PixelSelectEvent&) {
            writePixelReadout();
            writeColorReadout();
        });
        body().onZoomChange([this](ZoomChangeEvent&) {
            writeZoomReadout();
            followZoom();
        });
        // A pixel acted on is a pixel to edit, and the editor keeps the one home it has: under the
        // readout whose digits follow it while it is open.
        body().onPixelActivate([this](PixelActivateEvent&) {
            openEditor(m_colorReadout);
        });
        body().onContextPopup([this](ContextPopupEvent& event) {
            showPixelMenu(event);
        });
        connectCopyAction();
        writePixelReadout();
        writeColorReadout();
        writeZoomReadout();
        followZoom();
    }

    // WHAT THE FORMAT OFFERS, decoded by whoever knows how to. The standard picture format is
    // served from whichever spelling the platform ranks first and answers pixels whichever that
    // was - a PNG or a DIB out of its bytes, a GDI bitmap read off the object its handle names -
    // so there is one route here and no table of decoders. The bitmap is moved out of the answer
    // rather than copied, a screenshot being megabytes of it.
    void PicturePage::showContent(Transfer::Offer& offer, const Transfer::Format& format)
    {
        m_picture = Graphics::Bitmap{};
        Transfer::Payload payload = offer.read(format);
        if (Graphics::Bitmap* decoded = std::any_cast<Graphics::Bitmap>(&payload))
            m_picture = std::move(*decoded);

        body().setPicture(m_picture.empty() ? nullptr : &m_picture);

        Text status{};
        status << TextStyleId::SubBody;
        if (m_picture.empty())
            status << k_noAnswer;
        else
            status << Fmt{ L"{} \u00d7 {} pixels", m_picture.width(), m_picture.height() };

        writeStatus(std::move(status));
    }

    void PicturePage::writePixelReadout()
    {
        Text reading{};
        reading << PushFontSize{ k_readoutFontSize };
        if (const std::optional<IntPoint> pixel = body().selection())
            reading << Fmt{ L"{}, {}", pixel->x, pixel->y };

        writeReadout(std::move(reading));
    }

    void PicturePage::writeColorReadout()
    {
        m_colorReadout.text() = colorText(m_editedColor ? m_editedColor : body().selectedColor());
        m_colorReadout.invalidate();
    }

    void PicturePage::writeZoomReadout()
    {
        Text reading{};
        reading << PushFontSize{ k_readoutFontSize };
        reading << Fmt{ L"{}%", static_cast<int>(std::round(body().zoom() * 100.0f)) };
        m_zoomReadout.text() = std::move(reading);
        m_zoomReadout.invalidate();
    }

    void PicturePage::followZoom()
    {
        m_zoomSlider.setPosition(positionOf(body().zoom()), false);
    }

    void PicturePage::zoomFromSlider(const float position)
    {
        body().setZoom(zoomOf(position));
    }

    // Every spelling of the colour on the readout, the one in use marked; a line picked writes
    // the readout again in it.
    void PicturePage::dropSpellings(DropdownEvent& event)
    {
        const std::optional<Color> shown = m_editedColor ? m_editedColor : body().selectedColor();

        Menu menu{ *event.control };
        for (const ColorSpelling spelling : k_spellings)
        {
            std::wstring line{ nameOf(spelling) };
            if (shown)
            {
                line += L"    ";
                line += spell(*shown, spelling);
            }

            MenuItem& item = menu.add(line, [this, spelling](Control&) {
                m_spelling = spelling;
                writeColorReadout();
            });
            if (spelling != m_spelling)
                continue;

            item.setShowSelectionOnSurface(ShowSelectionOnSurface::Yes);
            item.onGetState([](GetStateEvent& stateEvent) {
                stateEvent.state.selected = true;
            });
        }
        menu.executeUnder(m_colorReadout);
    }

    // WHAT A COPY TAKES IS THE COLOUR, spelled the way the readout spells it now. The picture came
    // off the clipboard and putting it back says nothing; the digits are what a pixel is worth
    // taking away. The page claims the command rather than the view, so the key answers wherever
    // the focus rests on the page - the picture, the readout, the zoom slider.
    void PicturePage::connectCopyAction()
    {
        onGetActionState([this](GetActionStateEvent& event) {
            if (&event.action != &StdActions::copy)
                return;

            // Claimed whether or not a pixel is selected: a page that cannot copy right now is not
            // a page the command is about something else on.
            event.claim({ .enabled = body().selectedColor().has_value() });
        });

        onActionClick([this](ActionClickEvent& event) {
            if (&event.action == &StdActions::copy)
                copyColor(event.stamp);
        });
    }

    // ONE FORMAT OUT: the digits, which an application that has never heard of this viewer reads
    // as the text they are.
    void PicturePage::copyColor(const InputStamp stamp)
    {
        const std::optional<Color> color = body().selectedColor();
        if (!color)
            return;

        Transfer::Source source{};
        source.add<Transfer::PlainText>(spell(*color, m_spelling));
        formContext().clipboard().set(std::move(source), stamp);
    }

    // THE CORNER'S TWO COMMANDS, AT THE PIXEL ITSELF. The view has already selected the pixel the
    // press landed on - see PictureView::contextPopup - so both act on what was pointed at. The
    // editor is owned by the line that opened it, which is what stands it over the menu.
    void PicturePage::showPixelMenu(ContextPopupEvent& event)
    {
        event.stopPropagation();

        Menu menu{ body() };
        menu.add(StdActions::copy);
        MenuItem& edit = menu.add(L"Edit color", [this](Control& item) {
            openEditor(item);
        });
        edit.setOpensWindow(OpensWindow::Yes);
        if (!body().selectedColor())
        {
            edit.onGetState([](GetStateEvent& stateEvent) {
                stateEvent.state.enabled = false;
            });
        }

        menu.execute();
    }

    // THE EDITOR'S COLOUR STANDS IN THE READOUT WHILE IT IS OPEN, so the digits follow the
    // sliders, and the pixel's own comes back once it closes. A copy out of the editor takes
    // the clipboard, and this viewer then shows what it took - the picture goes with its tab,
    // and the editor with the picture.
    void PicturePage::openEditor(Control& owner)
    {
        const std::optional<Color> selected = body().selectedColor();
        if (!selected)
            return;

        ColorEditDialog editor{
            owner,
            L"Edit color",
            *selected,
            [spelling = m_spelling](const Color color) {
                return spell(color, spelling);
            }
        };
        // Declared after the editor, so it is dropped before the emitter it is on.
        const ScopedEventConnection follow = editor.onEdit([this, &editor](ColorEditEvent&) {
            m_editedColor = editor.color();
            writeColorReadout();
        });
        editor.execute();

        m_editedColor.reset();
        writeColorReadout();
    }

    Text PicturePage::colorText(const std::optional<Color> color) const
    {
        Text text{};
        text << PushFontSize{ k_readoutFontSize };
        if (!color)
        {
            text << InkGrade::Muted << L"No pixel" << PopColor{};
            return text;
        }

        const Color shown = *color;
        text << InTextIcon{ k_swatchSize, [shown](PaintIconEvent& event) {
            const float radius = event.iconWidth() / 2.0f;
            event.canvas().fillCircle(event.iconCenter(), radius, shown);
            event.canvas().drawCircle(event.iconCenter(), radius, event.textRgb(InkGrade::Muted),
                event.scaledStrokeWidth(Thickness::Thin));
        } };
        text << Space{ k_swatchGap };
        text << TextStyleId::Code;
        appendSpelling(text, shown, m_spelling);
        return text;
    }

    std::wstring PicturePage::spell(const Color color, const ColorSpelling spelling)
    {
        switch (spelling)
        {
            case ColorSpelling::HexRgb:
                return std::format(L"{:02X}{:02X}{:02X}", color.red, color.green, color.blue);
            case ColorSpelling::HexBgr:
                return std::format(L"{:02X}{:02X}{:02X}", color.blue, color.green, color.red);
            case ColorSpelling::DecimalRgb:
                return std::format(L"{} {} {}", color.red, color.green, color.blue);
            case ColorSpelling::CssHex:
                return std::format(L"#{:02X}{:02X}{:02X}", color.red, color.green, color.blue);
        }
        unreachable("a colour spelling with no writer of its own");
    }

    std::wstring_view PicturePage::nameOf(const ColorSpelling spelling)
    {
        switch (spelling)
        {
            case ColorSpelling::HexRgb:
                return L"RGB hex";
            case ColorSpelling::HexBgr:
                return L"BGR hex";
            case ColorSpelling::DecimalRgb:
                return L"RGB decimal";
            case ColorSpelling::CssHex:
                return L"CSS";
        }
        unreachable("a colour spelling with no name of its own");
    }

    float PicturePage::zoomOf(const float position)
    {
        const float zoom = PictureView::k_minZoom * std::pow(PictureView::k_zoomStep, position);
        return std::clamp(zoom, PictureView::k_minZoom, PictureView::k_maxZoom);
    }

    float PicturePage::positionOf(const float zoom)
    {
        return std::log(zoom / PictureView::k_minZoom) / std::log(PictureView::k_zoomStep);
    }
}
