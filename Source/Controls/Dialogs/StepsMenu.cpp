module ClaFi.Controls.StepsMenu;

import ClaFi.Controls.Menu;
import ClaFi.Controls.Divider;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;

import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    StepsMenu::StepsMenu(Control& owner, Text verb)
        :
        StepsMenuForm{
            // The same window as a menu: the pointer and the focus, never the activation.
            owner.appContext(), WindowRole::Menu, &owner,
            owner.themeMetrics().secondaryWindow,
            owner.themeMetrics().secondaryWindowShadow,
            UiElement::Menu,
            Interactivity::MouseOnly
        },
        m_verb{ std::move(verb) }
    {
        setAutoFit(AutoFit::Yes);
        m_foot.connectEvent(this, &StepsMenu::footText);
        m_foot.onClick([this](ClickEvent&) {
            take(m_run);
        });
    }

    void StepsMenu::add(const Text& step)
    {
        const std::size_t index = m_items.size();
        MenuItem& item = m_list.add<MenuItem>(
            step,
            OnClick{ [this, index](ClickEvent&) {
                take(index + 1ull);
            } }
        );
        // The pointer has left this step for another, which names the run now, or for anything
        // else, which names none. Input has already moved on, so it says which.
        item.connectEvent([this](HoverLeaveEvent&) {
            setRun(runOf(Input::hoveredControl()));
        });
        m_items.push_back(&item);
    }

    std::size_t StepsMenu::executeUnder(const Control& control)
    {
        if (empty())
            return 0ull;
        // Dropped the way a menu drops under a control - see Menu::executeUnder.
        setDropdownClearance(1.0f);
        setPlacement(FormPlacement::Bottom, control.boundsInForm());
        setMinWidth(control.width() / scaler().factor());
        StepsMenuForm::execute();
        return m_taken;
    }

    // Every step down to the one in effect reads as hovered, which is what shows the run.
    void StepsMenu::adjustNestedControlVisualState(const Control& control, VisualState& state) const
    {
        const std::size_t place = runOf(&control);
        if (place != 0ull && place <= m_run)
            state.hovered = true;
    }

    // A step under the pointer names the run; the pointer arriving from off the steps is what
    // this hears, the leave of the step before it is what the items hear - see add.
    void StepsMenu::nestedControlHovered(Control* hovered)
    {
        Panel::nestedControlHovered(hovered);
        if (const std::size_t run = runOf(hovered))
            setRun(run);
    }

    // The arrows walk the steps, and the run follows the one they stand on.
    void StepsMenu::nestedControlFocusing(FocusEvent& event)
    {
        Panel::nestedControlFocusing(event);
        if (const std::size_t run = runOf(event.control))
            setRun(run);
    }

    std::size_t StepsMenu::runOf(const Control* control) const
    {
        for (std::size_t i = 0ull; i != m_items.size(); ++i)
            if (m_items[i] == control)
                return i + 1ull;
        return 0ull;
    }

    // The lit look is a STATE, reached through invalidateState and animated from there - a repaint
    // alone draws the factors the item last settled on. The items between the two runs are the
    // ones whose state moved.
    void StepsMenu::setRun(const std::size_t value)
    {
        if (value == m_run)
            return;
        const std::size_t first = std::min(m_run, value);
        const std::size_t last = std::max(m_run, value);
        m_run = value;
        for (std::size_t i = first; i != last; ++i)
            m_items[i]->invalidateState();
        // The foot's words change length with the count.
        m_foot.invalidateFormAlign();
    }

    // The item closes the menu behind its click - see MenuItem::nestedClick - so all that is
    // left to say is the answer.
    void StepsMenu::take(const std::size_t steps)
    {
        m_taken = steps;
    }

    void StepsMenu::footText(GetTextEvent& event)
    {
        if (m_run == 0ull)
        {
            event.text << L"Cancel";
            return;
        }
        event.text << m_verb << L' ' << std::to_wstring(m_run);
        event.text << (m_run == 1ull ? L" step" : L" steps");
    }
}
