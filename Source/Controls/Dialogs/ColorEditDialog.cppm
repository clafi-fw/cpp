module;
#include "../Y-Core/System/EventBindings.h"

export module ClaFi.Controls.ColorEditDialog;

import ClaFi.Controls.ColorSlider;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.Base.PanelBase;

import ClaFi.Core.Foundation;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    export class ColorEditDialog;

    // The colour moved under one of the sliders. See Controls
    export struct ColorEditEvent : public EventOf<ColorEditDialog>
    {
        using EventOf<ColorEditDialog>::EventOf;
    };

    // How a colour is written when it is copied.
    export using ColorSpeller = std::function<std::wstring(Color)>;

    using ColorEditForm = Form<PanelBase>;

    // A colour under three sliders, with a preview, Copy and Close. See Controls
    export class ColorEditDialog : public ColorEditForm
    {
    public:
        ColorEditDialog(Control& owner, std::wstring_view title, Color original, ColorSpeller);
    public:
        // The colour moved under one of the sliders. See Controls
        DECLARE_EVENT(ColorEditEvent, OnEdit, onEdit)
        [[nodiscard]] Color color() const { return m_color.toColor(); }
        [[nodiscard]] Color original() const { return m_original; }
        // Whether any slider stands away from where it was given.
        [[nodiscard]] bool edited() const;
        // Drops the dialog under its owner and runs it until it is closed.
        void execute();
        [[nodiscard]] std::wstring_view diagnosticText() const override
        {
            return L"ColorEditDialog";
        }
    private:
        // Where the colour is seen: over a run from black to white, as a ring and as a bar, so
        // it is read against both ends and against itself.
        class Preview : public Panel
        {
        public:
            template<typename... Args>
            explicit Preview(const CreateParams&, const Hsl& shown, Args&&...);
        protected:
            void paintSurface(PaintEvent&) override;
        private:
            const Hsl& m_shown;
        };
        // The three sliders and the label reading each one's value.
        struct Row
        {
            ColorSlider* slider{ nullptr };
            WithTextLayout<Label>* value{ nullptr };
        };
    private:
        [[nodiscard]] Row addRow(std::wstring_view name, ColorAttribute);
        void colorChanged();
        void copy(InputStamp);
        void writeValues();
    private:
        // What a bar keeps clear around its own content, and the gaps inside the middle.
        static constexpr float k_barPaddingX = 12.0f;
        static constexpr float k_barPaddingY = 8.0f;
        static constexpr float k_contentPadding = 12.0f;
        static constexpr float k_rowSpacing = 6.0f;
        static constexpr float k_cellSpacing = 8.0f;
        static constexpr float k_answerSpacing = 8.0f;
        static constexpr float k_previewWidth = 300.0f;
        static constexpr float k_previewHeight = 96.0f;
        static constexpr float k_ringInset = 12.0f;
        static constexpr float k_barInset = 24.0f;
        static constexpr float k_nameWidth = 72.0f;
        static constexpr float k_valueWidth = 64.0f;
        static constexpr float k_sliderWidth = 220.0f;
    private:
        Color m_original;
        Hsl m_originalHsl;
        Hsl m_color;
        ColorSpeller m_speller;
        Label& m_title;
        Panel& m_content;
        StackPanel& m_column;
        Preview& m_preview;
        Label& m_originalSample;
        Row m_hue;
        Row m_saturation;
        Row m_luminosity;
        ColorSlidersLinker m_linker;
        Panel& m_answerBar;
        StackPanel& m_answers;
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    ColorEditDialog::Preview::Preview(const CreateParams& params, const Hsl& shown, Args&&... args)
        :
        Panel{ params, std::forward<Args>(args)... },
        m_shown{ shown }
    {
    }
}
