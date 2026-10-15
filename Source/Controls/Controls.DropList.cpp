module ClaFi.Controls.DropList;

import ClaFi.Controls.Button;
import ClaFi.Controls.Divider;
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

        // Whether every modifier named is held, where any is named.
        [[nodiscard]] bool holdsAll(const KeyModifiers held, const KeyModifiers named)
        {
            if (named.empty())
                return false;
            return (held.shift || !named.shift)
                && (held.ctrl || !named.ctrl)
                && (held.alt || !named.alt);
        }
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

    void DropListRow::click(ClickEvent& event)
    {
        m_list.pick({ .line = m_line, .modifiers = event.modifiers, .stamp = event.stamp });
    }

    // DropListStack

    DropListStack::DropListStack(const CreateParams& params, DropList& list)
        :
        StackPanel{ params,
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
        const DropListLines& lines = m_list.lines();
        const std::size_t count = lines.size();
        for (std::size_t i = m_rows.size(); i < count; ++i)
        {
            m_rows.push_back(&add<DropListRow>(m_list, i));
            m_dividers.push_back(nullptr);
        }
        for (std::size_t i = 0; i != m_rows.size(); ++i)
        {
            m_rows[i]->setVisible(i < count);
            // None under the last line, which nothing follows.
            showDivider(i, i + 1 < count && lines[i].dividerBelow);
        }
        Control* row = nullptr;
        if (current.has_value() && *current < count)
            row = m_rows[*current];
        setCurrentRow(row);
        // The lines are read as the rows are laid out, and a place may read another line now.
        invalidateFormAlign();
        invalidate();
    }

    // **Measured, not assumed**: a row is as tall as its text and padding make it, which a tool
    // button's least height says nothing of. A cap that moved asks for the pass again - the box
    // above was measured against the one before. See AlignEvent::invalidatePass
    ScaledSize DropListStack::calculateContent(AlignEvent& event)
    {
        const ScaledSize result = StackPanel::calculateContent(event);
        const std::vector<Control*> shown = shownRows();
        if (shown.empty())
            return result;
        const float rowHeight = shown.front()->scaledSize().y / event.scaleFactor();
        if (m_list.fitShownLines(rowHeight, cappedDividers(event.scaleFactor())))
            event.invalidatePass();
        return result;
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

    // **A list the keys can leave** enters at its first row from none, and Up on that row leaves
    // it with none - the target's own text is what the keys stand on then. Any other stays at its
    // ends, and enters from none at the end the key heads for. See UpFromFirst
    void DropListStack::moveCurrent(const ScrollDirection direction)
    {
        const std::vector<Control*> shown = shownRows();
        if (shown.empty())
            return;
        const bool leaves = m_list.options().upFromFirst == UpFromFirst::Leaves;
        const auto at = std::ranges::find(shown, currentItem());
        if (at == shown.end())
        {
            if (direction == ScrollDirection::ToEnd)
                setCurrentRow(shown.front());
            else if (!leaves)
                setCurrentRow(shown.back());
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
        else if (leaves)
            setCurrentRow(nullptr);
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
        StackPanel::getControlState(event);
        if (event.propagationStopped())
            return;
        event.state.selected = &event.control == currentItem();
        event.stopPropagation();
    }

    std::vector<Control*> DropListStack::shownRows() const
    {
        std::vector<Control*> shown;
        for (DropListRow* row : m_rows)
        {
            if (row->visible())
                shown.push_back(row);
        }
        return shown;
    }

    // **Made where one is first shown**, and moved up to stand right under its row: most lists
    // show none, and a row made later goes in after every divider there is.
    void DropListStack::showDivider(const std::size_t place, const bool shown)
    {
        Divider* divider = m_dividers[place];
        if (!divider)
        {
            if (!shown)
                return;
            divider = &add<Divider>(HorizontalAlign::Fill, Padding{ 0.0f, k_dividerPadding });
            std::size_t at = 0;
            while (controls()[at].get() != m_rows[place])
                ++at;
            moveControl(*divider, at + 1);
            m_dividers[place] = divider;
        }
        divider->setVisible(shown);
    }

    // A divider under the last line the cap holds stands below the cap, and adds nothing.
    float DropListStack::cappedDividers(const float scaleFactor) const
    {
        const float lines = m_list.options().shownLines;
        float result = 0.0f;
        for (std::size_t i = 0; i + 1 < m_dividers.size(); ++i)
        {
            if (static_cast<float>(i + 1) >= lines)
                break;
            const Divider* divider = m_dividers[i];
            if (divider && divider->visible())
                result += divider->scaledSize().y / scaleFactor + spacing().y;
        }
        return result;
    }

    void DropListStack::setCurrentRow(Control* row)
    {
        setCurrentItem(row);
        // Into view on the pass that follows, which is also the pass the hint is put up from, or
        // taken down from with no row - the hint is placed off the row's bounds, and those are
        // the pass's to give.
        if (row)
            row->scrollIntoViewOnAlign();
        invalidateFormAlign();
    }

    // DropListForm

    const Control* DropListForm::placementTarget() const
    {
        if (m_anchor)
            return m_anchor;
        return Form<DropListBox>::placementTarget();
    }

    // DropList

    DropList::DropList(Control& target, DropListPickFunc pick, const DropListOptions options)
        :
        m_target{ target },
        m_pick{ std::move(pick) },
        m_options{ options }
    {
    }

    bool DropList::shown() const
    {
        return m_form.has_value() && m_form->visible();
    }

    bool DropList::standsAbove() const
    {
        return shown() && m_form->window().standsAbove();
    }

    void DropList::setPlacedFunc(std::function<void()> placed)
    {
        m_placed = std::move(placed);
    }

    std::optional<std::size_t> DropList::current()
    {
        if (!shown())
            return std::nullopt;
        return m_form->content().body().current();
    }

    // **The anchor holds the target**: the target going down takes the list down, so the control
    // the list follows stands for as long as the list does.
    void DropList::show(DropListLines lines, const std::optional<std::size_t> current,
        const FormPlacement placement, const Control& anchor)
    {
        if (!anchor.containsNested(m_target))
            throw std::logic_error{ "A drop list's anchor holds its target" };
        showAt(std::move(lines), current, placement, anchor.boundsInForm(), &anchor);
    }

    void DropList::show(DropListLines lines, const std::optional<std::size_t> current,
        const FormPlacement placement, const FloatRect& anchor)
    {
        showAt(std::move(lines), current, placement, anchor, nullptr);
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
    // done with the list out of the way, and a list it shows again is shown anew. A pick holding
    // the modifiers the list is kept by leaves it standing, its current line where it was.
    void DropList::pick(const DropListPick& picked)
    {
        if (!holdsAll(picked.modifiers, m_options.keptBy))
            hide();
        if (m_pick)
            m_pick(picked);
    }

    // At least as wide as the anchor and the least width stated, and placed off the anchor - both
    // read now rather than once, since what the anchor stands for may have moved or grown since
    // the list last stood. In design units, which a popup shares with its parent.
    void DropList::showAt(DropListLines lines, const std::optional<std::size_t> current,
        const FormPlacement placement, const FloatRect& anchor, const Control* anchorControl)
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
        m_form->setMinWidth(std::max(anchor.width() / m_target.scaler().factor(),
            m_options.minWidth));
        // Ahead of the placement, which records where the control it follows stands.
        m_form->setAnchor(anchorControl);
        m_form->setPlacement(placement, anchor);
        if (shown())
            return;
        // Opened at its first row: where the list was left is not on screen to glide from.
        m_form->content().scrollToBegin();
        m_form->show();
    }

    bool DropList::fitShownLines(const float rowHeight, const float dividers)
    {
        const float lines = m_options.shownLines;
        if (lines <= 0.0f)
            return false;
        const DropListStack& stack = m_form->content().body();
        const float rows = lines * rowHeight + (lines - 1.0f) * stack.spacing().y + dividers
            + 2.0f * stack.padding().y;
        const float height = rows + 2.0f * k_padding;
        DropListBox& box = m_form->content();
        if (std::abs(box.maxSize().y - height) < k_capTolerance)
            return false;
        box.setMaxSize(MaxSize{ k_maxFloat, height });
        return true;
    }

    void DropList::ensureForm()
    {
        if (m_form.has_value())
            return;
        const ThemeMetrics& metrics = m_target.themeMetrics();
        // The shown lines at a tool button's least height until the rows are measured, and no
        // cap where none is stated.
        const float lines = m_options.shownLines;
        const float height = lines > 0.0f
            ? lines * metrics.toolButton.minSize.y + 2.0f * k_padding
            : k_maxFloat;
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
                MaxSize{ k_maxFloat, height },
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
        m_form->setDropdownClearance(m_options.clearance);
        // The hint is placed off the current row's bounds, which the pass that lays the rows out
        // gives, so it is put up from that pass - and the owner told where the list now stands.
        m_aligned = m_form->onAligned([this](FormAlignedEvent&){
            m_form->content().body().showCurrentHint();
            if (m_placed)
                m_placed();
        });
    }
}
