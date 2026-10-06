export module ThisApp.HueRuleControl;

import ThisApp.Consts;
import ThisApp.Utils;

import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Controls.Base.DropdownControlBase;
import ClaFi.Controls.Base.SliderBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.ColorSlider;
import ClaFi.Controls.Divider;
import ClaFi.Controls.LabeledDivider;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Stack;

import ClaFi.Icons.XMark;
import ClaFi.StdActions;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Metrics;

import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;

import ClaFi.Core.Context.FormContext;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.InkWell;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // Told when the rule moves, where the edit stands and what it is called - held by address.
    export using OnHueRuleChanged = std::function<void(EditPhase, const Text& what)>;

    class HueRulePopup;
    class HueSlider;

    // The saturation and luminosity every hue blob is drawn at. A rule names a hue and nothing
    // else, so two rules carrying the same hue have to read the same whatever the theme puts
    // around them. Both the control's face and the popup's items draw against these.
    constexpr float k_blobSaturation{ 0.5f };
    constexpr float k_blobLuminosity{ 0.5f };
    // The mark against a k_harmonyItemSize tile, matching the one the harmony picker's maps carry.
    constexpr MinSize k_indicatorSize{ 16.0f };

    // What sits on a blob. One character, because the blob is the size of one and the name it
    // stands for is in the hint.
    constexpr std::array<std::wstring_view, 3> k_paletteDigits{ L"1", L"2", L"3" };
    // The exact hue is set outright, and is marked with the character the operation combo boxes
    // already give ColorRuleOp::Set.
    constexpr std::wstring_view k_exactHueGlyph{ L"=" };
    constexpr std::wstring_view hueOpGlyph(ColorRuleHueOp value)
    {
        if (value == ColorRuleHueOp::ExactValue)
            return k_exactHueGlyph;
        // Anything that is not a palette entry writes nothing: the digits table holds three, and
        // NoChange sits past them. The guard is here rather than in the callers, so a second
        // caller cannot index off the end of it.
        if (value >= ColorRuleHueOp::EndPalette)
            return {};
        return k_paletteDigits[static_cast<std::size_t>(value)];
    }

    // What an operation puts on the control, written the same way wherever it appears - the
    // cell's face and the popup's item are one statement about one rule.
    //
    // No change says so in words, there being no colour to show, and says it in the subdued
    // style the operation combo boxes give their own No change item - which is what makes the
    // two dropdowns read alike. Everything else writes its one character, which the control's
    // text anchors centre for it. Every descriptive name is the hint's.
    void writeHueOpContent(Text& text, ColorRuleHueOp value, bool onBlob)
    {
        if (value == ColorRuleHueOp::NoChange)
        {
            text << InkWell::textInk(InkGrade::Subtle)
                << TextStyleId::SubBody
                << k_hueOpNames[static_cast<std::size_t>(value)]
                << PopTextStyle{}
                << PopColor{};
            return;
        }

        // On a blob the glyph is written as every blob's is, at the blob's size. Off one it is
        // written the way the operation combo boxes write their own - bold, in the spot ink.
        // See ThemePage::operationCells.
        if (onBlob)
        {
            text << k_blobGlyphStyle;
            writeBlobGlyph(text, hueOpGlyph(value));
            text << PopTextStyle{};
            return;
        }
        text
            << InkWell::spotInk()
            << TextOp::PushBold
            << hueOpGlyph(value)
            << TextOp::PopBold
            << PopColor{};
    }

    // A degree sign, written as a hex code so the file needs no /utf-8 flag - the spelling
    // ColorSlider uses for the same character.
    constexpr wchar_t k_degreeSign{ L'\xb0' };

    // The longest operation name there is, which is the one the summary line asks for room for.
    // Picked by length rather than measured: the names are written in one style at one size, so
    // the longest string is the widest line to within a character's width, and a name added to the
    // table is covered without touching this.
    constexpr std::wstring_view widestHueOpName()
    {
        std::wstring_view result{};
        for (std::wstring_view name : k_hueOpNames)
            if (name.size() > result.size())
                result = name;
        return result;
    }

    // How an operation shows itself in the popup. Both views carry the radio mark and the glyph
    // that names the operation; a Tile also carries the blob of its own colour, and a Glyph leaves
    // that out because whatever stands beside it already states the colour.
    enum class HueItemView
    {
        Tile,
        Glyph
    };


    // HueRuleControl

    /// The Hue cell's editor. Its face carries the hue the rule resolves to as a blob, with the
    /// palette index written on it where the rule names one; its popup carries the three palette
    /// hues, a slider standing for the custom one, and the Clear command that names none of them
    /// - left out where the rule may not be cleared.
    ///
    /// What it edits is bound after construction. A grid cell is built from a blueprint that names
    /// a column, not a row, so which rule the control stands for is only settled once the row it
    /// landed in exists - see ThemePage::updateHueControl.
    export class HueRuleControl : public DropdownControlBase
    {
        friend HueRulePopup;
    public:
        template <typename... Args>
        HueRuleControl(const CreateParams&, Args&&...);
    public:
        // canClear says whether No change is a hue this rule may take. A window root's surface
        // states an absolute colour, so its hue has to be named and Clear is refused on it.
        // Takes the rule by address, the palette, what to call and what to call the edit by.
        void bind(ColorRuleHue&, const ThemeColors&, const OnHueRuleChanged&, bool canClear,
            Text what);
        [[nodiscard]] bool bound() const { return m_rule; }
        [[nodiscard]] bool canClear() const { return m_canClear; }
        [[nodiscard]] ColorRuleHueOp operation() const { return m_rule->operation(); }
        /// The hue an operation stands for. Palette operations read the theme's palette, the exact
        /// one reads what the slider is holding, and No change has no hue at all - which is what
        /// the blob is omitted on rather than filled with some neutral colour.
        [[nodiscard]] float hueOf(ColorRuleHueOp) const;
        [[nodiscard]] bool hasHue(ColorRuleHueOp value) const { return value != ColorRuleHueOp::NoChange; }
        // The colour an operation's blob is filled with.
        [[nodiscard]] Color blobColor(ColorRuleHueOp) const;
    protected:
        // Every part of the control opens the same popup, so there is no separate primary action.
        [[nodiscard]] bool dropOnPrimaryPress() const override { return true; }
        void showDropdown(Control&) override;
        void getMainText(GetTextEvent&) const override;
        void nestedGetHint(GetHintEvent&) override;
        DrawTextResult drawText(PaintEvent&, const FloatRect& textBounds, const Text&) override;
    private:
        void setOperation(ColorRuleHueOp);
        void exactHueChanged(EditPhase);
        void changed(EditPhase);
    private:
        ColorRuleHue* m_rule{};
        bool m_canClear{ true };
        const ThemeColors* m_colors{};
        const OnHueRuleChanged* m_onChanged{};
        Text m_what{}; // what an edit here is called, as the page records it
        // What the popup's slider edits. ColorSlider tracks an Hsl and a hue rule holds a bare
        // float, so the colour lives here and its hue is written back to the rule on every change.
        Hsl m_exactColor{ 0.0f, k_blobSaturation, k_blobLuminosity };
    };

    // HueRuleItem

    /// One operation in the popup. Picking settles the rule and leaves the popup up: every choice
    /// here is one the eye wants to compare against its neighbours, and a popup shutting on the
    /// first pick would have to be reopened to see the second. The close button is the way out.
    class HueRuleItem : public Button
    {
    public:
        template <typename... Args>
        HueRuleItem(const CreateParams&, HueRulePopup&, ColorRuleHueOp, HueItemView, Args&&...);
    protected:
        [[nodiscard]] MinSize indicatorSize(const AppTheme&) const override { return k_indicatorSize; }
        void getText(GetTextEvent&) const override;
        void nestedGetHint(GetHintEvent&) override;
        void getControlState(GetStateEvent&) const override;
        void nestedClick(ClickEvent&) override;
        DrawTextResult drawText(PaintEvent&, const FloatRect& textBounds, const Text&) override;
    private:
        HueRulePopup& m_popup;
        ColorRuleHueOp m_operation;
        HueItemView m_view;
    };

    // HueSlider

    /// The popup's hue slider, which is the custom hue's control and its radio button at once:
    /// taking hold of it says the hue is this one, whether or not the thumb then moves.
    ///
    /// A press that moves the thumb reports itself as a change, and the popup listens for that. A
    /// press that stays put reports nothing, and a press landing on the thumb is stopped there
    /// before any listener sees it - so the standing press is taken here, at the one hook a slider
    /// offers for it.
    class HueSlider : public ColorSlider
    {
    public:
        template <typename... Args>
        HueSlider(const CreateParams&, HueRulePopup&, Args&&...);
    protected:
        void thumbPressDown() override;
    private:
        HueRulePopup& m_popup;
    };

    // HueRulePopup

    /// What the rule says now, the three palette hues, and a slider standing for the fourth
    /// choice. Everything is built with the popup: a popup is sized to its content once, when it
    /// opens, so a control appearing in it afterwards would have nowhere to go, and a line of text
    /// growing in it would have no room to grow into.
    class HueRulePopup : public Stack
    {
        friend HueRuleItem;
        friend HueSlider;
    public:
        HueRulePopup(const CreateParams&, HueRuleControl&);
    private:
        Control& record(Control&);
        void pick(ColorRuleHueOp);
        // The head line: which operation the rule names and the hue it stands for.
        void writeSummary(Text&, EventPhase) const;
    private:
        static constexpr float k_minWidth{ 220.0f };
        static constexpr float k_dividerPadding{ 4.0f };
        static constexpr float k_closeIconSize{ 12.0f };
        // Larger than the close mark: a cross is two strokes corner to corner and fills its box,
        // while the wipe is a shape with a trail and needs the room to stay legible beside it.
        static constexpr float k_clearIconSize{ 16.0f };
        HueRuleControl& m_owner;
        std::vector<Control*> m_stateItems{};
        Label* m_summary{};
        HueSlider* m_slider{};
    };


    //----------------------------------------------------------------------------


    // The blob a hue reads as, drawn behind the run that stands for it - a digit sits on it, and
    // the custom hue's empty run leaves it bare. A rounded square, the shape the harmony picker
    // gives its palette map tiles.
    void paintHueBlob(PaintEvent& event, const FloatRect& bounds, Color color)
    {
        const float radius = bounds.height() * k_blobCornerShare;
        event.canvas().fillRoundedRectangle(bounds, radius, radius, event.applyDisabledFactor(color));
    }

    // A square of the given side, centred on the run. The control's text anchors have already
    // centred the glyph, so centring the blob on the same run puts one over the other.
    FloatRect blobBounds(const FloatRect& textBounds, float side)
    {
        FloatRect result = textBounds;
        result.inflate((side - result.width()) / 2.0f, (side - result.height()) / 2.0f);
        return result;
    }

    // HueRuleControl

    template <typename... Args>
    HueRuleControl::HueRuleControl(const CreateParams& params, Args&&... args)
        :
        DropdownControlBase{ params,
            HorizontalTextAnchor::Center,
            VerticalTextAnchor::Center,
            std::forward<Args>(args)...
        }
    {
    }

    void HueRuleControl::bind(ColorRuleHue& rule, const ThemeColors& colors,
        const OnHueRuleChanged& onChanged, bool canClear, Text what)
    {
        m_rule = &rule;
        m_canClear = canClear;
        m_colors = &colors;
        m_onChanged = &onChanged;
        m_what = std::move(what);
        m_exactColor.hue = rule.exactValue();
        invalidate();
    }

    float HueRuleControl::hueOf(ColorRuleHueOp value) const
    {
        if (value == ColorRuleHueOp::ExactValue)
            return m_exactColor.hue;
        if (value < ColorRuleHueOp::EndPalette)
            return m_colors->paletteHues[static_cast<std::size_t>(value)];
        return 0.0f;
    }

    Color HueRuleControl::blobColor(ColorRuleHueOp value) const
    {
        Hsl hsl = { hueOf(value), k_blobSaturation, k_blobLuminosity };
        return hsl.toColor();
    }

    void HueRuleControl::showDropdown(Control&)
    {
        if (!bound())
            return;
        // The slider edits this colour in place, so it starts from what the rule holds rather than
        // from wherever the last popup left it.
        m_exactColor.hue = m_rule->exactValue();
        dropPopup<HueRulePopup>(form(), *this);
    }

    // Measured as the widest face, No change, so a column of these keeps one width whatever
    // operation each row holds, and holds it while one changes.
    void HueRuleControl::getMainText(GetTextEvent& event) const
    {
        if (!bound())
            return;
        const ColorRuleHueOp shown = event.phase() == EventPhase::Calculate
            ? ColorRuleHueOp::NoChange
            : operation();
        writeHueOpContent(event.text, shown, true);
    }

    void HueRuleControl::nestedGetHint(GetHintEvent& event)
    {
        if (bound())
            event.text << k_hueOpNames[static_cast<std::size_t>(operation())];
        DropdownControlBase::nestedGetHint(event);
    }

    DrawTextResult HueRuleControl::drawText(PaintEvent& event, const FloatRect& textBounds, const Text& text)
    {
        // The popup tile's blob, so the face and the tile read as one mark.
        if (bound() and hasHue(operation()))
            paintHueBlob(event, blobBounds(textBounds, event.scaleF(k_blobSize)), blobColor(operation()));
        return DropdownControlBase::drawText(event, textBounds, {text});
    }

    void HueRuleControl::setOperation(ColorRuleHueOp value)
    {
        if (m_rule->operation() == value)
            return;
        m_rule->setOperation(value);
        changed(EditPhase::Settled);
    }

    void HueRuleControl::exactHueChanged(const EditPhase phase)
    {
        m_rule->setExactValue(m_exactColor.hue);
        changed(phase);
    }

    void HueRuleControl::changed(const EditPhase phase)
    {
        invalidate();
        if (m_onChanged and *m_onChanged)
            (*m_onChanged)(phase, m_what);
    }

    // HueRuleItem

    template <typename... Args>
    HueRuleItem::HueRuleItem(const CreateParams& params, HueRulePopup& popup, ColorRuleHueOp operation,
        HueItemView view, Args&&... args)
        :
        Button{ params,
            // Not a tool, so not a ToolButton - but it wears the same look, and the element and
            // the metrics are the whole of that look.
            params.themeMetrics().toolButton,
            UiElement::ToolButton,
            IndicatorVisibility::Always,
            IndicatorStyle::Radio,
            ShowSelectionOnSurface::Yes,
            Tag{ operation },
            std::forward<Args>(args)...
        },
        m_popup{ popup },
        m_operation{ operation },
        m_view{ view }
    {
    }

    void HueRuleItem::getText(GetTextEvent& event) const
    {
        writeHueOpContent(event.text, m_operation, m_view == HueItemView::Tile);
    }

    void HueRuleItem::nestedGetHint(GetHintEvent& event)
    {
        event.text << k_hueOpNames[static_cast<std::size_t>(m_operation)];
        Button::nestedGetHint(event);
    }

    void HueRuleItem::getControlState(GetStateEvent& event) const
    {
        if (&event.control != this)
            return;
        event.state.selected = m_popup.m_owner.operation() == m_operation;
        // The mark says which operation the rule names. The walk runs leaf to root and the
        // last writer wins, so it is stopped here before the enclosing ActiveContainer writes
        // its own current item over it - what is focused is not what is chosen.
        event.stopPropagation();
    }

    void HueRuleItem::nestedClick(ClickEvent&)
    {
        m_popup.pick(m_operation);
    }

    DrawTextResult HueRuleItem::drawText(PaintEvent& event, const FloatRect& textBounds, const Text& text)
    {
        const HueRuleControl& owner = m_popup.m_owner;
        // A Glyph item skips the blob and nothing else. It keeps its mark and its glyph, which are
        // what make it read as one of the four choices rather than as decoration beside a slider.
        if (m_view == HueItemView::Tile and owner.hasHue(m_operation))
            paintHueBlob(event, blobBounds(textBounds, event.scaleF(k_blobSize)), owner.blobColor(m_operation));
        return Button::drawText(event, textBounds, text);
    }

    // HueSlider

    template <typename... Args>
    HueSlider::HueSlider(const CreateParams& params, HueRulePopup& popup, Args&&... args)
        :
        ColorSlider{ params, std::forward<Args>(args)... },
        m_popup{ popup }
    {
    }

    void HueSlider::thumbPressDown()
    {
        ColorSlider::thumbPressDown();
        m_popup.pick(ColorRuleHueOp::ExactValue);
    }

    // HueRulePopup

    HueRulePopup::HueRulePopup(const CreateParams& params, HueRuleControl& owner)
        :
        Stack{ params,
            Orientation::Vertical,
            Interactivity::ActiveContainer,
            params.themeMetrics().secondaryWindow,
            params.themeMetrics().secondaryWindowShadow,
            UiElement::Menu,
            Padding{ k_dividerPadding },
            Spacing{ k_dividerPadding },
            MinSize{ k_minWidth, 0.0f }
        },
        m_owner{ owner }
    {
        onGetActionState([this](GetActionStateEvent& event) {
            if (&event.action == &StdActions::clear)
                event.claim({ .enabled = m_owner.canClear()
                    and m_owner.operation() != ColorRuleHueOp::NoChange });
            });
        onActionClick([this](ActionClickEvent& event) {
            if (&event.action == &StdActions::clear)
                pick(ColorRuleHueOp::NoChange);
            });

        add<Controls::LabeledDivider>(
            L"Palette hues",
            Padding{ 0.0f, k_dividerPadding }
        );

        // The three palette operations, laid out the way the harmony picker lays out its palette
        // maps: one lane of radio marked tiles. Written out rather than looped over
        // ColorRuleHueOp, whose order leads with the palette entries so that an enumerator indexes
        // straight into paletteHues - a storage concern, not a reading one.
        Stack& tileRow = add<Stack>(
            Orientation::HorizontalWrap,
            Interactivity::ActiveContainer,
            LaneSize{ 3 },
            Spacing{ k_dividerPadding }
        );
        record(tileRow.add<HueRuleItem>(*this, ColorRuleHueOp::PaletteColor1, HueItemView::Tile,
            k_harmonyItemSize, HorizontalTextAnchor::Center, VerticalTextAnchor::Center));
        record(tileRow.add<HueRuleItem>(*this, ColorRuleHueOp::PaletteColor2, HueItemView::Tile,
            k_harmonyItemSize, HorizontalTextAnchor::Center, VerticalTextAnchor::Center));
        record(tileRow.add<HueRuleItem>(*this, ColorRuleHueOp::PaletteColor3, HueItemView::Tile,
            k_harmonyItemSize, HorizontalTextAnchor::Center, VerticalTextAnchor::Center));

        add<Controls::LabeledDivider>(
            L"Custom hue",
            Padding{ 0.0f, k_dividerPadding }
        );

        // The custom hue stands beside the slider rather than in the tile row above it: the slider
        // is what states the colour, so the item that says it is the chosen one belongs next to it.
        // It is an item like the other three - the same mark, the same glyph, picked and given its
        // hint by the same code - and drops only the blob, which would say what the slider a few
        // pixels away is already saying.
        Panel& sliderRow = static_cast<Panel&>(record(add<Panel>(
            HorizontalAlign::Fill,
            UiElement::Button
            ) ) );
        Control& exactMark = record(sliderRow.createLeftBar<HueRuleItem>(
            *this,
            ColorRuleHueOp::ExactValue,
            HueItemView::Glyph,
            HorizontalTextAnchor::Center,
            VerticalTextAnchor::Center,
            VerticalAlign::Center,
            ShowSelectionOnSurface::No
        ));
        m_slider = &sliderRow.createBody<HueSlider>(*this,
            EditedColor{ m_owner.m_exactColor },
            HslChannel::Hue,
            ScrollButtons::No,
            HorizontalAlign::Fill,
            Padding{ 4.0f }
        );
        // The thumb starts on the hue the rule already holds. Nothing is propagated from it:
        // the popup is only being opened, and a change would raise the thumb's hint.
        m_slider->trackingValueChanged(false);
        // Moving the slider is a statement that the hue is this one, so it settles the operation
        // as well as the value: the user takes hold of the slider and the rule follows, rather
        // than having to name the custom hue first and then say which. A press that never moves
        // the thumb says the same thing and reaches HueSlider::thumbPressDown instead.
        m_slider->connectEvent([this, &exactMark](SliderChangeEvent&) {
            // Guarded, because pick() asks every item for its state. A drag reports continuously,
            // and only its first report changes the operation.
            if (m_owner.operation() != ColorRuleHueOp::ExactValue)
                pick(ColorRuleHueOp::ExactValue);
            exactMark.invalidate();
            m_owner.exactHueChanged(m_slider->editPhase());
            if (m_summary)
                m_summary->invalidate();
            });
        // The hue the pointer let go of is the one it stated - said once more, settled.
        m_slider->onSettle([this](SliderSettleEvent&) {
            m_owner.exactHueChanged(EditPhase::Settled);
            });

        Stack& commandBar = add<Stack>(
            UiElement::Section,
            Orientation::Horizontal,
            Spacing{ 12.0f },
            Padding{ 12.0f },
            ItemSizing::Equal
        );

        // The button is not the clear action, so the action's state never reaches it: a rule that
        // may not be cleared has to go without the button instead.
        if (m_owner.canClear())
        {
            commandBar.add<Button>(
                //ButtonViewMode::LeftIcon,
                //IconSize{ k_clearIconSize },
                //StdActions::clear,
                OnClick{ [this](ClickEvent& event) {
                    pick(ColorRuleHueOp::NoChange);
                    event.closeForm();
                } },
                Text{ L"Clear" },
                HorizontalTextAnchor::Center
            );
        }
        commandBar.add<Button>(
            //ButtonViewMode::LeftIcon,
            //IconSize{ k_closeIconSize },
            Events{
                //Icons::XMark::paint,
                [](ClickEvent& event) { event.closeForm(); }
            },
            Text{ L"Close" },
            HorizontalTextAnchor::Center
        );
    }

    Control& HueRulePopup::record(Control& item)
    {
        m_stateItems.push_back(&item);
        return item;
    }

    void HueRulePopup::pick(ColorRuleHueOp value)
    {
        m_owner.setOperation(value);
        // The popup stays up for a pick made on the slider or on Clear, so the item that lost the
        // mark is still on screen and has to be asked for its state again - a repaint alone reads
        // the state already recorded.
        for (Control* item : m_stateItems)
            item->invalidateState();
        if (m_summary)
            m_summary->invalidate();
        // Clear is available exactly while there is something to clear, and that has just moved.
        // Asking the action refreshes every presenter of it, this popup's button among them.
        StdActions::clear.invalidateState();
    }

    void HueRulePopup::writeSummary(Text& text, EventPhase phase) const
    {
        return;
        // At calculate time the widest line it could ever show, rather than the one it shows now:
        // the popup is sized to its content once, so the summary has to ask for the room every
        // later value will need. 360 is the widest hue there is.
        if (phase == EventPhase::Calculate)
        {
            text << InkWell::textInk(InkGrade::Muted) << widestHueOpName() << L": " << PopColor{}
                << L"360 " << k_degreeSign;
            return;
        }
        const ColorRuleHueOp operation = m_owner.operation();
        // No change has no hue to state, and says so in the subdued style the operation
        // combo boxes give their own No change item - which is what makes the two dropdowns read
        // alike.
        if (!m_owner.hasHue(operation))
        {
            writeHueOpContent(text, operation, false);
            return;
        }
        // The operation names the line and the hue is what the line is about, so the name is
        // written in the gray the page gives its own field labels and the value in the ordinary
        // ink - the reading order the grid's own rows are laid out in.
        text << InkWell::textInk(InkGrade::Muted)
            << k_hueOpNames[static_cast<std::size_t>(operation)]
            << L": "
            << PopColor{}
            << static_cast<int>(std::round(m_owner.hueOf(operation) * 360.0f))
            << L' '
            << k_degreeSign;
    }

}
