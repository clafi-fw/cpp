export module ClaFi.Controls.ScrollBar;

import ClaFi.Controls.Base.SliderBase;
import ClaFi.Icons.ScrollMark;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.Core.Foundation;
import ClaFi.StdLib;

namespace ClaFi::Controls
{

    // The bar a scroll box is moved by.
    export class ScrollBar : public SliderBase
    {
    public:
        // Not the inherited constructors: the role has to be set, and it has to be set for every
        // scroll bar rather than at each creation site. A scroll bar is hovered whenever the
        // pointer is on it or on one of its parts, which is the whole of what MouseOnly claims -
        // it stays out of the focus chain, unlike Slider, which is Focusable.
        template<typename... Args>
        explicit ScrollBar(const CreateParams& params, Args&&... args)
            :
            SliderBase{ params, SliderViewMode::ScrollBar, Spacing{ 0.0f },
                // UiElement::Section, looks bad in tabbed box
                std::forward<Args>(args)... }
        {
            setInteractivity(Interactivity::MouseOnly);
        }
    public:
        std::wstring_view diagnosticText() const override { return L"ScrollBar"; }
        void setScrollInfo(const ScrollInfo&);
        float maxPosition() const { return m_scrollInfo.maxPos(); }
    protected:
        ScrollInfo controlScrollInfo() const override;
        float stepSize() override { return form().scaler().scaled20; }
        float buttonSize(const AppTheme&) override;
        void adjustButtonMetrics(AdjustMetricsEvent& event) const override { event.metrics = event.themeMetrics().scrollButton; }
        void adjustButtonPaint(AdjustPaintEvent&) override;
        [[nodiscard]] FloatPoint buttonInset() const override { return { 0.0f, 0.0f }; }
        void adjustThumbMetrics(AdjustMetricsEvent& event) const override { event.metrics = event.themeMetrics().scrollThumb; }
        void adjustThumbPaint(AdjustPaintEvent&) override;
        void adjustPaint(AdjustPaintEvent& event) override { SliderBase::adjustPaint(event); event.setDisabledBlendAmount(0.0f); }

        void paintButtonMark(PaintEvent&, float size, ScrollDirection) override;
        void paintThumb(PaintEvent& pp) override;
    private:
        ScrollInfo m_scrollInfo{};
    };


    //----------------------------------------------------------------------------


    // ScrollBar

    // A page of a new size is the viewport itself changing, and the position follows it in the
    // same pass: a window being dragged to a new size would otherwise trail its own content by
    // the length of a glide. A page that has kept its size under a smaller max is content that
    // has gone from under the view, and the gap that leaves is closed by gliding.
    void ScrollBar::setScrollInfo(const ScrollInfo& value)
    {
        bool pageResized = !sameFactors(m_scrollInfo.page, value.page);
        m_scrollInfo = value;
        m_scrollInfo.max = std::max(m_scrollInfo.max, m_scrollInfo.page);

        if (pageResized)
            setPosition(position(), false);
        else
            revalidatePosition();
        alignThumb();
        // A paint event multiplies its control's enabled factor by its parent's, so a bar
        // reading disabled paints its thumb and its buttons away whatever their own states
        // say. Whether there is anything to scroll is this bar's own answer - see
        // SliderBase::getControlState - and the range arriving here is what decides it, so
        // the bar's own state is invalidated alongside its parts'.
        invalidateState();
        invalidateChildrenStates();
    }

    ScrollInfo ScrollBar::controlScrollInfo() const
    {
        return m_scrollInfo;
    }

    float ScrollBar::buttonSize(const AppTheme& theme)
    {
        switch (axis())
        {
        case ScrollAxis::Vertical:
            return theme.metrics.scrollButton.minSize.y;
        // case ScrollAxis::horizontal:
        default: // warn fixing
            return theme.metrics.scrollButton.minSize.x;
        }
    }

    void ScrollBar::adjustButtonPaint(AdjustPaintEvent& event)
    {
        event.setColorRules(UiElement::ScrollButton);
        event.setDisabledBlendAmount(1.0f);
    }

    void ScrollBar::adjustThumbPaint(AdjustPaintEvent& event)
    {
        event.setColorRules(UiElement::ScrollThumb);
        event.setDisabledBlendAmount(1.0f);
    }

    void ScrollBar::paintButtonMark(PaintEvent& event, float size, ScrollDirection scrollDirection)
    {
        // Packed the same way ButtonBase packs one for its own icon. It belongs in
        // SliderBase::paintButton, so that the virtual takes the icon event directly and both
        // marks are reached the same way - that move waits on Slider's mark, which still reads
        // controlBounds off the paint event, and PaintIconEvent cannot offer that without
        // depending on Foundation.
        PaintIconEvent iconEvent{
            event.controlContext(),
            event.centerRect(size, size),
            event.control().tag(),
            event.disabledAmount()
        };
        Icons::ScrollMark::paint(iconEvent, axis(), scrollDirection);
    }

    void ScrollBar::paintThumb(PaintEvent& event)
    {
        FloatRect rect = event.controlBounds();
        float margin = event.scaler().scaled3F * (1.0f - hoveredFactor());
        if (axis() == ScrollAxis::Vertical)
        {
            rect.inflate(-event.scaler().scaled1 - margin, 0.0f);
        }
        else
            rect.inflate(0.0f, -event.scaler().scaled1 - margin);
        float radius = event.radius();
        radius = std::min(radius, std::min(rect.width(), rect.height()) / 2.0f);
        event.canvas().fillRoundedRectangle(rect, radius, radius, event.surfaceRgb());
    }
}
