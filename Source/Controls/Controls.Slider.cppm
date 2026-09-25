module;
#include "../Y-Core/System/EventBindings.h"

export module ClaFi.Controls.Slider;

import ClaFi.Controls.Base.SliderBase;
import ClaFi.Controls.Panel;
import ClaFi.Icons.Magnifier;
import ClaFi.Icons.PlusMark;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Props;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    export struct MaxPosition
    {
        float value;
    };

    export struct RelativePosition
    {
        float value;
    };

    export struct Position
    {
        float value;
    };

    // What the buttons at the two ends of a slider show. See Controls
    export enum class SliderButtonMark
    {
        Magnifier,
        PlusMinus
    };

    // Whether a context click on a slider opens a finer one over part of its range. See Controls
    export enum class FineAdjust
    {
        No,     // the context click is left to whatever else answers it
        Yes     // a slider over a tenth of the range opens under this one
    };

    // The part of a slider's range its slot is drawn over, in relative positions.
    export struct SlotSpan
    {
        float begin{ 0.0f };   // the relative position at the start of the slot
        float end{ 1.0f };     // the relative position at the end of the slot
        // The relative position a fraction of the way along the slot.
        [[nodiscard]] float at(float fraction) const { return begin + (end - begin) * fraction; }
        // How far along the slot a relative position lies, outside 0 to 1 where it is off it.
        [[nodiscard]] float fractionOf(float relative) const { return (relative - begin) / (end - begin); }
        [[nodiscard]] bool operator==(const SlotSpan&) const = default;
    };

    class FineSlider;

    // A control moved along a track to pick a value.
    export class Slider : public SliderBase
    {
        friend FineSlider;
    public:
        template<typename... Args>
        explicit Slider(const CreateParams&, Args&&...);
    public:
        // The mark on the buttons: a glass for a size, a sign for a value.
        DECLARE_WRITABLE_PROPERTY(SliderButtonMark, buttonMark, setButtonMark, SliderButtonMark::Magnifier)
        // Whether a context click opens a finer slider over a tenth of the range.
        DECLARE_PROPERTY(FineAdjust, fineAdjust, FineAdjust::Yes)
    public:
        void setButtonMark(SliderButtonMark);
        float relativePosition() const;
        void setRelativePosition(float value, bool triggerChange = true);
        [[nodiscard]] float maxPosition() const { return m_scrollInfo.max; }
        void setMaxPosition(float value);
        [[nodiscard]] bool paintTails() const { return m_paintTails; }
        void setPaintTails(bool value) { m_paintTails = value; }
    protected:
        ScrollInfo controlScrollInfo() const override { return m_scrollInfo; }
        void mouseWheel(MouseWheelEvent&) override;
        void contextPopup(ContextPopupEvent&) override;
        float stepSize() override { return 1.0f; }
        float buttonSize(const AppTheme&) override { return 24.0f; }

        void adjustButtonMetrics(AdjustMetricsEvent& event) const override { event.metrics = event.themeMetrics().button; }
        void adjustButtonPaint(AdjustPaintEvent&) override;
        void adjustThumbMetrics(AdjustMetricsEvent& event) const override;
        void adjustThumbPaint(AdjustPaintEvent&) override;
        void adjustPaint(AdjustPaintEvent&) override;

        void paintButtonMark(PaintEvent&, float size, ScrollDirection) override;
        void paintThumb(PaintEvent& pp) override;
        void paintSurface(PaintEvent&) override;
        void paintSlotBorder(PaintEvent&, FloatRect, float radius);

        virtual Color thumbColor(PaintEvent&, FloatPoint&);
        // Draws the slot over the part of the range the span names.
        virtual void paintSlot(PaintEvent&, const FloatRect&, SlotSpan);
    private:
        // Opens the finer slider under this one and runs it until it closes.
        void showFineSlider();
    private:
        // The share of the range a finer slider spans.
        static constexpr float k_fineShare = 0.1f;
        ScrollInfo m_scrollInfo{ .page = 0, .max = 100 };
        bool m_paintTails{ false };
    };

    // A slider over part of another's range, moving that one as it moves.
    class FineSlider : public Slider
    {
    public:
        FineSlider(const CreateParams&, Slider& owner, SlotSpan);
    protected:
        float stepSize() override;
        Color thumbColor(PaintEvent&, FloatPoint&) override;
        void paintSlot(PaintEvent&, const FloatRect&, SlotSpan) override;
        void changed(SliderChangeEvent&) override;
        void pressUp(PressUpEvent&) override;
    private:
        // As long as the owner, so a drag moves the value a tenth as far, and never below k_minLength.
        [[nodiscard]] static MinSize trackLengthOf(const Slider& owner);
    private:
        // The least length a finer slider takes, in design units.
        static constexpr float k_minLength = 200.0f;
        Slider& m_owner;
        SlotSpan m_span;
    };

    // The popup a context click on a slider opens, holding a finer slider over part of the range.
    class FineSliderPopup : public Panel
    {
    public:
        FineSliderPopup(const CreateParams&, Slider& owner, SlotSpan);
    protected:
        void keyDown(KeyDownEvent&) override;
    private:
        static constexpr float k_padding = 8.0f;
        Slider& m_owner;
        // Where the owner stood when the popup opened, which Escape puts back.
        float m_openedAt;
    };


    //-------------------------------------------------------------------------

    // intellisense fixing
    class BaseForm;

    template<typename ...Args>
    Slider::Slider(const CreateParams& params, Args&&... args)
        :
        SliderBase{ params, SliderViewMode::Slider, ScrollAxis::Horizontal, Interactivity::Focusable, std::forward<Args>(args)... },
        INIT_PROPERTY(buttonMark),
        INIT_PROPERTY(fineAdjust)
    {
        // TODO: a per-axis minimum size does not belong here - the axis can change after
        // construction, and this would not follow it.
        //setMinSize(
        //  axis == ScrollAxis::vertical ? MinSize{0.0f, 120.0f} : MinSize{140.0f, 0.0f}
        //);
    }

    void Slider::setButtonMark(const SliderButtonMark value)
    {
        m_buttonMark = value;
        invalidate();
    }

    float Slider::relativePosition() const
    {
        return position() / maxPosition();
    }

    void Slider::setRelativePosition(float value, bool triggerChange)
    {
        setPosition(value * maxPosition(), triggerChange);
    }

    void Slider::setMaxPosition(float value)
    {
        m_scrollInfo.max = value;
        controlFeedBack();
    }

    void Slider::mouseWheel(MouseWheelEvent&)
    {
        // That may be useful, but it we want to process the wheel here,
        // it MUST be optional and turned off by default.
        // Think of a slider on a scrollbox body.
        // event.handled = true;
        // offsetPositionBySteps(event.delta);
    }

    // The application answers first, and a handler that stops the event has replaced the finer
    // slider outright.
    void Slider::contextPopup(ContextPopupEvent& event)
    {
        SliderBase::contextPopup(event);
        if (event.propagationStopped())
            return;
        if (m_fineAdjust == FineAdjust::No || !enabled(true) || maxPosition() <= 0.0f)
            return;
        event.stopPropagation();
        showFineSlider();
    }

    void Slider::adjustButtonPaint(AdjustPaintEvent& event)
    {
        event.setColorRules(UiElement::Button);
        event.dropSurfaceAtRest();
    }

    void Slider::adjustThumbMetrics(AdjustMetricsEvent& event) const
    {
        event.metrics = event.themeMetrics().button;
        event.metrics.minSize = event.metrics.maxSize = { k_thumbSize, k_thumbSize };
        event.metrics.radius = k_thumbSize / 2.0f;
    }

    void Slider::adjustThumbPaint(AdjustPaintEvent& event)
    {
        event.setColorRules(UiElement::ScrollThumb);
    }

    void Slider::adjustPaint(AdjustPaintEvent& event)
    {
        PanelBase::adjustPaint(event);
        // The slot's border and nothing else. A slider paints a slot and its parts, never a
        // surface of its own, so it carries no rule that would describe one: a surface rule
        // left here moves m_surfaceHsl, and every colour downstream then answers to a
        // surface that is on screen nowhere - the thumb raised twice over the cell it sits in,
        // and everything fading toward a phantom.
        //
        // The thumb and the buttons state their own through adjustThumbPaint and
        // adjustButtonPaint, on their own events, and are unaffected.
        BakedElement slotRules{};
        slotRules.stroke = event.bakedColors().element(UiElement::Button).stroke;
        event.setColorRules(slotRules);
        event.setBorderWidth(event.scaledStrokeWidth(ThemeMetrics::border));
    }

    void Slider::paintButtonMark(PaintEvent& event, float size, ScrollDirection scrollDirection)
    {
        if (m_buttonMark == SliderButtonMark::PlusMinus)
        {
            // A ring the size of the glass, with the sign inside it.
            const Color ink = event.textRgb(InkGrade::Strong);
            const float stroke = event.scaleF(1.5f);
            event.canvas().drawCircle(event.center(), size, ink, stroke);
            Icons::PlusMark sign{
                .canvas = event.canvas(),
                .center = event.center(),
                .size = size,
                .lineWidth = stroke,
                .color = ink
            };
            sign.paintPlusOrMinus(scrollDirection == ScrollDirection::ToEnd);
            return;
        }

        // The glass moved out to Icons::Magnifier, where it is drawn through the canvas instead of
        // being staged into a pixel buffer - which is what it took for it to follow the canvas
        // transform. It also stops ignoring the size the base class works out, which is where the
        // parent's hover already reaches the scroll bar's mark.
        //
        // Packed the same way ScrollBar packs one for its own mark. size is the lens radius, and
        // the icon rect has to be wide enough to hold the tail beside it.
        PaintIconEvent iconEvent{
            event.controlContext(),
            Icons::Magnifier::rectForLensRadius(event.center(), size),
            event.control().tag(),
            event.disabledAmount()
        };
        Icons::Magnifier::paint(
            iconEvent,
            scrollDirection == ScrollDirection::ToEnd ? Icons::Magnifier::Lens::Plus : Icons::Magnifier::Lens::Minus,
            m_paintTails ? Icons::Magnifier::Tail::Yes : Icons::Magnifier::Tail::No
        );
    }

    void Slider::paintThumb(PaintEvent& event)
    {
        event.defaultPaintSurface();
        FloatPoint pt = event.center();
        const Color innerColor = event.parentEvent()->applyDisabledFactor(thumbColor(event, pt));
        const float thisHotFactor = hoveredFactor();
        const float innerRadius = event.scaleF(4.0f + thisHotFactor + event.hoveredFactor() - pressedFactor() * 2.0f);
        event.canvas().fillCircle(pt, innerRadius, innerColor);
    }

    void Slider::paintSurface(PaintEvent& pp)
    {
        // Focus Rect!
        //pp.surface.paint();

        // Slot
        FloatRect slotRect = bodyRect();
        float radius = pp.scaler().scaled2;
        slotRect.inflate(-radius);
        float slotSize = radius * 2.0f;
        slotRect.offset(pp.topLeft() + pp.padding());

        switch (axis())
        {
        case ScrollAxis::Horizontal:
            slotRect.top = slotRect.centerY() - radius;
            slotRect.bottom = slotRect.top + slotSize;
            slotRect.left += radius;
            slotRect.right -= radius;
            break;
        case ScrollAxis::Vertical:
            slotRect.left = slotRect.centerX() - radius;
            slotRect.right = slotRect.left + slotSize;
            slotRect.top += radius;
            slotRect.bottom -= radius;
            break;
        }

        paintSlot(pp, slotRect, SlotSpan{});
    }

    void Slider::paintSlotBorder(PaintEvent& event, FloatRect slotRect, float radius)
    {
        slotRect.inflate(event.borderWidth() / 2.0f);
        event.canvas().drawRoundedRectangle(slotRect, radius, radius, event.strokeRgb(), event.borderWidth());
    }

    Color Slider::thumbColor(PaintEvent& event, FloatPoint&)
    {
        return event.indicatorRgb();
    }

    void Slider::paintSlot(PaintEvent& event, const FloatRect& slotRect, SlotSpan span)
    {
        const float filled = std::clamp(span.fractionOf(relativePosition()), 0.0f, 1.0f);
        RoundedRectangleParts slot;
        slot.bounds = slotRect;
        float radius = 0.0f;
        const Color grayColor = event.textRgb(InkGrade::Muted);
        Color accentColor = grayColor;
        accentColor.blend(event.accentRgb(InkGrade::Strong), event.enabledFactor());

        switch (axis())
        {
        case ScrollAxis::Horizontal:
        {
            float w = slotRect.width() * filled;
            slot.bounds.right = slot.bounds.left + w;
            radius = slotRect.height() / 2.0f;
            // Paint first part of slot
            slot.radii = { radius, 0.0f, 0.0f, radius };
            event.canvas().fillPartialRoundedRectangle(slot, accentColor);

            // Paint second part of slot
            slot.bounds.left = slot.bounds.right;
            slot.bounds.right = slotRect.right;
            slot.radii = { 0.0f, radius, radius, 0.0f };
            event.canvas().fillPartialRoundedRectangle(slot, grayColor);
            break;
        }
        case ScrollAxis::Vertical:
        {
            float dblPos = filled;
            if (verticalDirection() == ScrollDirection::ToBegin)
                dblPos = 1.0f - dblPos;
            slot.bounds.bottom = slot.bounds.top + slotRect.height() * dblPos;
            radius = slotRect.width() / 2.0f;

            // Paint top part of slot
            slot.radii = { radius, radius, 0.0f, 0.0f };
            event.canvas().fillPartialRoundedRectangle(slot, grayColor);

            // Paint bottom part of slot
            slot.bounds.top = slot.bounds.bottom;
            slot.bounds.bottom = slotRect.bottom;
            slot.radii = { 0.0f, 0.0f, radius, radius };
            event.canvas().fillPartialRoundedRectangle(slot, accentColor);
            break;
        }
        }
        // Border
        paintSlotBorder(event, slotRect, radius);
    }

    // A tenth of the range centred on the value, moved inward where it would run past an end.
    void Slider::showFineSlider()
    {
        const float begin = std::clamp(relativePosition() - k_fineShare / 2.0f, 0.0f, 1.0f - k_fineShare);
        const SlotSpan span = {
            begin,
            begin + k_fineShare,
        };
        Form<FineSliderPopup> popup = form().createPopup<FineSliderPopup>(this, *this, span);
        popup.setDropdownClearance(1.0f);
        if (axis() == ScrollAxis::Vertical)
            popup.setPlacement(FormPlacement::ContextMenu, contextMenuAnchor());
        else
            popup.setPlacement(FormPlacement::Bottom, boundsInForm());
        popup.execute();
    }

    // FineSlider

    FineSlider::FineSlider(const CreateParams& params, Slider& owner, SlotSpan span)
        :
        Slider{ params, owner.axis(), FineAdjust::No, ScrollButtons::No, trackLengthOf(owner) },
        m_owner{ owner },
        m_span{ span }
    {
        setMaxPosition(owner.maxPosition() * (span.end - span.begin));
        setRelativePosition(span.fractionOf(owner.relativePosition()), false);
    }

    float FineSlider::stepSize()
    {
        return m_owner.stepSize() * k_fineShare;
    }

    Color FineSlider::thumbColor(PaintEvent& event, FloatPoint& point)
    {
        return m_owner.thumbColor(event, point);
    }

    // The owner draws its own slot, over the part of its range this slider stands for.
    void FineSlider::paintSlot(PaintEvent& event, const FloatRect& slotRect, SlotSpan)
    {
        m_owner.paintSlot(event, slotRect, m_span);
    }

    void FineSlider::changed(SliderChangeEvent& event)
    {
        m_owner.setRelativePosition(m_span.at(relativePosition()));
        Slider::changed(event);
    }

    // The popup goes when the pointer lets go of the position, and keeps where it was let go.
    // Read before the base clears the claim.
    void FineSlider::pressUp(PressUpEvent& event)
    {
        const bool held = positionHeldByPointer();
        Slider::pressUp(event);
        if (held)
            form().close();
    }

    MinSize FineSlider::trackLengthOf(const Slider& owner)
    {
        const float factor = owner.scaler().factor();
        if (owner.axis() == ScrollAxis::Vertical)
            return MinSize{ 0.0f, std::max(owner.height() / factor, k_minLength) };
        return MinSize{ std::max(owner.width() / factor, k_minLength), 0.0f };
    }

    // FineSliderPopup

    FineSliderPopup::FineSliderPopup(const CreateParams& params, Slider& owner, SlotSpan span)
        :
        Panel{ params,
            params.themeMetrics().secondaryWindow,
            params.themeMetrics().secondaryWindowShadow,
            UiElement::Menu,
            Padding{ k_padding }
        },
        m_owner{ owner },
        m_openedAt{ owner.position() }
    {
        createBody<FineSlider>(owner, span);
    }

    // Escape puts the owner back where it stood and Return keeps where it is. Both close.
    void FineSliderPopup::keyDown(KeyDownEvent& event)
    {
        switch (event.key)
        {
        case Keys::Escape:
            m_owner.setPosition(m_openedAt);
            break;
        case Keys::Return:
            break;
        default:
            Panel::keyDown(event);
            return;
        }
        event.handled = true;
        form().close();
    }
}
