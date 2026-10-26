module;
#include "../Y-Core/System/EventBindings.h"
export module SeeDocs_App.Ui.BasesTree;

import SeeDocs_App.Pages;

import ClaFi.Controls.Base.Container;
import ClaFi.Controls.Base.DropdownControlBase;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;
import ClaFi.Core.AppTheme_Colors;

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
        BaseCrumb(const CreateParams&, const BaseNode&);
    public:
        DECLARE_EVENT(BaseLinkEvent, OnBaseLink, onBaseLink);
    public:
        std::wstring_view diagnosticText() const override { return L"BaseCrumb"; }
        const BaseNode& node() const { return m_node; }
        int moved() const { return m_moved; }
        void incMoved() { ++m_moved; }
    protected:
        ScaledSize calculateContent(AlignEvent&) override;
        void getMainText(GetTextEvent&) const override;
        void showDropdown(Control& initiator) override;
        void adjustPaint(AdjustPaintEvent&) override;
        void click(ClickEvent&) override;
        /// The mark sits on the crumb's face, as a path crumb's does.
        [[nodiscard]] Ink dropdownMarkInk() const override { return InkWell::textInk(); }
        /// The mark points back toward the type the way up came from, and turns down into its list.
        [[nodiscard]] DropdownMarkTurn dropdownMarkTurn() const override;
    private:
        const BaseNode& m_node;
        int m_moved;
    };

    using BaseSpan = std::span<const BaseNode>;

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
        virtual void setBases(const BaseNodes&);
        virtual void clear();
    protected:
        ScaledSize calculateContent(AlignEvent&) override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledSize&) override;
        void paintSurface(PaintEvent&) override;
    private:
        ScaledSize calculateBases(AlignEvent&, const BaseSpan&, ScaledPosition, float wallX);
        void offsetBases(AlignEvent&, const BaseSpan&, FloatPoint);
        ScaledSize calculateBranch(AlignEvent&, const BaseNode&, ScaledPosition, float wallX);
        void addCrumbs(BaseSpan);
        BaseSpan m_bases{};
        using CrumbsVector = std::vector<BaseCrumb*>;
        using CrumbsMap = std::unordered_map<const BaseNode*, BaseCrumb*>;
        //CrumbsVector m_crumbs{};   // in the order addCrumbs made them
        CrumbsMap m_crumbsMap{};
    };

    template <typename ... Args>
    BasesTree::BasesTree(const CreateParams& params, Args&&... args)
        :
        ContainerBase{
            params,
            Spacing{ 8.0f },
            HorizontalAlign::Fill,
            VerticalAlign::Top,
            UiElement::Section,
            std::forward<Args>(args)...
        }
    {
        
    }
}
