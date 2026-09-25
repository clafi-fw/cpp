module ClaFi.Controls.InPlaceEdit;

import ClaFi.Controls.TextItems;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.Button;
import ClaFi.Controls.Panel;

import ClaFi.Core.Foundation;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.Core.System.Utils;
import ClaFi.Core.System.Props;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // AcceptEditEvent

    AcceptEditEvent::AcceptEditEvent(Control& askedBy, const Text& text)
        :
        askedBy{ askedBy },
        text{ text }
    {
    }

    void AcceptEditEvent::refuse(const std::wstring_view why)
    {
        m_refused = true;
        m_reason = why;
    }

    void AcceptEditEvent::refuse()
    {
        m_refused = true;
    }

    // EditBox

    FloatPoint EditBox::textOriginInForm() const
    {
        return textBounds(formContext(), boundsInForm()).topLeft();
    }

    void EditBox::setRefusal(std::wstring_view value)
    {
        m_refusal = value;
    }

    void EditBox::showRefusal()
    {
        if (m_refusal.empty())
            return;
        // Raised rather than waited for. A message is shown at once and goes as any tooltip
        // goes, which is what puts the reason in front of a user who is looking at the keyboard;
        // getTooltip below is what answers every hover after that, for as long as the value is
        // still refused.
        ContextMessage::show(*this, Text{ m_refusal });
    }

    void EditBox::getTooltip(GetTooltipEvent& event)
    {
        if (m_refusal.empty())
        {
            TextBox::getTooltip(event);
            return;
        }
        // A refusal stands in front of anything else this box would say, and it is placed under
        // the box rather than at the pointer: it answers what was TYPED, and the user reaching
        // for Enter has left the pointer wherever it happened to be.
        event.text << m_refusal;
        event.placement = FormPlacement::Bottom;
        event.anchorRect = boundsInForm();
    }

    void EditBox::keyDown(KeyDownEvent& event)
    {
        // Any press at all answers a refusal - reading it is what makes it stale, and the user
        // is now doing something about it. This runs before the root sees the key, so a Return
        // that is refused again puts the reason back up afterwards.
        clearRefusal();
        m_sawPress = true;
        if (event.key == Keys::Return)
            m_takeLineBreak = event.modifiers.shift;
        // THE LIST'S KEYS. Up and Down are the list's while it is up, and Alt+Down - the key
        // that drops a combobox's list - is the list's always. The walk reaches the box before
        // the root, and the box would move the caret on them; left alone here, they reach the
        // root next. F4, the other dropping key, is a key the box never takes.
        const bool arrow = event.key == Keys::Up || event.key == Keys::Down;
        const bool dropsList = event.key == Keys::Down && event.modifiers.alt;
        if (dropsList || (arrow && isDroppedDown()))
            return;
        TextBox::keyDown(event);
    }

    void EditBox::pressDown(PressDownEvent& event)
    {
        // Clicking into the box to correct the value answers the refusal as much as typing
        // does. The tooltip window is hidden by the form's own mouse handling; what has to go
        // here is the box's memory of it, or every later hover reads the stale reason back.
        clearRefusal();
        TextBox::pressDown(event);
    }

    void EditBox::charPress(CharPressEvent& event)
    {
        // The press that opened this box's window has its character queued behind it, and the
        // window's own loop delivers it here. It is not the box's: a Space that opened an editor
        // would replace the value with a space.
        if (!m_sawPress)
            return;
        if (event.character() == Keys::Return)
        {
            if (!m_takeLineBreak)
                return;
            m_takeLineBreak = false;
        }
        TextBox::charPress(event);
    }

    void EditBox::clearRefusal()
    {
        if (m_refusal.empty())
            return;
        m_refusal.clear();
        Tooltip::stopAndHide();
    }

    // SuggestionRow

    SuggestionRow::SuggestionRow(const CreateParams& params, InPlaceEditRoot& root,
        const std::size_t itemIndex)
        :
        Button{ params,
            // The look of a dropdown's line: no surface at rest, and the current row on its own.
            ShowSurfaceAtRest::No,
            ShowSelectionOnSurface::Yes,
            // Reached by the pointer and never focused - the caret stays in the box.
            Interactivity::MouseOnly
        },
        m_root{ root },
        m_itemIndex{ itemIndex },
        m_name{ typedName((*root.suggestions())[itemIndex]) }
    {
    }

    void SuggestionRow::getText(GetTextEvent& event) const
    {
        // The item's own text, or its placeholder muted while it has none - what a combobox
        // face shows for it, without what a face adds.
        const TextItem& item = (*m_root.suggestions())[m_itemIndex];
        if (item.text().empty())
            event.text << InkGrade::Muted << item.placeHolderText() << PopColor{};
        else
            event.text << item.text();
    }

    void SuggestionRow::click(ClickEvent&)
    {
        m_root.takeSuggestion(m_itemIndex);
    }

    // SuggestionStack

    SuggestionStack::SuggestionStack(const CreateParams& params, InPlaceEditRoot& root)
        :
        StackPanel{ params,
            Orientation::Vertical,
            // A SCROLLED BODY KEEPS THE SIZE IT MEASURED - see ComboboxDropdownStack.
            WordWrap::No
        }
    {
        const TextItems& items = *root.suggestions();
        for (std::size_t i = 0; i != items.size(); ++i)
            add<SuggestionRow>(root, i);
    }

    std::size_t SuggestionStack::filter(const std::wstring_view typed)
    {
        setCurrentItem(nullptr);
        std::size_t shown = 0;
        for (const ControlPtr& control : controls())
        {
            SuggestionRow& row = static_cast<SuggestionRow&>(*control);
            const bool matches = startsWithFolded(row.name(), typed);
            row.setVisible(matches);
            if (matches)
                ++shown;
        }
        return shown;
    }

    void SuggestionStack::moveCurrent(const ScrollDirection direction)
    {
        std::vector<Control*> shown{};
        for (const ControlPtr& row : controls())
        {
            if (row->visible())
                shown.push_back(row.get());
        }
        if (shown.empty())
            return;

        const Control* current = currentItem();
        const auto at = std::ranges::find(shown, current);
        if (direction == ScrollDirection::ToEnd)
        {
            if (at == shown.end())
                setCurrentItem(shown.front());
            else if (std::next(at) != shown.end())
                setCurrentItem(*std::next(at));
            return;
        }
        if (at == shown.end())
            return;
        setCurrentItem(at == shown.begin() ? nullptr : *std::prev(at));
    }

    ItemIndexValue SuggestionStack::currentItemIndex() const
    {
        if (const Control* current = currentItem())
            return static_cast<const SuggestionRow*>(current)->itemIndex();
        return {};
    }

    bool SuggestionStack::defaultCanFocusItem(Control& value)
    {
        return value.parent() == this;
    }

    void SuggestionStack::getControlState(GetStateEvent& event) const
    {
        StackPanel::getControlState(event);
        if (event.propagationStopped())
            return;
        event.state.selected = &event.control == currentItem();
        event.stopPropagation();
    }

    // InPlaceEditRoot

    InPlaceEditRoot::InPlaceEditRoot(const CreateParams& params)
        :
        Base{
            params,
            // NOTHING ON THE FRAME BUT THE BOX. The panel is a holder, not a border: it takes
            // no inset of its own, so the box reaches every edge of the window.
            HostProps{
                params.themeMetrics().secondaryWindow,
                params.themeMetrics().secondaryWindowShadow,
                UiElement::Menu,
                Padding{ 0.0f },
                Border{ Thickness::None },
                // The box inside takes the focus; the holder around it is reachable by the
                // pointer and is not a focus scope of its own.
                Interactivity::MouseOnly
            },
            // THE PADDING AND THE BORDER ARE THE BOX'S. The ring a focused text box draws sits
            // on its own edge, so with the inset here it falls OUTSIDE the text rather than
            // through the selection - and that same ring is then the window's edge, which is
            // what says at a glance that this is an editor and not the control it covers.
            BodyProps{
                UiElement::Section,
                params.themeMetrics().secondaryWindow
            }
        }
    {
        // Connected here rather than given to the timer as a construction property: MSVC rejects
        // a this-capturing lambda in a default member initializer.
        m_suggestionsRequest.onTick([this](TimerEvent&){
            updateSuggestions();
        });
    }

    void InPlaceEditRoot::setTarget(const EditTarget& target, const Text& source, AcceptEditFunc accept)
    {
        m_accept = std::move(accept);

        EditBox& box = body();
        box.setHorizontalTextAnchor(target.textAnchor);
        box.setReadOnly(target.readOnly);

        // A READER HAS NOTHING TO COMPLETE, and an editor with nothing listed has no list. Built
        // before the value is written below: the keys that opened the editor are its first edit,
        // and that edit is what first asks for the list.
        const bool lists = target.readOnly == ReadOnly::No
            && target.suggestions
            && !target.suggestions->empty();
        if (lists)
        {
            m_suggestions = target.suggestions;
            m_list.emplace(
                appContext(),
                WindowRole::Menu,
                &box,
                HostProps{
                    themeMetrics().secondaryWindow,
                    themeMetrics().secondaryWindowShadow,
                    UiElement::Section,
                    // A list taller than its room scrolls - see ComboBox::showDropdown.
                    ScrollBars::Auto,
                    // Nothing in the list takes the focus: the rows are reached by the pointer,
                    // and by the keys the box hands on.
                    Interactivity::MouseOnly
                },
                BodyProps{ *this }
            );
            // Sized by what is listed, which changes under it as the text does; and hidden
            // rather than destroyed when nothing is listed, since the next character may list
            // something again.
            m_list->setAutoFit(AutoFit::Yes);
            m_list->setCloseAction(CloseAction::Hide);
            m_list->setDropdownClearance(1.0f);
            m_textEdit = box.onTextEdit([this](TextEditEvent&){
                m_suggestionsRequest.start(MilliSeconds{ 0u });
            });
            m_aligned = form().onAligned([this](FormAlignedEvent&){
                if (std::exchange(m_suggestionsWaitOnAlign, false))
                    m_suggestionsRequest.start(MilliSeconds{ 0u });
            });
        }

        // The rects arrive scaled, because they were measured off a laid-out control, while a
        // metric is stated in design units and scaled again by the pass that reads it. Both go
        // back through the scale once here, so the two agree.
        const float scale = scaler().factor();
        // Both rects describe TEXT, and these are the BOX's sizes - the box is bigger than its
        // text by the inset it carries, so that inset is added back. The POSITION needs no such
        // correction: textOrigin() reads the box's text rect out of the finished layout, so it
        // already counts whatever the box puts between its edge and its first glyph.
        const Padding boxInset = box.padding();

        // NEVER SMALLER THAN THE TEXT IT COVERS, IN EITHER AXIS. The covered rect is a floor and
        // the text grows the box from there. Each axis floors it for its own reason:
        //
        // width  - the placement lands the box's text rect on the target's, and it can only land
        //          the two anchors together while the two rects are the same width.
        // height - ONLY THE NEW TEXT IS VISIBLE IN EDIT MODE, and the editor is what hides the
        //          old one. A box that shrank to what has been typed would let the rest of a
        //          wrapped caption show out from under it. The blank under a short line is not
        //          waste - it is the cover.
        box.setMinSize(
            target.textRect.width() / scale + boxInset.x * 2.0f,
            target.textRect.height() / scale + boxInset.y * 2.0f);
        // Zero says the caller states no limit on that axis, and the monitor the placement
        // clamps to is then the only bound left.
        box.setMaxSize({
            target.maxTextSize.x > 0.0f ? target.maxTextSize.x / scale + boxInset.x * 2.0f : k_maxFloat,
            target.maxTextSize.y > 0.0f ? target.maxTextSize.y / scale + boxInset.y * 2.0f : k_maxFloat
        });

        box.text().clear();
        box.text() << source;
        box.selectAll();
        if (!target.typed.empty())
            box.replaceSelectedText(target.typed);
    }

    bool InPlaceEditRoot::offerText()
    {
        if (m_settled)
            return true;
        // THE CONTROL THE SINK WAS BUILT TO WRITE TO MAY BE GONE. The editor pumps messages for
        // as long as the user wants it up, and a list rebuilt behind it takes its items with it.
        // The form clears a dead control out of its popup's target - see FormBase::forgetControl
        // - and this popup's target IS that control, so a target still here is a control still
        // here. It is set at every point a sink is offered the text and nulled only afterwards.
        //
        // Nothing to write to means nothing to refuse either: the edit is simply over. This is
        // what lets a sink capture the control it belongs to and use it without a guard of its
        // own - see WithInPlaceEdit.
        //
        // A READ-ONLY BOX HAS NOTHING TO OFFER: what it holds is the value the caller handed in,
        // and asking a sink to take its own value back invites a refusal the user cannot answer,
        // since nothing in the box can be corrected. Enter and a click away would then both be
        // vetoed and only Escape would close the reader.
        if (!m_accept || body().readOnly() == ReadOnly::Yes || !form().popupTarget())
        {
            m_settled = true;
            m_result = EditResult::Accepted;
            return true;
        }

        AcceptEditEvent event{ body(), body().text() };
        m_asked = true;
        m_accept(event);
        if (event.refused())
        {
            m_result = EditResult::Refused;
            body().setRefusal(event.reason());
            return false;
        }
        m_settled = true;
        m_result = EditResult::Accepted;
        return true;
    }

    void InPlaceEditRoot::offerTextIfUnasked()
    {
        if (m_asked)
            return;
        offerText();
    }

    void InPlaceEditRoot::takeSuggestion(const std::size_t itemIndex)
    {
        EditBox& box = body();
        box.selectAll();
        box.replaceSelectedText(typedName((*m_suggestions)[itemIndex]));
        // Down before the offer, and after the edit that would have asked for it again: the
        // reason a refusal shows goes under the box, where the list stands.
        hideSuggestions();
        acceptText();
    }

    FloatPoint InPlaceEditRoot::textOrigin() const
    {
        // The box answers in the form's coordinates, which start at the surface's top left. A
        // text origin is measured from this root's, which is the window's geometry - the two
        // stand a frame margin apart.
        return body().textOriginInForm() - boundsInForm().topLeft();
    }

    // THE EDITOR OWNS THE POINTER FOR AS LONG AS IT IS UP. Nothing behind it can be reached
    // without going through readyToClose, so where the pointer happens to be says nothing about
    // where the user is: the box is what they are on, and it reads that way whatever they point
    // at. Both halves of the focus ring follow from that, and both are stated here rather than
    // in the painting, because what is being said is about the STATE - see the ring's two terms
    // in PaintEvent::applyFocus2.
    //
    // hovered - what keeps the ring drawn LIVE rather than in the inactive grey. That is a
    //           question the input device decides for every other control, and here there is
    //           nothing for it to decide.
    // current - what says there is a ring at all. A ring the keyboard put on is gated on the
    //           keyboard, so a box focused after a mouse press would wear none; a container's
    //           current item wears one ungated, and this box is the one item of this container.
    //
    // Only this box. A text box anywhere else answers for itself the ordinary way.
    void InPlaceEditRoot::adjustNestedControlVisualState(const Control& child, VisualState& state) const
    {
        if (&child != &body())
            return;
        state.hovered = true;
        state.current = true;
    }

    void InPlaceEditRoot::keyDown(KeyDownEvent& event)
    {
        switch (event.key)
        {
        case Keys::Return:
            {
                // Shift+Return is the box's - it breaks the line and the edit goes on. The box
                // has already read the modifier off this same press; see EditBox::charPress.
                if (event.modifiers.shift)
                    break;

                event.handled = true;
                // A row the keys are on is what Return means while the list is up. With the
                // list up and no row current, Return means what was typed, as it always does.
                if (suggestionsShown())
                {
                    const ItemIndexValue picked = m_list->content().body().currentItemIndex();
                    if (picked.has_value())
                    {
                        takeSuggestion(picked.value());
                        return;
                    }
                    hideSuggestions();
                }
                acceptText();
                return;
            }

        case Keys::Escape:
            {
                event.handled = true;
                // WITH THE LIST UP, ESCAPE TAKES THE LIST DOWN and leaves the edit standing;
                // the next one cancels it.
                if (suggestionsShown())
                {
                    hideSuggestions();
                    return;
                }
                // The one ending that does not offer the text. Every other one does, which is
                // what makes committing the default.
                m_settled = true;
                m_result = EditResult::Cancelled;
                form().close();
                return;
            }

        case Keys::F4:
        case Keys::Up:
        case Keys::Down:
            {
                // The keys that drop a combobox's list drop this one whole, the filter lifted;
                // the arrows walk it while it is up. The box leaves them to the root - see
                // EditBox::keyDown.
                const bool drops = event.key == Keys::F4
                    || (event.key == Keys::Down && event.modifiers.alt);
                if (drops)
                {
                    if (!m_list.has_value())
                        break;
                    event.handled = true;
                    showAllSuggestions();
                    return;
                }
                if (!suggestionsShown())
                    break;
                event.handled = true;
                m_list->content().body().moveCurrent(
                    event.key == Keys::Up ? ScrollDirection::ToBegin : ScrollDirection::ToEnd);
                return;
            }
        }
        Base::keyDown(event);
    }

    void InPlaceEditRoot::acceptText()
    {
        if (offerText())
        {
            form().close();
            return;
        }
        // Refused, so the editor stays and the reason goes over it. This is the one
        // ending that can be vetoed - the window is still here to argue in.
        body().showRefusal();
    }

    bool InPlaceEditRoot::suggestionsShown() const
    {
        // The list registers on this form as the box's popup while it is up, and on nothing
        // while it is down - the same fact a combobox face reads about its list.
        return body().isDroppedDown();
    }

    void InPlaceEditRoot::updateSuggestions()
    {
        // THE LIST STANDS ON A BOX THAT HAS BEEN LAID OUT. A request can arrive ahead of the pass
        // that lays the edit out - a platform that paints when the compositor asks rather than
        // when the queue empties delivers a 0 ms tick first - and a list placed then stands on
        // where the box was. Asked for again from the pass that settles the form.
        if (!form().contentAligned())
        {
            m_suggestionsWaitOnAlign = true;
            return;
        }

        // What the sink is offered, matched the way the sink matches it - see
        // ComboBox::acceptEditorText. Nothing typed lists nothing; the whole list is asked for
        // by the keys that drop it.
        const std::wstring typedText = typedForm(body().text());
        std::wstring_view typed = typedText;
        trimLeft(typed);
        trimRight(typed);
        if (typed.empty())
        {
            hideSuggestions();
            return;
        }
        showSuggestions(typed);
    }

    void InPlaceEditRoot::showAllSuggestions()
    {
        // A request still pending would narrow the list to what is typed the moment it is up.
        m_suggestionsRequest.stop();
        m_suggestionsWaitOnAlign = false;
        showSuggestions({});
    }

    void InPlaceEditRoot::showSuggestions(const std::wstring_view typed)
    {
        SuggestionStack& rows = m_list->content().body();
        if (rows.filter(typed) == 0)
        {
            hideSuggestions();
            return;
        }

        // At least as wide as the box, and under it - both read now rather than once, since
        // the box grows with what is typed. In design units, which a popup shares with its
        // parent.
        const EditBox& box = body();
        m_list->setMinWidth(box.width() / scaler().factor());
        m_list->setPlacement(FormPlacement::Bottom, box.boundsInForm());
        if (!suggestionsShown())
            m_list->show();
    }

    void InPlaceEditRoot::hideSuggestions()
    {
        // A request still pending would put the list back up after the user took it down.
        m_suggestionsRequest.stop();
        m_suggestionsWaitOnAlign = false;
        if (suggestionsShown())
            m_list->close();
    }

    // InPlaceEditForm

    InPlaceEditForm::InPlaceEditForm(const EditTarget& target, const Text& source, AcceptEditFunc accept)
        :
        Form{
            target.control.appContext(),
            WindowRole::Menu,
            &target.control
        }
    {
        // The window grows with what is typed into it rather than clipping it.
        setAutoFit(AutoFit::Yes);
        content().setTarget(target, source, std::move(accept));
    }

    bool InPlaceEditForm::readyToClose()
    {
        if (content().offerText())
            return true;
        // The value is refused, so the click that would have dismissed the editor leaves it
        // standing with the reason over it. This is what makes clicking away as vetoable as
        // Enter, and it is the whole of "an edit is never lost to a value nothing would take".
        content().showRefusal();
        return false;
    }

    // InPlaceEdit

    EditResult InPlaceEdit::run(const EditTarget& target, const Text& source, AcceptEditFunc accept)
    {
        InPlaceEditForm editor{ target, source, std::move(accept) };
        // OverText is placed by the text rather than by the box, so what it is given is where
        // the text being replaced is drawn - see FormControlBase::textOrigin.
        editor.setPlacement(FormPlacement::OverText, target.textRect);
        s_editorOnScreen = true;
        editor.execute();
        // CLEARED AS THE WINDOW GOES, not as this call returns: the tidy-up offer below runs a
        // sink with nothing on screen, and a sink is free to ask for an editor of its own.
        s_editorOnScreen = false;

        // The endings NOBODY DROVE end here, and only here: a loop that stopped underneath the
        // editor, an owner form that went away. Everything the user actually did - Enter, a
        // click outside, Escape - has already had its answer, and an answer is not asked for
        // again.
        editor.offerTextIfUnasked();
        return editor.result();
    }
}
