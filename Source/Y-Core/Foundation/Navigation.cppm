export module ClaFi.Core.Foundation :Navigation;

import :Control;
import :Form;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    // Moves the focus on the keys no control on the focus path took. See Control-Foundation
    export class FocusNavigator
    {
    public:
        explicit FocusNavigator(FormBase& form) : m_form{ form } {}
    public:
        void formKeyDown(KeyDownEvent& event);
        // Puts the focus where showing a control lands it. See Control-Foundation
        Control* enterShown(Control&);
    private:
        // The form's root has no outside, so the focus never enters, leaves or lands on it.
        [[nodiscard]] bool isFormRoot(const Control*) const;
        // The stop a step from item lands on, its parents asked up to boundary.
        [[nodiscard]] Control* arrowStop(Control& item, const NavigationStep&,
            const Control* boundary);
        // The stop a step entering control lands on.
        [[nodiscard]] Control* enter(Control& control, const NavigationStep&);
        // The next stop in reading order for Tab, the one before it for Shift+Tab.
        [[nodiscard]] Control* tabStop(Control& item, Control* sourceContainer, bool reverse);
        // The stop at one end of a container, in reading order.
        [[nodiscard]] Control* edgeStop(Control& container, ScrollDirection edge);
        // The stop control is, or holds at the end entered from.
        [[nodiscard]] Control* readingEntry(Control& control, ScrollDirection edge);
        // The stop showing control lands on, every container's current item followed first.
        [[nodiscard]] Control* shownEntry(Control& control);
        // Home, End and the page keys inside the container in scope.
        void moveInContainer(Control& container, Control& item, KeyCode, KeyModifiers);
        // The scrolled content holding item inside container, or container where none does.
        [[nodiscard]] Control& pagedContent(Control& container, Control& item) const;
        // The way a page press travels, none where the content shows all it holds.
        [[nodiscard]] std::optional<NavigationDirection> pageDirection(Control& content,
            ScrollDirection) const;
        // The stop a run of steps from item reaches before it has crossed a page of content.
        [[nodiscard]] Control* pageStop(Control& container, Control& content, Control& item,
            const NavigationStep&);
        // A step from item, with the line the run it continues keeps.
        [[nodiscard]] NavigationStep stepFrom(const Control& item, NavigationDirection,
            NavigationSource) const;
        // Moves the focus to stop by a key, keeping the line the step ran along for the next press.
        void land(Control& stop, const NavigationStep&, KeyModifiers);
        // Moves the focus to stop by a key, a move that keeps no line.
        void land(Control& stop, KeyModifiers);
    private:
        FormBase& m_form;
    };

}
