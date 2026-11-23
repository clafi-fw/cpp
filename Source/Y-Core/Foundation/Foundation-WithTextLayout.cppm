module;
#include "../System/EventBindings.h"
#include "../System/Switches.h"

export module ClaFi.Core.Foundation :WithTextLayout;

import :Control;
import :Form;
import :Input;
import :RichControl;
import :PaintEvent;
import :Tooltip;

import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    // A link in a control's text was followed. See Controls
    export struct LinkClickEvent : public EventOf<Control>
    {
        LinkClickEvent(Control&, std::wstring target, InputStamp);
        std::wstring target;
        InputStamp stamp;

        template<typename Notify>
        void dispatch(FormBase& source, Notify&&);
    };

    // The tooltip for the link under the pointer is being asked for. See Controls
    export struct GetLinkTooltipEvent : public EventOf<Control>
    {
        GetLinkTooltipEvent(Control&, std::wstring_view target, TextRange, AdjustTooltipEvent&,
            FloatRect anchorRect);
        const std::wstring target;
        const TextRange range;
        AdjustTooltipEvent& tooltip;

        void writeDefaultTooltip() const;
    };

    // The context menu for a link is being asked for. See Controls
    export struct LinkContextPopupEvent : public EventOf<Control>
    {
        LinkContextPopupEvent(Control&, std::wstring target, TextRange, ContextPopupEvent&);
        const std::wstring target;
        const TextRange range;
        ContextPopupEvent& popup;
    };

    template<typename Notify>
    void LinkClickEvent::dispatch(FormBase& source, Notify&& notify)
    {
        const AppContext& application = source.appContext();
        std::forward<Notify>(notify)();
        if (!propagationStopped())
        {
            application.openLink(target, &source, stamp);
        }
    }

    // A control that shapes its own text in place, for a text that changes. See Control-Foundation
    export template <IsRichControl HostClass>
    class WithTextLayout : public HostClass
    {
    public:
        template <typename... Args>
        explicit WithTextLayout(const CreateParams&, Args&&...);
    public:
        DECLARE_EVENT(LinkClickEvent, OnLinkClick, onLinkClick)
        DECLARE_EVENT(GetLinkTooltipEvent, OnGetLinkTooltip, onGetLinkTooltip)
        DECLARE_EVENT(LinkContextPopupEvent, OnLinkContextPopup, onLinkContextPopup)
    public:
        // What a line set in one monospace font does with a character the font lacks.
        [[nodiscard]] CellFallback cellFallback() const { return m_cellFallback; }
        void setCellFallback(CellFallback);
        [[nodiscard]] CursorShape cursor() const override;
    protected:
        DrawTextResult drawText(PaintEvent&, const FloatRect& textBounds, const Text&) override;
        ScaledSize measureText(AlignEvent&, ScaledSize maxDimensions, const Text&) override;
        // The layout, in step with the control's text, the rect it draws in and the scale.
        [[nodiscard]] TextLayout& syncedLayout(const FormContext&,
            const FloatRect& textBounds) const;
        // The same, once the answer a gather gave is what the layout draws.
        [[nodiscard]] TextLayout& syncedLayout(const FormContext&, const FloatRect& textBounds,
            const Text& answer);
        // The width the lines are broken at; zero, the standing answer, breaks them at the box.
        [[nodiscard]] virtual float lineBreakWidth() const;
        void setBalancedWrapStart(std::size_t);
        // Tells the layout the control's text at its present revision - the whole of it.
        virtual void takeText() const;
        void mouseMove(MouseMoveEvent&) override;
        void hoverLeave() override;
        void click(ClickEvent&) override;
        void nestedAdjustTooltip(AdjustTooltipEvent&) override;
        void nestedContextPopup(ContextPopupEvent&) override;
        [[nodiscard]] virtual bool followsLinks(KeyModifiers) const;
        [[nodiscard]] virtual bool clickFollowsLink(const ClickEvent&) const;
        [[nodiscard]] virtual std::optional<LinkSpan> contextLink(const ContextPopupEvent&) const;
        virtual void followLink(const LinkSpan&, InputStamp);
        virtual void writeLinkTooltip(AdjustTooltipEvent&) const {}
        virtual void appendLinkTooltip(Text&) const {}
        [[nodiscard]] std::optional<LinkHit> linkAt(PointInForm) const;
        [[nodiscard]] std::vector<FloatRect> linkRectsInForm() const;
        [[nodiscard]] std::wstring_view pointedLinkTarget() const { return m_pointedLink.target; }
        void dispatchLink(std::wstring target, FormBase& source, InputStamp);
        void setLinksEnabled(bool);
        void clearLinkHover();
    protected:
        // Mutable because measuring is a const question, and the layout is the answer's working.
        mutable TextLayout m_layout;
    private:
        // The last measurement and everything its answer depends on. A wrapping control is
        // measured again whenever the align pass grants it a box, and a box granted the same one
        // twice is owed the same answer rather than a second shaping - so the question is what is
        // compared, and nothing here is read from the layout.
        struct Measurement
        {
            TextStamp text{};
            ScaledSize maxDimensions{ 0.0f, 0.0f };
            ScaleFactor scaleFactor{ 0.0f };
            float breakWidth{ 0.0f };
            bool wrap{ false };
            std::optional<std::size_t> balancedWrapStart{};
            CellFallback cellFallback{ CellFallback::Native };
            bool editable{ false };
            std::uint64_t layoutGeneration{ 0 };
            std::size_t paragraphVisibilityRevision{ 0 };
            bool operator==(const Measurement&) const = default;
        };
        struct PointedLink
        {
            TextRange range{};
            std::wstring target{};
            std::size_t pos{ 0 };
        };
    private:
        // The text the layout draws: the control's own, or the answer a gather composed around it.
        [[nodiscard]] const ControlText& layoutSource() const;
        // What a gather answered: the control's own text, or a composed one kept beside it.
        void takeAnswer(const Text& answer);
        void hoverLink(TextRange);
        void pointAtLink(const std::optional<LinkHit>&);
        [[nodiscard]] FloatRect pointedLinkLine() const;
    private:
        mutable TextStamp m_layoutStamp{}; // the revision of the source the layout holds
        // An answer composed around the control's text - a parent's or a handler's words - kept
        // apart from the text, which stays whatever its owner wrote. See Control-Foundation
        ControlText m_composedAnswer{};
        bool m_answerComposed{ false }; // whether the layout draws m_composedAnswer
        Measurement m_measurement{};
        ScaledSize m_measuredTextSize{};
        TextFormat m_measuredFormat{}; // what the last measurement was laid out over
        // The engine generation the layout was built at. invalidateLayouts drops the cache and
        // moves that number on for anything a key does not carry - the font table above all - and
        // this layout is not in the cache to be dropped.
        mutable std::uint64_t m_layoutGeneration{ 0 };
        CellFallback m_cellFallback{ CellFallback::Native };
        std::optional<std::size_t> m_balancedWrapStart{};
        PointedLink m_pointedLink{};
        bool m_linksEnabled{ true };
    };


//-----------------------------------------------------------------------------


    template <IsRichControl HostClass>
    template <typename... Args>
    WithTextLayout<HostClass>::WithTextLayout(const CreateParams& params, Args&&... args)
        :
        HostClass{ params, std::forward<Args>(args)... }
    {
        // Pointed at the control's own text once. From here on syncedLayout keeps the layout in
        // step with the text's revision, and nothing else is ever stated to it.
        m_layout.setText(this->text());
    }

    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::setCellFallback(const CellFallback value)
    {
        if (m_cellFallback == value)
            return;

        m_cellFallback = value;
        this->invalidateFormAlign();
    }

    template <IsRichControl HostClass>
    CursorShape WithTextLayout<HostClass>::cursor() const
    {
        return m_layout.hoveredLink().length ? CursorShape::Hand : HostClass::cursor();
    }

    template <IsRichControl HostClass>
    DrawTextResult WithTextLayout<HostClass>::drawText(PaintEvent& event,
        const FloatRect& textBounds, const Text& answer)
    {
        TextLayout& layout = syncedLayout(event.formContext(), textBounds, answer);
#if CLAFI_TEXT_MOVING
        return layout.draw(
            event.controlContext(),
            anchoredOrigin(textBounds, layout.calculatedDimensions(), this->textAnchor()),
            this->editProps(),
            this->textRenderMode()
        );
#else
        return layout.draw(
            event.controlContext(),
            anchoredOrigin(textBounds, layout.calculatedDimensions(), this->textAnchor()),
            this->editProps()
        );
#endif
    }

    // Measured on the layout the paint will draw from. Through TextEngine's cache instead, a
    // control building a text of its own gets a **second** layout of it: the key carries the text's
    // hash, so a control whose text moved misses on every measurement, shapes it again to answer
    // one question, and leaves the entry for the evictor to sweep.
    template <IsRichControl HostClass>
    ScaledSize WithTextLayout<HostClass>::measureText(AlignEvent& event,
        ScaledSize maxDimensions, const Text& answer)
    {
        // Ahead of the stamp, which is the source's once the answer is taken.
        takeAnswer(answer);
        const Measurement measurement{
            .text = layoutSource().stamp(),
            .maxDimensions = maxDimensions,
            .scaleFactor = event.scaleFactor(),
            .breakWidth = this->lineBreakWidth(),
            .wrap = this->wordWrap(),
            .balancedWrapStart = m_balancedWrapStart,
            .cellFallback = m_cellFallback,
            .editable = this->editProps() != nullptr,
            .layoutGeneration = Control::textEngine().layoutGeneration(),
            .paragraphVisibilityRevision = m_layout.paragraphVisibilityRevision(),
        };
        const TextFormat* format = this->textFormat();
        if (measurement == m_measurement && m_measuredFormat.matches(format))
            return m_measuredTextSize;

        // Only the size reaches the layout, so the rect is built at the origin - see syncedLayout,
        // which reads nothing else from it.
        m_measuredTextSize = syncedLayout(
            event.formContext(),
            FloatRect::FromDimensions({ 0.0f, 0.0f }, maxDimensions)
        ).calculatedDimensions();
        m_measurement = measurement;
        if (!m_measuredFormat.matches(format))
            m_measuredFormat = format ? *format : TextFormat{};
        return m_measuredTextSize;
    }

    template <IsRichControl HostClass>
    TextLayout& WithTextLayout<HostClass>::syncedLayout(const FormContext& formContext,
        const FloatRect& textBounds) const
    {
        // The stamp says whether the source was written since the layout took it, without reading
        // it. Compared rather than taken every time: setText invalidates, so a control measured
        // four times in one pass would rebuild four times.
        const TextStamp stamp = layoutSource().stamp();
        if (stamp != m_layoutStamp)
        {
            if (m_answerComposed)
                m_layout.setText(m_composedAnswer);
            else
                takeText();
            m_layoutStamp = stamp;
        }
        if (m_layoutGeneration != Control::textEngine().layoutGeneration())
        {
            m_layoutGeneration = Control::textEngine().layoutGeneration();
            m_layout.invalidate();
        }
        m_layout.setFormat(this->textFormat());
        // Set here rather than in the constructor: it is a property, and a control told to stop
        // wrapping after it was built draws the answer to that on the next paint.
        if (this->wordWrap() && m_balancedWrapStart)
            m_layout.setWrapMode(LineWrapMode::Balanced, *m_balancedWrapStart);
        else
            m_layout.setWrap(this->wordWrap());
        m_layout.setCellFallback(m_cellFallback);
        // Ahead of the bounds, which are accepted or refused against it.
        m_layout.setBreakWidth(this->lineBreakWidth());
        m_layout.setHorizontalAnchor(this->horizontalTextAnchor());
        // Only the size: where the rect sits is the caller's business and the anchor's, and a
        // control scrolled sideways would otherwise rebuild for every pixel it moved.
        m_layout.setBoundsAndScale(textBounds.dimensions(), formContext.scaleFactor());
        return m_layout;
    }

    template <IsRichControl HostClass>
    TextLayout& WithTextLayout<HostClass>::syncedLayout(const FormContext& formContext,
        const FloatRect& textBounds, const Text& answer)
    {
        takeAnswer(answer);
        return syncedLayout(formContext, textBounds);
    }

    template <IsRichControl HostClass>
    float WithTextLayout<HostClass>::lineBreakWidth() const
    {
        return 0.0f;
    }

    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::setBalancedWrapStart(std::size_t position)
    {
        m_balancedWrapStart = position;
    }

    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::takeText() const
    {
        m_layout.setText(this->text());
    }

    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::mouseMove(MouseMoveEvent& event)
    {
        HostClass::mouseMove(event);
        const std::optional<LinkHit> hit = linkAt(event.posOnForm);
        pointAtLink(hit);
        hoverLink(hit && followsLinks(Platform::keyModifiers()) ? hit->link.range : TextRange{});
    }

    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::hoverLeave()
    {
        HostClass::hoverLeave();
        clearLinkHover();
    }

    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::click(ClickEvent& event)
    {
        if (Input::mouse().active() && clickFollowsLink(event))
        {
            if (const std::optional<LinkHit> hit = linkAt(event.clickPos()))
            {
                event.stopPropagation();
                followLink(hit->link, event.stamp);
                return;
            }
        }
        HostClass::click(event);
    }

    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::nestedAdjustTooltip(AdjustTooltipEvent& event)
    {
        if (!m_pointedLink.range.length)
        {
            HostClass::nestedAdjustTooltip(event);
            return;
        }

        GetLinkTooltipEvent linkEvent{ *this, m_pointedLink.target, m_pointedLink.range, event,
            pointedLinkLine() };
        this->emitEvent(linkEvent);
        if (event.text.empty())
        {
            writeLinkTooltip(event);
        }
        linkEvent.writeDefaultTooltip();
        appendLinkTooltip(event.text);
    }

    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::nestedContextPopup(ContextPopupEvent& event)
    {
        if (const std::optional<LinkSpan> link = contextLink(event))
        {
            LinkContextPopupEvent linkEvent{ *this, std::wstring{ link->value }, link->range, event };
            this->emitEvent(linkEvent);
            if (event.propagationStopped())
                return;
        }
        HostClass::nestedContextPopup(event);
    }

    template <IsRichControl HostClass>
    bool WithTextLayout<HostClass>::followsLinks(KeyModifiers) const
    {
        return m_linksEnabled && this->enabled(true);
    }

    template <IsRichControl HostClass>
    bool WithTextLayout<HostClass>::clickFollowsLink(const ClickEvent& event) const
    {
        return followsLinks(event.modifiers);
    }

    template <IsRichControl HostClass>
    std::optional<LinkSpan> WithTextLayout<HostClass>::contextLink(const ContextPopupEvent& event) const
    {
        if (event.mousePos)
            if (const std::optional<LinkHit> hit = linkAt(*event.mousePos))
                return hit->link;
        return std::nullopt;
    }

    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::followLink(const LinkSpan& link, const InputStamp stamp)
    {
        dispatchLink(std::wstring{ link.value }, this->form(), stamp);
    }

    template <IsRichControl HostClass>
    std::optional<LinkHit> WithTextLayout<HostClass>::linkAt(const PointInForm point) const
    {
        if (!m_linksEnabled)
            return std::nullopt;
        const FormContext& context = this->formContext();
        const FloatRect bounds = this->textBounds(context, this->boundsInForm());
        if (!FloatRect::intersection(bounds, this->visibleRectInForm()).contains(point))
            return std::nullopt;
        TextLayout& layout = syncedLayout(context, bounds);
        const FloatPoint origin = anchoredOrigin(bounds, layout.calculatedDimensions(), this->textAnchor());
        return layout.linkAt(point - origin);
    }

    template <IsRichControl HostClass>
    std::vector<FloatRect> WithTextLayout<HostClass>::linkRectsInForm() const
    {
        std::vector<FloatRect> rects;
        if (!m_linksEnabled)
            return rects;
        const FormContext& context = this->formContext();
        const FloatRect bounds = this->textBounds(context, this->boundsInForm());
        const FloatRect clip = FloatRect::intersection(bounds, this->visibleRectInForm());
        TextLayout& layout = syncedLayout(context, bounds);
        const FloatPoint origin = anchoredOrigin(bounds, layout.calculatedDimensions(), this->textAnchor());
        for (FloatRect rect : layout.linkRects())
        {
            rect.offset(origin);
            if (rect.intersectWith(clip))
                rects.push_back(rect);
        }
        return rects;
    }

    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::dispatchLink(std::wstring target, FormBase& source,
        const InputStamp stamp)
    {
        LinkClickEvent event{ *this, std::move(target), stamp };
        event.dispatch(source, [&](){
            this->emitEvent(event);
        });
    }

    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::setLinksEnabled(const bool value)
    {
        m_linksEnabled = value;
        if (!value)
            clearLinkHover();
    }

    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::clearLinkHover()
    {
        m_pointedLink = {};
        hoverLink({});
    }

    template <IsRichControl HostClass>
    const ControlText& WithTextLayout<HostClass>::layoutSource() const
    {
        return m_answerComposed ? m_composedAnswer : this->text();
    }

    // Compared first: a composed answer has no stamp of its own, and one saying what the kept copy
    // already says would move its stamp on for nothing.
    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::takeAnswer(const Text& answer)
    {
        const Text& own = this->text();
        if (&answer == &own)
        {
            m_answerComposed = false;
            return;
        }
        if (!m_answerComposed || m_composedAnswer != answer)
            m_composedAnswer = answer;
        m_answerComposed = true;
    }

    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::hoverLink(const TextRange link)
    {
        if (link == m_layout.hoveredLink())
            return;
        m_layout.setHoveredLink(link);
        this->invalidate();
    }

    template <IsRichControl HostClass>
    void WithTextLayout<HostClass>::pointAtLink(const std::optional<LinkHit>& hit)
    {
        const TextRange range = hit ? hit->link.range : TextRange{};
        if (hit)
            m_pointedLink.pos = hit->pos;
        const std::wstring_view target = hit ? hit->link.value : std::wstring_view{};
        if (range == m_pointedLink.range && target == m_pointedLink.target)
            return;

        m_pointedLink.range = range;
        m_pointedLink.target = target;
        Tooltip::hoveredZoneChanged();
    }

    template <IsRichControl HostClass>
    FloatRect WithTextLayout<HostClass>::pointedLinkLine() const
    {
        const FormContext& context = this->formContext();
        const FloatRect bounds = this->textBounds(context, this->boundsInForm());
        TextLayout& layout = syncedLayout(context, bounds);
        const FloatPoint origin = anchoredOrigin(bounds, layout.calculatedDimensions(), this->textAnchor());
        const CaretHit hit{ m_pointedLink.pos, false };
        const std::size_t start = std::max(m_pointedLink.range.start, layout.rowStart(hit));
        const std::size_t end = std::min(m_pointedLink.range.end(), layout.rowEnd(hit));
        const FloatRect first = layout.getCaretRect({ start, false });
        const FloatRect last = layout.getCaretRect({ std::max(end, start + 1), true });
        FloatRect rect{ std::min(first.left, last.left), first.top,
            std::max(first.left, last.left), first.bottom };
        rect.offset(origin);
        return rect;
    }

}
