export module ClaFi.Controls.ScrollBox;

import ClaFi.Controls.Panel;
import ClaFi.Controls.Button;
import ClaFi.Controls.ScrollBar;
import ClaFi.Controls.Base.SliderBase;
import ClaFi.Controls.Base.PanelBase;
import ClaFi.Diagnostic.Log;
import ClaFi.Diagnostic.Options;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Props;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // Which scroll bars the box carries.
    export enum class ScrollBars
    {
        Both,
        Vertical,
        Horizontal,
        None,
        // A bar on whichever axis the content overruns, and none on the axis it fits. See Controls
        Auto
    };

    // A box that scrolls whatever stands in it, with the bars it is told to carry.
    export class ScrollBox : public PanelBase
    {
    public:
        using PanelBase::createBody;
        using PanelBase::createTopBar;
    public:
        template<typename... Args>
        explicit ScrollBox(const CreateParams& params, Args&&... args);
    public:
        std::wstring_view diagnosticText() const override { return L"ScrollBox"; }
        // Fills the corner - the square the two bars leave between them - so what is put here
        // stands only while BOTH bars are up, and a box that shows one bar shows no corner. The
        // control is sized by itself and the horizontal bar shortens by it; a corner nothing was
        // put in is k_cornerSize wide, which is what holds the horizontal bar clear of the
        // vertical one.
        template<IsControl ControlClass, typename... Args>
        ControlClass& createCorner(Args&&... args)
        {
            return m_corner.createBody<ControlClass>(std::forward<Args>(args)...);
        }
    public:
        void setScrollBars(ScrollBars value);
        void scrollToBegin(); // both bars back to the start, without a glide
        // The surface the box's bars stand on, where the host has painted something of its own
        // behind the column they sit in - see TabbedBox::paintPageRun. Nothing in the parent
        // chain says it: the box stands on the host's surface and its bars no longer do. Empty
        // is a box whose bars stand where the box does.
        void setBarSurface(std::optional<Hsl> value) { m_barSurface = value; }
        ScrollBar& vScrollBar() const { return m_vScrollBar; }
        [[nodiscard]] FloatRect viewPort(FormBase&) const override;
        [[nodiscard]] FloatPoint scrollTravelRemaining() const override;
    protected:
        void scrollBy(FloatPoint) override;
        ScaledDimensions calculateContent(AlignEvent&) override;
        // States what this box can be cut to, into the event the measuring pass carries.
        void stateSizeGivenWay(AlignEvent&) const;
        void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&) override;
        bool controlIsOnScrollBox(Control&) override;
        [[nodiscard]] bool scrollsChild(const Control&, ScrollAxis) const override;

        void adjustChildMetrics(AdjustMetricsEvent&) const override;
        void bodySlotSettled(AlignEvent&) override;
        void adjustChildViewport(AdjustViewportEvent& event) const override;
        void adjustChildClip(const Control& child, FloatRect& clip) const override;
        void adjustChildPaint(AdjustPaintEvent&) override;
        void paintChildSurface(PaintEvent&) override;
        void childPainted(PaintEvent& event) override;

        void scrollChildIntoView(Control&, FloatRect) override;
        // user input
        void mouseWheel(MouseWheelEvent&) override;
        void mouseHWheel(MouseWheelEvent&) override;
        void drag(DragEvent&) override;
        void pressDown(PressDownEvent&) override;
        void pressUp(PressUpEvent&) override;
    private:
        using PanelBase::createRightBar;
        using PanelBase::createBottomBar;
    private:
        [[nodiscard]] FloatRect bodySlotInForm(FloatPoint contentOriginInForm) const;
        // Whether the body's extent along the axis is its own, with a bar to reach what overruns.
        [[nodiscard]] bool givesWay(ScrollAxis) const;
        // One place the three visibilities are settled together: the corner is the square the
        // two bars leave between them, so it belongs to whether BOTH are up.
        void showBars(bool vVisible, bool hVisible);
        // Whether Auto wants the vertical bar beside content this tall. See Controls
        [[nodiscard]] bool autoVerticalBarNeeded(const AlignEvent&, float contentHeight) const;
        // Whether Auto wants the horizontal bar beside content this wide. See Controls
        [[nodiscard]] bool autoHorizontalBarNeeded(const AlignEvent&, float contentWidth) const;
        // Stands the bars named and measures the content again beside them.
        [[nodiscard]] ScaledDimensions remeasureWithBars(AlignEvent&, bool vVisible, bool hVisible);
        // Answers the vertical bar from the slot the body was laid out into. See Controls
        void settleAutoVerticalBar(AlignEvent&);
        void wheelScrolled(MouseWheelEvent&, ScrollBar&, float steps);
        void startOrStopAutoScrolling(SliderBase::ScrollButton& button, float current, float boundary);
        void bodyScrolled(FloatPoint change);
    private:
        static constexpr float k_autoScrollZone = 16.0f;
        // What an unfilled corner comes to: the width of the vertical bar, so the horizontal one
        // stops where the vertical one begins.
        static constexpr float k_cornerSize = 12.0f;
        // Past this the pointer is not asking for more - it is making the same request from
        // further away, and an unbounded multiplier would carry the content off in one frame.
        static constexpr float k_maxAutoScrollSpeed = 8.0f;
        // Under this two viewports are the same viewport: a pass asked for on floating-point
        // noise is a pass every frame.
        static constexpr float k_viewportEpsilon = 0.5f;
        // Under this past the slot is the measure's rounding, not content out of sight.
        static constexpr float k_overrunTolerance{ 1.0f };
    private:
        // The colour the body's surface resolved to. What paints across the gap between the body
        // and something standing beside it needs the colour and not the rule: a vertical tab strip
        // is the body of a ScrollBox, and its open tab meets the page THROUGH the strip's scroll
        // bar, which is outside the body. The body's own paint is the only place the rules are
        // settled into a colour.
        Color m_actualBodyColor{};
        std::optional<Hsl> m_barSurface{};
        // What the box was told to show. Both bars stand until something says otherwise, which
        // is what they do with no ScrollBars prop passed at all.
        ScrollBars m_scrollBars{ ScrollBars::Both };
        // The width of the body's viewport IN DESIGN UNITS, taken with the scale that laid it
        // out - see adjustChildMetrics, the only reader, and bodySlotSettled, the only writer.
        float m_viewportWidthInDesign{};
        // Whether the body stood past its slot the last time one was laid out. See Controls
        std::optional<bool> m_bodyOverranSlot{};
        ScrollBar& m_vScrollBar{ createRightBar<ScrollBar>(ScrollAxis::Vertical)};
        Panel& m_bottomBar{ createBottomBar<Panel>(UiElement::Section, themeMetrics().page) };
        ScrollBar& m_hScrollBar{ m_bottomBar.createBody<ScrollBar>(ScrollAxis::Horizontal)};
        Panel& m_corner{ m_bottomBar.createRightBar<Panel>(MinSize{ k_cornerSize, 0.0f }) };
        bool m_scrolledInDragOrByWheel{};
        bool m_inDrag{};
        float m_autoScrollZone{};
    };


//-----------------------------------------------------------------------------


    template<typename ...Args>
    ScrollBox::ScrollBox(const CreateParams& params, Args &&... args)
        :
        PanelBase{ params, std::forward<Args>(args)... }
    {
        Props::ifThereIs<ScrollBars>([&](const auto& p) {
            setScrollBars(p);
            }, std::forward<Args>(args)...);

        m_hScrollBar.onChange([this](const SliderChangeEvent& params) {
            setControlLeft(*body(), bodyRect().left - params.newPosition);
            bodyScrolled({ params.newPosition - params.previousPosition, 0 });
            });
        m_vScrollBar.onChange([this](const SliderChangeEvent& params) {
            setControlTop(*body(), bodyRect().top - params.newPosition);
            bodyScrolled({ 0, params.newPosition - params.previousPosition });
            });

        m_vScrollBar.setPadding(0.0f);
    }

    void ScrollBox::setScrollBars(ScrollBars value)
    {
        m_scrollBars = value;
        // Auto names no bar here. It says the content decides, and the content has not been
        // measured yet - so the box starts with neither bar and calculateContent puts up the
        // ones the measurement asks for.
        showBars(
            value == ScrollBars::Both || value == ScrollBars::Vertical,
            value == ScrollBars::Both || value == ScrollBars::Horizontal
        );
    }

    // BOTH BARS HOME, AND WITHOUT A GLIDE. A glide explains a change to a view the user is
    // already looking at, and what stands under the bars after this is not the content they were
    // measuring - so there is no travel across it to show.
    void ScrollBox::scrollToBegin()
    {
        // The pointer has not moved the content here, so the mouse-down anchor does not follow it.
        m_scrolledInDragOrByWheel = false;
        m_hScrollBar.setPosition(0.0f);
        m_vScrollBar.setPosition(0.0f);
    }

    FloatRect ScrollBox::viewPort(FormBase& form) const
    {
        return bodySlotInForm(form.contentOriginOfControl(this));
    }

    // A bar that is not up holds both its position and its target at zero, so it names no
    // travel of its own.
    FloatPoint ScrollBox::scrollTravelRemaining() const
    {
        return {
            m_hScrollBar.positionTarget() - m_hScrollBar.position(),
            m_vScrollBar.positionTarget() - m_vScrollBar.position(),
        };
    }

    // From where the bar is, not where it is heading: the caller has measured `delta` against the
    // view as it stands, and a glide in flight is ended where it stands.
    void ScrollBox::scrollBy(FloatPoint delta)
    {
        // The pointer has not moved the content here, so the mouse-down anchor does not follow it.
        m_scrolledInDragOrByWheel = false;
        if (delta.x != 0.0f)
            m_hScrollBar.setPosition(m_hScrollBar.position() + delta.x);
        if (delta.y != 0.0f)
            m_vScrollBar.setPosition(m_vScrollBar.position() + delta.y);
    }

    ScaledDimensions ScrollBox::calculateContent(AlignEvent& event)
    {
        ScaledDimensions result = PanelBase::calculateContent(event);
        if (m_scrollBars == ScrollBars::Auto)
        {
            // Against the content maximum rather than the box's: what comes back from here is the
            // content, and Control::calculate is what adds the padding and applies the maximum to
            // the sum. The two are compared on the same terms.
            //
            // ANSWERED AGAINST A MAXIMUM, SO A PASS THAT STATES NONE ANSWERS NOTHING - the bars
            // stay as they stand. A form measuring what to ASK a placement for is measured against
            // no maximum by design: that pass is where it finds out what it wants. Re-deciding
            // there would take down the bar the pass before it put up, every time - the ask says
            // "no bar, and this is my height", the placement cuts that height, the pass laying the
            // content into the cut window puts the bar back, and the next ask takes it down again.
            // The two passes would disagree for as long as they were asked. See
            // FormBase::initPlacement, which runs them in turn.
            //
            // A box inside another reads the slot the last align found it in, and the maximum
            // answers only until there is one - see settleAutoVerticalBar.
            bool vNeeded = autoVerticalBarNeeded(event, result.y);
            bool hNeeded = autoHorizontalBarNeeded(event, result.x);
            if (vNeeded != m_vScrollBar.visible() || hNeeded != m_bottomBar.visible())
            {
                result = remeasureWithBars(event, vNeeded, hNeeded);
                // A BAR TAKES ITS STRIP FROM THE OTHER AXIS, so what fitted beside no bar is read
                // again beside the one that came up - and only for a bar to come up. See Controls
                vNeeded = vNeeded || autoVerticalBarNeeded(event, result.y);
                hNeeded = hNeeded || autoHorizontalBarNeeded(event, result.x);
                if (vNeeded != m_vScrollBar.visible() || hNeeded != m_bottomBar.visible())
                    result = remeasureWithBars(event, vNeeded, hNeeded);
            }
        }
        stateSizeGivenWay(event);
        return result;
    }

    void ScrollBox::stateSizeGivenWay(AlignEvent& event) const
    {
        // WHAT IS LEFT WHEN THE SCROLLED BODY GIVES UP ITS SIZE. A box that scrolls can be made
        // as small as the parts of it that do not scroll - its bars, and a strip held above them -
        // and still show everything it holds, because what falls past the edge is reached by
        // scrolling. That is the whole of how a menu says it would rather be short than stand
        // over the button it dropped from: nobody asks it, it states it. See
        // Control::calculatedMinSize.
        //
        // The panel's arrangement is composed again with the body giving way, rather than the
        // body's size being taken off what the panel came to, so a side bar taller than the body
        // states its own height here.
        ScaledDimensions bodyMin = bodyMinSize();
        if (givesWay(ScrollAxis::Vertical))
            bodyMin.y = 0.0f;
        if (givesWay(ScrollAxis::Horizontal))
            bodyMin.x = 0.0f;
        event.calculatedMinSize = composeMinSize(event, bodyMin);
    }

    void ScrollBox::alignContent(AlignEvent& event, ScaledPosition position, ScaledDimensions& contentSize)
    {
        PanelBase::alignContent(event, position, contentSize);

        if (Control* control = body())
        {
            // hScroll
            {
                ScrollInfo scrollInfo{
                    .page = bodyRect().width(),
                    .max = control->width()
                };
                m_hScrollBar.setScrollInfo(scrollInfo);
            }

            // vScroll
            m_vScrollBar.setScrollInfo({
                    .page = bodyRect().height(),
                    .max = control->height()
                });

            offsetControl(control, { -m_hScrollBar.position(), -m_vScrollBar.position() });
        }
        settleAutoVerticalBar(event);
    }

    bool ScrollBox::controlIsOnScrollBox(Control& control)
    {
        return &control == body();
    }

    bool ScrollBox::scrollsChild(const Control& child, const ScrollAxis axis) const
    {
        return &child == body() && givesWay(axis);
    }

    // A body that WRAPS is broken at the width the align pass grants it, and that width is the
    // viewport - so the viewport is a maximum the box states here rather than something the body
    // has to find out by being laid out. Whatever bars are up: a wrapping body is never wider
    // than its slot, so a horizontal bar over one has nothing to carry the view across and no say
    // in what the body is measured against. An unstated maximum reaches the measuring pass as
    // k_maxFloat, and a wrapping body measured against that shapes its whole text at a width no
    // line can reach, which the align pass then discards and pays for again at the real width.
    //
    // Design units, which is what a metric is stated in. Scaler::scale rounds to whole pixels, so
    // a whole-pixel viewport divided by the factor comes back out of the scaling as itself - and
    // the measuring pass and the paint then ask the layout for one width rather than two.
    void ScrollBox::adjustChildMetrics(AdjustMetricsEvent& event) const
    {
        PanelBase::adjustChildMetrics(event);

        if (&event.control != body())
            return;

        // ONLY A BODY THAT WRAPS. A wrapping body is broken at the width the align pass grants
        // it, which is the slot, so the viewport is the one width it is ever shaped at. A body
        // that does not wrap is as wide as its longest line whatever it is offered, and that
        // width is the range the horizontal bar carries the view across: capped here, the body
        // comes out the width of the slot the PREVIOUS pass laid out, so a drag inward hands
        // the bar one mouse move of travel, a drag outward hands it none, and the lines past
        // the edge are clipped with nothing to reach them.
        if (!event.control.wordWrap())
            return;

        // ONLY WHERE THIS BOX'S WIDTH IS GIVEN FROM OUTSIDE, ASKED ALL THE WAY UP. The viewport is
        // this box's width less its bars, so on a box measured FROM its body the viewport is
        // derived from the very thing it would bound - and that is a ratchet: whatever narrows the
        // body once narrows the box, narrows the viewport, and caps the body there for good. A tab
        // strip that wrapped one label while the DPI changed stayed wrapped, because a wrapped
        // label is narrower and that width became the ceiling. A menu's list lost a scrollbar's
        // width the same way. One level is not enough: a box in a slot of a window placed around
        // its content is still measured from its body, and a section collapsed there would hold
        // the options at the collapsed width once it opened again. See
        // Control::isWidthGivenFromOutside
        //
        // Where the width IS given - a page filling a window the user sized, a body in a slot of
        // one - the viewport is an independent number and stating it is what keeps a wrapping
        // body from shaping its whole text at a width no line can reach.
        if (!isWidthGivenFromOutside())
            return;

        // NOT WHILE THE FORM IS ASKING FOR A WINDOW. That pass is where a form finds out what it
        // wants, and a body held to the viewport of a window it is about to replace can only ask
        // for that window again - see FormBase::isMeasuringPlacement. The viewport it would be
        // held to is not even a viewport it ever showed: a form handed a scale is laid out once
        // inside the window it still has, and the body squeezed by that pass is what the writer
        // records. bodySlotSettled asks the same question, and both sides have to.
        const FormBase* hostForm = getForm();
        if (hostForm && hostForm->isMeasuringPlacement())
            return;

        // Before the box has been laid out once there is no viewport to state, and the body
        // measures against the maximum it states for itself.
        if (m_viewportWidthInDesign <= 0.0f)
            return;

        // A METRIC IS IN DESIGN UNITS, AND SO IS THIS ONE ALREADY. The viewport it came from is
        // scaled pixels measured at the scale of the pass that laid it out, which is not always
        // the scale in force now: converting it here, with whatever factor is current, reads the
        // two at different scales. What that costs is a latch. Take a form from 250% to 100% and
        // back: the viewport measured at 100% divided by 2.5 caps the body at a fifth of the
        // width it needs, the body is what the box is measured from, so the box comes back that
        // narrow and measures the same viewport again. It never recovers, and every label in it
        // stays cut.
        event.metrics.maxSize.x = m_viewportWidthInDesign;
    }

    // WRITTEN BEFORE THE BODY IS LAID OUT INTO THE SLOT, which is what makes it this pass's
    // viewport rather than the last one's. The body reads it through adjustChildMetrics, and that
    // runs inside the very alignControl call PanelBase makes on the line after this one - see
    // PanelBase::bodySlotSettled. A body that FILLS is therefore right on the pass that resized
    // it: the align hands it the slot, and the slot is what it now breaks its lanes at.
    //
    // AND THE BOX STILL ASKS FOR ONE MORE PASS, because a body that does NOT fill keeps the width
    // it MEASURED - see Control::align - and the measure ran before this line, against the
    // viewport of the pass before. There is no way round that ordering: the viewport is this box's
    // width less its bars, and this box has no width until its own parent lays it out. So the
    // width the align hands over reaches a filling body immediately and a non-filling one on the
    // pass after; the pass after that hands back the same width, which is what ends it. Maximizing
    // with the body aligned Left was where the miss showed - the rows stayed broken at the width
    // the small window had, with the rest of the screen empty beside them.
    //
    // The scale that measured it is the scale in force here, which is why the conversion belongs
    // here and not in adjustChildMetrics.
    void ScrollBox::bodySlotSettled(AlignEvent& event)
    {
        // NOT WHILE THE FORM IS ASKING FOR A WINDOW. That pass lays the box out at the content's
        // own size and not into a window, so its slot is no viewport - see
        // FormBase::isMeasuringPlacement.
        const FormBase* hostForm = getForm();
        if (hostForm && hostForm->isMeasuringPlacement())
            return;
        const float previous = m_viewportWidthInDesign;
        m_viewportWidthInDesign = bodyRect().width() / event.scaleFactor();
        // Nothing is owed for the first viewport: with none to state, the body measured against
        // its own maximum and the align pass broke it at the width it was granted.
        if (previous <= 0.0f)
            return;
        if (std::abs(m_viewportWidthInDesign - previous) < k_viewportEpsilon)
            return;
        // Only where the body READS the maximum - the same three questions adjustChildMetrics
        // asks before stating it. Where it is not stated, nothing was measured against it.
        const Control* content = body();
        if (!content || !content->wordWrap() || !isWidthGivenFromOutside())
            return;
        event.invalidatePass();
    }

    // The body is laid out to its whole content and carried under the slot by the scroll
    // position, so its own rect says where the content has reached, not where it shows. The
    // viewport is the slot itself: it holds still while the body travels, and it starts below a
    // top bar because PanelBase::alignContent puts the slot there.
    //
    // Stated outright rather than narrowed out of the rect that arrives, which is the body's
    // bounds already clipped into this box: a body scrolled past the top of the slot has that
    // clip standing at the box's own top edge, and the slot's top is no longer recoverable from
    // it. This is still a narrowing - the slot lies inside the body and inside the box alike -
    // so the walk's clip into the ancestors above still holds.
    void ScrollBox::adjustChildViewport(AdjustViewportEvent& event) const
    {
        if (&event.control == body())
            event.viewport = bodySlotInForm(event.control.parentContentOrigin());
    }

    // The slot again, in the coordinates the body's own topLeft is measured in, which is what
    // bodyRect already is. The body is as tall as its whole content and the scroll carries it
    // under the slot, so its bounds cover the bars this box lays out around the slot; without
    // this a press on one of them lands on the body instead.
    void ScrollBox::adjustChildClip(const Control& child, FloatRect& clip) const
    {
        if (&child == body())
            clip.intersectWith(bodyRect());
    }

    // The bottom bar rather than the horizontal scroll bar inside it: what stands behind a bar
    // stands behind the corner square beside it too.
    void ScrollBox::adjustChildPaint(AdjustPaintEvent& event)
    {
        PanelBase::adjustChildPaint(event);
        if (!m_barSurface)
            return;
        if (&event.control() == &m_vScrollBar || &event.control() == &m_bottomBar)
            event.setSurfaceHsl(*m_barSurface);
    }

    // The base is what paints the child - see Control::paintChildSurface - so it runs whatever
    // this reads off the event.
    void ScrollBox::paintChildSurface(PaintEvent& event)
    {
        if (&event.control() == body())
            m_actualBodyColor = event.surfaceRgb().withOpacity(1.0f);
        PanelBase::paintChildSurface(event);
    }

    void ScrollBox::childPainted(PaintEvent& event)
    {
        if (&event.control() != body()) return;

        FloatRect viewPort = event.viewport();
        // The corners the body paints, so a fade ends on the same arc the body does.
        CornerRadii corners = event.cornerRadii();
        // A fade says the content carries on past this edge, so it is drawn where the content
        // actually leaves the view. Something held against the scroll at the top - a grid keeping
        // its header there - moves that place to its own inner edge: it is what the rows pass
        // under, and it is opaque, so a fade laid over it would fade the strip and leave the rows
        // arriving beneath it with a hard edge. The sides start below it for the same reason, and
        // the top corners are then the strip's, not the view's.
        if (event.pinnedTop() > 0.0f)
        {
            viewPort.top += event.pinnedTop();
            corners[cornerIndex(Corner::TopLeft)] = 0.0f;
            corners[cornerIndex(Corner::TopRight)] = 0.0f;
        }
        const Color baseColor = event.surfaceRgb().withOpacity(1.0f);
        const float fadeSize = event.scaleF(16.0f);
        if (fadeSize <= 0.0f) return;

        // Zero-overhead lambda to draw a dynamically scaled fade on any side
        auto drawFade = [&](RectSide side, float currentOffset) {
            if (currentOffset <= 0.0f) return;

            const float intensity = std::min(currentOffset / fadeSize, 1.0f);
            event.canvas().fadeEdge(viewPort, corners, side, fadeSize, baseColor.withOpacity(intensity));
            };

        if (m_vScrollBar.visible())
        {
            const float y = m_vScrollBar.position();
            const float maxY = m_vScrollBar.maxPosition();

            drawFade(RectSide::Top, y);
            drawFade(RectSide::Bottom, maxY - y);
        }

        if (m_hScrollBar.visible())
        {
            float x = m_hScrollBar.position();
            float maxX = m_hScrollBar.maxPosition();

            drawFade(RectSide::Left, x);
            drawFade(RectSide::Right, maxX - x);
        }
    }

    // MEASURED WHERE EVERYTHING COMES TO REST, NOT WHERE IT STANDS. A glide already in flight
    // is going to carry the item by the travel still to come, and the bar is going to end at
    // its target, so the overlaps are taken against where the item will be and the new position
    // is named from where the bar is heading. A run of requests then accumulates - each asks
    // for its own distance on top of the one before it, rather than for ground a glide is still
    // covering. With nothing in flight the target IS the position and this is the layout as it
    // stands, so the two readings are one.
    //
    // A caller measuring a distance on screen for itself has to read the same view - see
    // Control::viewTravelRemaining - or it names a rect against the view in flight and this
    // scrolls back to it.
    //
    // A glide explains a change to a view the user is already looking at. A box short of its
    // first paint has no such change to explain and nothing on screen to travel from, so it
    // arrives at its position instead - a page being built lands where it belongs.
    void ScrollBox::scrollChildIntoView(Control& item, FloatRect itemRect)
    {
        // A panel's body slot can be empty - every other reader here tests it - and this line was
        // reaching through it. The answer it got was right by accident: containsNested opened
        // with a null-this test, which the language does not define and clang deletes. Asked
        // properly, a box with no body scrolls the same way a box whose body does not hold the
        // item does, which is what the base class is for.
        const Control* content = body();
        if (!content || !content->containsNested(item, CheckSelf::Yes))
            return PanelBase::scrollChildIntoView(item, itemRect);

        // The pointer has not moved the content here, so the mouse-down anchor does not follow
        // it: this scroll answers a control asking to be seen, not a drag or a wheel.
        m_scrolledInDragOrByWheel = false;

        const bool glide = isPainted() && visible();
        auto sendTo = [glide](ScrollBar& bar, float newPosition) {
            if (glide)
                bar.animatePosition(newPosition);
            else
                bar.setPosition(newPosition);
        };

        FloatRect clRect = bodyRect();
        // we won't get here if there's no client, right?
        // IntPoint margin = client()->scaledPadding();
        FloatPoint margin{ form().scaler().scaled8 };
        clRect.inflate(-margin);

        // The client rect is fixed to the box, and the content is what travels: the item is
        // carried by what is left of the glide, so this is where it comes to rest.
        itemRect.offset(-scrollTravelRemaining());

        // Temporary, for the scroll-into-view defect.
        if constexpr (Diagnostic::Options::logScrollIntoView)
        {
            const FloatPoint travel{ scrollTravelRemaining() };
            diagnosticLog(std::format(
                L"intoView item {:.0f} {:.0f} {:.0f} {:.0f}  client {:.0f} {:.0f} {:.0f} {:.0f}"
                L"  lrtb {:.0f} {:.0f} {:.0f} {:.0f}  travel {:.0f} {:.0f}"
                L"  h {:.0f} -> {:.0f}  v {:.0f} -> {:.0f}",
                itemRect.left, itemRect.top, itemRect.right, itemRect.bottom,
                clRect.left, clRect.top, clRect.right, clRect.bottom,
                clRect.left - itemRect.left, itemRect.right - clRect.right,
                clRect.top - itemRect.top, itemRect.bottom - clRect.bottom,
                travel.x, travel.y,
                m_hScrollBar.position(), m_hScrollBar.positionTarget(),
                m_vScrollBar.position(), m_vScrollBar.positionTarget()),
                &item);
        }

        if (m_vScrollBar.visible())
        {
            float topOverlap = clRect.top - itemRect.top;
            float bottomOverlap = itemRect.bottom - clRect.bottom;
            if (bottomOverlap < 0.0f and topOverlap > 0.0f)
                sendTo(m_vScrollBar, m_vScrollBar.positionTarget() - topOverlap);
            else if (topOverlap < 0.0f and bottomOverlap > 0.0f)
                sendTo(m_vScrollBar, m_vScrollBar.positionTarget() + std::min(-topOverlap, bottomOverlap));
        }
        if (m_hScrollBar.visible())
        {
            float leftOverlap = clRect.left - itemRect.left;
            float rigthOverlap = itemRect.right - clRect.right;
            if (rigthOverlap < 0.0f and leftOverlap > 0.0f)
                sendTo(m_hScrollBar, m_hScrollBar.positionTarget() - leftOverlap);
            else if (leftOverlap < 0.0f and rigthOverlap > 0.0f)
                sendTo(m_hScrollBar, m_hScrollBar.positionTarget() + std::min(-leftOverlap, rigthOverlap));
        }
        // alternatively, we may correct the itemRect here and pass it to the inherited scrollControlIntoView()
        scrollIntoView();
    }

    void ScrollBox::mouseWheel(MouseWheelEvent& event)
    {
        if (m_vScrollBar.visible())
            return wheelScrolled(event, m_vScrollBar, -event.delta);
        if (m_hScrollBar.visible())
            return wheelScrolled(event, m_hScrollBar, -event.delta);
    }

    void ScrollBox::mouseHWheel(MouseWheelEvent& event)
    {
        if (m_hScrollBar.visible())
            return wheelScrolled(event, m_hScrollBar, event.delta);
    }

    // Answered for a drag that started in the body, where the pointer is carrying the content
    // and leaving the view is a request for more of it. A drag that started on a bar travels up
    // through this box on its way out and asks nothing of it: the caption drags the window, and
    // the system's move loop reports the pointer here for as long as it lasts.
    void ScrollBox::drag(DragEvent& event)
    {
        const Control* content = body();
        if (!content || !content->containsNested(event.control(), CheckSelf::Yes))
            return;
        constexpr float autoscrollDragThreshhold = 16.0f;
        if (event.unscaledDistance() < autoscrollDragThreshhold)
            return;

        m_autoScrollZone = event.form().scaler().scale(k_autoScrollZone);

        FloatRect bounds = viewPort(event.form());

        // if the rect is too small making sure that scroll zones do not intersect each other
        float enlargeX = std::max(m_autoScrollZone - bounds.width() / 2.0f, 0.0f);
        float enlargeY = std::max(m_autoScrollZone - bounds.height() / 2.0f, 0.0f);
        bounds.inflate(enlargeX, enlargeY);

        m_inDrag = true;
        if (m_vScrollBar.visible())
        {
            startOrStopAutoScrolling(m_vScrollBar.endButton(), event.currentPos().y, bounds.bottom);
            startOrStopAutoScrolling(m_vScrollBar.beginButton(), event.currentPos().y, bounds.top);
        }
        if (m_hScrollBar.visible())
        {
            startOrStopAutoScrolling(m_hScrollBar.endButton(), event.currentPos().x, bounds.right);
            startOrStopAutoScrolling(m_hScrollBar.beginButton(), event.currentPos().x, bounds.left);
        }
        m_inDrag = false;
    }

    void ScrollBox::pressDown(PressDownEvent&)
    {
        m_scrolledInDragOrByWheel = false;
    }

    void ScrollBox::pressUp(PressUpEvent& event)
    {
        if (m_scrolledInDragOrByWheel)
        {
            event.handled.propagationStopped = true;
            event.handled.preventClick = true;
        }

        if (m_vScrollBar.visible())
        {
            m_vScrollBar.endButton().stopAutoScroll();
            m_vScrollBar.beginButton().stopAutoScroll();
        }
        if (m_hScrollBar.visible())
        {
            m_hScrollBar.endButton().stopAutoScroll();
            m_hScrollBar.beginButton().stopAutoScroll();
        }
        // after stopping the timers!
        m_scrolledInDragOrByWheel = false;
    }

    // The body slot in form coordinates: the window the body is seen through, and the one
    // answer behind every reader of it. bodyRect is measured from this box's CONTENT origin -
    // the point a child's topLeft is measured from - so the origin handed in is that point and
    // not the box's bounds, which sit a padding outside it.
    FloatRect ScrollBox::bodySlotInForm(FloatPoint contentOriginInForm) const
    {
        return FloatRect::fromDimensions(contentOriginInForm + bodyRect().topLeft(), bodyRect().dimensions());
    }

    bool ScrollBox::givesWay(const ScrollAxis axis) const
    {
        if (m_scrollBars == ScrollBars::Both || m_scrollBars == ScrollBars::Auto)
            return true;
        return axis == ScrollAxis::Vertical
            ? m_scrollBars == ScrollBars::Vertical
            : m_scrollBars == ScrollBars::Horizontal;
    }

    void ScrollBox::showBars(bool vVisible, bool hVisible)
    {
        m_vScrollBar.setVisible(vVisible);
        m_bottomBar.setVisible(hVisible);
        m_corner.setVisible(vVisible && hVisible);
        invalidate();
    }

    bool ScrollBox::autoVerticalBarNeeded(const AlignEvent& event,
        const float contentHeight) const
    {
        if (m_bodyOverranSlot)
            return *m_bodyOverranSlot;
        if (event.maxSize.y != k_maxFloat)
            return contentHeight > event.maxContentHeight();
        return m_vScrollBar.visible();
    }

    bool ScrollBox::autoHorizontalBarNeeded(const AlignEvent& event,
        const float contentWidth) const
    {
        if (event.maxSize.x != k_maxFloat)
            return contentWidth > event.maxContentWidth();
        return m_bottomBar.visible();
    }

    ScaledDimensions ScrollBox::remeasureWithBars(AlignEvent& event, const bool vVisible,
        const bool hVisible)
    {
        showBars(vVisible, hVisible);
        // A hidden bar was never measured at all - calculateChildren passes over what is not
        // visible - and the room one takes is part of what the box comes out as.
        calculateChildren(form());
        return PanelBase::calculateContent(event);
    }

    // THE SLOT A BOX INSIDE ANOTHER GOT is the one fact its maximum cannot state: a window cut
    // short by its placement lowers the root's ceiling and no one else's. See Controls
    void ScrollBox::settleAutoVerticalBar(AlignEvent& event)
    {
        if (m_scrollBars != ScrollBars::Auto)
            return;
        // A root is measured against its window already - see FormBase::adjustRootMetrics.
        if (!parent())
            return;
        // NOT WHILE THE FORM IS ASKING FOR A WINDOW. That pass lays the box out at the content's
        // own size, so its slot is no viewport - see FormBase::isMeasuringPlacement.
        const FormBase* hostForm = getForm();
        if (hostForm && hostForm->isMeasuringPlacement())
            return;
        const Control* content = body();
        const bool overran = content
            && content->visible()
            && content->height() > bodyRect().height() + k_overrunTolerance;
        m_bodyOverranSlot = overran;
        // The pass this asks for stands the bar the answer names, and the body it lays out gives
        // the same answer: a body laid out narrower is never shorter, nor a wider one taller.
        if (overran != m_vScrollBar.visible())
            event.invalidatePass();
    }

    // A notch names a place to send the view to, so the content travels there and is watched on
    // the way. The steps are measured from where the position is heading rather than from where
    // it has reached, so notches arriving faster than a glide lasts add up into one longer
    // journey instead of each one starting the distance over.
    //
    // The event is answered only where a bar takes it: a wheel over a box that scrolls in
    // neither direction belongs to whatever encloses that box.
    void ScrollBox::wheelScrolled(MouseWheelEvent& event, ScrollBar& bar, float steps)
    {
        event.handled = true;
        // The pointer holds still while the content moves under it, so the hover and the
        // mouse-down anchor are carried along with the content for the length of the glide.
        m_scrolledInDragOrByWheel = true;
        bar.animatePositionBySteps(steps);
    }

    void ScrollBox::startOrStopAutoScrolling(SliderBase::ScrollButton& button, float current, float boundary)
    {
        float sign = button.direction() == ScrollDirection::ToBegin ? -1.0f : 1.0f;
        float spot = boundary - sign * m_autoScrollZone;
        float distance = sign * (current - spot);
        if (distance > 0.0f)
        {
            float speed = std::min(1.0f + distance / m_autoScrollZone, k_maxAutoScrollSpeed);
            m_scrolledInDragOrByWheel = true;
            button.startAutoScroll(speed);
        }
        else
            button.stopAutoScroll();
    }

    // The mouse-down anchor and the hover follow the content whenever the content travelled
    // without the pointer carrying it there - a wheel glide, a drag autoscroll. A bar held by
    // the pointer is the one case they do not: the content is moving because the pointer is
    // moving, so an anchor following it would add the content's travel to the pointer's own and
    // every move would name a larger one. The repeat is skipped with it, which also keeps a
    // thumb drag from re-entering itself through the mouse move it would ask for.
    void ScrollBox::bodyScrolled(FloatPoint change)
    {
        // Said before the two answers below, which are about the POINTER and not about the
        // content: whatever else a scroll does or does not do, the content has moved, and
        // anything holding a picture of where it was has to be told. See FormBase::contentMoved.
        form().contentMoved(*body());

        if (!m_scrolledInDragOrByWheel)
            return;
        if (m_vScrollBar.positionHeldByPointer() || m_hScrollBar.positionHeldByPointer())
            return;
        form().offsetMouseDownPos(-change);
        if (!m_inDrag)
            form().mouseTick();
    }



    //----------------------------------------------------------------------------


    export template <IsControl BodyType>
        // A scroll box whose body is a control of the named type.
        class ScrollBoxWith : public WithBody<ScrollBox, BodyType>
    {
    public:
        using WithBody<ScrollBox, BodyType>::WithBody;
    };


}
