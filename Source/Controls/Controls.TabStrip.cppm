module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.TabStrip;

export import ClaFi.Controls.Base.SplitButtonBase;
import ClaFi.Controls.Base.Container;

import ClaFi.Core.Graphics.TabRenderer;

import ClaFi.Core.AppTheme_Colors;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;

import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.Fit;

import ClaFi.StdLib;
import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Controls.StackPanel;

namespace ClaFi::Controls
{

    export struct Page
    {
        RichControl& value;
    };

    // How heavy the line the open tab draws along the strip is.
    export struct TabLineThickness
    {
        Thickness value;
    };

    export class TabStripBase;

    // A tab is a button that paints a tab silhouette instead of a rounded rect, and whose
    // secondary part - where it has one - is a close button rather than a dropdown strip. Nothing
    // is ever put inside a tab: its caption is text and its icon is painted, so the container it
    // would otherwise be buys it nothing.
    using TabBaseClass = SplitButtonBase;

    // One tab of a strip, standing for what it opens.
    export class Tab : public TabBaseClass, public IFittable
    {
    public:
        template<typename... Args>
        explicit Tab(const CreateParams& params, Args&&... args);
    public:
        std::wstring_view diagnosticText() const override { return L"Tab"; }
        // IFittable
        FitData fitData() const override;
        void takeOffWidth(float value) override;
        //
        void setPage(RichControl* page);
        void setPage(RichControl& page) { setPage(&page); }
        RichControl* page() const { return m_page; }
        TabStripBase& parent() const;
        Tab& select();
    protected:
        virtual RichControl* visualPage(RichControl* page);
        // Pressing a tab makes it the strip's current one, and a part on the tab is there to do
        // something else. Closing a tab must not open it on the way.
        [[nodiscard]] bool secondaryPressPropagates() const override { return false; }
        void adjustPaint(AdjustPaintEvent&) override;
        void paintSurface(PaintEvent&) override;
        void pressDown(PressDownEvent&) override;
        void drag(DragEvent&) override;
    private:
        void tabCreated(Control* parent);
        void prepareGeometry(const PaintEvent&, float visibilityFactor, PaintEvent*& outEvent);
        // How far the line stops short of the page corner named. A round corner is what the line
        // has to stop short of; a square one it runs the whole way to.
        [[nodiscard]] float pageCornerInset(Corner) const;
        // The backdrop and shadow a held tab stands on - see PaintEvent::paintHeldBackdrop.
        void paintHeldBackdrop(PaintEvent&) const;
        // The tab's drawn rect, round at the two corners away from the line.
        [[nodiscard]] RoundedRectangleParts silhouette(const PaintEvent&) const;
    private:
        TabGeometry m_profile;
        RichControl* m_page{};
        Color m_actualPageColor{};
        CornerRadii m_actualPageRadii{};
        Hsl m_actualPageHsl{};
        Hsl m_actualPageTextHsl{};
        Lightness m_actualPageLightness{ k_darkLightness };
        ScopedEventConnection m_pageConnection{};
    };

    // A row of tabs, sharing the room between them.
    export class TabStripBase : public StackPanel, public IFittableList
    {
        friend Tab;
    public:
        template <typename... Args>
        explicit TabStripBase(const CreateParams&, Args&&...);
    public:
        std::wstring_view diagnosticText() const override { return L"TabStripBase"; }
        FitData fitData() const override { return { width() }; }
        void traverseFittables(const FittableFunc func) override;
        template<typename...Args>
        Tab& addTab(Args&&... args) { return add<Tab>(std::forward<Args>(args)...); }
        virtual Thickness pageBorder() const = 0;
        TabsOrientation tabsOrientation() const { return m_tabsOrientation; }
        void setTabsOrientation(TabsOrientation value);
        [[nodiscard]] Thickness tabLineThickness() const { return m_tabLineThickness; }
        void setOverlayHost(ContainerBase&);
        void fitTabs(float delta);
        void tabCreated(Tab*);
    protected:
        using StackPanel::orientation;
        using StackPanel::setOrientation;
        void nestedControlDeleted(Control*) override;
        void childHoverEnter(Control&) override;
        void paintChildren(PaintEvent&) override;
        void paintSurface(PaintEvent&) override;
        // Keeps the open tab in view as a vertical strip scrolls - see Control::floatOffset.
        [[nodiscard]] FloatPoint overlayChildOffset(const Control&) const override;
        // Brings a tab to rest inside the band the open tab is held in.
        void scrollChildIntoView(Control&, FloatRect) override;
        bool defaultCanFocusItem(Control&) override;
        void currentItemChanged(CurrentItemChangeEvent&) override;
        virtual RichControl* visualPage(RichControl* page) { return page; }
    private:
        ContainerBase* m_overlayHost{};
        TabsOrientation m_tabsOrientation;
        Thickness m_tabLineThickness{ Thickness::Thin };
        std::vector<Tab*> m_hotTabs{};
        std::unordered_set<Control*> m_allTabs;
    };

    // A row of tabs across the edge of a box.
    export class TabStrip : public TabStripBase
    {
    public:
        using TabStripBase::TabStripBase;
    public:
        Thickness pageBorder() const override { return theme().metrics.border; }
    };


    //-------------------------------------------------------------------------


    Orientation toStackOrientation(TabsOrientation orientation)
    {
        switch (orientation)
        {
        case TabsOrientation::HorizontalTop:
        case TabsOrientation::HorizontalBottom:
            return Orientation::Horizontal;
        case TabsOrientation::VerticalLeft:
        case TabsOrientation::VerticalRight:
            return Orientation::Vertical;
        default:
            unreachable();
        }
    }



    // Tab

    template<typename ...Args>
    Tab::Tab(const CreateParams& params, Args && ...args)
        :
        // The tab's theme metrics go in ahead of the caller's props so that a MinSize or a
        // Padding handed to addTab lands on top of them: RichControl binds ControlMetrics
        // first and each individual metric prop after it, whatever order they arrive in.
        TabBaseClass{
            params,
            Interactivity::Focusable,
            params.themeMetrics().tab,
            std::forward<Args>(args)...
        }
    {
        Props::ifThereIs<Page>([&](const auto& p) {
            setPage(&p.value);
            }, std::forward<Args>(args)...);
        tabCreated(params.parent);
    }

    FitData Tab::fitData() const
    {
        return {
            width(),
            selected() ? form().scaler().scale(48.8f) : 0
        };
    }

    void Tab::takeOffWidth(float value)
    {
        setWidth(width() - value);
        Control& strip = *TabBaseClass::parent();
        setControlWidth(strip, strip.width() - value);
    }

    void Tab::setPage(RichControl* page)
    {
        if (m_page == page)
            return;

        m_page = page;
        m_actualPageColor.clear();
        m_actualPageRadii = {};

        if (RichControl* visPage = visualPage(m_page))
            m_pageConnection = std::move(visPage->onPaint(
                [this](PaintEvent& event) {
                    m_actualPageColor = event.surfaceRgb();
                    m_actualPageColor.alpha = 255;
                    m_actualPageHsl = event.surfaceHsl();
                    m_actualPageTextHsl = event.textHsl();
                    m_actualPageLightness = event.lightness();
                    m_actualPageRadii = event.cornerRadii();
                })
            );
    }

    TabStripBase& Tab::parent() const
    {
        return *static_cast<TabStripBase*>(RichControl::parent());
    }

    Tab& Tab::select()
    {
        parent().recordCurrentItem(this);
        return *this;
    }

    RichControl* Tab::visualPage(RichControl* page)
    {
        return parent().visualPage(page);
    }

    void Tab::adjustPaint(AdjustPaintEvent& event)
    {
        TabBaseClass::adjustPaint(event);
        event.setColorRules(UiElement::Button);
        event.setStrokeRule(event.bakedColors().rule(UiElement::TabLine));

        if (m_actualPageColor.alpha)
        {
            // A tab becomes the page it opens, and that is both of the page's colours or neither -
            // a tab wearing the page's surface under the strip's ink is the one combination
            // nothing chose. Both rules below name the same colour, so what the surface reaches
            // is the two factors composed, which is what hoveredOrSelected() is; the ink is taken
            // the same distance so the two arrive together.
            const float pageFactor = factors().hoveredOrSelected() * factors().enabled();

            // The lightness comes with the colours. Past the half way point the tab is more the
            // page's surface than the strip's, so an ink or a border raised off it has to rise
            // the way that surface leaves room for - the page's answer, not the strip's.
            if (pageFactor > 0.5f)
                event.setLightness(m_actualPageLightness);

            // Spelled in the direction the rules will be read in, so the two agree at the moment
            // the lightness changes hands: a Set states where in that direction the value lands,
            // and stating it one way while reading it the other would move the colour the tab
            // arrives at without the page having moved.
            const Lightness eventLightness = event.lightness();
            event.colorRules().active.setExactHsl(m_actualPageHsl, eventLightness);
            event.colorRules().hovered.setExactHsl(m_actualPageHsl, eventLightness);

            event.setTextHsl(Hsl{ event.textHsl(), m_actualPageTextHsl, pageFactor });
        }
        event.colorRules().pressed.clear();
        // A tab has no surface at rest by being a split button, so the surface rule states a
        // colour nothing reaches - and clearing it keeps the surface a tab's children stand on
        // the strip's, which is what is behind the tab until the page's colour arrives.
        event.colorRules().surface.clear();
    }

    void Tab::paintSurface(PaintEvent& event)
    {
        const float geometryVisibilityFactor = factors().hoveredOrSelected(0.75f, 1.0f);
        PaintEvent* outEvent;
        prepareGeometry(event, geometryVisibilityFactor, outEvent);

        TabColors tabColors;
        //prepareColors
        {
            float visibilityFactor = factors().hoveredOrSelected(1.0f, 1.0f);
            tabColors.surface = event.surfaceRgb();
            if (visibilityFactor)
            {
                tabColors.indicator = event.indicatorRgb().withOpacity(factors().selected() * 0.5f + 0.5f);
                tabColors.indicatorFactor = factors().selected();
            }
            tabColors.lineCaps = event.strokeRgb();
            tabColors.tabLine = event.strokeRgb();

            event.applyFocus2(tabColors.tabLine);
        }

        if (m_profile.tabStart < m_profile.lineStart)
        {
#ifdef _DEBUG
            throw std::logic_error{ "TabStart is less than lineStart! Increase tabstrip padding." };
#endif
            m_profile.tabStart = m_profile.lineStart;
        }

        paintHeldBackdrop(event);
        if (tabColors.tabLine.alpha || tabColors.surface.alpha)
            Graphics::drawTab(outEvent->canvas(), m_profile, tabColors);

        // The silhouette stands in for the surface the base would have painted, so the icon
        // layer is asked for on its own rather than arriving with that surface.
        paintIconLayer(event);
    }

    void Tab::pressDown(PressDownEvent& event)
    {
        TabBaseClass::pressDown(event);

        if (event.control == this)
            select();
    }

    void Tab::drag(DragEvent& event)
    {
        // While it seems convenient for a strip placed on a Form Title,
        // if the strip is on a popup, an accidental drag closes the window, that's awful
        //if (&event.control() == this)
        //{
        //    event.stopPropagation();
        //    form().window().initiateWindowDrag(event.currentPos().toInt(), event.stamp());
        //    return;
        //}
        TabBaseClass::drag(event);
    }

    void Tab::tabCreated(Control* parent)
    {
#ifdef _DEBUG
        if (parent && parent->diagnosticText() != L"TabStripBase")
        {
            throw std::runtime_error("Architecture Error: A 'Tab' control can only be added to a 'TabStripBase' container.");
        }
#endif
        if (parent)
        {
            static_cast<TabStripBase*>(parent)->tabCreated(this);
        }
    }

    void Tab::prepareGeometry(const PaintEvent& event, float visibilityFactor, PaintEvent*& outEvent)
    {
        const TabStripBase& th = parent();
        m_profile.cornerRadius = event.radius();
        float headerStart;
        float headerEnd;

        PaintEvent* stripEvent = event.parentEvent();
        FloatRect contentRect = stripEvent->controlBounds();
        contentRect.inflate(-stripEvent->padding());
        FloatPoint pt = stripEvent->topLeft();
        pt.x += stripEvent->padding().x;
        pt.y += stripEvent->padding().y;
        // Where the tab is drawn, in the strip's content space: a held tab stands away from where
        // it was laid out - see Control::floatOffset.
        FloatRect tabRect = event.controlBounds();
        tabRect.offset(-pt);


        outEvent = stripEvent;

        if (th.m_overlayHost)
        {
            PaintEvent* curr = outEvent;
            while (curr)
            {
                if (&curr->control() == th.m_overlayHost)
                {
                    outEvent = curr;
                    break;
                }
                curr = curr->parentEvent();
            }
        }

        const float tabMargin = event.scale(4.0f) * (1.0f - visibilityFactor);

        m_profile.orientation = th.m_tabsOrientation;
        switch (m_profile.orientation)
        {
        case TabsOrientation::HorizontalTop:
            m_profile.origin = contentRect.bottomLeft().toFloat();
            m_profile.tabStart = tabRect.left + tabMargin;
            m_profile.tabEnd = tabRect.right - tabMargin;
            headerStart = outEvent->left() - pt.x;
            headerEnd = outEvent->right() - pt.x;
            m_profile.tabProtrusion = height() - tabMargin;
            break;

        case TabsOrientation::HorizontalBottom:
            m_profile.origin = contentRect.topLeft().toFloat();
            m_profile.tabStart = tabRect.left + tabMargin;
            m_profile.tabEnd = tabRect.right - tabMargin;
            headerStart = outEvent->left() - pt.x;
            headerEnd = outEvent->right() - pt.x;
            m_profile.tabProtrusion = height() - tabMargin;
            break;

        case TabsOrientation::VerticalLeft:
            m_profile.origin = contentRect.topRight().toFloat();
            m_profile.tabStart = tabRect.top + tabMargin;
            m_profile.tabEnd = tabRect.bottom - tabMargin;
            // A page that squares the corners on the strip's side is met along its whole edge,
            // which is what lets the line and the page's border read as one - see
            // TabbedBox::paintPageRun.
            headerStart = outEvent->top() + outEvent->padding().y - pt.y
                + pageCornerInset(Corner::TopLeft);
            headerEnd = outEvent->bottom() - outEvent->padding().y - pt.y
                - pageCornerInset(Corner::BottomLeft);
            m_profile.tabProtrusion = width() - tabMargin;
            break;

        case TabsOrientation::VerticalRight:
            m_profile.origin = contentRect.topLeft().toFloat();
            m_profile.tabStart = tabRect.top + tabMargin;
            m_profile.tabEnd = tabRect.bottom - tabMargin;
            headerStart = outEvent->top() + outEvent->cornerRadii()[cornerIndex(Corner::TopRight)] - pt.y;
            headerEnd = outEvent->bottom() - outEvent->cornerRadii()[cornerIndex(Corner::BottomRight)] - pt.y;
            m_profile.tabProtrusion = width() - tabMargin;
            break;

        default: unreachable();
        }
        headerStart += outEvent->borderWidth();
        headerEnd -= outEvent->borderWidth();

        if (selected())
        {
            m_profile.lineStart = headerStart;
            m_profile.lineEnd = headerEnd;
        }
        else
        {
            m_profile.lineStart = m_profile.tabStart - m_profile.cornerRadius;
            m_profile.lineEnd = m_profile.tabEnd + m_profile.cornerRadius;
        }

        m_profile.lineWidth = event.scaledStrokeWidth(th.m_tabLineThickness);
        m_profile.indicatorHeight = m_profile.lineWidth * 2.0f; //event.scaledStrokeWidth(Thickness::Heavy);
        // was in TabRenderer before:
        //const float indicatorHeight = std::min(
        //    geometry.lineWidth * 2.5f,
        //    geometry.tabProtrusion - geometry.lineWidth
        //);
    }

    float Tab::pageCornerInset(Corner corner) const
    {
        return m_actualPageRadii[cornerIndex(corner)];
    }

    // Drawn from the strip's event, whose surface is what the tabs stand on. The shadow stays on
    // the strip's content in view: the tab line runs along its edge, and the page starts there.
    void Tab::paintHeldBackdrop(PaintEvent& event) const
    {
        const float travel = std::abs(floatOffset().y);
        if (travel == 0.0f)
            return;
        PaintEvent& stripEvent = *event.parentEvent();
        FloatRect stripContent = stripEvent.controlBounds();
        stripContent.inflate(-stripEvent.padding());
        stripEvent.canvas().pushClip(FloatRect::intersection(stripContent, stripEvent.viewport()));
        stripEvent.paintHeldBackdrop(silhouette(event), travel);
        stripEvent.canvas().popClip();
    }

    RoundedRectangleParts Tab::silhouette(const PaintEvent& event) const
    {
        const float radius = m_profile.cornerRadius;
        RoundedRectangleParts result = {
            .bounds = event.controlBounds()
        };
        switch (m_profile.orientation)
        {
            case TabsOrientation::HorizontalTop:
                result.radii = { radius, radius, 0.0f, 0.0f };
                break;
            case TabsOrientation::HorizontalBottom:
                result.radii = { 0.0f, 0.0f, radius, radius };
                break;
            case TabsOrientation::VerticalLeft:
                result.radii = { radius, 0.0f, 0.0f, radius };
                break;
            case TabsOrientation::VerticalRight:
                result.radii = { 0.0f, radius, radius, 0.0f };
                break;
        }
        return result;
    }

    // TabStripBase

    template<typename ...Args>
    TabStripBase::TabStripBase(const CreateParams& params, Args && ...args)
        :
        // ActiveContainer is what makes a strip keep a current item at all - see
        // StackPanelBase::followsUser - and the open tab is that item. Load-bearing and quiet:
        // drop it and everything still compiles, the tabs just stop answering to a click.
        StackPanel{ params, Interactivity::ActiveContainer, std::forward<Args>(args)... }
    {
        // Which edge of the box the tabs run along.
        TabsOrientation tabsOrientation = READ_PROPERTY(TabsOrientation, TabsOrientation::HorizontalTop);
        setTabsOrientation(tabsOrientation);
        m_tabLineThickness = READ_PROPERTY(TabLineThickness, Thickness::Thin).value;
        setOverlayHost(*this);
    }

    void TabStripBase::traverseFittables(const FittableFunc func)
    {
        for (Control* ptr : m_allTabs)
        {
            Tab& tab = static_cast<Tab&>(*ptr);
            func(tab);
        }
    }

    void TabStripBase::setTabsOrientation(TabsOrientation value)
    {
        m_tabsOrientation = value;
        setOrientation(toStackOrientation(value));
    }

    void TabStripBase::setOverlayHost(ContainerBase& value)
    {
        if (m_overlayHost == &value)
            return;
        m_overlayHost = &value;
        if (currentItem())
            m_overlayHost->addOverlayControl(*currentItem(), ClippingMode::Unbounded);
    }

    void TabStripBase::fitTabs(float delta)
    {
        shrinkBy(delta);
    }

    void TabStripBase::tabCreated(Tab* tab)
    {
        m_hotTabs.push_back(tab);
        m_allTabs.insert(tab);
        switch (m_tabsOrientation)
        {
        case TabsOrientation::HorizontalTop:
            tab->setVerticalAlign(VerticalAlign::Bottom);
            break;
        case TabsOrientation::HorizontalBottom:
            tab->setVerticalAlign(VerticalAlign::Top);
            break;
        case TabsOrientation::VerticalLeft:
            break;
        case TabsOrientation::VerticalRight:
            break;
        default: unreachable();
        }
    }

    void TabStripBase::nestedControlDeleted(Control* control)
    {
        StackPanel::nestedControlDeleted(control);

        auto it = std::find(m_hotTabs.begin(), m_hotTabs.end(), control);
        if (it != m_hotTabs.end())
            m_hotTabs.erase(it);
        m_allTabs.erase(control);
    }

    void TabStripBase::childHoverEnter(Control& control)
    {
        StackPanel::childHoverEnter(control);

        auto it = std::find(m_hotTabs.begin(), m_hotTabs.end(), &control);
        if (it != m_hotTabs.end())
            std::rotate(it, it + 1, m_hotTabs.end());
    }

    void TabStripBase::paintChildren(PaintEvent& event)
    {
        Control* selItem = currentItem();

        for (ControlPtr& control : controls())
            if (!m_allTabs.contains(&*control))
                event.paintChild(*control);

        for (Control* tab : m_hotTabs)
            if (tab != selItem)
                event.paintChild(*tab);

        // --- DYNAMIC OVERLAY PASS-THROUGH ---
        // Paint the selected item if we are in the overlay stage,
        // or if there is no overlay host managing it.
        if (selItem && (event.overlayStage() || !event.overlayHost()))
        {
            event.paintChild(*selItem);
        }
    }

    void TabStripBase::paintSurface(PaintEvent& pp)
    {
        StackPanel::paintSurface(pp);
        if (!currentItem())
        {
            // Optional: Draw line fallback when there is no active selection
        }
    }

    // A held header's rest line, and its mirror at the bottom, so the tab rests where a header
    // would and stacks under one held above it. A view too short for the tab holds it at the top.
    FloatPoint TabStripBase::overlayChildOffset(const Control& child) const
    {
        if (&child != currentItem() || orientation() != Orientation::Vertical)
            return {};
        const float origin = child.parentContentOrigin().y;
        const float restTop = restLineInForm() - origin;
        const float restBottom = footLineInForm() - origin;
        const float home = child.top();
        const float drawnTop = std::max(restTop, std::min(home, restBottom - child.height()));
        return { 0.0f, drawnTop - home };
    }

    // A taller request, the way a grid clears its held header. The band stops a scroll content
    // inset short of each edge of the view, and a tab brought to rest in that inset would open
    // held away from its place.
    void TabStripBase::scrollChildIntoView(Control& control, FloatRect controlRect)
    {
        if (orientation() == Orientation::Vertical)
            controlRect.inflate(0.0f, scrollContentInset().y);
        StackPanel::scrollChildIntoView(control, controlRect);
    }

    bool TabStripBase::defaultCanFocusItem(Control& item)
    {
        return m_allTabs.contains(&item);
    }

    void TabStripBase::currentItemChanged(CurrentItemChangeEvent& event)
    {
        Control* newPage{ nullptr };
        auto it = m_allTabs.find(currentItem());
        if (it != m_allTabs.end())
            if ((newPage = static_cast<Tab*>(*it)->page()))
                newPage->show();
        if (event.previousItem)
        {
            it = m_allTabs.find(event.previousItem);
            if (it != m_allTabs.end())
                if (Control* prevPage = static_cast<Tab*>(*it)->page())
                    if (prevPage != newPage)
                        prevPage->hide();
        }
        StackPanel::currentItemChanged(event);
        if (m_overlayHost)
        {
            if (event.previousItem)
                m_overlayHost->removeOverlayControl(*event.previousItem);
            if (currentItem())
                m_overlayHost->addOverlayControl(*currentItem(), ClippingMode::Unbounded);
        }
        // A tab opened while the strip has it cut off is held from that moment, and a held control
        // is not scrolled - see Control::scrollIntoView. The strip brings it to its place instead.
        if (Control* item = currentItem(); item && item->isHeldInView())
            scrollChildIntoView(*item, item->boundsInParent());
    }

}
