module;
#include "../Y-Core/System/EventBindings.h"
export module SeeDocs_App.Ui.BasesTree;

import SeeDocs_App.Pages;

import ClaFi.Controls.Base.Container;
import ClaFi.Controls.Base.DropdownControlBase;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.StyleSheet;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App::Ui
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // A base's crumb, or a line of the list behind its mark, was chosen.
    export class BaseLinkEvent : public ::ClaFi::Event
    {
    public:
        explicit BaseLinkEvent(std::wstring linkTarget);
    public:
        const std::wstring target;   // a type's qualified name
    };

    // A menu was asked for on a base's crumb.
    export class BaseLinkContextPopupEvent : public ::ClaFi::Event
    {
    public:
        BaseLinkContextPopupEvent(Control& owner, std::wstring_view linkTarget,
            ContextPopupEvent& request);
    public:
        Control& crumb;
        const std::wstring_view target;   // the page the crumb names
        ContextPopupEvent& popup;
    };

    export class BasesTree;

    /// One base on the bar: its spelling, and behind the mark before it the types standing on it.
    class BaseCrumb : public DropdownControlBase
    {
    public:
        BaseCrumb(const CreateParams&, const BaseNode&, const StyleSheet&,
            std::optional<Color> nameColor);
    public:
        DECLARE_EVENT(BaseLinkEvent, OnBaseLink, onBaseLink);
    public:
        std::wstring_view diagnosticText() const override { return L"BaseCrumb"; }
        const BaseNode& node() const { return m_node; }
        // The mark's centre, from the crumb's left edge: where a line to the bases under it leaves.
        [[nodiscard]] float markCenterX(const FormContext&) const;
    protected:
        void adjustMainText(AdjustTextEvent&) const override;
        void showDropdown(Control& initiator) override;
        void adjustPaint(AdjustPaintEvent&) override;
        void click(ClickEvent&) override;
        /// The mark sits on the crumb's face, as a path crumb's does.
        [[nodiscard]] Ink dropdownMarkInk() const override { return InkWell::textInk(); }
        /// The mark points back toward the type the way up came from, and turns down into its list.
        [[nodiscard]] DropdownMarkTurn dropdownMarkTurn() const override;
    private:
        const BaseNode& m_node;
        const StyleSheet& m_styles;   // what the base's spelling is set in
        const std::optional<Color> m_nameColor;   // the base's code, where it stands more than once
    };

    using BaseSpan = std::span<const BaseNode>;

    // A type's bases as crumbs joined by lines, a way up turning down where its row ends.
    export class BasesTree : public ContainerBase
    {
    public:
        template <typename... Args>
        explicit BasesTree(const CreateParams&, Args&&...);
    public:
        // Connects a handler raised when a crumb or a line of its list is chosen.
        DECLARE_EVENT(BaseLinkEvent, OnBaseLink, onBaseLink);
        // Connects a handler raised when a menu is asked for on a crumb.
        DECLARE_EVENT(BaseLinkContextPopupEvent, OnBaseLinkContextPopup, onBaseLinkContextPopup);
    public:
        // The crumbs of the bases, their words set in the sheet's styles; the sheet outlives them.
        virtual void setBases(const BaseNodes&, const StyleSheet&);
        virtual void clear();
    protected:
        ScaledSize calculateContent(AlignEvent&) override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledSize&) override;
        void paintSurface(PaintEvent&) override;
        void paintOwnText(PaintEvent&) override;
        // Right to the crumb's first base, Left to the crumb it is a base of, Up and Down by row.
        [[nodiscard]] Control* navigationNeighbor(const Control& child,
            const NavigationStep&) override;
        // From above or below, the crumb nearest the line on the near row; from a side, the first.
        [[nodiscard]] Control* navigationEntry(const NavigationStep&) override;
    private:
        // A line's two ends, in the tree's content coordinates.
        struct Line
        {
            FloatPoint from;
            FloatPoint to;
        };
        // The line down from a crumb to the bases stacked under it, turning into the last of them.
        struct Trunk
        {
            FloatPoint top;
            float bottomY;   // the last stacked base's row centre
            float endX;      // where the turn meets that base
        };
        // The room a crumb or a line takes on a row.
        struct Span
        {
            float left;
            float right;
        };
        using Lines = std::vector<Line>;
        using Trunks = std::vector<Trunk>;
        using RowSpans = std::vector<Span>;
        using Rows = std::vector<RowSpans>;
        using Ellipses = std::vector<FloatRect>;
        using ColorCodesBySpelling = ColorCodes<std::wstring>;
        // One placing of the crumbs: the room it may use, and what every row holds so far.
        struct Placing
        {
            float wallX{ 0.0f };       // where a row ends
            ScaledSpacing spacing{};
            float rowHeight{ 0.0f };   // a crumb's
            float rowPitch{ 0.0f };    // from one row's top to the next
            float indent{ 0.0f };      // from a crumb's left to a base stacked under it
            float clearance{ 0.0f };   // a crumb keeps this much from what it is not joined to
            ScaledSize ellipsisSize{};   // of the mark after a crumb whose bases are left out
            Rows rows{};
            float rightX{ 0.0f };      // the widest row's end
            int lastRow{ 0 };
        };
        using CrumbsMap = std::unordered_map<const BaseNode*, BaseCrumb*>;
        using ParentCrumbsMap = std::unordered_map<const BaseCrumb*, BaseCrumb*>;
    private:
        ScaledSize placeCrumbs(AlignEvent&, float wallX);
        void placeDirectBases(AlignEvent&, Placing&);
        void placeBases(AlignEvent&, Placing&, const BaseNode& parent);
        [[nodiscard]] bool fitsOnRow(const Placing&, const BaseCrumb& parent,
            const BaseNode&) const;
        [[nodiscard]] int freeRow(const Placing&, float width, float x, float trunkX,
            int previousRow) const;
        [[nodiscard]] float requiredWidth(const Placing&, const BaseNode&) const;
        // The room a crumb takes on its row: its width, plus its ellipsis where bases are left out.
        [[nodiscard]] float crumbRoom(const Placing&, const BaseNode&) const;
        void put(Placing&, const BaseNode&, int row, float x);
        void addLink(Placing&, float fromX, float toX, int row);
        void addStub(Placing&, Lines& stubs, float trunkX, int row, float x);
        void addTrunk(Placing&, const Lines& stubs, float trunkX, float topY, int fromRow,
            int toRow);
        [[nodiscard]] static bool rowFree(const Placing&, int row, Span);
        static void takeSpan(Placing&, int row, Span);
        [[nodiscard]] static int rowOf(const Placing&, const BaseCrumb&);
        [[nodiscard]] static float rowCenterY(const Placing&, int row);
        [[nodiscard]] BaseCrumb& crumbOf(const BaseNode&);
        [[nodiscard]] const BaseCrumb& crumbOf(const BaseNode&) const;
        void addCrumbs(BaseSpan, BaseCrumb* parentCrumb);
        // A step's line in the coordinates the crumbs are placed in.
        [[nodiscard]] FloatPoint lineAmongCrumbs(const NavigationStep&) const;
        // On the nearest row past fromTopY the way downward says, the crumb nearest lineX.
        [[nodiscard]] BaseCrumb* crumbNearLine(float fromTopY, bool downward, float lineX);
    private:
        BaseSpan m_bases{};
        const StyleSheet* m_styles{ &StyleSheet::application() };   // what the crumbs are set in
        CrumbsMap m_crumbsMap{};
        ParentCrumbsMap m_parentCrumbs{};   // each to the crumb it is a base of
        Lines m_links{};      // between row neighbours
        Lines m_branches{};   // the stubs a trunk runs on past, each leaving it with a turn
        Trunks m_trunks{};
        Ellipses m_ellipses{};   // after the crumbs whose bases are left out
        ColorCodesBySpelling m_colorCodes{};   // worn by the bases standing more than once
        float m_cornerRadius{ 0.0f };   // of a trunk's turn
    };

    template <typename ... Args>
    BasesTree::BasesTree(const CreateParams& params, Args&&... args)
        :
        ContainerBase{
            params,
            Spacing{ 8.0f },
            HorizontalAlign::Fill,
            VerticalAlign::Top,
            std::forward<Args>(args)...
        }
    {
    }
}
