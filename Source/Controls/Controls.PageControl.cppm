export module ClaFi.Controls.PageControl;

import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // Which of its pages a page control answers for its own size with. See Item-Containers
    export enum class PageSizing
    {
        CurrentPage,   // the page that shows, and nothing about the ones that do not
        WidestPage     // the widest of them all - see PageControl::calculateChildren
    };

    // A stack showing one of its pages at a time.
    export class PageControl : public StackPanelBase
    {
    public:
        using StackPanelBase::StackPanelBase;
        using StackPanelBase::add;
        std::wstring_view diagnosticText() const override { return L"PageControl"; }
        // Whether the pages nobody is looking at count towards this control's size. See Controls
        [[nodiscard]] PageSizing pageSizing() const { return m_pageSizing; }
        void setPageSizing(PageSizing);
    protected:
        // No page open. The pages are kept whether or not one shows, so the count cannot say.
        [[nodiscard]] bool isEmpty() const override { return !currentItem(); }
        void controlAdded(Control&) override;
        void nestedControlDeleted(Control*) override;
        void childVisibilityChanged(Control&) override;
        bool defaultCanFocusItem(Control&) override;
        void currentItemChanged(CurrentItemChangeEvent&) override;
        void calculateChildren(FormBase&) override;
        ScaledDimensions calculateContent(AlignEvent&) override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&) override;
    private:
        PageSizing m_pageSizing{ PageSizing::CurrentPage };
        // The scale the hidden pages were last measured at. Unset until they have been, and unset
        // again whenever the set of pages changes - see calculateChildren.
        std::optional<ScaleFactor> m_measuredScale{};
    };


    //----------------------------------------------------------------------------


    void PageControl::setPageSizing(const PageSizing value)
    {
        if (m_pageSizing == value)
            return;
        m_pageSizing = value;
        m_measuredScale.reset();
        invalidateFormAlign();
    }

    void PageControl::controlAdded(Control& control)
    {
        StackPanelBase::controlAdded(control);
        control.hide();
        // A page arriving is a page nobody has measured.
        m_measuredScale.reset();
    }

    void PageControl::nestedControlDeleted(Control* control)
    {
        StackPanelBase::nestedControlDeleted(control);
        m_measuredScale.reset();
    }

    void PageControl::childVisibilityChanged(Control& control)
    {
        StackPanelBase::childVisibilityChanged(control);

        if (control.visible())
            recordCurrentItem(control);
        else
            if (currentItem() == &control)
                recordCurrentItem(nullptr);
    }

    // Every page qualifies. A PageControl does not follow the user - the tab strip decides
    // which page shows - so the base rule would turn all of them down.
    bool PageControl::defaultCanFocusItem(Control&)
    {
        return true;
    }

    void PageControl::currentItemChanged(CurrentItemChangeEvent& event)
    {
        StackPanelBase::currentItemChanged(event);
        if (event.previousItem)
            event.previousItem->hide();
        if (currentItem())
            currentItem()->show();
        // And align is invalidated in the base updateVisibility()
    }

    // A CONTROL IS CALCULATED WHILE IT IS VISIBLE, and a page control shows one page - so the
    // pages nobody is looking at are never measured, and most of what makes this cheap is that
    // they are not. Asking for their size is asking for a layout of each, so it is asked ONCE:
    // again when a page is added or taken away, and again when the scale they were measured at
    // is no longer the form's, and not on the passes in between.
    //
    // The stale case that leaves: a page that grows on its own after it was measured, while it
    // is hidden and the scale holds. A page control rebuilt every time it opens - the
    // application menu is - never meets it.
    void PageControl::calculateChildren(FormBase& form)
    {
        StackPanelBase::calculateChildren(form);
        if (m_pageSizing == PageSizing::CurrentPage)
            return;

        const ScaleFactor scale = form.scaler().factor();
        if (m_measuredScale == scale)
            return;

        for (const ControlPtr& page : controls())
            if (!page->visible())
                calculateControl(*page, form);
        m_measuredScale = scale;
    }

    // THE BOX A PAGE CAME TO, NOT THE FLOOR IT CAN BE CUT TO. Control::calculate keeps both: the
    // measurement in its dimensions and the content's floor in calculatedMinSize, and a floor is
    // what a page survives being squeezed to - a button's text ellipsizes long before its page
    // runs out of room. What this control has to be is the measurement.
    //
    // BOTH EXTENTS, so that turning to another page does not resize what holds this one. The menu
    // is as big as the largest page it can show, whichever it happens to be showing.
    ScaledDimensions PageControl::calculateContent(AlignEvent& event)
    {
        ScaledDimensions result = StackPanelBase::calculateContent(event);
        if (m_pageSizing == PageSizing::CurrentPage)
            return result;

        for (const ControlPtr& page : controls())
        {
            result.x = std::max(result.x, page->dimensions().x);
            result.y = std::max(result.y, page->dimensions().y);
        }
        return result;
    }

    void PageControl::alignContent(AlignEvent& event, ScaledPosition, ScaledDimensions& contentDimensions)
    {
        if (currentItem())
        {
            alignControl(currentItem(), event, { 0, 0 }, contentDimensions);
            contentDimensions = currentItem()->dimensions();
        }
    }
}
