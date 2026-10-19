module ClaFi.Core.Foundation;

// :Navigation is not re-exported by Foundation_Facade, so the implicit import of the primary
// interface does not bring it in. Without this the whole partition is invisible here.
import :Navigation;
import :Control;
import :Input;
import :Form;
import :Spatial;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    // Where a control was laid out, in form space - the place a container reads its children at.
    // A hold draws a control elsewhere, and the row after a held header is still the row laid out
    // after it.
    [[nodiscard]] static FloatRect laidOutRect(const Control& control)
    {
        return FloatRect::FromDimensions(control.parentContentOrigin() + control.topLeft(),
            control.scaledSize());
    }

    // The part of a rect a step travels over.
    [[nodiscard]] static AxisSpan spanAlong(const FloatRect& rect, const NavigationStep& step)
    {
        if (step.vertical())
            return { rect.top, rect.bottom };
        return { rect.left, rect.right };
    }

    [[nodiscard]] static NavigationDirection directionOf(const KeyCode key)
    {
        switch (key)
        {
        case Keys::Left:
            return NavigationDirection::Left;
        case Keys::Right:
            return NavigationDirection::Right;
        case Keys::Up:
            return NavigationDirection::Up;
        default:
            return NavigationDirection::Down;
        }
    }

    void FocusNavigator::formKeyDown(KeyDownEvent& event)
    {
        // **Navigation is within the form the key was delivered to**, and the focus is one per
        // application rather than one per form - so a form that has not taken it sees a control
        // in another window. Starting there searches a tree this form has nothing to do with:
        // a menu shown over a text box is opened with the focus still on the box, and its very
        // first move - the emulated Tab that gives the menu its first item - would walk the form
        // **behind** it.
        //
        // The same case as no focus at all, and it has the same answer. Nothing has been focused
        // yet, or whatever held the focus has been deleted - the pointer is dropped without a
        // replacement being named, and navigation would have nowhere to start from for the rest
        // of the form's life. The form is the container this navigation belongs to, and it
        // answers with its current item or with itself, which is the start a walk needs either
        // way.
        Control* focusHolder = Input::focusedControl();
        if (focusHolder && &focusHolder->form() != &m_form)
            focusHolder = nullptr;
        if (!focusHolder)
            focusHolder = &m_form.content();

        Control* focusedItem = focusHolder->focusDelegate();
        if (!focusedItem)
            focusedItem = focusHolder;

        const KeyCode key = event.key;
        if (key == Keys::Return || key == Keys::Space)
        {
            focusedItem->animatedClick(m_form, event.stamp);
            return;
        }

        // **The nearest container keeping a current item is the scope.** Tab steps out of it as a
        // whole, the arrows and the page keys move inside it, and Home and End go to its ends.
        Control* sourceContainer = focusedItem->navigationContainer(CheckSelf::Yes);

        switch (key)
        {
        case Keys::Tab:
            if (Control* stop = tabStop(*focusedItem, sourceContainer, event.modifiers.shift))
                land(*stop);
            return;
        case Keys::Left:
        case Keys::Right:
        case Keys::Up:
        case Keys::Down:
        {
            const NavigationStep step = stepFrom(*focusedItem, directionOf(key),
                NavigationSource::Key);
            // The scope holding the focus itself, with no item current, is entered - and so is
            // the form, when nothing in it holds the focus.
            const bool entersItem = focusedItem == sourceContainer || isFormRoot(focusedItem);
            Control* stop = entersItem
                ? enter(*focusedItem, step)
                : arrowStop(*focusedItem, step, sourceContainer);
            if (stop && stop != focusedItem)
                land(*stop, step);
            return;
        }
        case Keys::Home:
        case Keys::End:
        case Keys::Prior:
        case Keys::Next:
            // These four address a container. With none in scope there is nothing for them to
            // move within, and the focus stays where it is.
            if (sourceContainer)
                moveInContainer(*sourceContainer, *focusedItem, key);
            return;
        }
    }

    bool FocusNavigator::isFormRoot(const Control* control) const
    {
        return control == &m_form.content();
    }

    // **Each container answers for its own children**, from what its layout already says about
    // them, and the first one up the parents that has a child the step lands in names it. A
    // container with nothing that way leaves the step to the one holding it. The walk stops at
    // the boundary, which is asked last.
    Control* FocusNavigator::arrowStop(Control& item, const NavigationStep& step,
        const Control* boundary)
    {
        for (Control* child = &item; child != boundary; )
        {
            Control* container = child->parent();
            if (!container)
                return nullptr;
            if (Control* neighbour = container->navigationNeighbour(*child, step))
            {
                if (Control* stop = enter(*neighbour, step))
                    return stop;
            }
            child = container;
        }
        return nullptr;
    }

    // **An active container answers for the item it holds.** A step entering it lands on its
    // current item, wherever that is and however far it is scrolled away; scrollIntoView brings
    // it back. A container with nothing current is entered the way any container is, and one with
    // nothing to hand the focus to keeps it.
    Control* FocusNavigator::enter(Control& control, const NavigationStep& step)
    {
        if (control.interactivity() == Interactivity::ActiveContainer)
        {
            Control* currentItem = control.focusDelegate();
            if (currentItem != &control && currentItem->canTakeFocus())
                return currentItem;
        }
        else if (control.canTakeFocus())
            return &control;

        if (Control* child = control.navigationEntry(step))
        {
            if (Control* stop = enter(*child, step))
                return stop;
        }
        return control.canTakeFocus() ? &control : nullptr;
    }

    Control* FocusNavigator::tabStop(Control& item, Control* sourceContainer, const bool reverse)
    {
        // A container met on the way is entered at the end the walk arrives from.
        const ScrollDirection entryEdge = reverse
            ? ScrollDirection::ToEnd
            : ScrollDirection::ToBegin;

        // **Tab leaves a container as a whole**, so the walk starts at the container rather than
        // at the item inside it - a grid or a list is one stop, not one stop per cell. The form is
        // the one container with nothing outside it to step out to, and an item whose nearest
        // container is the form starts the walk at itself.
        Control* child = sourceContainer && !isFormRoot(sourceContainer)
            ? sourceContainer
            : &item;
        while (!isFormRoot(child) && child->parent())
        {
            Control& parent = *child->parent();
            const std::size_t count = parent.navigationCount();
            std::size_t index = 0;
            while (index != count && parent.navigationChild(index) != child)
                ++index;
            // A child the container keeps out of its reading order has no siblings in it, and the
            // walk carries on from the container.
            const std::size_t siblings = index == count ? 0 : (reverse ? index : count - 1 - index);
            for (std::size_t i = 1; i <= siblings; ++i)
            {
                Control* sibling = parent.navigationChild(reverse ? index - i : index + i);
                if (!sibling)
                    continue;
                if (Control* stop = readingEntry(*sibling, entryEdge))
                    return stop;
            }
            child = &parent;
        }

        // Nothing past it anywhere: the walk comes round to the end it started from, which is
        // where entering the form from outside would have put it.
        return edgeStop(m_form.content(), entryEdge);
    }

    // **Reading order is the only thing consulted**, never geometry: the item at the end of a
    // container worth paging through sits outside the viewport, and the container's own order
    // is already the order its layout reads in.
    Control* FocusNavigator::edgeStop(Control& container, const ScrollDirection edge)
    {
        const std::size_t count = container.navigationCount();
        for (std::size_t i = 0; i != count; ++i)
        {
            Control* child = container.navigationChild(edge == ScrollDirection::ToBegin
                ? i
                : count - 1 - i);
            if (!child)
                continue;
            if (Control* stop = readingEntry(*child, edge))
                return stop;
        }
        return nullptr;
    }

    // The active container rule of enter, read in order rather than by a step.
    Control* FocusNavigator::readingEntry(Control& control, const ScrollDirection edge)
    {
        if (!control.holdsNavigationStop())
            return nullptr;
        if (control.interactivity() == Interactivity::ActiveContainer)
        {
            Control* currentItem = control.focusDelegate();
            if (currentItem != &control && currentItem->canTakeFocus())
                return currentItem;
            Control* stop = edgeStop(control, edge);
            return stop ? stop : &control;
        }
        if (control.canTakeFocus())
            return &control;
        return edgeStop(control, edge);
    }

    // **A page is as many steps as fit in the container's viewport**, along the axis it scrolls
    // on, so a page press lands where a run of arrow presses would have. A container that holds
    // a single screenful has no page to cross, and a press goes to its end.
    void FocusNavigator::moveInContainer(Control& container, Control& item, const KeyCode key)
    {
        const ScrollDirection edge = key == Keys::Home || key == Keys::Prior
            ? ScrollDirection::ToBegin
            : ScrollDirection::ToEnd;
        const bool pages = key == Keys::Prior || key == Keys::Next;
        const std::optional<NavigationDirection> pageWay = pages
            ? pageDirection(container, edge)
            : std::nullopt;
        if (!pageWay)
        {
            if (Control* stop = edgeStop(container, edge))
                land(*stop);
            return;
        }

        const NavigationStep step = stepFrom(item, *pageWay, NavigationSource::Page);
        Control* stop = pageStop(container, item, step);
        if (!stop)
            stop = edgeStop(container, edge);
        if (stop)
            land(*stop, step);
    }

    std::optional<NavigationDirection> FocusNavigator::pageDirection(Control& container,
        const ScrollDirection direction) const
    {
        const FloatRect bounds = m_form.rectOfControl(&container);
        const FloatRect viewport = container.visibleRectInForm(m_form);

        // The axis carrying the greater overflow is the one the container scrolls on. Which axis
        // overflows at all is not enough to tell them apart: a wrapping layout wraps at the
        // viewport extent rather than at a whole item, so it spills a few pixels onto the axis it
        // fills, and a container is free to overflow an axis whose scroll bar is hidden.
        const float verticalOverflow = bounds.height() - viewport.height();
        const float horizontalOverflow = bounds.width() - viewport.width();
        const bool vertical = verticalOverflow >= horizontalOverflow;
        if ((vertical ? verticalOverflow : horizontalOverflow) <= m_form.scaler().scaled8)
            return std::nullopt;

        const bool forward = direction == ScrollDirection::ToEnd;
        if (vertical)
            return forward ? NavigationDirection::Down : NavigationDirection::Up;
        return forward ? NavigationDirection::Right : NavigationDirection::Left;
    }

    // What a page crosses includes the item it starts from, so that item is still in sight at the
    // far edge of the page that follows. The first step is taken whatever it crosses: a stop
    // longer than the viewport is still the next one.
    Control* FocusNavigator::pageStop(Control& container, Control& item, const NavigationStep& step)
    {
        const FloatRect viewport = container.visibleRectInForm(m_form);
        const float page = step.vertical() ? viewport.height() : viewport.width();
        const AxisSpan source = spanAlong(laidOutRect(item), step);

        Control* result = nullptr;
        for (Control* next = arrowStop(item, step, &container);
             next;
             next = arrowStop(*next, step, &container))
        {
            const AxisSpan reached = spanAlong(laidOutRect(*next), step);
            const float crossed = step.forward()
                ? reached.end - source.start
                : source.end - reached.start;
            if (result && crossed > page)
                break;
            result = next;
        }
        return result;
    }

    // **A run of steps one way keeps its line**, and the first step of a run seeds it from the
    // middle of the item it starts at. Any other move of the focus starts a new run, because the
    // line is kept against the stop it was written on.
    NavigationStep FocusNavigator::stepFrom(const Control& item,
        const NavigationDirection direction, const NavigationSource source) const
    {
        const FloatRect rect = laidOutRect(item);
        const FormBase::NavigationLine& kept = m_form.m_navigationLine;
        const FloatPoint line = kept.holder == &item
            ? rect.topLeft() + kept.offset
            : rect.center();
        return { direction, source, line };
    }

    // **A step writes only the line across it**, brought into the stop it landed on - at the edge
    // the step crossed, which is where the eye is - and leaves the line along it as it was. Kept
    // as an offset from the stop, so a scroll carrying the stop away carries the line with it.
    void FocusNavigator::land(Control& stop, const NavigationStep& step)
    {
        const FloatRect rect = laidOutRect(stop);
        FloatPoint line = step.line;
        if (step.vertical())
            line.y = AxisSpan{ rect.top, rect.bottom }.clampInto(line.y);
        else
            line.x = AxisSpan{ rect.left, rect.right }.clampInto(line.x);
        m_form.m_navigationLine = { &stop, line - rect.topLeft() };
        stop.scrollIntoView();
        stop.setFocus();
    }

    void FocusNavigator::land(Control& stop)
    {
        m_form.m_navigationLine = {};
        stop.scrollIntoView();
        stop.setFocus();
    }
}
