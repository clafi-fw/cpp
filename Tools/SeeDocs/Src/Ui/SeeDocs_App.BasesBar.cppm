export module SeeDocs_App.BasesBar;

import SeeDocs_App.Pages;

import ClaFi.Controls.Base.DropdownControlBase;
import ClaFi.Controls.Stack;

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

    // One base on the bar: its spelling, and behind the mark before it the types standing on it.
    class BaseCrumb : public DropdownControlBase
    {
    public:
        BaseCrumb(const CreateParams&, BasesBar&, const BaseNode&);
    public:
        std::wstring_view diagnosticText() const override { return L"BaseCrumb"; }
    protected:
        void getMainText(GetTextEvent&) const override;
        void showDropdown(Control& initiator) override;
        void adjustPaint(AdjustPaintEvent&) override;
        void nestedClick(ClickEvent&) override;
        // The mark sits on the crumb's face, as a path crumb's does.
        [[nodiscard]] Ink dropdownMarkInk() const override { return InkWell::textInk(); }
        // The mark points back toward the type the way up came from, and turns down into its list.
        [[nodiscard]] DropdownMarkTurn dropdownMarkTurn() const override;
    private:
        BasesBar& m_bar;
        const BaseNode& m_node;
    };

    // A class's bases as reversed crumbs, the ways up running right and parting where they fork.
    export class BasesBar : public Stack
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
    private:
        // A crumb per base along one way up, and a column of rows where the way parts.
        void addRun(Stack& row, const BaseNodes&);
        void choose(std::wstring target);
        void askMenu(Control& crumb, std::wstring target, ContextPopupEvent&);
    };


    //-------------------------------------------------------------------------


    // BasesBar

    template<typename... Args>
    BasesBar::BasesBar(const CreateParams& params, Args&&... args)
        :
        Stack{
            params,
            Orientation::Horizontal,
            Padding{ 0.0f },
            Spacing{ 0.0f },
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
