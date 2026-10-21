module ClaFi.Controls.InPlaceEdit;

import ClaFi.Icons.MessageIcons;

import ClaFi.Controls.DropList;
import ClaFi.Controls.TextItems;
import ClaFi.Controls.TextBox;
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
    namespace
    {
        // The items whose typed names begin with what is typed. See Controls#suggestionlist
        class ItemsSuggester : public Suggester
        {
        public:
            explicit ItemsSuggester(const TextItems&);
        public:
            [[nodiscard]] DropListLines suggest(std::wstring_view typed) override;
            [[nodiscard]] std::wstring pick(std::size_t line) override;
        private:
            const TextItems& m_items;
            std::vector<std::wstring> m_names{};   // what each item is matched by, settled once
            std::vector<std::size_t> m_listed{};   // the item each line stands for
        };

        ItemsSuggester::ItemsSuggester(const TextItems& items)
            :
            m_items{ items }
        {
            for (const TextItem& item : m_items)
                m_names.push_back(typedName(item));
        }

        // A line shows the item's own text, or its placeholder muted while it has none - what a
        // combo box face shows for it, without what a face adds.
        DropListLines ItemsSuggester::suggest(const std::wstring_view typed)
        {
            m_listed.clear();
            DropListLines lines;
            for (std::size_t i = 0; i != m_names.size(); ++i)
            {
                if (!startsWithFolded(m_names[i], typed))
                    continue;
                const TextItem& item = m_items[i];
                Text text;
                if (item.text().empty())
                    text << InkGrade::Muted << item.placeHolderText() << PopColor{};
                else
                    text << item.text();
                m_listed.push_back(i);
                lines.push_back({ .text = std::move(text) });
            }
            return lines;
        }

        std::wstring ItemsSuggester::pick(const std::size_t line)
        {
            return typedName(m_items[m_listed[line]]);
        }
    }

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

    // PromptBox

    FloatPoint PromptBox::textOriginInForm() const
    {
        return textBounds(formContext(), boundsInForm()).topLeft();
    }

    void PromptBox::setRefusal(std::wstring_view value)
    {
        m_refusal = value;
    }

    void PromptBox::showRefusal()
    {
        if (m_refusal.empty())
            return;
        // Raised rather than waited for. A message is shown at once and goes as any tooltip goes,
        // which is what puts the reason in front of a user who is looking at the keyboard;
        // nestedGetTooltip below is what answers every hover after that, for as long as the value
        // is still refused.
        ContextMessage::show(*this, Icons::messageText(MessageIcon::Error, m_refusal));
    }

    void PromptBox::nestedGetTooltip(GetTooltipEvent& event)
    {
        if (m_refusal.empty())
        {
            TextBox::nestedGetTooltip(event);
            return;
        }
        // A refusal stands in front of anything else this box would say, and it is placed under
        // the box rather than at the pointer: it answers what was **typed**, and the user reaching
        // for Enter has left the pointer wherever it happened to be.
        event.text << Icons::messageText(MessageIcon::Error, m_refusal);
        event.placement = FormPlacement::BottomLeft;
        event.anchorRect = boundsInForm();
    }

    void PromptBox::nestedKeyDown(KeyDownEvent& event)
    {
        // Any press at all answers a refusal - reading it is what makes it stale, and the user
        // is now doing something about it. This runs before the root sees the key, so a Return
        // that is refused again puts the reason back up afterwards.
        clearRefusal();
        m_sawPress = true;
        if (event.key == Keys::Return)
            m_takeLineBreak = event.modifiers.shift;
        // **The list's keys.** Up and Down are the list's while it is up, and Alt+Down - the key
        // that drops a combo box's list - is the list's always. The walk reaches the box before
        // the root, and the box would move the caret on them; left alone here, they reach the
        // root next. F4, the other dropping key, is a key the box never takes.
        const bool arrow = event.key == Keys::Up || event.key == Keys::Down;
        const bool dropsList = event.key == Keys::Down && event.modifiers.alt;
        if (dropsList || (arrow && isDroppedDown()))
            return;
        TextBox::nestedKeyDown(event);
    }

    void PromptBox::nestedPressDown(PressDownEvent& event)
    {
        // Clicking into the box to correct the value answers the refusal as much as typing
        // does. The tooltip window is hidden by the form's own mouse handling; what has to go
        // here is the box's memory of it, or every later hover reads the stale reason back.
        clearRefusal();
        TextBox::nestedPressDown(event);
    }

    void PromptBox::charPress(CharPressEvent& event)
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

    void PromptBox::clearRefusal()
    {
        if (m_refusal.empty())
            return;
        m_refusal.clear();
        Tooltip::stopAndHide();
    }

    // InPlaceEditRoot

    InPlaceEditRoot::InPlaceEditRoot(const CreateParams& params)
        :
        Base{
            params,
            // **Nothing on the frame but the box.** The panel is a holder, not a border: it takes
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
            // **The padding and the border are the box's.** The ring a focused text box draws sits
            // on its own edge, so with the inset here it falls **outside** the text rather than
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

        PromptBox& box = body();
        box.setHorizontalTextAnchor(target.textAnchor);
        box.setTextFormat(target.textFormat ? *target.textFormat : TextFormat{});
        box.setReadOnly(target.readOnly);

        // **A reader has nothing to complete**, and an editor with nothing listed has no list.
        // Built before the value is written below: the keys that opened the editor are its first
        // edit, and that edit is what first asks for the list.
        const bool listsItems = target.suggestions && !target.suggestions->empty();
        const bool lists = target.readOnly == ReadOnly::No && (target.suggester || listsItems);
        if (lists)
        {
            m_suggester = target.suggester;
            if (!m_suggester)
            {
                m_itemsSuggester = std::make_unique<ItemsSuggester>(*target.suggestions);
                m_suggester = m_itemsSuggester.get();
            }
            // As long as the screen lets it, as a combo box's list is, and walked from the box:
            // nothing is current until Down, and Up from the first line goes back to the box.
            m_list.emplace(box, [this](const DropListPick& picked) {
                takeSuggestion(picked.line);
            }, DropListOptions{
                .shownLines = 0.0f,
                .clearance = 1.0f,
                .upFromFirst = UpFromFirst::Leaves
            });
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
        // Both rects describe **text**, and these are the **box's** sizes - the box is bigger than
        // its text by the inset it carries, so that inset is added back. The **position** needs no
        // such correction: textOrigin() reads the box's text rect out of the finished layout, so it
        // already counts whatever the box puts between its edge and its first glyph.
        const Padding boxInset = box.padding();

        // **Never smaller than the text it covers, in either axis.** The covered rect is a floor
        // and the text grows the box from there. Each axis floors it for its own reason:
        //
        // width  - the placement lands the box's text rect on the target's, and it can only land
        //          the two anchors together while the two rects are the same width.
        // height - **only the new text is visible in edit mode**, and the editor is what hides the
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
        // **The control the sink was built to write to may be gone.** The editor pumps messages for
        // as long as the user wants it up, and a list rebuilt behind it takes its items with it.
        // The form clears a dead control out of its popup's target - see FormBase::forgetControl
        // - and this popup's target **is** that control, so a target still here is a control still
        // here. It is set at every point a sink is offered the text and nulled only afterwards.
        //
        // Nothing to write to means nothing to refuse either: the edit is simply over. This is
        // what lets a sink capture the control it belongs to and use it without a guard of its
        // own - see WithInPlaceEdit.
        //
        // **A read-only box has nothing to offer**: what it holds is the value the caller handed
        // in, and asking a sink to take its own value back invites a refusal the user cannot
        // answer, since nothing in the box can be corrected. Enter and a click away would then both
        // be vetoed and only Escape would close the reader.
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

    void InPlaceEditRoot::takeSuggestion(const std::size_t line)
    {
        PromptBox& box = body();
        const std::wstring taken = m_suggester->pick(line);
        box.selectAll();
        box.replaceSelectedText(taken);
        // The edit goes on from what was picked, and the list follows it as it follows any edit.
        if (!m_suggester->pickAccepts())
            return;
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

    // **The editor owns the pointer for as long as it is up.** Nothing behind it can be reached
    // without going through readyToClose, so where the pointer happens to be says nothing about
    // where the user is: the box is what they are on, and it reads that way whatever they point
    // at. Both halves of the focus ring follow from that, and both are stated here rather than
    // in the painting, because what is being said is about the **state** - see the ring's two terms
    // in the Focus ring list's default rules.
    //
    // hovered - what keeps the ring drawn **live** rather than in the inactive grey. That is a
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

    void InPlaceEditRoot::nestedKeyDown(KeyDownEvent& event)
    {
        switch (event.key)
        {
        case Keys::Return:
            {
                // Shift+Return is the box's - it breaks the line and the edit goes on. The box
                // has already read the modifier off this same press; see PromptBox::charPress.
                if (event.modifiers.shift)
                    break;

                event.handled = true;
                // A row the keys are on is what Return means while the list is up. With the
                // list up and no row current, Return means what was typed, as it always does.
                if (suggestionsShown())
                {
                    if (m_list->pickCurrent(event.modifiers, event.stamp))
                        return;
                    hideSuggestions();
                }
                acceptText();
                return;
            }

        case Keys::Escape:
            {
                event.handled = true;
                // **With the list up, Escape takes the list down** and leaves the edit standing;
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
                // The keys that drop a combo box's list drop this one whole, the filter lifted;
                // the arrows walk it while it is up. The box leaves them to the root - see
                // PromptBox::nestedKeyDown.
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
                m_list->moveCurrent(
                    event.key == Keys::Up ? ScrollDirection::ToBegin : ScrollDirection::ToEnd);
                return;
            }
        }
        Base::nestedKeyDown(event);
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
        return m_list.has_value() && m_list->shown();
    }

    void InPlaceEditRoot::updateSuggestions()
    {
        // **The list stands on a box that has been laid out.** A request can arrive ahead of the
        // pass that lays the edit out - a platform that paints when the compositor asks rather than
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

    // Under the box and at least as wide, both read now rather than once, since the box grows
    // with what is typed; none current.
    void InPlaceEditRoot::showSuggestions(const std::wstring_view typed)
    {
        DropListLines lines = m_suggester->suggest(typed);
        if (lines.empty())
        {
            hideSuggestions();
            return;
        }
        m_list->show(std::move(lines), std::nullopt, FormPlacement::BottomLeft, body());
    }

    void InPlaceEditRoot::hideSuggestions()
    {
        // A request still pending would put the list back up after the user took it down.
        m_suggestionsRequest.stop();
        m_suggestionsWaitOnAlign = false;
        if (m_list.has_value())
            m_list->hide();
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
        // **Cleared as the window goes**, not as this call returns: the tidy-up offer below runs a
        // sink with nothing on screen, and a sink is free to ask for an editor of its own.
        s_editorOnScreen = false;

        // The endings **nobody drove** end here, and only here: a loop that stopped underneath the
        // editor, an owner form that went away. Everything the user actually did - Enter, a
        // click outside, Escape - has already had its answer, and an answer is not asked for
        // again.
        editor.offerTextIfUnasked();
        return editor.result();
    }
}
