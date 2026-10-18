export module SeeDocs_App.BasesBar;

import SeeDocs_App.Pages;

import ClaFi.Controls.Base.Container;
import ClaFi.Controls.Base.DropdownControlBase;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

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
        BaseLinkContextPopupEvent(Control& owner, std::wstring linkTarget,
            ContextPopupEvent& request);
    public:
        Control& crumb;
        const std::wstring target;   // the page the crumb names
        ContextPopupEvent& popup;
    };

    export class BasesBar;

    /// One base on the bar: its spelling, and behind the mark before it the types standing on it.
    class BaseCrumb : public DropdownControlBase
    {
    public:
        BaseCrumb(const CreateParams&, BasesBar&, const BaseNode&);
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
        BasesBar& m_bar;
        const BaseNode& m_node;
    };

    export class BasesTreeView : public ContainerBase
    {
        
    };

    // A class's bases as reversed crumbs, the ways up running right and parting where they fork.
    export class BasesBar : public ContainerBase
    {
        friend BaseCrumb;
    public:
        template<typename... Args>
        explicit BasesBar(const CreateParams&, Args&&...);
    public:
        // Connects a handler raised when a crumb or a line of its list is chosen.
        template<typename F>
        EventConnection onBaseLink(F&& callback);
        // Connects a handler raised when a menu is asked for on a crumb.
        template<typename F>
        EventConnection onBaseLinkContextPopup(F&& callback);
        // Builds a crumb per base, the bases kept by whoever shows them while the crumbs stand.
        void setBases(const BaseNodes&);
        void clearBases();
    protected:
        ScaledSize calculateContent(AlignEvent&) override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledSize&) override;
        // A crumb's width is read off it and narrowed here, as a row reads its items' widths.
        [[nodiscard]] bool isChildWidthGiven(const Control&) const override { return false; }
    private:
        using BaseSpan = std::span<const BaseNode>;
        // What a way up comes to from where it starts, the column it parts into included.
        struct RunExtent
        {
            float width{ 0.0f };
            float height{ 0.0f };
        };
    private:
        // A crumb per base, each way walked down before the next - the order every walk keeps.
        void addCrumbs(BaseSpan);
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

        BaseSpan m_bases{};
        std::vector<BaseCrumb*> m_crumbs{};   // in the order addCrumbs made them
        std::vector<float> m_fullWidths{};    // what each crumb measured
    };


    //-------------------------------------------------------------------------


    // BasesBar

    template<typename... Args>
    BasesBar::BasesBar(const CreateParams& params, Args&&... args)
        :
        ContainerBase{
            params,
            Padding{ 0.0f },
            std::forward<Args>(args)...
        }
    {
    }

    template<typename F>
    EventConnection BasesBar::onBaseLink(F&& callback)
    {
        return connectEvent<BaseLinkEvent>(std::forward<F>(callback));
    }

    template<typename F>
    EventConnection BasesBar::onBaseLinkContextPopup(F&& callback)
    {
        return connectEvent<BaseLinkContextPopupEvent>(std::forward<F>(callback));
    }
}
