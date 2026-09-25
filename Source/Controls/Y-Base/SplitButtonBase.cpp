module ClaFi.Controls.Base.SplitButtonBase;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // SplitButtonBase

    Control* SplitButtonBase::shownSecondaryPart() const
    {
        if (!m_secondaryPart || !m_secondaryPart->visible())
            return nullptr;

        return m_secondaryPart;
    }

    void SplitButtonBase::placeSecondaryPart(ScaledDimensions childArea)
    {
        Control* part = shownSecondaryPart();
        ScaledDimensions size = part->dimensions();
        if (secondaryEdge() == SecondaryEdge::Bottom)
        {
            setControlPlacement(
                *part,
                { (childArea.x - size.x) / 2.0f, childArea.y - size.y },
                size
            );
            return;
        }
        setControlPlacement(
            *part,
            { childArea.x - size.x, (childArea.y - size.y) / 2.0f },
            size
        );
    }

    void SplitButtonBase::secondaryClicked(ClickEvent&)
    {
        unreachable("a secondary part was pressed, and the control that made it answers nothing");
    }

    bool SplitButtonBase::pressedSecondary(const ClickEventBase& event) const
    {
        const Control* part = shownSecondaryPart();
        return part && part->containsNested(event.control);
    }

    ScaledDimensions SplitButtonBase::secondaryExtent(
        const FormContext& formContext, ScaledPadding padding, ScaledSpacing spacing) const
    {
        const Control* part = shownSecondaryPart();
        if (!part)
            return { 0.0f, 0.0f };

        // A part that reaches the control's edge stops at the child inset rather than at the edge
        // itself, so it takes over only the part of the padding lying beyond that inset. One that
        // stays inside the padding takes none of it.
        ScaledPadding taken = { 0.0f, 0.0f };
        if (secondaryReachesEdge())
            taken = padding - childInset(formContext, padding);

        // A content part shrunk to nothing leaves no spacing behind. A caller collapses one this
        // way to buy room back - see BrowserControl::alignContent - and a gap where it used to be
        // would defeat that. An edge part holds its axis either way, being structure.
        bool collapses = !secondaryReachesEdge();
        ScaledDimensions size = part->dimensions();
        if (secondaryEdge() == SecondaryEdge::Bottom)
        {
            if (collapses && !size.y)
                return { 0.0f, 0.0f };

            return { 0.0f, std::max(spacing.y + size.y - taken.y, 0.0f) };
        }
        if (collapses && !size.x)
            return { 0.0f, 0.0f };

        return { std::max(spacing.x + size.x - taken.x, 0.0f), 0.0f };
    }

    void SplitButtonBase::calculateChildren(FormBase& form)
    {
        // Places the selection indicator, which stays ButtonBase's business.
        //
        // ButtonBase does not chain to Control::calculateChildren, so nothing walks controls()
        // here and the part is sized on its own. Dropping this call leaves it at zero and the
        // extent computed from it at zero with it.
        ButtonBase::calculateChildren(form);
        Control* part = shownSecondaryPart();
        if (!part)
            return;

        calculateControl(*part, form);
    }

    ScaledDimensions SplitButtonBase::calculateContent(AlignEvent& event)
    {
        ScaledDimensions result = ButtonBase::calculateContent(event);
        const Control* part = shownSecondaryPart();
        if (!part)
            return result;

        ScaledDimensions extent = secondaryExtent(event.formContext(), event.padding, event.spacing);
        ScaledDimensions size = part->dimensions();
        if (secondaryEdge() == SecondaryEdge::Bottom)
        {
            result.y += extent.y;
            result.x = std::max(result.x, size.x - event.padding.x * 2.0f);
            return result;
        }
        result.x += extent.x;
        result.y = std::max(result.y, size.y - event.padding.y * 2.0f);
        return result;
    }

    void SplitButtonBase::alignContent(AlignEvent& event, ScaledPosition position, ScaledDimensions& dimensions)
    {
        if (!shownSecondaryPart())
        {
            ButtonBase::alignContent(event, position, dimensions);
            return;
        }
        // The base is given the box left over above the part, so that the selection indicator
        // centres in the half it belongs to instead of straddling the seam. Only the part's own
        // axis is taken away: narrowing x as well would read as the control having been squeezed,
        // and Control::alignContent would re-measure the text and drop the icon's width with it.
        ScaledDimensions offered = dimensions;
        ScaledDimensions extent = secondaryExtent(event.formContext(), event.padding, event.spacing);
        ScaledDimensions mainBox = { dimensions.x, std::max(dimensions.y - extent.y, 0.0f) };
        ButtonBase::alignContent(event, position, mainBox);
        dimensions.x = mainBox.x;
        dimensions.y = mainBox.y + extent.y;

        // Control::alignContent hands back the size of the text in place of the box whenever the
        // control was given less width than it calculated - which is exactly what a grid cell
        // does. For a plain button that only means the surface hugs its text. Here the part is
        // placed against this box, so it would be sized for one line of text and left sitting in a
        // corner of a control that is still full size. The part belongs to the control's box, so
        // keep that.
        dimensions.x = std::max(dimensions.x, offered.x);
        dimensions.y = std::max(dimensions.y, offered.y);

        // Child coordinates start at the child inset, not at the control's top left, so the part
        // is placed inside the box that inset leaves. Measuring it against the whole control would
        // push it past the far edge by the inset, and the parent's clip would trim it there.
        ScaledPadding inset = childInset(event.formContext(), event.padding);
        ScaledDimensions childArea = dimensions;
        childArea += event.padding * 2;
        childArea -= inset * 2;
        placeSecondaryPart(childArea);
    }

    void SplitButtonBase::adjustTextRect(AdjustTextRectEvent& event) const
    {
        ScaledDimensions extent = secondaryExtent(event.formContext(), event.padding, event.spacing);
        event.textBounds.right -= extent.x;
        event.textBounds.bottom -= extent.y;
        ButtonBase::adjustTextRect(event);
    }

    FloatRect SplitButtonBase::iconRect(const PaintEvent& event) const
    {
        FloatRect result = ButtonBase::iconRect(event);
        ScaledDimensions extent = secondaryExtent(event.formContext(), event.padding(), event.spacing());
        // The base anchored the icon against the whole content box, which still includes the part.
        // Pull it back by whatever the part took, on the axis the anchoring measures: a top left
        // icon is held against a corner no part ever moves, so it takes no correction at all.
        switch (viewMode())
        {
            case ButtonViewMode::IconOnly:
                result.offset(-extent.x / 2.0f, -extent.y / 2.0f);
                break;
            case ButtonViewMode::LeftIcon:
                result.offset(0.0f, -extent.y / 2.0f);
                break;
            case ButtonViewMode::TopCenterIcon:
                result.offset(-extent.x / 2.0f, 0.0f);
                break;
            case ButtonViewMode::BottomIcon:
                result.offset(0.0f, -extent.y);
                break;
            case ButtonViewMode::TextLabel:
            case ButtonViewMode::TopLeftIcon:
                break;
        }
        return result;
    }

    void SplitButtonBase::adjustPaint(AdjustPaintEvent& event)
    {
        // The property is read here rather than left to ButtonBase::adjustPaint, which this does
        // not reach.
        //
        // TODO: nothing above this runs for a split button - not ButtonBase, which is where
        // ShowSelectionOnSurface is answered, and not RichControl, which is where a ColorRules
        // property is applied. Calling the base would answer both, and would start dropping the
        // selected surface on every split button that has not asked for it - Combobox and
        // BreadCrumbBarItem have, Tab and SplitButton have not. Which of those two is the tab's
        // selected fill: the active rule it names itself, or a state the base would take away?
        if (showSurfaceAtRest() == ShowSurfaceAtRest::No)
            event.dropSurfaceAtRest();
    }

    void SplitButtonBase::pressDown(PressDownEvent& event)
    {
        if (!secondaryPressPropagates() && pressedSecondary(event))
        {
            // The part is its own target, and the press stops here rather than reading as a press
            // on the control. A tab must not become the current one because its close button was
            // pressed, and the container above would take it as one if the press reached it.
            event.stopPropagation();
            return;
        }
        ButtonBase::pressDown(event);
    }

    void SplitButtonBase::click(ClickEvent& event)
    {
        if (pressedSecondary(event))
        {
            // The part is never the primary action, so the click stops here instead of reaching
            // the control's own OnClick handlers.
            event.stopPropagation();
            secondaryClicked(event);
            return;
        }
        ButtonBase::click(event);
    }

} // of namespace ClaFi
