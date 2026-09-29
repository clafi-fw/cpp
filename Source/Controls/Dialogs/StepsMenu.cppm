export module ClaFi.Controls.StepsMenu;

import ClaFi.Controls.Menu;
import ClaFi.Controls.Divider;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    using StepsMenuForm = Form<Panel>;

    // A popup listing steps from the top, where the pointer picks how many to take. See Controls
    export class StepsMenu : public StepsMenuForm
    {
    public:
        // The verb is what the foot says of the run - Undo, Redo.
        StepsMenu(Control& owner, Text verb);
    public:
        void add(const Text& step); // the next step down - the first added stands at the top
        [[nodiscard]] bool empty() const { return m_items.empty(); }
        // Drops the list under a control and runs it; answers the steps taken from the top, if any.
        [[nodiscard]] std::size_t executeUnder(const Control&);
    protected:
        void adjustNestedControlVisualState(const Control&, VisualState&) const override;
        void nestedControlHovered(Control* hovered) override;
        void nestedControlFocusing(FocusEvent&) override;
    private:
        using Items = std::vector<MenuItem*>;
    private:
        // The run a control names: its place from the top plus one for an item, none otherwise.
        [[nodiscard]] std::size_t runOf(const Control*) const;
        void setRun(std::size_t);
        void take(std::size_t steps);
        void footText(GetTextEvent&);
    private:
        // How tall the list may stand before it scrolls, in design units.
        static constexpr float k_maxListHeight = 400.0f;
    private:
        Text m_verb;
        Items m_items{};
        std::size_t m_run{ 0 }; // how many steps from the top are lit, none for 0
        std::size_t m_taken{ 0 }; // what the menu answers, written by the click that closed it

        // A maximum is what puts the bar up: Auto answers against the ceiling the pass has.
        ScrollBox& m_box{ createBody<ScrollBox>(
            ScrollBars::Auto,
            MaxSize{ k_maxFloat, k_maxListHeight },
            Padding{ 4.0f }
        ) };

        StackPanel& m_list{ m_box.createBody<StackPanel>(
            Orientation::Vertical
        ) };

        StackPanel& m_footBar{ createBottomBar<StackPanel>(
            Orientation::Vertical,
            Padding{ 4.0f }
        ) };

        Divider& m_rule{ m_footBar.add<Divider>(
            Padding{ 0.0f, 4.0f }
        ) };

        // Reads what a click takes, or Cancel; mouse only keeps the arrows on the steps.
        MenuItem& m_foot{ m_footBar.add<MenuItem>(
            Interactivity::MouseOnly
        ) };
    };
}
