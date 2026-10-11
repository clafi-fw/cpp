export module ClaFi.Controls.DropList;

import ClaFi.Controls.Button;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // One line of a drop list: its words, and the hint beside the list while it is current.
    export struct DropListLine
    {
        Text text;
        Text hint;   // empty for none
    };

    export using DropListLines = std::vector<DropListLine>;

    // A line taken - Return on the current one, or a click on any - and how. See Controls#droplist
    export struct DropListPick
    {
        std::size_t line{ 0 };
        KeyModifiers modifiers{};   // held with the key or the click that took it
        InputStamp stamp{};         // the input that took it
    };

    // What the owner of a list does with a line taken.
    export using DropListPickFunc = std::function<void(const DropListPick&)>;

    export class DropList;

    // One line of the list, reached by the pointer and never focused.
    class DropListRow : public Button
    {
    public:
        DropListRow(const CreateParams&, DropList&, std::size_t line);
    protected:
        void getText(GetTextEvent&) const override;
        // The line's hint beside the list, at the row's height. See Controls#droplist
        void nestedGetHint(GetHintEvent&) override;
        void nestedClick(ClickEvent&) override;
    private:
        DropList& m_list;
        std::size_t m_line;
    };

    // The rows, one per line in order, and the row the keys are on. See Controls#droplist
    class DropListStack : public Stack
    {
    public:
        DropListStack(const CreateParams&, DropList&);
        // Shows a row per line of the list, the one given current, and hides the rows past them.
        void showLines(std::optional<std::size_t> current);
        [[nodiscard]] std::optional<std::size_t> current() const;
        void moveCurrent(ScrollDirection);         // a row on, staying at the ends
        void moveCurrentByPage(ScrollDirection);   // as many rows as the view holds
        // Puts the current row's hint up beside the list, or takes the hint down.
        void showCurrentHint();
    protected:
        // Every row may be current: the owner moves it, and the base's answer would rule that out.
        bool defaultCanFocusItem(Control&) override;
        // The current row reads selected - the base says so only for a stack that follows the user.
        void getControlState(GetStateEvent&) const override;
    private:
        [[nodiscard]] std::vector<Control*> shownRows() const;
        void setCurrentRow(Control*);
    private:
        DropList& m_list;
    };

    // The list as it is dropped: a box that scrolls, holding the rows.
    using DropListBox = ScrollBoxWith<DropListStack>;

    // A list dropped under a control that keeps the focus. See Controls#droplist
    export class DropList
    {
    public:
        DropList(Control& target, DropListPickFunc);
        DropList(const DropList&) = delete;
        DropList& operator=(const DropList&) = delete;
    public:
        [[nodiscard]] const DropListLines& lines() const { return m_lines; }
        [[nodiscard]] bool shown() const;
        // The line the keys are on, while the list is up and one is.
        [[nodiscard]] std::optional<std::size_t> current();
        // Shows the lines under the target, the one given current; no lines take the list down.
        void show(DropListLines, std::optional<std::size_t> current, FormPlacement);
        void hide();
        // Moves the current line; false where the list is down and the key is not its to take.
        bool moveCurrent(ScrollDirection);
        bool moveCurrentByPage(ScrollDirection);
        // Takes the current line as Return does; false where the list is down or no line current.
        bool pickCurrent(KeyModifiers, InputStamp);
        // Takes the line: the list goes down first, then the owner is told.
        void pick(const DropListPick&);
    private:
        void ensureForm();
    private:
        static constexpr float k_shownLines = 12.0f;   // what stands without a scroll bar
        static constexpr float k_padding = 4.0f;
        static constexpr float k_clearance = 2.0f;     // between the target and the list

        Control& m_target;
        DropListPickFunc m_pick;
        DropListLines m_lines{};
        std::optional<Form<DropListBox>> m_form{};
        ScopedEventConnection m_aligned{};
    };
}
