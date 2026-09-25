module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.TabbedBox;

import ClaFi.Controls.Panel;
import ClaFi.Controls.TabStrip;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.PageControl;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;
import ClaFi.Core.Foundation;

namespace ClaFi::Controls
{
    // A strip of tabs over a body that follows the open one.
    export class TabbedBox : public PanelBase
    {
    public:
        template<typename... Args>
        explicit TabbedBox(const CreateParams&, Args&&...);
    public:
        // Which edge of the box the tabs run along.
        DECLARE_PROPERTY(TabsOrientation, tabsOrientation, TabsOrientation::VerticalLeft)
        // How heavy the line the open tab draws along the strip is.
        DECLARE_PROPERTY(TabLineThickness, tabLineThickness, Thickness::Thin)
    public:
        TabStripBase& strip() const { return m_strip; }
        PageControl& pageControl() const { return m_pageControl; }
    protected:
        void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&) override;
        void adjustChildPaint(AdjustPaintEvent&) override;
        void paintChildSurface(PaintEvent&) override;
        void paintChildren(PaintEvent&) override;
    private:
        class TabStrip2 : public TabStripBase
        {
        public:
            using TabStripBase::TabStripBase;
        public:
            Thickness pageBorder() const override;
        protected:
            RichControl* visualPage(RichControl* page) override;
        private:
            const TabbedBox& ownerBox() const;
        };
        // What the page came out as in the frame being painted. The run beside it is drawn from
        // these, and paintChildren paints the page first so they are this frame's answers.
        struct PageSurface
        {
            FloatRect bounds{};
            Color surface{};
            Color stroke{};
            Hsl hsl{};
            float strokeWidth{};
        };
        using PanelBase::createLeftBar;
        using PanelBase::createBody;
    private:
        // Only initialization by pass TabsOrientation to constructor allowed for now
        void setTabsOrientation(TabsOrientation);
        Thickness pageBorder() const;
        // Where the open tab draws its line: the strip's content right edge, which is the origin
        // Tab::prepareGeometry measures its silhouette from.
        [[nodiscard]] float tabLineX() const;
        void paintPageRun(PaintEvent&);
        template <typename... Args>
        TabStripBase& createTabStrip(Args&&... args);
    private:
        // The run is open at both ends, so its top and bottom carry on into the page's own border
        // instead of closing a shape of their own.
        static constexpr RectSidesBoolArray k_runSides{ true, false, true, false };
    private:
        PageSurface m_pageSurface{};
        // The box the strip is the body of, where there is one. Declared before m_strip, which is
        // what createTabStrip builds it for.
        ScrollBox* m_stripBox{ nullptr };
        TabStripBase& m_strip{ createTabStrip(
            m_tabsOrientation,
            m_tabLineThickness
        ) };
        PageControl& m_pageControl = createBody<PageControl>(
            UiElement::Page,
            themeMetrics().page
        );
    };


    //----------------------------------------------------------------------------


    // TabbedBox::TabStrip2

    Thickness TabbedBox::TabStrip2::pageBorder() const
    {
        return ownerBox().pageBorder();
    }

    RichControl* TabbedBox::TabStrip2::visualPage(RichControl* page)
    {
        return static_cast<PageControl*>(page->parent()); // our m_pageControl
    }

    const TabbedBox& TabbedBox::TabStrip2::ownerBox() const
    {
        switch (tabsOrientation())
        {
        case TabsOrientation::HorizontalTop:
        case TabsOrientation::HorizontalBottom:
            return *static_cast<TabbedBox*>(parent());
        case TabsOrientation::VerticalLeft:
            return parentAs<TabStripBase>().parentAs<TabbedBox>();
        default: unreachable();
        }
    }

    // TabbedBox

    template<typename ...Args>
    TabbedBox::TabbedBox(const CreateParams& params, Args&&... args)
        :
        PanelBase{ params, std::forward<Args>(args)..., Spacing{0.f, 0.f} },
        INIT_PROPERTY(tabsOrientation),
        INIT_PROPERTY(tabLineThickness)
    {
        m_strip.setOverlayHost(*this);
    }

    void TabbedBox::alignContent(AlignEvent& alignEvent, ScaledPosition scaledPosition,
                                 ScaledDimensions& scaledDimensions)
    {
        PanelBase::alignContent(alignEvent, scaledPosition, scaledDimensions);

        // A horizontal strip closes the seam by moving its bar a border's width into the page:
        // the tab line then lands on the page's own border, and the open tab's fill covers the
        // stretch of it under the tab.
        // TODO: should it expand the visible page rect on paint instead, the way VerticalLeft
        // does in paintPageRun?
        const float seam = alignEvent.scaledStrokeWidth(pageBorder());
        switch (m_tabsOrientation)
        {
        case TabsOrientation::HorizontalTop:
            offsetControl(topBar(), { 0.0f, seam });
            break;
        case TabsOrientation::HorizontalBottom:
            offsetControl(bottomBar(), { 0.0f, -seam });
            break;
        case TabsOrientation::VerticalLeft:
        case TabsOrientation::VerticalRight:
            break;
        }
    }

    void TabbedBox::adjustChildPaint(AdjustPaintEvent& event)
    {
        PanelBase::adjustChildPaint(event);
        if (m_tabsOrientation != TabsOrientation::VerticalLeft)
            return;
        // The page's left corners are square: the tab line stands along that whole edge, and a
        // round corner would leave the two meeting at a curve neither of them draws.
        if (&event.control() == body())
        {
            event.setCornerRadius(Corner::TopLeft, 0.0f);
            event.setCornerRadius(Corner::BottomLeft, 0.0f);
        }
    }

    void TabbedBox::paintChildSurface(PaintEvent& event)
    {
        if (&event.control() == body())
        {
            m_pageSurface = {
                .bounds = event.controlBounds(),
                .surface = event.surfaceRgb().withOpacity(1.0f),
                .stroke = event.strokeRgb(),
                .hsl = event.surfaceHsl(),
                .strokeWidth = event.borderWidth()
            };
        }
        PanelBase::paintChildSurface(event);
    }

    // The page goes first, so the run beside it is filled with the colour the page has just
    // settled on rather than with the frame before's. Nothing is covered by taking it out of
    // order: the run ends where the page begins, and the strip's box has no surface of its own.
    void TabbedBox::paintChildren(PaintEvent& event)
    {
        if (m_tabsOrientation != TabsOrientation::VerticalLeft)
            return PanelBase::paintChildren(event);

        event.paintChild(m_pageControl);
        // Every stage paints the children, because an overlay control is reached through them,
        // and the run belongs to the stage that draws content. See PaintEvent::overlayStage.
        if (!event.overlayStage())
            paintPageRun(event);
        // Said before the box is painted, so its bars are resolved against the run they now stand
        // on rather than against the surface this box stands on.
        m_stripBox->setBarSurface(m_pageSurface.hsl);
        for (ControlPtr& control : controls())
        {
            if (&*control != &m_pageControl && control->visible())
                event.paintChild(*control);
        }
    }

    void TabbedBox::setTabsOrientation(TabsOrientation value)
    {
        if (m_tabsOrientation == value)
            return;
        m_tabsOrientation = value;
        m_strip.setTabsOrientation(value);
        ;   }

    Thickness TabbedBox::pageBorder() const
    {
        return static_cast<const RichControl*>(body())->border();
    }

    float TabbedBox::tabLineX() const
    {
        return form().rectOfControl(&m_strip).right - m_strip.scaledPadding().x;
    }

    // The page's surface carried back across the strip's scroll bar, so the bar stands on the page
    // and the page reaches the tabs. It runs to the far side of the page's own left border: the
    // tab line is drawn on that edge, and one edge carries one border.
    void TabbedBox::paintPageRun(PaintEvent& event)
    {
        if (!m_pageSurface.surface.alpha)
            return;

        RoundedRectangleParts run{
            .bounds = m_pageSurface.bounds,
            .radii = k_squareCorners,
            .sides = k_allRectSidesTrue
        };
        run.bounds.left = tabLineX();
        run.bounds.right = m_pageSurface.bounds.left + m_pageSurface.strokeWidth;
        if (run.bounds.right <= run.bounds.left)
            return;

        Graphics::Canvas& canvas = event.canvas();
        canvas.fillPartialRoundedRectangle(run, m_pageSurface.surface);
        if (!m_pageSurface.stroke.alpha || !m_pageSurface.strokeWidth)
            return;
        run.sides = k_runSides;
        canvas.drawPartialRoundedRectangle(run, m_pageSurface.stroke, m_pageSurface.strokeWidth);
    }

    template <typename ... Args>
    TabStripBase& TabbedBox::createTabStrip(Args&&... args)
    {
        switch (m_tabsOrientation)
        {
        case TabsOrientation::HorizontalTop:
            return createTopBar<TabStrip2>(
                Padding{ 12.f, 0.f },
                HorizontalAlign::Left,
                std::forward<Args>(args)...
            );
        case TabsOrientation::HorizontalBottom:
            return createBottomBar<TabStrip2>(
                Padding{ 12.f, 0.f },
                HorizontalAlign::Right,
                std::forward<Args>(args)...
            );
        case TabsOrientation::VerticalLeft:
        {
            // No surface of its own. The column its scroll bar stands in is the page's, laid
            // down by paintPageRun before this box paints, and a fill here would cover it.
            ScrollBox& scrollBox{ createLeftBar<ScrollBox>(
                    ScrollBars::Vertical,
                    Padding{ 0.0f, 8.0f })
            };
            m_stripBox = &scrollBox;
            return scrollBox.createBody<TabStrip2>(
                Padding{ 0.f, 12.f },
                std::forward<Args>(args)...
            );
        }
        case TabsOrientation::VerticalRight:
            break;
        }
        unreachable();
    }

}
