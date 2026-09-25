module ClaFi.Core.Foundation;

// :Navigation is not re-exported by Foundation_Facade, so the implicit import of the primary
// interface does not bring it in. Without this the whole partition is invisible here.
import :Navigation;
import :Control;
import :Input;
import :Form;
import :Spatial;
import :Traversal;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    void FocusNavigator::formKeyDown(KeyDownEvent& event)
    {
        // NAVIGATION IS WITHIN THE FORM THE KEY WAS DELIVERED TO, and the focus is one per
        // application rather than one per form - so a form that has not taken it sees a control
        // in another window. Starting there searches a tree this form has nothing to do with:
        // a menu shown over a text box is opened with the focus still on the box, and its very
        // first move - the emulated Tab that gives the menu its first item - walked the form
        // BEHIND it and focused whatever stood after the box there. It reached the menu at all
        // only when that walk found nothing and fell through to the wrap.
        //
        // The same case as no focus at all, and it has the same answer. Nothing has been focused
        // yet, or whatever held the focus has been deleted - the pointer is dropped without a
        // replacement being named, and navigation would have nowhere to start from for the rest
        // of the form's life. The form is the container this navigation belongs to, and it
        // answers with its current item or with itself, which is the entry the search needs
        // either way.
        Control* focusHolder = Input::focusedControl();
        if (focusHolder && &focusHolder->form() != &m_form)
            focusHolder = nullptr;
        if (!focusHolder)
            focusHolder = &m_form.content();

        Control* focusedItem = focusHolder->focusDelegate();
        if (!focusedItem)
            focusedItem = focusHolder;

        KeyCode key = event.key;

        if (key == Keys::Return || key == Keys::Space) {
            focusedItem->animatedClick(m_form, event.stamp);
            return;
        }

        // The container the search starts inside, and the one Tab steps out of as a whole. It is
        // also what tells an item reached by moving within the container from an item reached by
        // entering the container from outside - see entryPoint.
        Control* sourceContainer = focusedItem->navigationContainer(CheckSelf::Yes);

        // Which end of a container the focus lands on when navigation enters one holding nothing
        // selected. Travel forward enters at the first item and travel back at the last, so the
        // item taking the focus is the one nearest the edge that was crossed. Home, End and the
        // page keys name the end they head for themselves.
        ScrollDirection entryEdge = ScrollDirection::ToBegin;

        Control* bestCandidate = nullptr;
        if (key == Keys::Tab)
        {
            bool isReverse = event.modifiers.shift;
            key = isReverse ? Keys::Left : Keys::Right;
            entryEdge = isReverse ? ScrollDirection::ToEnd : ScrollDirection::ToBegin;

            // Tab leaves a container as a whole, so the walk starts AT the container rather
            // than at the item inside it - a grid or a list is one stop, not one stop per cell.
            //
            // The form is a container too, and it is the one that cannot be left: there is
            // nothing outside it to step out to, and starting the walk there excludes the whole
            // tree as the source's own subtree, which leaves Tab with nothing but the wrap. An
            // item whose nearest container is the form starts the walk at itself.
            Control* startNode = sourceContainer && !isFormRoot(sourceContainer)
                ? sourceContainer
                : focusedItem;

            while (startNode && !bestCandidate)
            {
                Control* outerScope = startNode->parent();
                bestCandidate = structuralSearch(
                    key,
                    entryEdge,
                    orientedRect(startNode, key),
                    outerScope,
                    startNode
                );
                startNode = outerScope;
            }

            // Nothing ahead anywhere: the walk has run out of form and starts again at the end it
            // came round to, which is where entering the form from outside would have put it.
            if (!bestCandidate)
                bestCandidate = edgeItem(m_form.content(), entryEdge);
        }
        else // Arrows
            if (key == Keys::Left
            || key == Keys::Right
            || key == Keys::Up
            || key == Keys::Down)
        {
            entryEdge = key == Keys::Left || key == Keys::Up
                ? ScrollDirection::ToEnd
                : ScrollDirection::ToBegin;

            OrientedRect rect = orientedRect(focusedItem, key);
            if (sourceContainer == focusedItem)
                rect.implode();

            NavigationWrap navigationWrap = focusedItem->navigationWrap();
            SearchMethod searchMethod = SearchMethod::Spatial;
            switch (key)
            {
            case Keys::Up:
            case Keys::Down:
                if (navigationWrap.vertical)
                    searchMethod = SearchMethod::WrapLanes;
                break;
            default:
                if (navigationWrap.horizontal)
                    searchMethod = SearchMethod::WrapLanes;
                break;
            }

            bestCandidate = spatialSearch(
                key,
                rect,
                sourceContainer,
                nullptr,
                searchMethod,
                SearchFilter::Focusable
            );
        }
        else if (key == Keys::Home
            || key == Keys::End
            || key == Keys::Prior
            || key == Keys::Next)
        {
            // These four address a container. With none in scope there is nothing for them to
            // move within, and the focus stays where it is.
            if (sourceContainer)
            {
                const ScrollDirection direction = key == Keys::Home || key == Keys::Prior
                    ? ScrollDirection::ToBegin
                    : ScrollDirection::ToEnd;
                entryEdge = direction;

                bestCandidate = key == Keys::Home || key == Keys::End
                    ? edgeItem(*sourceContainer, direction)
                    : pageItem(*sourceContainer, *focusedItem, direction);
            }
        }

        if (bestCandidate)
        {
            bestCandidate = entryPoint(bestCandidate, sourceContainer, entryEdge);
            bestCandidate->scrollIntoView();
            bestCandidate->setFocus();
        }
    }

    bool FocusNavigator::isFormRoot(const Control* control) const
    {
        return control == &m_form.content();
    }

    // An active container answers for the item it holds. A search that arrives from outside the
    // container has matched the container's own bounds, and the item takes the focus on that
    // match alone: where the item sits, and whether the container is scrolled far away from it,
    // decide nothing. scrollIntoView then brings the item back into the viewport.
    //
    // A container holding nothing selected takes the item at entryEdge, and selects it the way it
    // selects any item the keyboard puts the focus on, so a container is never left focused with
    // no item of its own. A search that matched an item inside keeps that item: the geometry
    // named it, which is more than the edge rule knows. An empty container has nothing to hand
    // the focus to and keeps it.
    Control* FocusNavigator::entryPoint(Control* candidate, const Control* sourceContainer,
        ScrollDirection entryEdge)
    {
        Control* container = candidate->navigationContainer(CheckSelf::Yes);
        // The form is not entered - the search was already inside it. Answering with its current
        // item here sends every move that lands on a form-level control straight back to wherever
        // the focus already was, which is a Tab that never leaves the first thing it reaches.
        if (!container || container == sourceContainer || isFormRoot(container))
            return candidate;

        Control* focusedItem = container->focusDelegate();
        if (focusedItem && focusedItem != container)
            return focusedItem;

        if (candidate != container)
            return candidate;

        Control* item = edgeItem(*container, entryEdge);
        return item ? item : candidate;
    }

    // The control at one end of a container that can take the focus, in the order navigation
    // reads the container in, descending through the ones that cannot. That order is the only
    // thing consulted, and geometry none of it: the item at the edge of a container worth paging
    // through sits outside the viewport, and the walker never visits what is off screen.
    Control* FocusNavigator::edgeItem(Control& container, ScrollDirection edge)
    {
        // A container that keeps no slots holds its children in reading order already.
        const ControlSlots slots = container.navigationSlots();
        const ControlSpanC controls = container.controls();
        const bool bySlot = !slots.empty();
        const std::size_t count = bySlot ? slots.size() : controls.size();

        for (std::size_t i = 0; i != count; ++i)
        {
            const std::size_t index = edge == ScrollDirection::ToBegin ? i : count - 1 - i;
            // Null is an unfilled slot.
            Control* child = bySlot ? slots[index] : controls[index].get();
            if (!child || !child->visible() || !child->enabled(true))
                continue;
            if (child->canTakeFocus())
                return child;
            if (Control* nested = edgeItem(*child, edge))
                return nested;
        }
        return nullptr;
    }

    // The item one screenful away along the axis the container scrolls on. The page is the
    // container's own visible span, so what the user sees is what one press crosses, and the item
    // currently at the far edge is the one landed on - it stays on screen as the anchor the new
    // page is read against. A container that overflows on neither axis has no page to cross, and
    // a page that reaches past the content ends at the edge item.
    Control* FocusNavigator::pageItem(Control& container, Control& focusedItem, ScrollDirection direction)
    {
        const FloatRect containerBounds = m_form.rectOfControl(&container);
        const FloatRect containerViewport = container.visibleRectInForm(m_form);

        // The axis carrying the greater overflow is the one the container scrolls on. Which axis
        // overflows at all is not enough to tell them apart: a wrapping layout wraps at the
        // viewport extent rather than at a whole item, so it spills a few pixels onto the axis it
        // fills, and a container is free to overflow an axis whose scrollbar is hidden. Reading
        // either as the scrolling axis pages along an axis that holds a single screenful, where
        // the search finds nothing ahead and every press ends at the edge item.
        const float verticalOverflow = containerBounds.height() - containerViewport.height();
        const float horizontalOverflow = containerBounds.width() - containerViewport.width();
        const bool scrollsVertically = verticalOverflow >= horizontalOverflow;

        const float overflow = scrollsVertically ? verticalOverflow : horizontalOverflow;
        if (overflow <= m_form.scaler().scaled8)
            return edgeItem(container, direction);

        const bool forward = direction == ScrollDirection::ToEnd;
        const KeyCode key = scrollsVertically
            ? (forward ? Keys::Down : Keys::Up)
            : (forward ? Keys::Right : Keys::Left);

        OrientedRect rect = orientedRect(&focusedItem, key);
        if (&container == &focusedItem)
            rect.implode();

        const OrientedRect pageRect = OrientedRect::orient(containerViewport, key);
        const float pageExtent = pageRect.primary.end - pageRect.primary.start;
        const float itemExtent = rect.primary.end - rect.primary.start;
        const float overlap = itemExtent < pageExtent ? itemExtent : 0.0f;

        // Orienting has already turned the direction of travel into the ascending primary axis,
        // so a page forward is a step up the primary axis whichever key it came from.
        if (FloatRect::intersection(m_form.rectOfControl(&focusedItem), containerViewport).empty())
        {
            // Scrolling has carried the focused item outside the viewport. The page to cross is
            // then the one on screen: an item a hundred pages away names a band the walker does
            // not reach, and widening the walk to reach it costs the whole distance scrolled.
            rect.primary.end = pageRect.primary.end - overlap;
            rect.primary.start = rect.primary.end - itemExtent;
        }
        else
        {
            const float step = pageExtent - overlap;
            rect.primary.start += step;
            rect.primary.end += step;
        }

        Control* result = spatialSearch(
            key,
            rect,
            &container,
            nullptr,
            SearchMethod::Spatial,
            SearchFilter::Focusable,
            pageExtent
        );

        return result ? result : edgeItem(container, direction);
    }

    // One step along the container the focus is in, and into whatever that step reaches. The
    // step itself is geometry - what sits next along the direction of travel - but a container
    // met on the way is entered by its own reading order rather than by where its children sit,
    // because that is what Tab moves in.
    Control* FocusNavigator::structuralSearch(
        KeyCode key,
        ScrollDirection entryEdge,
        OrientedRect rect,
        const Control* searchRoot,
        Control* current
        )
    {
        Control* result = nullptr;
        do
        {
            Control* nextSibling = spatialSearch(
                key,
                rect,
                searchRoot,
                current,
                SearchMethod::WrapLanes,
                SearchFilter::DirectChildren
            );
            current = nullptr;

            if (nextSibling)
            {
                Control* item = nextSibling->canTakeFocus()
                    ? nextSibling
                    : edgeItem(*nextSibling, entryEdge);
                if (item)
                    result = item;
                else
                {
                    // A container with nothing in it the focus can go to. The step carries on
                    // from where that container sits, so the next one along is tried.
                    current = nextSibling;
                    rect = orientedRect(current, key);
                }
            }
        } while (current && !result);

        return result;
    }

    Control* FocusNavigator::spatialSearch(
        KeyCode key, const OrientedRect& src,
        const Control* searchRoot, Control* excludeSubtree,
        SearchMethod searchMethod, SearchFilter filter, float lookAhead)
    {
        Control* bestCandidate = nullptr;
        Score bestScore;

        ControlTreeWalker controlTreeWalker{ m_form.content(), TraversalMode::Auto };
        controlTreeWalker.setLookAhead(m_form.scaler().scaled48 + lookAhead);
        controlTreeWalker.traverse([&](TraversalContext& context) {
            Control& candidate = context.control();

            // A control reaching past its viewport is judged by the part of it that is on
            // screen. The center of a container taller or wider than its viewport sits
            // somewhere the user cannot see, and it slides with every scroll, so scoring it
            // whole makes the container win or lose the match by its scroll position.
            // The look-ahead margin also visits controls that are entirely outside the
            // viewport; their viewport is empty, and their own bounds are what places them
            // ahead of the source.
            // A control that acts for a whole strip is placed by the strip instead, so it is
            // met from anywhere along it rather than from the corner it occupies.
            FloatRect candidateRect;
            if (const Control* extent = candidate.navigationExtent())
                candidateRect = extent->visibleRectInForm(m_form);
            else
            {
                candidateRect = context.viewport();
                if (candidateRect.empty())
                    candidateRect = context.controlBounds();
            }
            const OrientedRect candidateBounds = OrientedRect::orient(candidateRect, key);

            if (candidateBounds == src)
                return;
            // The form is where the search happens, not something it can land on. The walker
            // visits its own root before descending, and the root takes the focus like any other
            // ActiveContainer, so it arrives here as an ordinary candidate. Every search in use
            // keeps it out by other means - searchRoot excludes itself, and a DirectChildren
            // search wants a parent the root has not got - and this says so outright rather than
            // resting on that.
            if (isFormRoot(&candidate))
                return;
            // Ahead of the filter switch, so it holds for every search - canTakeFocus asks the
            // same question again for the focusable one, which costs a walk and keeps the two
            // filters from disagreeing about what a disabled subtree contains.
            if (!candidate.enabled(true))
                return;

            switch (filter)
            {
            case SearchFilter::Focusable:
                if (!candidate.canTakeFocus())
                    return;
                break;
            case SearchFilter::DirectChildren:
                break;
            }

            if (searchRoot)
            {
                if (filter == SearchFilter::DirectChildren)
                {
                    if (searchRoot != candidate.parent())
                        return;
                }
                else if (searchRoot && !searchRoot->containsNested(&candidate, CheckSelf::No))
                    return;
            }
            if (excludeSubtree && excludeSubtree->containsNested(&candidate, CheckSelf::Yes))
                return;

            Score currentScore;

            bool wrap = searchMethod == SearchMethod::WrapLanes;
            bool sameLane = candidateBounds.secondary.covers(src.secondary)
                || src.secondary.covers(candidateBounds.secondary);

            // 2. Is it in the next "Lane"? (e.g. Below if moving Right)
            bool nextLane = wrap && candidateBounds.secondary.start > src.secondary.start
                && candidateBounds.secondary.end > src.secondary.end;

            float primaryJump = candidateBounds.primary.center() - src.primary.center();
            if (sameLane)
            {
                // Must be strictly ahead in the primary direction
                if (primaryJump <= 0)
                    return;
                currentScore.inLane = true;
                currentScore.primary = primaryJump;
                currentScore.secondary = std::abs(candidateBounds.secondary.center() - src.secondary.center());
            }
            else if (nextLane)
            {
                // Wrapping logic
                currentScore.inLane = false;
                currentScore.primary = candidateBounds.secondary.start - src.secondary.end;
                currentScore.secondary = candidateBounds.primary.center();
            }
            else if (primaryJump > 0 && searchMethod == SearchMethod::Spatial)
            {
                currentScore.inLane = false;
                currentScore.primary = primaryJump;
                currentScore.secondary = std::abs(candidateBounds.secondary.center() - src.secondary.center());
            }
            else
                return; // Behind or in a previous lane

            if (currentScore.isBetterThan(bestScore)) {
                bestScore = currentScore;
                bestCandidate = &candidate;
            }

            });

        return bestCandidate;
    }

    OrientedRect FocusNavigator::orientedRect(Control* control, KeyCode key)
    {
        // The strip a control acts for is where a move away from it starts, the same rect a move
        // towards it is scored against.
        const Control* extent = control->navigationExtent();
        return OrientedRect::orient((extent ? extent : control)->visibleRectInForm(m_form), key);
    }
}
