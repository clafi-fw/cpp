module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Stack;

import ClaFi.Controls.Base.StackBase;
import ClaFi.Controls.Base.StackPanelBase;

import ClaFi.Core.AppTheme_AnimationSlots;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    export class StackViewBase;

    // Which item a stack raises its preview for. See Item-Containers
    export enum class PreviewMode
    {
        Focus,   // the item the container has settled on, whichever gesture moved it
        Hover    // the item under the pointer, and the current item as a key moves it
    };

    // The item a stack has settled on, before the user has picked anything. See Item-Containers
    export struct PreviewEvent : public Event
    {
        PreviewEvent(StackViewBase& itemsView, Control& item);
        StackViewBase& itemsView;   // the stack the preview belongs to
        Control& item;      // the item being looked at, never null
    };

    // Which way a lane runs, and whether the stack wraps into further lanes.
    export enum class Orientation
    {
        HorizontalWrap,
        VerticalWrap,
        Horizontal,
        Vertical,
    };

    // How a lane's count is read. See Item-Containers
    export enum class LaneSizing
    {
        Exact,  // every lane takes the count, and the remainder stands in a short last lane
        UpTo    // the count is a ceiling, and the items are spread over the fewest lanes it allows
    };

    // How many items one lane takes, and how that count is read.
    export struct LaneSize {
        std::size_t value{ k_maxSize };
        LaneSizing sizing{ LaneSizing::Exact };
    };

    // How a lane sizes the items in it. See Item-Containers
    export enum class ItemSizing
    {
        Natural,   // every item is the size it calculated for itself
        Equal      // the lane divided evenly, every item handed the same share of it
    };

    // Whether the user may carry an item to another place among its siblings. See Item-Containers
    export enum class ItemsOrder
    {
        Fixed,
        Rearrangeable
    };

    export class StackPanelBase;

    // May this item be carried to another place? See Item-Containers
    export struct CanMoveItemEvent : public Event
    {
        CanMoveItemEvent(StackPanelBase&, const Control&, bool canMove);
        StackPanelBase& itemsView;   // the stack asking
        const Control& item;         // the item the answer is about
        bool canMove;                // the answer, as the stack gave it before any handler
    };

    // Items standing at another place among their siblings now. See Item-Containers
    export struct ItemsMoveEvent : public Event
    {
        ItemsMoveEvent(StackPanelBase&, std::span<Control* const> items, std::size_t to);
        StackPanelBase& itemsView;         // the stack the items belong to
        std::span<Control* const> items;   // in the order they stand, which they kept
        std::size_t to;                    // the place the first of them stands at
    };

    // A container that places its items in lanes. See Item-Containers
    export class StackPanelBase : public StackBase
    {
    protected:
        using StackBase::add;
        using StackBase::reserve;
        using StackBase::controls;
        using StackBase::clearControls;
    public:
        template <typename... Args>
        explicit StackPanelBase(const CreateParams&, Args&&...);
    public:
        // Which way a lane runs, and whether the stack wraps into further lanes.
        DECLARE_WRITABLE_PROPERTY(Orientation, orientation, setOrientation, Orientation::VerticalWrap)
        // How many items one lane takes, and how that count is read.
        DECLARE_PROPERTY(LaneSize, laneSize, LaneSize{})
        // How a lane sizes the items in it.
        DECLARE_WRITABLE_PROPERTY(ItemSizing, itemSizing, setItemSizing, ItemSizing::Natural)
        // Whether the user may carry an item to another place among its siblings.
        DECLARE_PROPERTY(ItemsOrder, itemsOrder, ItemsOrder::Fixed)
    public:
        // Asked before an item is carried.
        DECLARE_EVENT(CanMoveItemEvent, OnCanMoveItem, onCanMoveItem)
        // Raised after items were put at another place, by a carry or by moveItems.
        DECLARE_EVENT(ItemsMoveEvent, OnItemsMove, onItemsMove)
    public:
        std::wstring_view diagnosticText() const override { return L"Stack"; }
        void setOrientation(Orientation);
        void setItemSizing(ItemSizing);
        TraversalOrder traversalOrder() const override;
        // Puts items, given in their order, at `to` and after. See Item-Containers
        void moveItems(std::span<Control* const> items, std::size_t to);
    protected:
        ScaledDimensions calculateContent(AlignEvent&) override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&) override;
        // A row reads its items' widths off them, unless one fills the lane.
        [[nodiscard]] bool isChildWidthGiven(const Control& child) const override
            { return !lanesRunAcross() || child.fillsLane(); }
        // A column reads its items' heights off them unless one fills the lane.
        [[nodiscard]] bool isChildHeightGiven(const Control& child) const override
            { return lanesRunAcross() || child.fillsLane(); }
        NavigationWrap navigationWrap() const override;
        // The answer after the handlers have had it.
        [[nodiscard]] bool canMoveItem(Control&);
        // The answer before any handler sees it: a visible item of a rearrangeable stack.
        virtual bool defaultCanMoveItem(Control&);
        // What a drag from this item carries, in their order; nothing says the drag is no carry.
        [[nodiscard]] virtual std::vector<Control*> carriedItems(Control& item);
        virtual void itemsMoved(ItemsMoveEvent&) {}
        void nestedDrag(DragEvent&) override;
        void nestedPressUp(PressUpEvent&) override;
        void nestedKeyDown(KeyDownEvent&) override;
        void nestedControlDeleted(Control*) override;
        [[nodiscard]] FloatPoint overlayChildOffset(const Control&) const override;
    private:
        // A carry under way: what the pointer holds, the run of siblings it may land among, and
        // the order to put back if it is cancelled. See Item-Containers
        struct Carry
        {
            std::vector<Control*> items;      // in their order; one block once gathered
            std::vector<Control*> lifted;     // the items this stack lists as overlays
            std::vector<Control*> original;   // the run as it stood before the carry
            Control* anchor;                  // the item the press landed on
            std::size_t first;                // the run's first child index
            std::size_t last;                 // the run's last child index
            float grab;                       // the pointer past the anchor's leading edge
            float spacing;                    // between two neighbours along the lane
            FloatPoint pointer;               // where the pointer last was, in form space
            bool cancelled;                   // Escape put the run back; ends with the press
        };
    private:
        void beginCarry(Control& item, DragEvent&);
        void continueCarry(DragEvent&);
        // The press ended: the run stands as the carry left it, and ItemsMoveEvent says so.
        void endCarry();
        void dropCarry();   // lets the items go, with no event
        // The block put at `to`, the run's other items keeping their order around it.
        void placeBlock(std::size_t to);
        // The children from `from` on put in this order, moving only those standing elsewhere.
        void placeChildren(std::span<Control* const> order, std::size_t from);
        // Where the block lands among the run's other items, drawn with this leading edge.
        [[nodiscard]] std::size_t carryIndex(float drawnLeading) const;
        // How far along the lane the block is drawn from where it is laid out.
        [[nodiscard]] float carriedBlockOffset() const;
        [[nodiscard]] bool isCarried(const Control&) const;
        [[nodiscard]] std::size_t indexOf(const Control&) const;
        [[nodiscard]] float leadingOf(const Control& child) const
            { return mainOf(child.topLeft()); }
        [[nodiscard]] float extentOf(const Control& child) const
            { return mainOf(child.dimensions()); }
    private:
        // What one lane holds: how many items it places, and the largest of them along the lane.
        // The two are read in one walk because an even lane needs both - the count divides it,
        // and the largest is what every share has to hold.
        struct LaneFill
        {
            std::size_t count;
            float largest;
        };
        // One lane of a wrapping stack as it was cut: the items on it, how far they reach along
        // it, how thick the largest of them makes it, and whether anything on it asked for what
        // the lane has over.
        struct LaneCut
        {
            ControlSpan::iterator begin;
            ControlSpan::iterator end;
            float main;
            float cross;
            std::size_t count;
            std::size_t takers;
        };
    private:
        // **Every wrapping stack, both ways round.**
        void alignIntoLanes(AlignEvent&, ScaledPosition, ScaledDimensions& contentDimensions);
        // The lane beginning at begin: as many items as the lane takes, as many as fit the length
        // it is given, and never fewer than one.
        [[nodiscard]] LaneCut nextLane(ControlSpan::iterator begin, const AlignEvent&,
            float limitMain, std::size_t perLane);
        // How many equal shares of this size fit the length given: never more than the lane takes,
        // and never fewer than one. See Item-Containers
        [[nodiscard]] std::size_t sharesThatFit(float largest, float spacing, float limitMain) const;
        // A lane with what the **stack** was given over what it measured divided among the items on
        // it - or among the items that asked for it, where any did. See Item-Containers
        void alignSharedLane(AlignEvent&, const LaneCut&, ScaledPosition, float laneSurplus,
            float laneCross, ScaledDimensions& contentDimensions);
        // One item in the part of a lane it was given.
        void placeInLane(AlignEvent&, Control&, ScaledPosition lanePosition, float from,
            float extent, float laneCross);
        // Whether a lane of this stack runs across the panel - a row - or down it - a column.
        [[nodiscard]] bool lanesRunAcross() const
        {
            return m_orientation == Orientation::HorizontalWrap
                || m_orientation == Orientation::Horizontal;
        }
        // Whether this stack was told to fill the way its lanes run.
        [[nodiscard]] bool fillsMain() const
        {
            return lanesRunAcross()
                ? horizontalAlign() == HorizontalAlign::Fill
                : verticalAlign() == VerticalAlign::Fill;
        }
        // **Main is the way a lane runs, cross the way the lanes stack.** Every extent a wrapping
        // stack reads goes through these, which is what lets one routine serve both.
        [[nodiscard]] float& mainOf(FloatPoint& value) const
            { return lanesRunAcross() ? value.x : value.y; }
        [[nodiscard]] const float& mainOf(const FloatPoint& value) const
            { return lanesRunAcross() ? value.x : value.y; }
        [[nodiscard]] float& crossOf(FloatPoint& value) const
            { return lanesRunAcross() ? value.y : value.x; }
        [[nodiscard]] const float& crossOf(const FloatPoint& value) const
            { return lanesRunAcross() ? value.y : value.x; }
        [[nodiscard]] FloatPoint lanePoint(float main, float cross) const
            { return lanesRunAcross() ? FloatPoint{ main, cross } : FloatPoint{ cross, main }; }
        // Whether the length the lanes run along is the host's to give.
        [[nodiscard]] bool isLaneLengthGivenFromOutside() const
            { return lanesRunAcross() ? isWidthGivenFromOutside() : isHeightGivenFromOutside(); }
        // The remembered length at the scale measuring now, or no limit.
        [[nodiscard]] float wrapLengthLimit(const AlignEvent&) const;
        // Carries the length this pass broke the lanes at to the next measure.
        void rememberWrapLength(AlignEvent&, float limitLength);
        // How many items one lane of this stack takes, which is what its count amounts to
        // read either way. Asked by the measure and the align alike, so that the lanes one
        // cuts are the lanes the other measured. See Item-Containers
        [[nodiscard]] std::size_t itemsPerLane() const;
        // How many of the items stand.
        [[nodiscard]] std::size_t visibleItems() const;
        [[nodiscard]] bool dividesEvenly() const;
        [[nodiscard]] bool hasFillingItem() const;
        [[nodiscard]] LaneFill laneFill(float (Control::*extent)() const) const;
        // The length an even lane needs: a share for every item, each as long as the largest,
        // with the gaps between them.
        [[nodiscard]] static float evenLaneLength(const LaneFill&, float spacing);
        // What a granted extent has over what stands in it, and nothing where that is under a
        // pixel. See Item-Containers
        [[nodiscard]] static float surplusOver(float granted, float taken);
        // Places the items along a lane already sized to hold them, each getting the same share.
        // Every edge is computed from the item's index rather than accumulated, so the shares stay
        // equal to the pixel and the last one ends exactly on the lane's own edge.
        void alignEvenLane(AlignEvent&, ControlSpan::iterator laneBegin,
            ControlSpan::iterator laneEnd, std::size_t shares, ScaledPosition,
            float laneMain, float laneCross, ScaledDimensions& contentDimensions);
    private:
        // Under this two extents are the same extent: what the align laid out is compared with
        // what the measure answered, and a pass asked for on floating-point noise is a pass
        // every frame.
        static constexpr float k_wrapEpsilon{ 0.5f };
        // Height of the tallest row, or width of the widest column, as measured by the
        // last wrapping align. Seeds to k_maxFloat so that a panel queried before it has
        // ever been aligned reports a lane nothing can precede, and culls nothing.
        float m_maxLaneExtent{ k_maxFloat };
        float m_wrapLengthInDesign{};   // the length the lanes were last broken at, in design units
        std::optional<Carry> m_carry{};
    };

    export class StackPanel : public StackPanelBase
    {
        using StackPanelBase::StackPanelBase;
    public:
        using StackPanelBase::add;
        using StackPanelBase::reserve;
        using StackPanelBase::controls;
        using StackPanelBase::clearControls;
    };

    export class StackViewBase : public StackPanelBase
    {
    public:
        using StackPanelBase::add;
        using StackPanelBase::reserve;
        using StackPanelBase::controls;
        using StackPanelBase::clearControls;
    public:
        template <typename... Args>
        explicit StackViewBase(const CreateParams&, Args&&...);
    public:
        // Which item the stack raises its preview for.
        DECLARE_WRITABLE_PROPERTY(PreviewMode, previewMode, setPreviewMode, PreviewMode::Focus)
    public:
        // Raised once the preview has settled on an item.
        DECLARE_EVENT(PreviewEvent, OnPreview, onPreview)
    public:
        void setPreviewMode(PreviewMode value) { m_previewMode = value; }
        // The item the preview has settled on, by whichever reading PreviewMode names. Null until
        // something has been previewed or recorded. Between an item being reached and the delay
        // elapsing this is the item about to be raised, which is the only moment it says more
        // than the last PreviewEvent did.
        [[nodiscard]] Control* previewItem() const { return m_previewItem; }
    protected:
        virtual void previewItemChanged(PreviewEvent&);
        // Says the preview already stands on this item, without raising one and without the wait.
        // For a container built around an item whose preview is what the caller is already
        // showing - a dropdown opened on its selected item - so that reaching that item asks for
        // nothing. Only the item: whatever a stack builds around it is the caller's to set.
        void recordPreviewItem(Control& value) { m_previewItem = &value; }
        //
        void nestedControlHovered(Control* hovered) override;
        void hoverLeave() override;
        void currentItemChanged(CurrentItemChangeEvent&) override;
        void nestedControlDeleted(Control*) override;
    private:
        void onPreviewTimer();
        // Aims the preview at an item and waits to see whether it stays there. An item already
        // aimed at costs nothing, which is what makes a held key one preview rather than one per
        // repeat.
        void startPreviewTimer(Control&);
    private:
        // Long enough that a pointer crossing the stack previews nothing it merely passes over,
        // short enough to read as the answer to stopping on an item.
        static constexpr MilliSeconds k_previewDelay{ 110 };
        // Connected in the constructor rather than handed its handler here. The constructor is a
        // template, so a default member initializer is instantiated in whatever translation unit
        // builds a Stack, and OnEvent's deduction does not survive the trip - onTick names
        // the event and deduces nothing.
        UiTimer m_previewTimer;
        Control* m_previewItem{ nullptr };
    };

    export class SingleSelectionView : public StackViewBase
    {
        using StackViewBase::StackViewBase;
    };


//-----------------------------------------------------------------------------


    // PreviewEvent

    PreviewEvent::PreviewEvent(StackViewBase& itemsView, Control& item)
        :
        itemsView{ itemsView },
        item{ item }
    {
    }

    // CanMoveItemEvent

    CanMoveItemEvent::CanMoveItemEvent(StackPanelBase& itemsView, const Control& item,
        const bool canMove)
        :
        itemsView{ itemsView },
        item{ item },
        canMove{ canMove }
    {
    }

    // ItemsMoveEvent

    ItemsMoveEvent::ItemsMoveEvent(StackPanelBase& itemsView, const std::span<Control* const> items,
        const std::size_t to)
        :
        itemsView{ itemsView },
        items{ items },
        to{ to }
    {
    }

    // StackPanelBase

    template<typename ...Args>
    StackPanelBase::StackPanelBase(const CreateParams& params, Args && ...args)
        :
        StackBase{ params, std::forward<Args>(args)... },
        INIT_PROPERTY(orientation),
        INIT_PROPERTY(laneSize),
        INIT_PROPERTY(itemSizing),
        INIT_PROPERTY(itemsOrder)
    {
    }

    template <typename ... Args>
    StackViewBase::StackViewBase(const CreateParams& params, Args&&... args)
        :
        StackPanelBase{
            params,
            Interactivity::ActiveContainer, // a view follows the user, unless the caller says not
            std::forward<Args>(args)...
        },
        INIT_PROPERTY(previewMode),
        m_previewTimer{ UiTimer::OnTick{ [this](TimerEvent&) { onPreviewTimer(); } } }
    {
    }

    void StackPanelBase::setOrientation(Orientation value)
    {
        if (m_orientation == value)
            return;
        m_orientation = value;
        // A length remembered along the other axis bounds nothing on this one.
        m_wrapLengthInDesign = 0.0f;
        invalidateFormAlign();
    }

    void StackPanelBase::setItemSizing(ItemSizing value)
    {
        if (m_itemSizing == value)
            return;
        m_itemSizing = value;
        invalidateFormAlign();
    }

    TraversalOrder StackPanelBase::traversalOrder() const
    {
        switch (m_orientation)
        {
        // A wrapping layout is sorted along the axis its lanes advance on, not along the
        // one its items flow on: rows advance downwards, columns advance rightwards. The
        // leading edges are ordered there because a lane ends before the next one starts,
        // while the trailing edges are not - alignSingleRow hands every item the full
        // lane and each then shrinks back to its own alignment.
        case Orientation::VerticalWrap:
            return { .x = true, .laneExtent = m_maxLaneExtent };
        case Orientation::HorizontalWrap:
            return { .y = true, .laneExtent = m_maxLaneExtent };
        case Orientation::Vertical:
            return { .y = true };
        case Orientation::Horizontal:
            return { .x = true };
        }
        return {};
    }

    ScaledDimensions StackPanelBase::calculateContent(AlignEvent& event)
    {
        if (controls().empty())
            // placeHolderText
            return StackBase::calculateContent(event);

        switch (m_orientation)
        {
        case Orientation::VerticalWrap:
            return calculateColumns(event, controls(), true, itemsPerLane(),
                wrapLengthLimit(event));
        case Orientation::HorizontalWrap:
            return calculateRows(event, controls(), true, itemsPerLane(), wrapLengthLimit(event));
        case Orientation::Vertical:
        {
            ScaledDimensions result = calculateColumns(event, controls(), false);
            // The lane's own length is restated, and only the length: the cross extent is still
            // the largest item, which an even division says nothing about.
            //
            // Restated only over a lane that holds something. A panel with nothing visible in it
            // measures nothing across the lane, and a lane length standing over a cross extent of
            // zero is a panel claiming a size in one direction and none in the other.
            if (dividesEvenly() && result.x)
                result.y = evenLaneLength(laneFill(&Control::height), event.spacing.y);
            return result;
        }
        default: // warn fix
        {
            ScaledDimensions result = calculateRows(event, controls(), false);
            if (dividesEvenly() && result.y)
                result.x = evenLaneLength(laneFill(&Control::width), event.spacing.x);
            return result;
        }
        }
    }

    void StackPanelBase::alignContent(AlignEvent& event, ScaledPosition position, ScaledDimensions& contentDimensions)
    {
        ScaledDimensions calculatedContent = dimensions() - event.padding * 2;
        switch (m_orientation)
        {
        case Orientation::HorizontalWrap:
        case Orientation::VerticalWrap:
            return alignIntoLanes(event, position, contentDimensions);

        case Orientation::Horizontal:
            calculatedContent.y = contentDimensions.y;
            // **The row is as wide as the panel was given**, which is what an even division
            // divides. dimensions() above is still the width the calculate pass arrived at - align
            // sets the new one only after this returns - and contentDimensions is what the parent
            // is handing over now. A natural row never had to tell the two apart: alignSingleRow
            // reads the height it is given and takes each item's width from the item.
            if (dividesEvenly() || hasFillingItem())
                calculatedContent.x = std::max(calculatedContent.x, contentDimensions.x);
            contentDimensions = { 0, 0 };
            if (dividesEvenly())
                return alignEvenLane(event, controls().begin(), controls().end(),
                                     laneFill(&Control::width).count, position,
                                     calculatedContent.x, calculatedContent.y, contentDimensions);
            return alignSingleRow(event, controls().begin(), controls().end(),
                                  position, calculatedContent, contentDimensions);

        case Orientation::Vertical:
            calculatedContent.x = contentDimensions.x;// std::max(calculatedContent.x, contentDimensions.x);
            if (dividesEvenly() || hasFillingItem())
                calculatedContent.y = std::max(calculatedContent.y, contentDimensions.y);
            contentDimensions = { 0, 0 };
            if (dividesEvenly())
                return alignEvenLane(event, controls().begin(), controls().end(),
                                     laneFill(&Control::height).count, position,
                                     calculatedContent.y, calculatedContent.x, contentDimensions);
            return alignSingleColumn(event, controls().begin(), controls().end(),
                                     position, calculatedContent, contentDimensions);
        }
    }

    NavigationWrap StackPanelBase::navigationWrap() const
    {
        switch (m_orientation)
        {
        case Orientation::VerticalWrap: return { false, true };
        case Orientation::HorizontalWrap: return { true, false };
        default: return { false, false };
        }
    }

    // **One routine, read through main and cross.** A wrapping stack does the same four things
    // whichever way its lanes run - cut the lane on its count or on the length it was given,
    // share what the stack has over the length among the items on each lane, share what it has
    // over the thickness among the lanes, and divide an Equal lane by its count - and writing
    // that twice is how each way round came to be missing whatever the other one had. See
    // Item-Containers
    void StackPanelBase::alignIntoLanes(AlignEvent& event, ScaledPosition position,
        ScaledDimensions& contentDimensions)
    {
        // The box the parent is handing over, held to whatever maximum this stack states. Read
        // main-first from here on, so that neither half of the pair can drift from the other.
        ScaledDimensions granted = {
            std::min(contentDimensions.x, event.maxContentWidth()),
            std::min(contentDimensions.y, event.maxContentHeight()) };
        // **A column is not broken at a height its host scrolls.** The measure breaks it at this
        // stack's own maximum alone, and a scroll box hands its body the viewport - so a cut
        // there makes lanes nobody measured. See Item-Containers
        if (!lanesRunAcross() && isScrolledByParent(ScrollAxis::Vertical))
            granted.y = std::max(granted.y, height() - event.padding.y * 2);
        const float limitMain = mainOf(granted);
        const float limitCross = crossOf(granted);
        contentDimensions = { 0.0f, 0.0f };
        m_maxLaneExtent = 0.0f;

        // **An even lane is cut on its shares**, not on the items. Every share is as long as the
        // largest item, so a lane holds as many of those as the length has room for, capped by the
        // lane's own count - and every lane then holds the **same** number. Cutting on the items
        // instead leaves the lane with the long names holding two while the one above it holds
        // three, at shares all one size, so the third slot stands empty for no reason.
        //
        // The largest is read here, in the align pass, where it is this pass's measure:
        // calculateChildren re-measures every child before the parent's own calculateContent, and
        // Control::calculate always writes what it came to.
        const bool even = dividesEvenly();
        const float spacingMain = mainOf(event.spacing);
        const std::size_t perLane = even
            ? sharesThatFit(laneFill(lanesRunAcross() ? &Control::width : &Control::height).largest,
                spacingMain, limitMain)
            : itemsPerLane();

        // **The lanes are cut twice**, because what each one gets is settled by all of them and
        // that is known only once the last one has been cut. The cut is a walk over sizes already
        // measured, so running it again costs less than the vector it would take to remember - and
        // one cut cannot then disagree with the other.
        std::size_t laneCount = 0ull;
        std::size_t fullestLane = 0ull;
        float crossTotal = 0.0f;
        float longestLane = 0.0f;
        for (ControlSpan::iterator it = controls().begin(); it != controls().end(); )
        {
            const LaneCut lane = nextLane(it, event, limitMain, perLane);
            it = lane.end;
            if (!lane.count)
                continue;
            crossTotal += lane.cross;
            longestLane = std::max(longestLane, lane.main);
            fullestLane = std::max(fullestLane, lane.count);
            ++laneCount;
        }

        // **What the length has over the longest lane**, which is all a lane has to spend - see
        // alignSharedLane. Read off **this** pass's cut, never off a walk asking the items how
        // large they are: an item that fills was left at the size the last pass gave it, so a
        // metric taken from the items is a metric taken from its own last answer - three tiles a
        // lane become wider tiles, wider tiles fit two, and two are wider still.
        const float surplus = surplusOver(limitMain, longestLane);
        // **Only where the stack was told to fill that way.** On a scrolled body the granted length
        // is the viewport whatever the alignment says - Control::align gives the surplus back only
        // to a control whose measure is **shorter** than its slot, and a wrapping stack measured
        // against no maximum is longer - so the length alone cannot say whether filling was asked
        // for.
        //
        // **Divided by the item, not by the lane**: the fullest lane's count is the divisor, so
        // every item in the stack grows by the same amount, the fullest lane comes out exactly at
        // the length, and a short last lane stays short.
        const float perItem = fillsMain() && fullestLane
            ? surplus / static_cast<float>(fullestLane)
            : 0.0f;
        // Every lane holds perLane shares, so the length is the longest lane's - stretched to what
        // the stack was granted where it fills.
        const float evenMain = fillsMain() ? std::max(longestLane, limitMain) : longestLane;

        const float crossSpacing = crossOf(event.spacing);
        const float crossSlack = surplusOver(limitCross, crossTotal + (lanesRunAcross()
            ? event.totalSpacingY(laneCount) : event.totalSpacingX(laneCount)));
        const float lanes = static_cast<float>(laneCount);

        // Each lane's share across is measured from the whole rather than accumulated, so a
        // rounding is never carried into the next one and the last lane ends on the edge it was
        // given.
        float shared = 0.0f;
        std::size_t index = 0ull;
        for (ControlSpan::iterator it = controls().begin(); it != controls().end(); )
        {
            const LaneCut lane = nextLane(it, event, limitMain, perLane);
            it = lane.end;
            if (!lane.count)
                continue;
            const float sharedNext = std::round(crossSlack * static_cast<float>(index + 1ull) / lanes);
            const float laneCross = lane.cross + sharedNext - shared;
            shared = sharedNext;
            m_maxLaneExtent = std::max(m_maxLaneExtent, laneCross);
            if (even)
            {
                alignEvenLane(event, lane.begin, lane.end, perLane, position, evenMain,
                    laneCross, contentDimensions);
            }
            else
            {
                // **A lane holding something that asked takes all the room it has**, whether the
                // stack fills or not, as a single lane does - see alignContent, which reaches for
                // the granted length on hasFillingItem() alone. Read off the cut rather than by
                // walking a million items. Otherwise a share for each item on it, and never more
                // room than the lane has: one long item can make a lane already at the length.
                const float laneRoom = surplusOver(limitMain, lane.main);
                const float laneSurplus = lane.takers
                    ? laneRoom
                    : std::min(laneRoom, perItem * static_cast<float>(lane.count));
                alignSharedLane(event, lane, position, laneSurplus, laneCross, contentDimensions);
            }
            crossOf(position) += laneCross + crossSpacing;
            ++index;
        }
        rememberWrapLength(event, limitMain);
    }

    StackPanelBase::LaneCut StackPanelBase::nextLane(ControlSpan::iterator begin, const AlignEvent& event,
        const float limitMain, const std::size_t perLane)
    {
        LaneCut lane{ begin, begin, 0.0f, 0.0f, 0ull, 0ull };
        const float spacingMain = mainOf(event.spacing);
        for (ControlSpan::iterator it = begin; it != controls().end(); ++it)
        {
            const Control& item = **it;
            // Hidden items are passed over here as the measure passes over them, or a lane holds
            // fewer than it was measured to hold. Taken into the open lane rather than left to
            // start the next one, which would begin on something that is never placed.
            if (!item.visible())
            {
                lane.end = std::next(it);
                continue;
            }
            const ScaledDimensions size = item.dimensions();
            float reach = mainOf(size);
            if (lane.count)
                reach += spacingMain;
            // The lane closes on the item past its count, or on the one that would run past the
            // length - and never on its first, which stands however long it is.
            const bool full = lane.count == perLane;
            if (lane.count && (full || lane.main + reach > limitMain))
                break;
            lane.main += reach;
            lane.cross = std::max(lane.cross, crossOf(size));
            lane.end = std::next(it);
            ++lane.count;
            if (item.fillsLane())
                ++lane.takers;
        }
        return lane;
    }

    // **A stated lane breaks the lane whatever the length.** The measure has already answered on
    // the count - see Control::calculateLanes - so a lane running past it here would be a lane
    // nobody measured, and the lanes it saves stand beside the items as dead space.
    // **The last share carries no gap after it**, so the length a lane has to fit is one gap longer
    // than the shares it holds.
    //
    // **Asked by the align pass and only by it.** The measure must not break its lanes on this: it
    // would be measuring against the length the last pass laid out - see wrapLengthLimit - so a
    // narrow pass would shrink what the panel asks for, a host measured from the panel would grant
    // that, and the count would walk down one pass at a time and never come back.
    std::size_t StackPanelBase::sharesThatFit(const float largest, const float spacing,
        const float limitMain) const
    {
        const std::size_t perLane = itemsPerLane();
        const float share = largest + spacing;
        if (share <= 0.0f)
            return perLane;
        const float fits = std::floor((limitMain + spacing) / share);
        // **One is the floor.** A share too long for the lane still stands in it, as the first item
        // on a natural lane does whatever its length.
        if (fits < 1.0f)
            return 1ull;
        // Compared before the cast: a length of k_maxFloat over a small share is a number no
        // std::size_t holds, and converting it is undefined rather than large.
        if (fits >= static_cast<float>(perLane))
            return perLane;
        return static_cast<std::size_t>(fits);
    }

    // **A wrapping stack that divides evenly is a grid, and a grid is measured as one.** Every
    // share is as long as the largest item, so a lane is that times the **lane's** count whatever
    // the items on it are called, and the lanes are as thick as the largest item across.
    //
    // Measured from the natural sum instead, a lane of four short items and a lane of two long
    // ones come to the same length, so the align breaks one at four and the other at two in the
    // same stack at the same width - and the item that overruns its share is clipped, because a
    // share it never measured against is not a width its text was broken at. A theme tile stating
    // its own fixed width was the workaround for this, and it is not needed once the grid is
    // measured as a grid.
    // **The lane shares what the stack was given over what it measured.** A stack that kept its own
    // size was given nothing over, and every lane stands where it measured.
    //
    // **The surplus is the stack's, not the lane's.** Sharing out the difference between this lane
    // and the longest instead would stretch a short lane to the length of a long one while nothing
    // at all had been granted - a last row of two spread across a lane of five.
    //
    // **What asked for the surplus takes it all**, and where nothing asked it goes to the items in
    // equal parts - see Control::fillsLane and FlexSpacer.
    //
    // **What an item does with its part is the item's to say.** One that fills takes it; one
    // aligned to an edge keeps the size it measured and stands in it - see Control::align.
    void StackPanelBase::alignSharedLane(AlignEvent& event, const LaneCut& lane,
        const ScaledPosition position, const float laneSurplus, const float laneCross,
        ScaledDimensions& contentDimensions)
    {
        crossOf(contentDimensions) = std::max(crossOf(contentDimensions),
            crossOf(position) + laneCross);
        // Reported even with nothing to place, as alignSingleRow reports it: the lane is where it
        // was put, and a lane of nothing still ends where it began.
        mainOf(contentDimensions) = std::max(mainOf(contentDimensions), mainOf(position));
        if (!lane.count)
            return;

        // The items that asked, or every item where none did.
        const float sharers = static_cast<float>(lane.takers ? lane.takers : lane.count);
        const float spacingMain = mainOf(event.spacing);
        // Each share is measured from the whole rather than accumulated, so a rounding is never
        // carried into the next one and the last item ends on the edge the lane was given.
        float handed = 0.0f;
        float from = 0.0f;
        float lastEnd = mainOf(position);
        std::size_t taken = 0ull;
        for (ControlSpan::iterator it = lane.begin; it != lane.end; ++it)
        {
            Control& control = **it;
            if (!control.visible())
                continue;
            const ScaledDimensions size = control.dimensions();
            float handedNext = handed;
            if (!lane.takers || control.fillsLane())
            {
                ++taken;
                handedNext = std::round(laneSurplus * static_cast<float>(taken) / sharers);
            }
            const float extent = mainOf(size) + handedNext - handed;
            handed = handedNext;
            placeInLane(event, control, position, from, extent, laneCross);
            lastEnd = mainOf(position) + from + extent;
            from += extent + spacingMain;
        }
        // Where the items ended, not the length the calculate pass predicted for them. Both walk
        // the same lane, but calculateLanes adds (size + spacing) to its running total while this
        // adds the two separately, so the results drift apart as the lane grows - and reporting
        // the larger leaves a scroll range the last item never reaches.
        mainOf(contentDimensions) = std::max(mainOf(contentDimensions), lastEnd);
    }

    // Along the lane by the part it was handed, across it by the whole thickness. What the item
    // then does inside that box is its own alignment's answer - see Control::align.
    void StackPanelBase::placeInLane(AlignEvent& event, Control& control,
        const ScaledPosition lanePosition, const float from, const float extent,
        const float laneCross)
    {
        ScaledPosition itemPosition = lanePosition;
        mainOf(itemPosition) += from;
        alignControl(&control, event, itemPosition, lanePoint(extent, laneCross));
    }

    float StackPanelBase::wrapLengthLimit(const AlignEvent& event) const
    {
        if (m_wrapLengthInDesign <= 0.0f)
            return k_maxFloat;
        // **Not while the form is asking for a window.** That pass is where a form finds out what
        // it wants, and one held to the size it was last given could never grow - see
        // FormBase::isMeasuringPlacement.
        const FormBase* hostForm = getForm();
        if (hostForm && hostForm->isMeasuringPlacement())
            return k_maxFloat;
        // **Not where the length is the content's own answer**, however many hosts down. A bound
        // stated there is derived from the thing it bounds: the lanes wrap, the panel comes out
        // shorter along them, the host measured from it follows, and the next pass wraps them
        // again - a backstage walked down to three tiles a lane, one pass at a time.
        if (!isLaneLengthGivenFromOutside())
            return k_maxFloat;
        return m_wrapLengthInDesign * event.scaleFactor();
    }

    // **In design units, with the scale that laid it out.** Converting it at the scale in force
    // when it is read puts the two at different scales, and a form taken to 250% and back is left
    // measuring against a fraction of the length it has - see ScrollBox::adjustChildMetrics.
    //
    // **A length that has not changed asks for nothing**, and that is what ends this. Comparing
    // what the lanes **came to** across against what they measured cannot: Control::calculate ceils
    // the content it measured and the align pass does not, so an extent with a fraction in it
    // differs by up to a whole unit for ever - every pass asks for another, and the program lays
    // itself out until it is killed. The length converges in one step because the pass that
    // measures against it hands back the same number.
    void StackPanelBase::rememberWrapLength(AlignEvent& event, const float limitLength)
    {
        if (!isLaneLengthGivenFromOutside())
            return;
        // **Not while the form is asking for a window.** That pass lays the lanes out at the
        // content's own size, not at a size the window gave - see FormBase::isMeasuringPlacement.
        const FormBase* hostForm = getForm();
        if (hostForm && hostForm->isMeasuringPlacement())
            return;
        const float lengthInDesign = limitLength / event.scaleFactor();
        if (std::abs(lengthInDesign - m_wrapLengthInDesign) < k_wrapEpsilon)
            return;
        m_wrapLengthInDesign = lengthInDesign;
        // The lanes this pass laid out were measured against another length. Ask, do not lay out
        // from here - see AlignEvent::invalidatePass.
        event.invalidatePass();
    }

    // **A ceiling is spread, a count is not.** Under UpTo the items go into the fewest lanes the
    // count allows and every lane but the last is filled to the same depth, so eleven under a
    // ceiling of ten stands as six and five rather than ten and one.
    std::size_t StackPanelBase::itemsPerLane() const
    {
        if (m_laneSize.sizing == LaneSizing::Exact)
            return m_laneSize.value;
        const std::size_t items = visibleItems();
        // One lane holds them all, and a ceiling of none is no ceiling to spread under.
        if (!m_laneSize.value || items <= m_laneSize.value)
            return m_laneSize.value;
        const std::size_t lanes = (items + m_laneSize.value - 1ull) / m_laneSize.value;
        return (items + lanes - 1ull) / lanes;
    }

    // A hidden item is passed over by every lane walk here, so it is not one of the items the
    // lanes are cut for.
    std::size_t StackPanelBase::visibleItems() const
    {
        std::size_t result = 0ull;
        for (const ControlPtr& item : controls())
        {
            if (item->visible())
                ++result;
        }
        return result;
    }

    // **What a wrapping stack lacked was a divisor known before the wrap.** Its lanes' own counts
    // are settled by the sizes the items came to, and dividing by those is dividing by the answer -
    // so Equal used to read only where a LaneSize stated the count outright. sharesThatFit is that
    // divisor without the statement: how many shares of the largest item the length has room for,
    // both of them known before a lane is cut. A stated lane is now a cap on it rather than the
    // only way to have one.
    bool StackPanelBase::dividesEvenly() const
    {
        return m_itemSizing == ItemSizing::Equal;
    }

    // **A lane with one is as long as it was granted**, not as long as it measured - there is no
    // surplus to hand out otherwise, and the item would take nothing. The same reach for the
    // granted box an even division makes, for the same reason. See Control::fillsLane
    bool StackPanelBase::hasFillingItem() const
    {
        for (const ControlPtr& item : controls())
            if (item->visible() && item->fillsLane())
                return true;
        return false;
    }

    StackPanelBase::LaneFill StackPanelBase::laneFill(float (Control::* extent)() const) const
    {
        LaneFill result{ 0ull, 0.0f };
        for (const ControlPtr& item : controls())
        {
            if (!item->visible())
                continue;
            ++result.count;
            result.largest = std::max(result.largest, ((*item).*extent)());
        }
        return result;
    }

    float StackPanelBase::evenLaneLength(const LaneFill& fill, const float spacing)
    {
        if (!fill.count)
            return 0.0f;
        const float count = static_cast<float>(fill.count);
        return fill.largest * count + spacing * (count - 1.0f);
    }

    // **Under a pixel is nothing over.** Control::calculate ceils the content it measured and the
    // align pass does not, so a stack that was granted exactly what it asked for still answers a
    // fraction here - and a fraction shared out is every item moved by part of a pixel on a pass
    // that changed nothing.
    float StackPanelBase::surplusOver(const float granted, const float taken)
    {
        const float surplus = granted - taken;
        return surplus < 1.0f ? 0.0f : surplus;
    }

    void StackPanelBase::alignEvenLane(AlignEvent& event, ControlSpan::iterator laneBegin,
        ControlSpan::iterator laneEnd, const std::size_t shares, ScaledPosition position,
        const float laneMain, const float laneCross, ScaledDimensions& contentDimensions)
    {
        crossOf(contentDimensions) = std::max(crossOf(contentDimensions),
            crossOf(position) + laneCross);
        // Reported even with nothing to place, as alignSingleRow reports it: the lane is where it
        // was put, and a lane of nothing still ends where it began.
        mainOf(contentDimensions) = std::max(mainOf(contentDimensions), mainOf(position));
        if (!shares)
            return;

        // The lane plus one gap, cut into equal steps: each item takes a step less the gap that
        // ends it, and the gap the last step ends with falls off the lane's edge. Every boundary
        // is measured from the lane's own start rather than accumulated, so a rounding is never
        // carried into the next share - no two differ by more than a pixel, and the last one lands
        // on the edge the lane was given.
        const float spacingMain = mainOf(event.spacing);
        const float step = (laneMain + spacingMain) / static_cast<float>(shares);
        float lastEnd = mainOf(position);
        std::size_t index = 0ull;
        for (ControlSpan::iterator it = laneBegin; it != laneEnd; ++it)
        {
            Control& control = **it;
            if (!control.visible())
                continue;
            const float from = std::round(step * static_cast<float>(index));
            const float to = std::round(step * static_cast<float>(index + 1ull)) - spacingMain;
            // A lane too short to hold its own gaps gives a negative share, and a negative extent
            // reaches setDimensions as one. Nothing is what a share that small is worth.
            placeInLane(event, control, position, from, std::max(0.0f, to - from), laneCross);
            lastEnd = mainOf(position) + to;
            ++index;
        }
        mainOf(contentDimensions) = std::max(mainOf(contentDimensions), lastEnd);
    }

    void StackPanelBase::moveItems(const std::span<Control* const> items, const std::size_t to)
    {
        ControlSpan children = controls();
        if (items.empty() || to + items.size() > children.size())
            return;
        for (const Control* item : items)
            if (!item || item->parent() != this)
                return;
        std::vector<Control*> order{};
        order.reserve(children.size());
        for (ControlPtr& child : children)
            if (std::ranges::find(items, &*child) == items.end())
                order.push_back(&*child);
        order.insert(order.begin() + static_cast<std::ptrdiff_t>(to), items.begin(), items.end());
        placeChildren(order, 0ull);
        ItemsMoveEvent event{ *this, items, to };
        itemsMoved(event);
        emitEvent(event);
    }

    bool StackPanelBase::canMoveItem(Control& item)
    {
        CanMoveItemEvent event{ *this, item, defaultCanMoveItem(item) };
        emitEvent(event);
        return event.canMove;
    }

    bool StackPanelBase::defaultCanMoveItem(Control& item)
    {
        return m_itemsOrder == ItemsOrder::Rearrangeable && item.visible();
    }

    std::vector<Control*> StackPanelBase::carriedItems(Control& item)
    {
        return { &item };
    }

    // The child the press landed in is what a carry takes - and in a stack following the user it
    // has to be the item the press names, so an item inside a group moves nothing here. A drag
    // some control inside the child has taken for its own is no carry either.
    void StackPanelBase::nestedDrag(DragEvent& event)
    {
        StackBase::nestedDrag(event);
        if (m_carry)
        {
            if (!m_carry->cancelled)
                continueCarry(event);
            return;
        }
        if (m_itemsOrder != ItemsOrder::Rearrangeable || event.hoveredControlLocked()
            || !event.pastThreshold())
            return;
        Control* child = &event.control();
        while (child && child->parent() != this)
            child = child->parent();
        if (!child || (followsUser() && itemAt(&event.control()) != child))
            return;
        if (canMoveItem(*child))
            beginCarry(*child, event);
    }

    void StackPanelBase::nestedPressUp(PressUpEvent& event)
    {
        StackBase::nestedPressUp(event);
        if (!m_carry)
            return;
        // A release ending a carry is the gesture's end, not a click on what it lands on.
        event.handled.preventClick = true;
        endCarry();
    }

    void StackPanelBase::nestedKeyDown(KeyDownEvent& event)
    {
        if (m_carry && !m_carry->cancelled && event.key == Keys::Escape)
        {
            // The run goes back as it stood, and the press still down starts nothing more.
            Carry& carry = *m_carry;
            placeChildren(carry.original, carry.first);
            for (Control* lifted : carry.lifted)
                removeOverlayControl(*lifted);
            carry.lifted.clear();
            carry.cancelled = true;
            event.handled = true;
            invalidate();
            return;
        }
        StackBase::nestedKeyDown(event);
    }

    void StackPanelBase::nestedControlDeleted(Control* control)
    {
        StackBase::nestedControlDeleted(control);
        if (m_carry && std::ranges::find(m_carry->original, control) != m_carry->original.end())
            dropCarry();
    }

    FloatPoint StackPanelBase::overlayChildOffset(const Control& child) const
    {
        if (!m_carry || m_carry->cancelled || !isCarried(child))
            return StackBase::overlayChildOffset(child);
        return lanePoint(carriedBlockOffset(), 0.0f);
    }

    // One lane only: a stack wrapping its items has no one line to carry them along.
    void StackPanelBase::beginCarry(Control& item, DragEvent& event)
    {
        if (m_orientation != Orientation::Horizontal && m_orientation != Orientation::Vertical)
            return;
        std::vector<Control*> items = carriedItems(item);
        if (items.empty())
            return;
        ControlSpan children = controls();
        // The run of siblings the carry may land among: as far as the movable ones reach on
        // either side of the pressed item. Items outside it stay where they are.
        const std::size_t pressed = indexOf(item);
        std::size_t first = pressed;
        while (first > 0ull && canMoveItem(*children[first - 1ull]))
            --first;
        std::size_t last = pressed;
        while (last + 1ull < children.size() && canMoveItem(*children[last + 1ull]))
            ++last;
        std::erase_if(items, [&](const Control* carried) {
            const std::size_t index = indexOf(*carried);
            return index < first || index > last;
        });
        if (std::ranges::find(items, &item) == items.end())
            return;
        Carry carry{
            .items = std::move(items),
            .anchor = &item,
            .first = first,
            .last = last,
            .grab = mainOf(event.currentPos() - item.parentContentOrigin()) - leadingOf(item),
            .pointer = event.currentPos()
        };
        for (std::size_t i = first; i <= last; ++i)
            carry.original.push_back(&*children[i]);
        if (first < last)
        {
            const Control& leader = *children[first];
            const float leaderTrailing = leadingOf(leader) + extentOf(leader);
            carry.spacing = leadingOf(*children[first + 1ull]) - leaderTrailing;
        }
        // Drawn over the siblings it passes, unless some host already draws it so.
        for (Control* carried : carry.items)
        {
            bool lifted = false;
            for (const Control* host = parent(); host && !lifted; host = host->parent())
                lifted = host->hasInOverlayControls(*carried);
            if (!lifted && addOverlayControl(*carried, ClippingMode::Standard))
                carry.lifted.push_back(carried);
        }
        m_carry = std::move(carry);
        // Gathered into one block around the pressed item, which keeps its place.
        std::size_t before = 0ull;
        for (const Control* carried : m_carry->items)
            if (indexOf(*carried) < pressed)
                ++before;
        placeBlock(pressed - before);
        event.lockHoveredControl();
        invalidate();
    }

    void StackPanelBase::continueCarry(DragEvent& event)
    {
        Carry& carry = *m_carry;
        carry.pointer = event.currentPos();
        event.lockHoveredControl();
        const float drawnLeading = leadingOf(*carry.items.front()) + carriedBlockOffset();
        const std::size_t to = carryIndex(drawnLeading);
        if (to != indexOf(*carry.items.front()))
            placeBlock(to);
        invalidate();
    }

    void StackPanelBase::endCarry()
    {
        Carry carry = std::move(*m_carry);
        m_carry.reset();
        for (Control* lifted : carry.lifted)
            removeOverlayControl(*lifted);
        invalidate();
        if (carry.cancelled)
            return;
        ControlSpan children = controls();
        bool moved = false;
        for (std::size_t i = 0ull; i != carry.original.size() && !moved; ++i)
            moved = &*children[carry.first + i] != carry.original[i];
        if (!moved)
            return;
        ItemsMoveEvent event{ *this, carry.items, indexOf(*carry.items.front()) };
        itemsMoved(event);
        emitEvent(event);
    }

    void StackPanelBase::dropCarry()
    {
        for (Control* lifted : m_carry->lifted)
            removeOverlayControl(*lifted);
        m_carry.reset();
    }

    void StackPanelBase::placeBlock(const std::size_t to)
    {
        const Carry& carry = *m_carry;
        ControlSpan children = controls();
        std::vector<Control*> order{};
        order.reserve(carry.last - carry.first + 1ull);
        for (std::size_t i = carry.first; i <= carry.last; ++i)
            if (!isCarried(*children[i]))
                order.push_back(&*children[i]);
        order.insert(order.begin() + static_cast<std::ptrdiff_t>(to - carry.first),
            carry.items.begin(), carry.items.end());
        placeChildren(order, carry.first);
    }

    void StackPanelBase::placeChildren(const std::span<Control* const> order,
        const std::size_t from)
    {
        ControlSpan children = controls();
        for (std::size_t i = 0ull; i != order.size(); ++i)
            if (&*children[from + i] != order[i])
                moveControl(*order[i], from + i);
    }

    // The block takes the slot nearest its leading edge among the slots the run's other items
    // leave, measured with the block taken out of the lane: it goes before the first item whose
    // middle, so measured, lies past that edge. Moving either way, the swap comes at the same
    // place.
    std::size_t StackPanelBase::carryIndex(const float drawnLeading) const
    {
        const Carry& carry = *m_carry;
        ControlSpanC children = controls();
        float shift = 0.0f;   // what taking the block out moves the items after it by
        std::size_t passed = 0ull;
        for (std::size_t i = carry.first; i <= carry.last; ++i)
        {
            const Control& child = *children[i];
            if (isCarried(child))
            {
                shift += i < carry.last
                    ? leadingOf(*children[i + 1ull]) - leadingOf(child)
                    : extentOf(child) + carry.spacing;
                continue;
            }
            const float slot = extentOf(child) + carry.spacing;
            const float middle = leadingOf(child) - shift + slot / 2.0f;
            if (middle >= drawnLeading)
                break;
            ++passed;
        }
        return carry.first + passed;
    }

    // The anchor stays under the pointer where it was pressed, and the block is kept within the
    // run.
    float StackPanelBase::carriedBlockOffset() const
    {
        const Carry& carry = *m_carry;
        ControlSpanC children = controls();
        const Control& block = *carry.items.front();
        const Control& blockEnd = *carry.items.back();
        const Control& runEnd = *children[carry.last];
        const float blockLeading = leadingOf(block);
        const float pointer = mainOf(carry.pointer - block.parentContentOrigin());
        const float runLeading = leadingOf(*children[carry.first]);
        const float runTrailing = leadingOf(runEnd) + extentOf(runEnd);
        const float blockExtent = leadingOf(blockEnd) + extentOf(blockEnd) - blockLeading;
        const float drawn = std::clamp(
            pointer - carry.grab - (leadingOf(*carry.anchor) - blockLeading),
            runLeading,
            std::max(runLeading, runTrailing - blockExtent));
        return drawn - blockLeading;
    }

    bool StackPanelBase::isCarried(const Control& child) const
    {
        return m_carry && std::ranges::find(m_carry->items, &child) != m_carry->items.end();
    }

    std::size_t StackPanelBase::indexOf(const Control& child) const
    {
        ControlSpanC children = controls();
        for (std::size_t i = 0ull; i != children.size(); ++i)
            if (&*children[i] == &child)
                return i;
        return children.size();
    }

    void StackViewBase::previewItemChanged(PreviewEvent& event)
    {
        emitEvent(event);
    }

    void StackViewBase::nestedControlHovered(Control* hovered)
    {
        StackPanelBase::nestedControlHovered(hovered);

        if (m_previewMode != PreviewMode::Hover)
            return;
        // What the pointer is on is a leaf, and the item is whatever ancestor of it this stack
        // takes for one - the same walk a press makes. A stack holding its items in groups is
        // told about the leaf and not about the group, which is the whole reason this is not
        // childHoverEnter: that one names the direct child and never reaches a container standing
        // above the group.
        //
        // Nothing happens where the walk finds no item - the pointer is on a caption or in the
        // gap between two groups - so crossing one of those keeps the preview where it was rather
        // than dropping it. Leaving the stack is what moves it, below.
        if (Control* item = itemAt(hovered))
            startPreviewTimer(*item);
    }

    void StackViewBase::hoverLeave()
    {
        StackPanelBase::hoverLeave();

        if (m_previewMode != PreviewMode::Hover)
            return;
        // The pointer has left this stack whole. Moving between two items inside it does not come
        // here: the walk that tells a control the hover has left it stops at the common parent of
        // the two, which is the stack.
        //
        // What the stack stands on is its current item, so that is what the preview goes back to.
        // A stack with none previews nothing, the same way it previewed nothing to begin with.
        if (Control* item = currentItem())
            startPreviewTimer(*item);
    }

    void StackViewBase::currentItemChanged(CurrentItemChangeEvent& currentItemChangeEvent)
    {
        StackPanelBase::currentItemChanged(currentItemChangeEvent);

        // Every route the current item moves by comes through here - a key, a click, a call from
        // code - so this is the whole of what Focus mode listens to, and the half of Hover mode a
        // key reaches. The delay does the rest: a held arrow moves the current item once per
        // repeat and previews once, for the item it stopped on.
        //
        // The current item cleared is not an item to preview. That is what a deleted current item
        // leaves behind, and nestedControlDeleted below is what answers it.
        if (Control* item = currentItem())
            startPreviewTimer(*item);
    }

    void StackViewBase::nestedControlDeleted(Control* control)
    {
        StackPanelBase::nestedControlDeleted(control);

        if (control != m_previewItem)
            return;
        // The list being rebuilt under the pointer is the ordinary case here, not a rare one - a
        // rename does it. The timer goes with the item: what it was waiting to raise no longer
        // exists, and there is nothing else it could name.
        m_previewItem = nullptr;
        m_previewTimer.stop();
    }

    void StackViewBase::onPreviewTimer()
    {
        // **A glide is not the user looking at anything.** Content travelling under a pointer that
        // is holding still hands the hover a new item on every frame - see ScrollBox's wheel glide,
        // which repeats the mouse move for exactly that reason - and what passes under the pointer
        // on the way is not what is being looked at. So the wait starts again, and the item that
        // is finally under the pointer when the movement lands is the one previewed.
        //
        // Any scroll anywhere, not this stack's own: a stack cannot name the box that scrolls it
        // without reaching for the type, and a glide elsewhere costs this nothing but its own
        // length. A stack in no form has no controller, and nothing glides there.
        AnimationController* controller = animator();
        if (controller && controller->isActive(AnimationSlots::scroll))
        {
            m_previewTimer.start(k_previewDelay);
            return;
        }

        PreviewEvent event{ *this, *m_previewItem };
        previewItemChanged(event);
    }

    void StackViewBase::startPreviewTimer(Control& item)
    {
        if (m_previewItem == &item)
            return;
        m_previewItem = &item;
        m_previewTimer.start(k_previewDelay);
    }

}
