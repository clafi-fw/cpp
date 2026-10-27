module SeeDocs_App.Ui.BasesTree;

import SeeDocs_App.Ui.Icons;
import SeeDocs_App.Ui.PageText;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Controls.Base.Container;
import ClaFi.Controls.Base.DropdownControlBase;
import ClaFi.Controls.Menu;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;
import ClaFi.Core.Context.FormContext;

namespace SeeDocs_App::Ui
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    namespace
    {
        // What a base's list says while no type the surface has stands on the base.
        constexpr std::wstring_view k_noDerived = L"No derived types";

        // The page a crumb names: the link its words carry; empty for a base with no page.
        [[nodiscard]] std::wstring_view targetOf(const BaseNode& node)
        {
            for (const Run& run : node.text)
            {
                if (!run.link.empty())
                    return run.link;
            }
            return {};
        }
    }

    // BaseLinkEvent

    BaseLinkEvent::BaseLinkEvent(std::wstring linkTarget)
        :
        ::ClaFi::Event{},
        target{ std::move(linkTarget) }
    {
    }

    // BaseLinkContextPopupEvent

    BaseLinkContextPopupEvent::BaseLinkContextPopupEvent(Control& owner, std::wstring_view linkTarget,
        ContextPopupEvent& request)
        :
        ::ClaFi::Event{},
        crumb{ owner },
        target{ std::move(linkTarget) },
        popup{ request }
    {
    }

    // BaseCrumb

    BaseCrumb::BaseCrumb(const CreateParams& params, const BaseNode& node)
        :
        DropdownControlBase{
            params,
            Interactivity::Focusable,
            SecondaryPartPlacement::Left,
            UiElement::Button,
            ShowSelectionOnSurface::Yes,
            params.themeMetrics().button,
            //Padding{ 4.0f },
            // The bar narrows its crumbs itself, so only the height is held: a button's.
            MinSize{ 0.0f, params.themeMetrics().button.minSize.y },
            // One line, cut short where the bar narrows it.
            WordWrap::No,
            // Level with the middle of the run it stands in, a column of bases beside it included.
            VerticalAlign::Center
        },
        m_node{ node }
    {
    }

    ScaledSize BaseCrumb::calculateContent(AlignEvent& alignEvent)
    {
        m_moved = 0;
        return DropdownControlBase::calculateContent(alignEvent);
    }

    void BaseCrumb::getMainText(GetTextEvent& event) const
    {
        writeRuns(event.text, m_node.text);
    }

    // The types standing directly on the base, the one the way up came through marked and entered
    // on, so the keys start from where the bar is - as a path crumb's list marks the page the path
    // goes on to.
    void BaseCrumb::showDropdown(Control& initiator)
    {
        Menu menu{ initiator };
        menu.setPlaceHolderText(Text{ k_noDerived });
        for (const Type* type : m_node.derived)
        {
            const RowIcon icon = rowIconOf(*type);
            MenuItem& line = menu.add(
                Text{ type->name },
                OnDrawIcon{ [icon](DrawIconEvent& event) {
                    drawRowIcon(event, icon);
                } },
                OnClick{ [this, type](ClickEvent&) {
                    BaseLinkEvent linkEvent{ typeLink(*type) };
                    emitEvent(linkEvent);
                } }
            );
            if (type != m_node.from)
                continue;
            line.setShowSelectionOnSurface(ShowSelectionOnSurface::Yes);
            line.onGetState([](GetStateEvent& event) {
                event.state.selected = true;
            });
            line.setFocus();
            // Once the list has its window: the pass that measures it has no scroll range to spend
            // a scroll on - see BreadCrumbBarItem::showDropdown.
            menu.onAligned([&line, scrolled = false](FormAlignedEvent&) mutable {
                if (scrolled)
                    return;
                scrolled = true;
                line.scrollIntoView();
            });
        }
        // The list hangs from the mark that drops it, ending where the mark does - a path crumb's
        // list mirrored.
        menu.executeUnder(*secondaryPart(), FormPlacement::BottomRight);
    }

    void BaseCrumb::adjustPaint(AdjustPaintEvent& event)
    {
        DropdownControlBase::adjustPaint(event);
    }

    // A click on the crumb's face opens the page of the base it names. A click on the strip is a
    // nested one, and the base drops the list from it.
    void BaseCrumb::click(ClickEvent& event)
    {
        DropdownControlBase::click(event);
        const std::wstring_view target = targetOf(m_node);
        if (target.empty())
            return;
        BaseLinkEvent linkEvent{ std::wstring{ target } };
        emitEvent(linkEvent);
    }

    DropdownMarkTurn BaseCrumb::dropdownMarkTurn() const
    {
        return { ChevronTurn::Left, ChevronTurn::Down };
    }


    // BasesTree

    void BasesTree::setBases(const BaseNodes& value)
    {
        clear();
        m_bases = value;
        addCrumbs(m_bases);
    }

    void BasesTree::clear()
    {
        m_bases = {};
        //m_crumbs.clear();
        m_crumbsMap.clear();
        while (!controls().empty())
            controls().back()->deleteSelf();
    }

    ScaledSize BasesTree::calculateContent(AlignEvent& event)
    {
        return calculateBases(event, m_bases, {}, k_maxFloat);
    }

    void BasesTree::alignContent(AlignEvent& event, ScaledPosition scaledPosition, ScaledSize& newSize)
    {
        const ScaledSize calculatedSize = scaledSize() - event.scaledPadding() * 2.0f;
        if (calculatedSize.x <= newSize.x)
            return;
        newSize = calculateBases(event, m_bases, {}, newSize.x);
    }

    void BasesTree::paintSurface(PaintEvent& paintEvent)
    {
        ContainerBase::paintSurface(paintEvent);
    }

    ScaledSize BasesTree::calculateBases(AlignEvent& event, const BaseSpan& bases, ScaledPosition position, float wallX)
    {
        ScaledSize resultSize{};
        for (const BaseNode& node : bases)
        {
            if (resultSize.y)
                resultSize.y += event.scaledSpacingY();
            ScaledSize branchSize = calculateBranch(event, node, { position.x, position.y + resultSize.y }, wallX);
            resultSize.y += branchSize.y;
            resultSize.x = std::max(branchSize.x, resultSize.x);
        }
        return resultSize;
    }

    void BasesTree::offsetBases(AlignEvent& event, const BaseSpan& bases, FloatPoint value)
    {
        for (const BaseNode& node : bases)
        {
            const auto& crumb = m_crumbsMap[&node];
            ScaledPosition old = crumb->topLeft();
            setScaledPosition(*crumb, old + value);
            crumb->incMoved();
            offsetBases(event, node.bases, value);
        }
    }

    ScaledSize BasesTree::calculateBranch(AlignEvent& event, const BaseNode& node, ScaledPosition position, float wallX)
    {
        const auto& crumb = m_crumbsMap[&node];
        ScaledSize result{ crumb->scaledWidth(), crumb->scaledHeight() };
        
        setScaledPosition(*crumb, { position.x, position.y });

        {
            const float spacingX = event.scaledSpacingX();
            const ScaledPosition nextPosition = { position.x + spacingX + result.x, position.y };
            const ScaledSize basesSize = calculateBases(event, node.bases, nextPosition, wallX);
            bool isOverflow = false;
            if (basesSize.x)
            {
                result.x += spacingX;
                result.x += basesSize.x;
                float overflow = nextPosition.x + basesSize.x - wallX;
                if (overflow > 0.0f)
                {
                    isOverflow = true;
                    constexpr float k_indent = 32.0f;
                    const float indent = event.scale(k_indent);
                    overflow = std::min(overflow, std::min(basesSize.x, crumb->scaledWidth() - indent));
                    offsetBases(event, node.bases, { -overflow, result.y + event.scaledSpacingY() });
                    result.x -= (overflow);
                }
            }
            if (isOverflow)
            {
                if (basesSize.y)
                    result.y += event.scaledSpacingY();
                result.y += basesSize.y;
            }
            else
                result.y = std::max(basesSize.y, result.y);
        }

        return result;
    }

    void BasesTree::addCrumbs(BaseSpan bases)
    {
        for (const BaseNode& base : bases)
        {
            BaseCrumb& newCrumb = add<BaseCrumb>(base);

            //m_crumbs.push_back(&newCrumb);

            m_crumbsMap.emplace(&base, &newCrumb);

            newCrumb.onContextPopup([this, &newCrumb](ContextPopupEvent& event) {
                // event.control could also be the dropdown button - it's "onNestedContextPopup" actually
                //BaseCrumb* crumb = static_cast<BaseCrumb*>(event.control);
                std::wstring_view target = targetOf(newCrumb.node());
                if (target.empty())
                    return;
                BaseLinkContextPopupEvent linkEvent{ newCrumb, target, event };
                emitEvent(linkEvent);
                });
            newCrumb.onBaseLink([this](BaseLinkEvent& event) {
                // re-emitting the crumb's event
                emitEvent(event);
                });
            addCrumbs(base.bases);
        }
    }

}
