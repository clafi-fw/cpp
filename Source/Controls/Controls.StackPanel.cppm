module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.StackPanel;

import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Core.AppTheme_AnimationSlots;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;
import ClaFi.Core.Foundation;

namespace ClaFi::Controls
{
    export class StackPanel;

    // Which item a stack raises its preview for. See Item-Containers
    export enum class PreviewMode
    {
        Focus,   // the item the container has settled on, whichever gesture moved it
        Hover    // the item under the pointer, falling back to the current item on leaving
    };

    // The item a stack has settled on, before the user has picked anything. See Item-Containers
    export struct PreviewEvent : public Event
    {
        PreviewEvent(StackPanel& itemsView, Control& item);
        StackPanel& itemsView;   // the stack the preview belongs to
        Control& item;           // the item being looked at, never null
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

    // A container that places its items in lanes. See Item-Containers
    export class StackPanel : public StackPanelBase
    {
    public:
        using StackPanelBase::add;
        using StackPanelBase::reserve;
        using StackPanelBase::controls;
        using StackPanelBase::clearControls;
    public:
        template <typename... Args>
        explicit StackPanel(const CreateParams&, Args&&...);
    public:
        // Which way a lane runs, and whether the stack wraps into further lanes.
        DECLARE_WRITABLE_PROPERTY(Orientation, orientation, setOrientation, Orientation::VerticalWrap)
        // How many items one lane takes, and how that count is read.
        DECLARE_PROPERTY(LaneSize, laneSize, LaneSize{})
        // How a lane sizes the items in it.
        DECLARE_WRITABLE_PROPERTY(ItemSizing, itemSizing, setItemSizing, ItemSizing::Natural)
        // Which item the stack raises its preview for.
        DECLARE_WRITABLE_PROPERTY(PreviewMode, previewMode, setPreviewMode, PreviewMode::Focus)
    public:
        // Raised once the preview has settled on an item.
        DECLARE_EVENT(PreviewEvent, OnPreview, onPreview)
    public:
        std::wstring_view diagnosticText() const override { return L"StackPanel"; }
        void setOrientation(Orientation);
        void setItemSizing(ItemSizing);
        TraversalOrder traversalOrder() const override;
        void setPreviewMode(PreviewMode value) { m_previewMode = value; }
        // The item the preview has settled on, by whichever reading PreviewMode names. Null until
        // something has been previewed or recorded. Between an item being reached and the delay
        // elapsing this is the item about to be raised, which is the only moment it says more
        // than the last PreviewEvent did.
        [[nodiscard]] Control* previewItem() const { return m_previewItem; }
    protected:
        ScaledDimensions calculateContent(AlignEvent&) override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&) override;
        NavigationWrap navigationWrap() const override;
        void nestedControlHovered(Control* hovered) override;
        void hoverLeave() override;
        void currentItemChanged(CurrentItemChangeEvent&) override;
        void nestedControlDeleted(Control*) override;
        virtual void previewItemChanged(PreviewEvent&);
        // Says the preview already stands on this item, without raising one and without the wait.
        // For a container built around an item whose preview is what the caller is already
        // showing - a dropdown opened on its selected item - so that reaching that item asks for
        // nothing. Only the item: whatever a stack builds around it is the caller's to set.
        void recordPreviewItem(Control& value) { m_previewItem = &value; }
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
        // EVERY WRAPPING STACK, BOTH WAYS ROUND. See Item-Containers
        void alignIntoLanes(AlignEvent&, ScaledPosition, ScaledDimensions& contentDimensions);
        // The lane beginning at begin: as many items as the lane takes, as many as fit the length
        // it is given, and never fewer than one.
        [[nodiscard]] LaneCut nextLane(ControlSpan::iterator begin, const AlignEvent&,
            float limitMain, std::size_t perLane);
        // How many equal shares of this size fit the length given: never more than the lane takes,
        // and never fewer than one. See Item-Containers
        [[nodiscard]] std::size_t sharesThatFit(float largest, float spacing, float limitMain) const;
        // A lane with what the STACK was given over what it measured divided among the items on
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
        // Whether this stack was told to fill the way its lanes run. See Item-Containers
        [[nodiscard]] bool fillsMain() const
        {
            return lanesRunAcross()
                ? horizontalAlign() == HorizontalAlign::Fill
                : verticalAlign() == VerticalAlign::Fill;
        }
        // MAIN IS THE WAY A LANE RUNS, CROSS THE WAY THE LANES STACK. Every extent a wrapping
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
        // The width the last align pass broke this panel's rows at, at the scale measuring now,
        // and no limit where there is none to state. See Item-Containers
        [[nodiscard]] float wrapWidthLimit(const AlignEvent&) const;
        // Carries the width this pass broke the rows at over to the pass that measures next, and
        // asks for that pass where it has changed. See Item-Containers
        void rememberWrapWidth(AlignEvent&, float limitWidth);
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
        // Aims the preview at an item and waits to see whether the pointer stays there. An item
        // already aimed at costs nothing, which is what makes a held key one preview rather than
        // one per repeat: Control::setFocus moves the hover with the focus while the keyboard
        // drives, so a key and a pointer reach this the same way.
        void startPreviewTimer(Control&);
        void onPreviewTimer();
    private:
        // Long enough that a pointer crossing the stack previews nothing it merely passes over,
        // short enough to read as the answer to stopping on an item.
        static constexpr MilliSeconds k_previewDelay{ 110 };
        // Under this two extents are the same extent: what the align laid out is compared with
        // what the measure answered, and a pass asked for on floating-point noise is a pass
        // every frame.
        static constexpr float k_wrapEpsilon{ 0.5f };
        // Height of the tallest row, or width of the widest column, as measured by the
        // last wrapping align. Seeds to k_maxFloat so that a panel queried before it has
        // ever been aligned reports a lane nothing can precede, and culls nothing.
        float m_maxLaneExtent{ k_maxFloat };
        // The content width the last align pass broke the rows at, in design units, and zero
        // until this panel has been laid out once. See Item-Containers
        float m_wrapWidthInDesign{};
        // Connected in the constructor rather than handed its handler here. The constructor is a
        // template, so a default member initializer is instantiated in whatever translation unit
        // builds a StackPanel, and OnEvent's deduction does not survive the trip - onTick names
        // the event and deduces nothing.
        UiTimer m_previewTimer{};
        Control* m_previewItem{ nullptr };
    };


//-----------------------------------------------------------------------------


    // PreviewEvent

    PreviewEvent::PreviewEvent(StackPanel& itemsView, Control& item)
        :
        itemsView{ itemsView },
        item{ item }
    {
    }

    // StackPanel

    template<typename ...Args>
    StackPanel::StackPanel(const CreateParams& params, Args && ...args)
        :
        StackPanelBase{ params, std::forward<Args>(args)... },
        INIT_PROPERTY(orientation),
        INIT_PROPERTY(laneSize),
        INIT_PROPERTY(itemSizing),
        INIT_PROPERTY(previewMode)
    {
        m_previewTimer.onTick([this](TimerEvent&) { onPreviewTimer(); });
    }

    void StackPanel::setOrientation(Orientation value)
    {
        if (m_orientation == value)
            return;
        m_orientation = value;
        invalidateFormAlign();
    }

    void StackPanel::setItemSizing(ItemSizing value)
    {
        if (m_itemSizing == value)
            return;
        m_itemSizing = value;
        invalidateFormAlign();
    }

    TraversalOrder StackPanel::traversalOrder() const
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

    ScaledDimensions StackPanel::calculateContent(AlignEvent& event)
    {
        if (controls().empty())
            // placeHolderText
            return StackPanelBase::calculateContent(event);

        switch (m_orientation)
        {
        case Orientation::VerticalWrap:
            return calculateColumns(event, controls(), true, itemsPerLane());
        case Orientation::HorizontalWrap:
            return calculateRows(event, controls(), true, itemsPerLane(), wrapWidthLimit(event));
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

    void StackPanel::alignContent(AlignEvent& event, ScaledPosition position, ScaledDimensions& contentDimensions)
    {
        ScaledDimensions calculatedContent = dimensions() - event.padding * 2;
        switch (m_orientation)
        {
        case Orientation::HorizontalWrap:
        case Orientation::VerticalWrap:
            return alignIntoLanes(event, position, contentDimensions);

        case Orientation::Horizontal:
            calculatedContent.y = contentDimensions.y;
            // THE ROW IS AS WIDE AS THE PANEL WAS GIVEN, which is what an even division divides.
            // dimensions() above is still the width the calculate pass arrived at - align sets
            // the new one only after this returns - and contentDimensions is what the parent is
            // handing over now. A natural row never had to tell the two apart: alignSingleRow
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

    NavigationWrap StackPanel::navigationWrap() const
    {
        switch (m_orientation)
        {
        case Orientation::VerticalWrap: return { false, true };
        case Orientation::HorizontalWrap: return { true, false };
        default: return { false, false };
        }
    }

    void StackPanel::nestedControlHovered(Control* hovered)
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

    void StackPanel::hoverLeave()
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

    void StackPanel::currentItemChanged(CurrentItemChangeEvent& event)
    {
        StackPanelBase::currentItemChanged(event);
        if (m_previewMode != PreviewMode::Focus)
            return;

        // Every route the current item moves by comes through here - a key, a click, a call from
        // code - so this is the whole of what Focus mode listens to. The delay does the rest: a
        // held arrow moves the current item once per repeat and previews once, for the item it
        // stopped on.
        //
        // The current item cleared is not an item to preview. That is what a deleted current item
        // leaves behind, and nestedControlDeleted below is what answers it.
        if (Control* item = currentItem())
            startPreviewTimer(*item);
    }

    void StackPanel::nestedControlDeleted(Control* item)
    {
        StackPanelBase::nestedControlDeleted(item);
        if (item != m_previewItem)
            return;
        // The list being rebuilt under the pointer is the ordinary case here, not a rare one - a
        // rename does it. The timer goes with the item: what it was waiting to raise no longer
        // exists, and there is nothing else it could name.
        m_previewItem = nullptr;
        m_previewTimer.stop();
    }

    void StackPanel::previewItemChanged(PreviewEvent& event)
    {
        emitEvent(event);
    }

    // ONE ROUTINE, READ THROUGH MAIN AND CROSS. A wrapping stack does the same four things
    // whichever way its lanes run - cut the lane on its count or on the length it was given,
    // share what the stack has over the length among the items on each lane, share what it has
    // over the thickness among the lanes, and divide an Equal lane by its count - and writing
    // that twice is how each way round came to be missing whatever the other one had. See
    // Item-Containers
    void StackPanel::alignIntoLanes(AlignEvent& event, ScaledPosition position,
        ScaledDimensions& contentDimensions)
    {
        // The box the parent is handing over, held to whatever maximum this stack states. Read
        // main-first from here on, so that neither half of the pair can drift from the other.
        ScaledDimensions granted = {
            std::min(contentDimensions.x, event.maxContentWidth()),
            std::min(contentDimensions.y, event.maxContentHeight()) };
        // A COLUMN IS NOT BROKEN AT A HEIGHT ITS HOST SCROLLS. The measure breaks it at this
        // stack's own maximum alone, and a scroll box hands its body the viewport - so a cut
        // there makes lanes nobody measured. See Item-Containers
        if (!lanesRunAcross() && isScrolledByParent(ScrollAxis::Vertical))
            granted.y = std::max(granted.y, height() - event.padding.y * 2);
        const float limitMain = mainOf(granted);
        const float limitCross = crossOf(granted);
        contentDimensions = { 0.0f, 0.0f };
        m_maxLaneExtent = 0.0f;

        // AN EVEN LANE IS CUT ON ITS SHARES, not on the items. Every share is as long as the
        // largest item, so a lane holds as many of those as the length has room for, capped by the
        // lane's own count - and every lane then holds the SAME number. Cutting on the items
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

        // THE LANES ARE CUT TWICE, because what each one gets is settled by all of them and that
        // is known only once the last one has been cut. The cut is a walk over sizes already
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

        // WHAT THE LENGTH HAS OVER THE LONGEST LANE, which is all a lane has to spend - see
        // alignSharedLane. Read off THIS pass's cut, never off a walk asking the items how large
        // they are: an item that fills was left at the size the last pass gave it, so a metric
        // taken from the items is a metric taken from its own last answer - three tiles a lane
        // become wider tiles, wider tiles fit two, and two are wider still.
        const float surplus = surplusOver(limitMain, longestLane);
        // ONLY WHERE THE STACK WAS TOLD TO FILL THAT WAY. On a scrolled body the granted length is
        // the viewport whatever the alignment says - Control::align gives the surplus back only to
        // a control whose measure is SHORTER than its slot, and a wrapping stack measured against
        // no maximum is longer - so the length alone cannot say whether filling was asked for.
        //
        // DIVIDED BY THE ITEM, NOT BY THE LANE: the fullest lane's count is the divisor, so every
        // item in the stack grows by the same amount, the fullest lane comes out exactly at the
        // length, and a short last lane stays short.
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
                // A LANE HOLDING SOMETHING THAT ASKED TAKES ALL THE ROOM IT HAS, whether the stack
                // fills or not, as a single lane does - see alignContent, which reaches for the
                // granted length on hasFillingItem() alone. Read off the cut rather than by
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
        // Only the rows: the width is the extent the measure runs against - see calculateRows -
        // and there is no height for it to answer the same question about.
        if (lanesRunAcross())
            rememberWrapWidth(event, limitMain);
    }

    StackPanel::LaneCut StackPanel::nextLane(ControlSpan::iterator begin, const AlignEvent& event,
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

    // A STATED LANE BREAKS THE LANE WHATEVER THE LENGTH. The measure has already answered on the
    // count - see Control::calculateLanes - so a lane running past it here would be a lane nobody
    // measured, and the lanes it saves stand beside the items as dead space.
    // THE LAST SHARE CARRIES NO GAP AFTER IT, so the length a lane has to fit is one gap longer
    // than the shares it holds.
    //
    // ASKED BY THE ALIGN PASS AND ONLY BY IT. The measure must not break its lanes on this: it
    // would be measuring against the width the last pass laid out - see wrapWidthLimit - so a
    // narrow pass would shrink what the panel asks for, a host measured from the panel would grant
    // that, and the count would walk down one pass at a time and never come back.
    std::size_t StackPanel::sharesThatFit(const float largest, const float spacing,
        const float limitMain) const
    {
        const std::size_t perLane = itemsPerLane();
        const float share = largest + spacing;
        if (share <= 0.0f)
            return perLane;
        const float fits = std::floor((limitMain + spacing) / share);
        // ONE IS THE FLOOR. A share too long for the lane still stands in it, as the first item on
        // a natural lane does whatever its length.
        if (fits < 1.0f)
            return 1ull;
        // Compared before the cast: a length of k_maxFloat over a small share is a number no
        // std::size_t holds, and converting it is undefined rather than large.
        if (fits >= static_cast<float>(perLane))
            return perLane;
        return static_cast<std::size_t>(fits);
    }

    // A WRAPPING STACK THAT DIVIDES EVENLY IS A GRID, AND A GRID IS MEASURED AS ONE. Every share
    // is as long as the largest item, so a lane is that times the LANE'S count whatever the items
    // on it are called, and the lanes are as thick as the largest item across.
    //
    // Measured from the natural sum instead, a lane of four short items and a lane of two long
    // ones come to the same length, so the align breaks one at four and the other at two in the
    // same stack at the same width - and the item that overruns its share is clipped, because a
    // share it never measured against is not a width its text was broken at. A theme tile stating
    // its own fixed width was the workaround for this, and it is not needed once the grid is
    // measured as a grid.
    // THE LANE SHARES WHAT THE STACK WAS GIVEN OVER WHAT IT MEASURED. A stack that kept its own
    // size was given nothing over, and every lane stands where it measured.
    //
    // THE SURPLUS IS THE STACK'S, NOT THE LANE'S. Sharing out the difference between this lane and
    // the longest instead would stretch a short lane to the length of a long one while nothing at
    // all had been granted - a last row of two spread across a lane of five.
    //
    // WHAT ASKED FOR THE SURPLUS TAKES IT ALL, and where nothing asked it goes to the items in
    // equal parts - see Control::fillsLane and FlexSpacer.
    //
    // WHAT AN ITEM DOES WITH ITS PART IS THE ITEM'S TO SAY. One that fills takes it; one aligned
    // to an edge keeps the size it measured and stands in it - see Control::align.
    void StackPanel::alignSharedLane(AlignEvent& event, const LaneCut& lane,
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
    void StackPanel::placeInLane(AlignEvent& event, Control& control,
        const ScaledPosition lanePosition, const float from, const float extent,
        const float laneCross)
    {
        ScaledPosition itemPosition = lanePosition;
        mainOf(itemPosition) += from;
        alignControl(&control, event, itemPosition, lanePoint(extent, laneCross));
    }

    float StackPanel::wrapWidthLimit(const AlignEvent& event) const
    {
        if (m_wrapWidthInDesign <= 0.0f)
            return k_maxFloat;
        // NOT WHILE THE FORM IS ASKING FOR A WINDOW. That pass is where a form finds out what it
        // wants, and one held to the width it was last given could never grow - see
        // FormBase::isMeasuringPlacement.
        const FormBase* hostForm = getForm();
        if (hostForm && hostForm->isMeasuringPlacement())
            return k_maxFloat;
        // NOT WHERE THE WIDTH IS THE CONTENT'S OWN ANSWER, however many hosts down. A bound
        // stated there is derived from the thing it bounds: the rows wrap, the panel comes out
        // narrower, the host measured from it comes out narrower, and the next pass wraps them
        // again - a backstage walked down to three tiles a lane, one pass at a time.
        if (!isWidthGivenFromOutside())
            return k_maxFloat;
        return m_wrapWidthInDesign * event.scaleFactor();
    }

    // IN DESIGN UNITS, WITH THE SCALE THAT LAID IT OUT. Converting it at the scale in force when
    // it is read puts the two at different scales, and a form taken to 250% and back is left
    // measuring against a fraction of the width it has - see ScrollBox::adjustChildMetrics.
    //
    // A WIDTH THAT HAS NOT CHANGED ASKS FOR NOTHING, and that is what ends this. Comparing what
    // the rows CAME TO against what they measured cannot: Control::calculate ceils the content it
    // measured and the align pass does not, so a height with a fraction in it differs by up to a
    // whole unit for ever - every pass asks for another, and the program lays itself out until it
    // is killed. The width converges in one step because the pass that measures against it hands
    // back the same number.
    void StackPanel::rememberWrapWidth(AlignEvent& event, const float limitWidth)
    {
        if (!isWidthGivenFromOutside())
            return;
        // NOT WHILE THE FORM IS ASKING FOR A WINDOW. That pass lays the rows out at the content's
        // own size, not at a width the window gave - see FormBase::isMeasuringPlacement.
        const FormBase* hostForm = getForm();
        if (hostForm && hostForm->isMeasuringPlacement())
            return;
        const float widthInDesign = limitWidth / event.scaleFactor();
        if (std::abs(widthInDesign - m_wrapWidthInDesign) < k_wrapEpsilon)
            return;
        m_wrapWidthInDesign = widthInDesign;
        // The rows this pass laid out were measured against another width. Ask, do not lay out
        // from here - see AlignEvent::invalidatePass.
        event.invalidatePass();
    }

    // A CEILING IS SPREAD, A COUNT IS NOT. Under UpTo the items go into the fewest lanes the
    // count allows and every lane but the last is filled to the same depth, so eleven under a
    // ceiling of ten stands as six and five rather than ten and one.
    std::size_t StackPanel::itemsPerLane() const
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
    std::size_t StackPanel::visibleItems() const
    {
        std::size_t result = 0ull;
        for (const ControlPtr& item : controls())
        {
            if (item->visible())
                ++result;
        }
        return result;
    }

    // WHAT A WRAPPING STACK LACKED WAS A DIVISOR KNOWN BEFORE THE WRAP. Its lanes' own counts are
    // settled by the sizes the items came to, and dividing by those is dividing by the answer - so
    // Equal used to read only where a LaneSize stated the count outright. sharesThatFit is that
    // divisor without the statement: how many shares of the largest item the length has room for,
    // both of them known before a lane is cut. A stated lane is now a cap on it rather than the
    // only way to have one.
    bool StackPanel::dividesEvenly() const
    {
        return m_itemSizing == ItemSizing::Equal;
    }

    // A LANE WITH ONE IS AS LONG AS IT WAS GRANTED, not as long as it measured - there is no
    // surplus to hand out otherwise, and the item would take nothing. The same reach for the
    // granted box an even division makes, for the same reason. See Control::fillsLane
    bool StackPanel::hasFillingItem() const
    {
        for (const ControlPtr& item : controls())
            if (item->visible() && item->fillsLane())
                return true;
        return false;
    }

    StackPanel::LaneFill StackPanel::laneFill(float (Control::* extent)() const) const
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

    float StackPanel::evenLaneLength(const LaneFill& fill, const float spacing)
    {
        if (!fill.count)
            return 0.0f;
        const float count = static_cast<float>(fill.count);
        return fill.largest * count + spacing * (count - 1.0f);
    }

    // UNDER A PIXEL IS NOTHING OVER. Control::calculate ceils the content it measured and the
    // align pass does not, so a stack that was granted exactly what it asked for still answers a
    // fraction here - and a fraction shared out is every item moved by part of a pixel on a pass
    // that changed nothing.
    float StackPanel::surplusOver(const float granted, const float taken)
    {
        const float surplus = granted - taken;
        return surplus < 1.0f ? 0.0f : surplus;
    }

    void StackPanel::alignEvenLane(AlignEvent& event, ControlSpan::iterator laneBegin,
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

    void StackPanel::startPreviewTimer(Control& item)
    {
        if (m_previewItem == &item)
            return;
        m_previewItem = &item;
        m_previewTimer.start(k_previewDelay);
    }

    void StackPanel::onPreviewTimer()
    {
        // A GLIDE IS NOT THE USER LOOKING AT ANYTHING. Content travelling under a pointer that is
        // holding still hands the hover a new item on every frame - see ScrollBox's wheel glide,
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

}
