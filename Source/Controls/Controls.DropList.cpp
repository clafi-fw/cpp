module ClaFi.Controls.DropList;

import ClaFi.Controls.Button;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    namespace
    {
        constexpr float k_hintGap = 6.0f;   // how far the hint stands off the list's edge
    }

    // DropListRow

    DropListRow::DropListRow(const CreateParams& params, DropList& list, const std::size_t line)
        :
        Button{ params,
            // The look of a dropdown's line: no surface at rest, and the current row on its own.
            params.themeMetrics().toolButton,
            UiElement::ToolButton,
            ShowSelectionOnSurface::Yes,
            HorizontalTextAnchor::Left,
            // Every row is as wide as the list, which is what a flex space in a line spans to.
            HorizontalAlign::Fill,
            // Reached by the pointer and never focused - the caret stays in the target.
            Interactivity::MouseOnly
        },
        m_list{ list },
        m_line{ line }
    {
    }

    void DropListRow::getText(GetTextEvent& event) const
    {
        event.text << m_list.lines()[m_line].text;
    }

    void DropListRow::nestedGetHint(GetHintEvent& event)
    {
        const Text& hint = m_list.lines()[m_line].hint;
        if (hint.empty())
            return;
        // Beside the list at this row's height, on a rect spanning it so either side stands clear
        // of the rows. In the list form's coordinates, which the row's own bounds are in.
        const FloatRect listBounds = form().content().boundsInForm();
        const float gap = scaler().scale(k_hintGap);
        const FloatRect row = boundsInForm();
        event.anchorRect = { listBounds.left - gap, row.top, listBounds.right + gap, row.bottom };
        event.placement = FormPlacement::Right;
        event.text << hint;
    }

    void DropListRow::nestedClick(ClickEvent& event)
    {
        m_list.pick({ .line = m_line, .modifiers = event.modifiers, .stamp = event.stamp });
    }

    // DropListStack

    DropListStack::DropListStack(const CreateParams& params, DropList& list)
        :
        Stack{ params,
            Orientation::Vertical,
            // **A scrolled body keeps the size it measured** - see ComboBoxDropdownStack.
            WordWrap::No
        },
        m_list{ list }
    {
    }

    // **A row stands for a place in the list, not for a line**: the row at a place reads the line
    // there, so new lines go into the rows standing and no row goes down under a click on it.
    void DropListStack::showLines(const std::optional<std::size_t> current)
    {
        const std::size_t count = m_list.lines().size();
        for (std::size_t i = controls().size(); i < count; ++i)
            add<DropListRow>(m_list, i);
        for (std::size_t i = 0; i != controls().size(); ++i)
            controls()[i]->setVisible(i < count);
        Control* row = nullptr;
        if (current.has_value() && *current < count)
            row = controls()[*current].get();
        setCurrentRow(row);
        // The lines are read as the rows are laid out, and a place may read another line now.
        invalidateFormAlign();
        invalidate();
    }

    std::optional<std::size_t> DropListStack::current() const
    {
        const Control* row = currentItem();
        if (!row)
            return std::nullopt;
        const std::vector<Control*> shown = shownRows();
        const auto at = std::ranges::find(shown, row);
        if (at == shown.end())
            return std::nullopt;
        return static_cast<std::size_t>(at - shown.begin());
    }

    void DropListStack::moveCurrent(const ScrollDirection direction)
    {
        const std::vector<Control*> shown = shownRows();
        if (shown.empty())
            return;
        const auto at = std::ranges::find(shown, currentItem());
        if (at == shown.end())
        {
            setCurrentRow(direction == ScrollDirection::ToEnd ? shown.front() : shown.back());
            return;
        }
        if (direction == ScrollDirection::ToEnd)
        {
            if (std::next(at) != shown.end())
                setCurrentRow(*std::next(at));
            return;
        }
        if (at != shown.begin())
            setCurrentRow(*std::prev(at));
    }

    void DropListStack::moveCurrentByPage(const ScrollDirection direction)
    {
        const std::vector<Control*> shown = shownRows();
        if (shown.empty())
            return;
        const auto at = std::ranges::find(shown, currentItem());
        if (at == shown.end())
        {
            setCurrentRow(shown.front());
            return;
        }

        // The rows from the current one to the one it lands on fill the view at most, so the row
        // left current is still in sight - the step FocusNavigator pages any items view by.
        const float view = windowInForm().height();
        const bool forward = direction == ScrollDirection::ToEnd;
        std::size_t target = static_cast<std::size_t>(at - shown.begin());
        float filled = shown[target]->boundsInForm().height();
        while (forward ? target + 1 < shown.size() : target > 0)
        {
            const std::size_t next = forward ? target + 1 : target - 1;
            filled += shown[next]->boundsInForm().height();
            if (filled > view)
                break;
            target = next;
        }
        setCurrentRow(shown[target]);
    }

    void DropListStack::showCurrentHint()
    {
        // A pass over a list that is down places nothing anyone can see.
        if (!form().visible())
            return;
        if (Control* row = currentItem())
            form().hint().showRightNow(*row);
        else
            Hint::stopAndHide();
    }

    bool DropListStack::defaultCanFocusItem(Control& value)
    {
        return value.parent() == this;
    }

    void DropListStack::getControlState(GetStateEvent& event) const
    {
        Stack::getControlState(event);
        if (event.propagationStopped())
            return;
        event.state.selected = &event.control == currentItem();
        event.stopPropagation();
    }

    std::vector<Control*> DropListStack::shownRows() const
    {
        std::vector<Control*> shown;
        for (const ControlPtr& row : controls())
        {
            if (row->visible())
                shown.push_back(row.get());
        }
        return shown;
    }

    void DropListStack::setCurrentRow(Control* row)
    {
        setCurrentItem(row);
        if (!row)
            return;
        // Into view on the pass that follows, which is also the pass the hint is put up from -
        // the hint is placed off the row's bounds, and those are the pass's to give.
        row->scrollIntoViewOnAlign();
        invalidateFormAlign();
    }

    // DropList

    DropList::DropList(Control& target, DropListPickFunc pick)
        :
        m_target{ target },
        m_pick{ std::move(pick) }
    {
    }

    bool DropList::shown() const
    {
        return m_form.has_value() && m_form->visible();
    }

    std::optional<std::size_t> DropList::current()
    {
        if (!shown())
            return std::nullopt;
        return m_form->content().body().current();
    }

    // At least as wide as the target, and placed off it - both read now rather than once, since
    // the target may have moved or grown since the list last stood. In design units, which a popup
    // shares with its parent.
    void DropList::show(DropListLines lines, const std::optional<std::size_t> current,
        const FormPlacement placement)
    {
        if (lines.empty())
        {
            // Down first, and its rows hidden: a row reads the line at its place, which goes with
            // the lines.
            hide();
            m_lines.clear();
            if (m_form.has_value())
                m_form->content().body().showLines(std::nullopt);
            return;
        }
        ensureForm();
        m_lines = std::move(lines);
        m_form->content().body().showLines(current);
        m_form->setMinWidth(m_target.width() / m_target.scaler().factor());
        m_form->setPlacement(placement, m_target.boundsInForm());
        if (shown())
            return;
        // Opened at its first row: where the list was left is not on screen to glide from.
        m_form->content().scrollToBegin();
        m_form->show();
    }

    void DropList::hide()
    {
        if (!shown())
            return;
        // The hint beside the current row goes with the list.
        Hint::stopAndHide();
        m_form->close();
    }

    bool DropList::moveCurrent(const ScrollDirection direction)
    {
        if (!shown())
            return false;
        m_form->content().body().moveCurrent(direction);
        return true;
    }

    bool DropList::moveCurrentByPage(const ScrollDirection direction)
    {
        if (!shown())
            return false;
        m_form->content().body().moveCurrentByPage(direction);
        return true;
    }

    bool DropList::pickCurrent(const KeyModifiers modifiers, const InputStamp stamp)
    {
        const std::optional<std::size_t> line = current();
        if (!line.has_value())
            return false;
        pick({ .line = *line, .modifiers = modifiers, .stamp = stamp });
        return true;
    }

    // Down before the owner hears: what it does with the line - a page opened, a text taken - is
    // done with the list out of the way, and a list it shows again is shown anew.
    void DropList::pick(const DropListPick& picked)
    {
        hide();
        if (m_pick)
            m_pick(picked);
    }

    void DropList::ensureForm()
    {
        if (m_form.has_value())
            return;
        const ThemeMetrics& metrics = m_target.themeMetrics();
        m_form.emplace(
            m_target.appContext(),
            WindowRole::Menu,
            &m_target,
            HostProps{
                metrics.secondaryWindow,
                metrics.secondaryWindowShadow,
                UiElement::Section,
                // A list taller than its room scrolls - see ComboBox::showDropdown.
                ScrollBars::Auto,
                MaxSize{
                    k_maxFloat,
                    k_shownLines * metrics.toolButton.minSize.y + 2.0f * k_padding
                },
                Padding{ k_padding },
                // Nothing in the list takes the focus: the rows are reached by the pointer, and
                // by the keys the target hands on.
                Interactivity::MouseOnly
            },
            BodyProps{ *this }
        );
        // Sized by what is listed, which changes under it as the target asks; and hidden rather
        // than destroyed when nothing is listed, since the next ask may list something again.
        m_form->setAutoFit(AutoFit::Yes);
        m_form->setCloseAction(CloseAction::Hide);
        m_form->setDropdownClearance(k_clearance);
        // The hint is placed off the current row's bounds, which the pass that lays the rows out
        // gives, so it is put up from that pass.
        m_aligned = m_form->onAligned([this](FormAlignedEvent&){
            m_form->content().body().showCurrentHint();
        });
    }
}
