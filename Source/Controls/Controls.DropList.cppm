export module ClaFi.Controls.DropList;

import ClaFi.Controls.Button;
import ClaFi.Controls.Divider;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // One line of a drop list: its words, and the tooltip beside the list while it is current.
    export struct DropListLine
    {
        Text text;
        Text tooltip;   // empty for none
        bool dividerBelow{ false };   // parts it from the lines after
    };

    export using DropListLines = std::vector<DropListLine>;

    // A line taken - Return on the current one, or a click on any - and how.
    export struct DropListPick
    {
        std::size_t line{ 0 };
        KeyModifiers modifiers{};   // held with the key or the click that took it
        InputStamp stamp{};         // the input that took it
    };

    // What the owner of a list does with a line taken.
    export using DropListPickFunc = std::function<void(const DropListPick&)>;

    // What Up does on a list's first line.
    export enum class UpFromFirst
    {
        Stays,    // the first line stays current
        Leaves    // no line is current, and the keys are the target's own again
    };

    // How a list stands and is walked, as its owner states it.
    export struct DropListOptions
    {
        float shownLines{ 12.0f };   // the lines that stand before it scrolls; none caps it
        float minWidth{ 0.0f };      // the least it is wide, the anchor's width aside
        float clearance{ 2.0f };     // between the anchor and the list
        KeyModifiers keptBy{};       // a pick holding all of these leaves the list up
        UpFromFirst upFromFirst{ UpFromFirst::Stays };
    };

    export class DropList;

    // One line of the list, reached by the pointer and never focused.
    class DropListRow : public Button
    {
    public:
        DropListRow(const CreateParams&, DropList&, std::size_t line);
    protected:
        void getText(GetTextEvent&) const override;
        // The line's tooltip beside the list, at the row's height. See Controls#droplistline
        void nestedGetTooltip(GetTooltipEvent&) override;
        void click(ClickEvent&) override;
    private:
        DropList& m_list;
        std::size_t m_line;
    };

    // The rows, one per line in order, the dividers under them, and the row the keys are on.
    class DropListStack : public StackPanel
    {
    public:
        DropListStack(const CreateParams&, DropList&);
        // Shows a row per line of the list, the one given current, and hides the rows past them.
        void showLines(std::optional<std::size_t> current);
        [[nodiscard]] std::optional<std::size_t> current() const;
        void moveCurrent(ScrollDirection);         // a row on, to the ends - see UpFromFirst
        void moveCurrentByPage(ScrollDirection);   // as many rows as the view holds
        // Puts the current row's tooltip up beside the list, or takes the tooltip down.
        void showCurrentTooltip();
    protected:
        // Measures the rows, then caps the list at twelve of them. See Controls#droplistoptions
        ScaledSize calculateContent(AlignEvent&) override;
        // Every row may be current: the owner moves it, and the base's answer would rule that out.
        bool defaultCanFocusItem(Control&) override;
        // The current row reads selected - the base says so only for an ActiveContainer.
        void getControlState(GetStateEvent&) const override;
    private:
        [[nodiscard]] std::vector<Control*> shownRows() const;
        void setCurrentRow(Control*);
        // Shows or hides the divider under the row at that place, made the first time it is shown.
        void showDivider(std::size_t place, bool shown);
        // What the dividers among the lines the cap holds add to its height, in design units.
        [[nodiscard]] float cappedDividers(float scaleFactor) const;
    private:
        static constexpr float k_dividerPadding = 4.0f;   // above and below a divider's line

        DropList& m_list;
        std::vector<DropListRow*> m_rows{};   // a row per place, in order
        std::vector<Divider*> m_dividers{};   // under the row at the same place; null until shown
    };

    // The list as it is dropped: a box that scrolls, holding the rows.
    using DropListBox = ScrollBoxWith<DropListStack>;

    // The list's window, which goes along with the anchor control where one is stated.
    class DropListForm : public Form<DropListBox>
    {
    public:
        using Form<DropListBox>::Form;
    public:
        void setAnchor(const Control* anchor) { m_anchor = anchor; }
    protected:
        [[nodiscard]] const Control* placementTarget() const override;
    private:
        const Control* m_anchor{ nullptr };   // the control the list follows; null for the target
    };

    // A list dropped under a control that keeps the focus. See Controls#droplist
    export class DropList
    {
        friend DropListStack;
    public:
        // The target is the control the focus stays in.
        DropList(Control& target, DropListPickFunc, DropListOptions = {});
        DropList(const DropList&) = delete;
        DropList& operator=(const DropList&) = delete;
    public:
        [[nodiscard]] const DropListOptions& options() const { return m_options; }
        [[nodiscard]] const DropListLines& lines() const { return m_lines; }
        [[nodiscard]] bool shown() const;
        // Whether the list stands above its anchor, for want of room below it.
        [[nodiscard]] bool standsAbove() const;
        // Told after every pass that lays the list out, which may have moved it to the other side.
        void setPlacedFunc(std::function<void()>);
        // The line the keys are on, while the list is up and one is.
        [[nodiscard]] std::optional<std::size_t> current();
        // Shows the lines off a control holding the target, and follows it.
        void show(DropListLines, std::optional<std::size_t> current, FormPlacement,
            const Control& anchor);
        // Shows the lines off the anchor, a rect in the target's form.
        void show(DropListLines, std::optional<std::size_t> current, FormPlacement,
            const FloatRect& anchor);
        void hide();
        // Moves the current line; false where the list is down and the key is not its to take.
        bool moveCurrent(ScrollDirection);
        bool moveCurrentByPage(ScrollDirection);
        // Takes the current line as Return does; false where the list is down or no line current.
        bool pickCurrent(KeyModifiers, InputStamp);
        // Takes the line: the list goes down first, then the owner is told.
        void pick(const DropListPick&);
    private:
        // What both overloads of show do, the list following the control given, else the target.
        void showAt(DropListLines, std::optional<std::size_t> current, FormPlacement,
            const FloatRect& anchor, const Control* anchorControl);
        void ensureForm();
        // Caps the list at its shown lines of the height measured, with the dividers among them;
        // true where the cap moved.
        bool fitShownLines(float rowHeight, float dividers);
    private:
        static constexpr float k_padding = 4.0f;
        static constexpr float k_capTolerance = 0.5f;  // a cap nearer than this stands

        Control& m_target;
        DropListPickFunc m_pick;
        DropListOptions m_options;
        std::function<void()> m_placed{};
        DropListLines m_lines{};
        std::optional<DropListForm> m_form{};
        ScopedEventConnection m_aligned{};
    };
}
