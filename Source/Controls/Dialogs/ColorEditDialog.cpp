module ClaFi.Controls.ColorEditDialog;

import ClaFi.Controls.ColorSlider;
import ClaFi.Controls.Slider;
import ClaFi.Controls.Button;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Controls.Base.PanelBase;
import ClaFi.Controls.Base.SliderBase;

import ClaFi.Core.Foundation;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Context.PaintIconEvent;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;

import ClaFi.Core.Transfer.Clipboard;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.Transfer.Source;

import ClaFi.Core.Graphics.Canvas;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    namespace
    {
        constexpr float k_previewRadius = 6.0f;
        constexpr float k_ringStroke = 1.5f;
        constexpr float k_sampleSwatch = 12.0f;
        constexpr float k_sampleGap = 6.0f;

        [[nodiscard]] bool sameHsl(const Hsl& first, const Hsl& second)
        {
            return first.hue == second.hue
                && first.saturation == second.saturation
                && first.luminosity == second.luminosity;
        }

        // The word with a swatch before it, both in the colour: a solid shape reads truer than
        // the strokes of the letters.
        [[nodiscard]] Text sampleCaption(const Color original)
        {
            Text caption{};
            caption << original;
            caption << InTextIcon{ k_sampleSwatch, [original](PaintIconEvent& event) {
                event.canvas().fillCircle(event.iconCenter(), event.iconWidth() / 2.0f, original);
            } };
            caption << Space{ k_sampleGap };
            caption << L"Original";
            caption << PopColor{};
            return caption;
        }
    }

    // ColorEditDialog

    ColorEditDialog::ColorEditDialog(Control& owner, const std::wstring_view title,
        const Color original, ColorSpeller speller)
        :
        ColorEditForm{
            owner.appContext(),
            WindowRole::Menu,
            &owner,
            owner.themeMetrics().secondaryWindow,
            owner.themeMetrics().secondaryWindowShadow,
            UiElement::Menu,
            // Clicked and hovered, never focused: the sliders and the answers carry the focus
            // themselves, the way a message box's text and answers do.
            Interactivity::MouseOnly,
            // The window keeps of itself exactly the border it draws, and the bars meet.
            Padding{ strokeWidth(owner.themeMetrics().secondaryWindow.border) },
            Spacing{ 0.0f }
        },
        m_original{ original },
        m_originalHsl{ original },
        m_color{ original },
        m_speller{ std::move(speller) },
        m_title{ createTopBar<Label>(
            UiElement::Header,
            Radius{ 0.0f },
            Padding{ k_barPaddingX, k_barPaddingY },
            Text{ title }
        ) },
        m_content{ createBody<Panel>(
            UiElement::Page,
            Radius{ 0.0f },
            Padding{ k_contentPadding }
        ) },
        m_column{ m_content.createBody<StackPanel>(
            Orientation::Vertical,
            Spacing{ k_rowSpacing }
        ) },
        m_preview{ m_column.add<Preview>(
            m_color,
            MinSize{ k_previewWidth, k_previewHeight },
            MaxSize{ k_maxFloat, k_previewHeight },
            Padding{ k_barInset }
        ) },
        // On the bar, in the middle of it, once the colour has moved off it - see colorChanged.
        m_originalSample{ m_preview.createBody<Label>(
            sampleCaption(original),
            HorizontalAlign::Center,
            VerticalAlign::Center
        ) },
        m_hue{ addRow(L"Hue", ColorAttribute::Hue) },
        m_saturation{ addRow(L"Saturation", ColorAttribute::Saturation) },
        m_luminosity{ addRow(L"Luminosity", ColorAttribute::Luminosity) },
        m_linker{ m_hue.slider, m_saturation.slider, m_luminosity.slider },
        m_answerBar{ createBottomBar<Panel>(
            UiElement::Bar,
            Radius{ 0.0f },
            Padding{ k_barPaddingX, k_barPaddingY }
        ) },
        m_answers{ m_answerBar.createBody<StackPanel>(
            Orientation::Horizontal,
            Spacing{ k_answerSpacing },
            ItemSizing::Equal
        ) }
    {
        // The window is whatever its rows came out as.
        setAutoFit(AutoFit::Yes);
        m_originalSample.setVisible(false);
        m_linker.onChange([this](SliderChangeEvent&) {
            colorChanged();
        });
        m_answers.add<Button>(
            L"Copy",
            VerticalTextAnchor::Center,
            HorizontalTextAnchor::Center,
            OnEvent{ [this](ClickEvent& event) {
                copy(event.stamp);
            } }
        );
        m_answers.add<Button>(
            L"Close",
            VerticalTextAnchor::Center,
            HorizontalTextAnchor::Center,
            OnEvent{ [](ClickEvent& event) {
                event.closeForm();
            } }
        );
        writeValues();
    }

    bool ColorEditDialog::edited() const
    {
        return !sameHsl(m_color, m_originalHsl);
    }

    void ColorEditDialog::execute()
    {
        // Under the control the colour was raised from. The target is tested rather than
        // assumed: FormBase accepts a popup with no target at all.
        if (const Control* owner = popupTarget())
        {
            setDropdownClearance(1.0f);
            setPlacement(FormPlacement::Bottom, owner->boundsInForm());
        }
        ColorEditForm::execute();
    }

    // Preview

    void ColorEditDialog::Preview::paintSurface(PaintEvent& event)
    {
        Panel::paintSurface(event);

        const FloatRect bounds = event.controlBounds();
        const Color shown = m_shown.toColor();
        const float radius = event.scaleF(k_previewRadius);

        const Color black = { 0, 0, 0 };
        const Color white = { 255, 255, 255 };
        event.canvas().fillLeftRightGradientRectangle(bounds, black, white);

        FloatRect ring = bounds;
        ring.inflate(-event.scaleF(k_ringInset));
        const float stroke = event.scaleF(k_ringStroke);
        event.canvas().drawRoundedRectangle(ring, radius, radius, shown, stroke);

        FloatRect bar = bounds;
        bar.inflate(-event.scaleF(k_barInset));
        event.canvas().fillRoundedRectangle(bar, radius, radius, shown);
    }

    ColorEditDialog::Row ColorEditDialog::addRow(const std::wstring_view name,
        const ColorAttribute attribute)
    {
        StackPanel& row = m_column.add<StackPanel>(
            Orientation::Horizontal,
            Spacing{ k_cellSpacing }
        );
        row.add<Label>(
            Text{ name },
            MinSize{ k_nameWidth, 0.0f },
            MaxSize{ k_nameWidth, k_maxFloat },
            VerticalTextAnchor::Center
        );
        Row result{};
        // THE READING OWNS ITS LAYOUT: it is a different text on every move of the slider.
        result.value = &row.add<WithTextLayout<Label>>(
            MinSize{ k_valueWidth, 0.0f },
            MaxSize{ k_valueWidth, k_maxFloat },
            WordWrap::No,
            HorizontalTextAnchor::Right,
            VerticalTextAnchor::Center
        );
        result.slider = &row.add<ColorSlider>(
            MinSize{ k_sliderWidth, 0.0f },
            &m_color,
            attribute,
            SliderButtonMark::PlusMinus
        );
        return result;
    }

    void ColorEditDialog::colorChanged()
    {
        m_originalSample.setVisible(edited());
        m_preview.invalidate();
        writeValues();

        ColorEditEvent event{ *this };
        emitEvent(event);
    }

    // ONE FORMAT OUT: the colour as the caller spells it, which is what an application that has
    // never heard of this dialog can take.
    void ColorEditDialog::copy(const InputStamp stamp)
    {
        Transfer::Source source{};
        source.add<Transfer::PlainText>(m_speller(color()));
        formContext().clipboard().set(std::move(source), stamp);
    }

    void ColorEditDialog::writeValues()
    {
        for (const Row& row : { m_hue, m_saturation, m_luminosity })
        {
            Text reading{};
            row.slider->paintValue(reading, EventPhase::Paint);
            row.value->text() = std::move(reading);
            row.value->invalidate();
        }
    }
}
