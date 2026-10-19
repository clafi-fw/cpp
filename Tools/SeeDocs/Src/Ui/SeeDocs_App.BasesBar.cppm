module;
#include "../Y-Core/System/EventBindings.h"
export module SeeDocs_App.BasesBar;

import SeeDocs_App.Pages;

import ClaFi.Controls.Base.Container;
import ClaFi.Controls.Base.DropdownControlBase;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;
import ClaFi.Core.AppTheme_Colors;

namespace SeeDocs_App
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

    export class BasesTreeBase;

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
    protected:
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
    };

    using BaseSpan = std::span<const BaseNode>;
    using CrumbsVector = std::vector<BaseCrumb*>;

    export class BasesTreeBase : public ContainerBase
    {
        using ContainerBase::ContainerBase;
    public:
        // Connects a handler raised when a crumb or a line of its list is chosen.
        DECLARE_EVENT(BaseLinkEvent, OnBaseLink, onBaseLink);
        // Connects a handler raised when a menu is asked for on a crumb.
        DECLARE_EVENT(BaseLinkContextPopupEvent, OnBaseLinkContextPopup, onBaseLinkContextPopup);
    public:
        virtual void setBases(const BaseNodes&);
        virtual void clear();
        const CrumbsVector& crumbs() const { return m_crumbs; }
    protected:
        BaseSpan m_bases{};
        CrumbsVector m_crumbs{};   // in the order addCrumbs made them
    private:
        void addCrumbs(BaseSpan);
    };

    export class BasesBar1 : public BasesTreeBase
    {
        using BasesTreeBase::BasesTreeBase;
    public:
        template<typename... Args>
        explicit BasesBar1(const CreateParams&, Args...);
    };

    template <typename... Args>
    BasesBar1::BasesBar1(const CreateParams& params, Args... args)
        :
        BasesTreeBase{
            params,
            UiElement::Tooltip,
            std::forward<Args>(args)...
        }
    {
        
    }

    // A class's bases as reversed crumbs, the ways up running right and parting where they fork.
    export class BasesBar2: public BasesTreeBase
    {
        using BasesTreeBase::BasesTreeBase;
    public:
        void setBases(const BaseNodes&) override;
        void clear() override;
    protected:
        ScaledSize calculateContent(AlignEvent&) override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledSize&) override;
        // A crumb's width is read off it and narrowed here, as a row reads its items' widths.
        [[nodiscard]] bool isChildWidthGiven(const Control&) const override { return false; }
    private:
        // What a way up comes to from where it starts, the column it parts into included.
        struct RunExtent
        {
            float width{ 0.0f };
            float height{ 0.0f };
        };
    private:
        [[nodiscard]] RunExtent runExtent(BaseSpan, std::size_t& crumb, float cap) const;
        // The crumbs of one way up side by side, level with the middle of the height given, and a
        // column of rows where the way parts.
        void placeRun(AlignEvent&, BaseSpan, std::size_t& crumb, float cap, ScaledPosition,
            float height);
        [[nodiscard]] float crumbWidth(std::size_t crumb, float cap) const;
        // The widest a crumb may stay for the bar to come to no more than the width.
        [[nodiscard]] float capFor(float width, float leastCrumb) const;
        void choose(std::wstring target);
        void askMenu(Control& crumb, std::wstring target, ContextPopupEvent&) const;
    private:
        static constexpr float k_leastCrumb = 48.0f;   // what a narrowed crumb keeps of itself
        static constexpr int k_capSteps = 24;          // halvings of the range the cap is found in
        std::vector<float> m_fullWidths{};    // what each crumb measured
    };

}
