module ClaFi.Controls.StackView;

import ClaFi.Controls.StackPanel;
import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Controls.Base.Container;
import ClaFi.StdActions;
import ClaFi.Core.Foundation;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // ItemsViewSelection

    ItemsViewSelection::ItemsViewSelection(StackView& owner)
        :
        m_owner{ owner }
    {
    }

    bool ItemsViewSelection::contains(const Control* value) const
    {
        Control* ptr = const_cast<Control*>(value);
        return m_set.contains(ptr);
    }

    void ItemsViewSelection::changed() const
    {
        m_owner.doSelectionChanged();
    }

    bool ItemsViewSelection::tryAdd(Control& value)
    {
        if (m_set.contains(&value))
            return false;

        m_set.insert(&value);
        return true;
    }

    bool ItemsViewSelection::tryRemove(Control& value)
    {
        return m_set.erase(&value);
    }

    bool ItemsViewSelection::tryClear()
    {
        if (empty())
            return false;

        m_set.clear();
        return true;
    }

    // SelectionChangeEvent

    SelectionChangeEvent::SelectionChangeEvent(StackView& control)
        :
        control{ control }
    {
    }

    // CanSelectItemEvent

    CanSelectItemEvent::CanSelectItemEvent(StackView& itemsView,
        const Control& item, bool canSelect)
        :
        itemsView{ itemsView },
        item{ item },
        canSelect{ canSelect }
    {
    }

    // StackView

    void StackView::toggleItemSelection(Control* value, bool keepSelection)
    {
        if (isNotNullAndCannotBeCurrent(value))
            return;

        bool isRemoved;
        bool isAdded = false;
        if (keepSelection)
        {
            if (!((isRemoved = value && m_selection.tryRemove(*value))))
                isAdded = tryAddToSelection(value);

            if (isRemoved && !isAdded)
            {
                // removing check by clicking on it
                value->invalidateState();
            }
        }
        else
        {
            isRemoved = clearSelection();
            isAdded = tryAddToSelection(value);
        }
        if (isRemoved || isAdded)
            doSelectionChanged();
    }

    // Both ends and everything between them in document order. The whole subtree is walked
    // rather than the direct children: a view's selection spreads over the panels nested
    // inside it, and the items a range runs between commonly live in one of those.
    //
    // ClipMode::None because a range is not a geometry question. The walker's other modes
    // start each child range at firstChildInViewport and end it at isViewportEnd, and a range
    // reaches whatever lies between its ends however far the view is scrolled away from it.
    //
    // The set is replaced rather than added to, so a range pulled back in on itself gives up
    // the items it no longer covers. keepSelection starts the range from the set the anchor
    // was recorded with, which is what lets several disjoint ranges stand at once.
    void StackView::selectRange(Control* anchor, Control* item, bool keepSelection)
    {
        if (!anchor || !item)
            return;

        if (keepSelection)
            s_tempSelectionBuffer = m_anchorSelection;
        else
            s_tempSelectionBuffer.clear();

        // How many of the two ends the walk has passed. One control named as both ends is a
        // range of one, and it opens and closes on the same visit.
        int endsPassed = 0;
        ControlTreeWalker controlTree{ *this, TraversalMode::Manual };
        controlTree.setClipMode(ClipMode::None);
        controlTree.traverse(
            [&](TraversalContext& context) {
                if (endsPassed == 2)
                    return;

                Control& control = context.control();
                if (&control == anchor || &control == item)
                    endsPassed += anchor == item ? 2 : 1;

                if (endsPassed > 0 && canSelectItem(&control))
                    s_tempSelectionBuffer.insert(&control);

                // The closing end keeps its own children out of the range: they come after it
                // in the order the range is measured in.
                if (endsPassed == 2)
                    return;

                context.traverseChildren();
            }
        );

        if (replaceSelection(s_tempSelectionBuffer))
            doSelectionChanged();
    }

    bool StackView::canSelectItem(Control* value)
    {
        if (!value)
            return false;

        CanSelectItemEvent event{
            *this,
            *value,
            defaultCanSelectItem(*value)
        };
        emitEvent(event);
        return event.canSelect;
    }

    // Only a view holding a set has one to add to, and what it adds are the controls the user
    // acts on - the same shape of thing the current item lands on, said here in its own right
    // rather than borrowed, because the two questions are answered apart.
    bool StackView::defaultCanSelectItem(Control& value)
    {
        return isMultiSelect() && value.interactivity() == Interactivity::Focusable;
    }

    // Nothing picked needs no branch of its own. A range to nowhere is no range, adding
    // nothing adds nothing, and replacing the set with nothing empties it. Which is also how a
    // stray press on a caption leaves the selection alone while a modifier is down: the user is
    // still building it, and the anchor keeps its place for the rest of what they build.
    void StackView::currentItemPicked(Control* item, Control* previousItem,
        KeyModifiers modifiers, bool applyGesture)
    {
        if (!isMultiSelect())
            return;

        // The pass that reads the gesture whole has not run yet, so the set is left as it
        // stands and only the current item has moved. The anchor follows it, because the user
        // is standing there and the next range starts from where they stand - but a Shift is
        // MEASURED from the anchor, so it must not move the thing it is measured from.
        // Nothing picked is nothing to stand on, and the anchor keeps the place it was given.
        if (!applyGesture)
        {
            if (item && !modifiers.shift)
                recordSelectionAnchor(item);
            return;
        }

        // A range runs from the anchor, not from the item the current one has just left. The
        // keyboard moves the current item on every arrow, so measuring from it would make each
        // press its own two-item range and the selection could only ever grow.
        if (modifiers.shift)
        {
            if (!m_selectionAnchor)
                recordSelectionAnchor(previousItem);
            selectRange(m_selectionAnchor, item, modifiers.ctrl);
            return;
        }

        toggleItemSelection(item, modifiers.ctrl);
        recordSelectionAnchor(item);
    }

    bool StackView::isItemSelected(const Control& item) const
    {
        if (!isMultiSelect())
            return StackPanel::isItemSelected(item);

        return m_selection.contains(&item);
    }

    void StackView::adjustNestedControlVisualState(const Control& control, VisualState& state) const
    {
        StackPanel::adjustNestedControlVisualState(control, state);

        // Only a multi-select view has anything to say here. Everywhere else the focused item
        // is the selected item and is already drawn as one, so marking it current on top of
        // that would put a ring around every open tab and every chosen combobox entry.
        if (isMultiSelect() && currentItem() == &control)
            state.current = true;
    }

    void StackView::nestedControlDeleted(Control* item)
    {
        StackPanel::nestedControlDeleted(item);
        if (m_selectionAnchor == item)
            m_selectionAnchor = nullptr;
        m_anchorSelection.erase(item);
        if (m_selection.tryRemove(*item))
            doSelectionChanged();
    }

    void StackView::drag(DragEvent& event)
    {
        StackPanel::drag(event);

        if (!isMultiSelect())
            return;

        // A band already under way settled both questions when it began: it is past the
        // threshold, and it is a band rather than a carry.
        if (!m_selectionOverlayRect.has_value())
        {
            if (event.unscaledDistance() < k_dragThreshold)
                return;

            // What the press landed on says which drag this is, and it says the same on every
            // move of the same drag, so the answer needs no keeping.
            if (m_dragMode == DragMode::EasyDrag && pressedItem(&event.control()))
                return dragSelection(event);
        }

        // TODO: move the modifiers onto the event and drop the global keyModifiers().
        const KeyModifiers modifiers = Platform::keyModifiers();

        if (!m_selectionOverlayRect.has_value() && (modifiers.shift || modifiers.ctrl))
            s_initialSelectionBuffer = m_selection.m_set;

        FloatRect bandRect = event.selectionRect();
        // The region the band held, then the region it holds now. The second call comes after the
        // walk, because the walk is what names the items the band reaches and those are part of
        // the region.
        invalidateSelectionOverlay();
        m_selectionOverlayRect = bandRect;
        selectItemsInRect(bandRect, modifiers);
        invalidateSelectionOverlay();

        // Don't:
        // params.handled = true;
        // We do not touch handled here, so the event is passed to the parent scrollbox,
        // which triggers autoscrolling when an object dragged close to its boundaries

        // lockHoveredControl() prevents stucking in autoscrolling state
        // when the mouse button pressed on the button, but releases somewhere else
        event.lockHoveredControl();
    }

    void StackView::pressUp(PressUpEvent& event)
    {
        StackPanel::pressUp(event);
        if (m_selectionOverlayRect)
        {
            invalidateSelectionOverlay();
            m_selectionOverlayRect.reset();
            m_selectionOverlayItemsRect.clear();
            // The band is the whole gesture. A pointer that wandered back to the control it
            // pressed on would otherwise raise a click there, and the click pass is where a
            // modifier is read - so a Shift band would end by replacing itself with a range.
            event.handled.preventClick = true;
            // Don't:
            // params.handled = true;
            // We do not touching handled here, so the event is passed to the parent scrollbox,
            // it need to process it to stop auto-scrolling
            event.scrollIntoView = false;
        }
    }

    void StackView::paintChildren(PaintEvent& event)
    {
        StackPanel::paintChildren(event);
        // The band is the view's own paint, laid over the rows and under whatever stands in front
        // of them, and it is translucent - drawn a second time it would come out at twice its
        // opacity. Two stages can reach here, and it belongs to neither of them:
        //
        // - paintsSelf() is false where this view is itself an overlay control of its parent. Its
        //   children are reached in the parent's standard pass so that anything nested under them
        //   can paint, while the view itself paints in the overlay pass.
        // - overlayStage() is true on the second of the two passes a view holding overlay controls
        //   runs over its own children. paintsSelf() cannot tell those apart - it is answered once
        //   per paint() and both passes are inside it.
        if (event.paintsSelf() && !event.overlayStage() && m_selectionOverlayRect.has_value())
        {
            // Raised off the view's own surface, so it rises the way that surface leaves room
            // for rather than the way the theme as a whole does.
            Hsl hsl{ event.surfaceHsl()};
            event.bakedColors().rule(UiElement::Accent).applyTo(hsl, 1.0f, event.lightness());

            const FloatRect& rect = m_selectionOverlayRect.value();
            event.canvas().fillRectangle(rect, hsl.toColor().withOpacity(0.5f));
            const float strokeWidth = event.scaledStrokeWidth(Thickness::Hairline);
            event.canvas().drawRectangle(
                rect,
                hsl.toColor(),
                strokeWidth
            );
        }
    }

    void StackView::doSelectionChanged()
    {
        selectionChanged();
        SelectionChangeEvent event{ *this };
        emitEvent(event);
    }

    void StackView::recordSelectionAnchor(Control* item)
    {
        m_selectionAnchor = item;
        m_anchorSelection = m_selection.m_set;
    }

    Control* StackView::pressedItem(Control* control)
    {
        return itemAt(control);
    }

    void StackView::selectItemsInRect(FloatRect& selectionRect, KeyModifiers modifiers)
    {
        if (modifiers.shift || modifiers.ctrl)
            s_tempSelectionBuffer = s_initialSelectionBuffer;
        else
            s_tempSelectionBuffer.clear();

        // The items the band reaches are collected beside the selection, off this one walk: it
        // already visits exactly them, and a second walk for the invalidation would be that cost
        // again on every pointer move.
        m_selectionOverlayItemsRect.clear();

        FloatRect expandedClipRect = selectionRect;
        //expandedClipRect.inflate(form().scaler().scaled16);
        //expandedClipRect.offset(boundsInForm().topLeft());
        ControlTreeWalker controlTree{ *this, TraversalMode::Manual, &expandedClipRect };
        controlTree.setClipMode(ClipMode::SystemClipOnly);
        controlTree.traverse(
            [this, modifiers](TraversalContext& context) {
                Control& control = context.control();
                if (containsNested(control, CheckSelf::Yes))
                    if (canSelectItem(&control))
                    {
                        // Taken where an item is recognised rather than where one is selected:
                        // the band paints over an item Ctrl takes back out of the set as much as
                        // over one it puts in, so both have to be repainted.
                        m_selectionOverlayItemsRect.unionWith(context.controlBounds());
                        if (modifiers.ctrl)
                        {
                            if (s_tempSelectionBuffer.contains(&control))
                                s_tempSelectionBuffer.erase(&control);
                            else
                                s_tempSelectionBuffer.insert(&control);
                        }
                        else
                            s_tempSelectionBuffer.insert(&control);
                    }

                context.traverseChildren();
            }
        );
        if (replaceSelection(s_tempSelectionBuffer))
            doSelectionChanged();
    }

    // Only the difference has anything new to paint. A member that was in the set before and is
    // in it now looks the same, and invalidating it walks its state leaf to root and starts an
    // animation on every state slot it has - a rubber band rebuilds the set on every pointer
    // move, so paying that for the whole selection each time is the selection's cost per move
    // rather than the difference's.
    //
    // The set is swapped before either walk, so each member is asked for its state with the set
    // already holding the answer: a leaving one is out of it, a joining one is in it.
    bool StackView::replaceSelection(const ItemsViewSelection::UnorderedSet& newSelection)
    {
        if (m_selection.m_set == newSelection)
            return false;

        s_previousSelectionBuffer = std::move(m_selection.m_set);
        m_selection.m_set = newSelection;

        for (Control* item : s_previousSelectionBuffer)
        {
            if (!m_selection.contains(item))
                item->invalidateState();
        }
        for (Control* item : m_selection)
        {
            if (!s_previousSelectionBuffer.contains(item))
                item->invalidateState();
        }
        return true;
    }

    bool StackView::clearSelection()
    {
        return replaceSelection(s_emptySelectionBuffer);
    }

    bool StackView::tryAddToSelection(Control* value)
    {
        if (canSelectItem(value))
        {
            if (m_selection.tryAdd(*value))
            {
                value->invalidateState();
                return true;
            }
        }
        return false;
    }

    // Unclipped: a selection is about what the view holds, not about what is in sight. The set is
    // built whole and handed to replaceSelection, so an item already in it is left alone - see the
    // note there for what invalidating the whole selection would cost.
    void StackView::selectAll()
    {
        s_tempSelectionBuffer.clear();
        ControlTreeWalker controlTree{ *this, TraversalMode::Manual };
        controlTree.setClipMode(ClipMode::None);
        controlTree.traverse(
            [this](TraversalContext& context) {
                Control& control = context.control();
                if (canSelectItem(&control))
                    s_tempSelectionBuffer.insert(&control);

                context.traverseChildren();
            }
        );

        if (replaceSelection(s_tempSelectionBuffer))
            doSelectionChanged();
    }

    // Stops at the first one, so the answer costs the same on a view of ten items and one of ten
    // million. That is what the question is chosen to be: "is anything still unselected" would
    // read better on a menu, and it walks every item once everything is selected - which is
    // exactly the state it would be asked in.
    bool StackView::hasSelectableItem()
    {
        bool result = false;
        ControlTreeWalker controlTree{ *this, TraversalMode::Manual };
        controlTree.setClipMode(ClipMode::None);
        controlTree.traverse(
            [this, &result](TraversalContext& context) {
                if (result)
                    return;

                if (canSelectItem(&context.control()))
                {
                    result = true;
                    return;
                }

                context.traverseChildren();
            }
        );
        return result;
    }

    void StackView::connectSelectionActions()
    {
        // Claiming says this view is what the action acts on; what it claims says whether it can
        // act right now. A view holding no set is not the subject at all, so it says nothing and
        // the walk carries on up - something above it may answer the same key for something else.
        // That is the difference from a view that holds a set and has nothing in it to select:
        // that one claims and reports disabled, because the command is still about it.
        //
        // What is claimed turns on the items the view holds, not on which of them are selected, so
        // selecting changes nothing here and doSelectionChanged has no invalidateState to make.
        onGetActionState([this](GetActionStateEvent& event) {
            if (!isMultiSelect())
                return;

            if (&event.action == &StdActions::selectAll)
                event.claim({ .enabled = hasSelectableItem() });
        });

        onActionClick([this](ActionClickEvent& event) {
            if (&event.action == &StdActions::selectAll)
                selectAll();
        });
    }

    void StackView::invalidateSelectionOverlay()
    {
        if (!m_selectionOverlayRect.has_value())
            return;

        // WHOLE ITEMS, NEVER A STRIP ACROSS ONE. An item is painted through its Z scale, about
        // its own centre, so what it puts on the screen is not the rect it occupies: a child
        // sitting near a corner is drawn a pixel or so in from where it was laid out. A strip
        // crossing part of an item repaints content the strip was not measured against, and the
        // rows that content vacated keep whatever the last frame left in them - which accumulates,
        // because the surface retains its contents between frames.
        //
        // An item asked for whole covers everything it can paint, because every press scale is a
        // contraction: pressRestScale and pressHeldScale are both below 1 and hover interpolates
        // to exactly 1. A scale above 1 would put paint outside the bounds and break this.
        //
        // The band's own rect is the whole of what the band draws - a stroke lies inside the rect
        // it is given.
        FloatRect tmpRect = m_selectionOverlayRect.value();
        tmpRect.unionWith(m_selectionOverlayItemsRect);
        form().invalidateRect(tmpRect);
    }

}
