export module ClaFi.Controls.InPlaceEdit;

import ClaFi.Controls.TextItems;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.Button;
import ClaFi.Controls.Panel;

import ClaFi.StdActions;

import ClaFi.Core.Foundation;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;
import ClaFi.Core.Context.FormContext;

namespace ClaFi::Controls
{
    /// @brief How an in-place edit ended.
    export enum class EditResult
    {
        Accepted,   // the sink was offered the text and did not refuse it
        Refused,    // the sink was offered the text and would not have it
        Cancelled   // Escape; the sink was never asked
    };

    // The text the user is committing, and the sink's answer to it. See Controls
    export class AcceptEditEvent : public Event
    {
    public:
        AcceptEditEvent(Control& askedBy, const Text&);
        /// The control that asked for this text to be taken - the answer that was pressed, or
        /// the box a Return was pressed in. A SINK WITH A QUESTION OF ITS OWN OWNS IT BY THIS,
        /// so the question stands on top of the editor with the value still on screen behind it.
        Control& askedBy;
        const Text& text;
        /// Refuses the text and says why, in a sentence the user reads.
        void refuse(std::wstring_view why);
        /// Refuses it with nothing to say, for a sink that has ALREADY had its say: one that put
        /// a question of its own to the user and was told no. A reason there would only repeat
        /// what they have just answered.
        void refuse();
        [[nodiscard]] bool refused() const { return m_refused; }
        [[nodiscard]] const std::wstring& reason() const { return m_reason; }
    private:
        // Its own answer rather than whether there is a reason: a refusal with nothing to say is
        // still a refusal, and it is the one a sink that has already asked gives.
        bool m_refused{ false };
        std::wstring m_reason{};
    };

    export using AcceptEditFunc = std::function<void(AcceptEditEvent&)>;

    // What an in-place editor is placed over, and how far it may grow. See Controls
    export struct EditTarget
    {
        Control& control;
        // Where the text being replaced is drawn, in the coordinates of the form the control
        // stands in - the space a placement rect is stated in, which is where this one goes.
        FloatRect textRect;
        HorizontalTextAnchor textAnchor{ HorizontalTextAnchor::Left };
        // How far the editor may grow, measured on the TEXT and not on the window: the frame
        // the editor draws around it is added on top, which is the padding correction a caller
        // stating its own control's MaxSize would otherwise have to make twice. Zero on an axis
        // means the caller sets no limit there and the monitor is the only bound left.
        //
        // A CONTROL WHOSE TEXT WRAPS PASSES ITS OWN WIDTH HERE - which is textRect's
        // width, since that rect is where the text is laid out. Floor and ceiling are then one
        // number, the editor wraps exactly as the covered text does, and only the height grows.
        // Leaving it open instead pulls a wrapped caption into a single long line the moment the
        // editor opens, which is the covered text MOVING rather than being covered.
        //
        // A control whose text does not wrap - a grid cell, which trims - wants the opposite: a
        // ceiling wider than the covered rect is how a value too long for its column is finally
        // seen whole.
        FloatPoint maxTextSize{};
        // Whether the box may be typed into. A READ-ONLY EDITOR IS A READER: it is how a value
        // too long for the place it is shown in is finally seen whole, selected and copied, and
        // it refuses every change to what it holds. Its text is the one handed in here, so the
        // sink is never offered it - see InPlaceEditRoot::offerText.
        ReadOnly readOnly{ ReadOnly::No };
        // What the user typed to open the editor. It replaces the value, which opens selected
        // whole, so the keys that asked for the editor are the first keys of the edit.
        std::wstring_view typed{};
        // What the editor completes from, NAMED rather than copied. See Controls#suggestionlist
        const TextItems* suggestions{ nullptr };
    };

    // A text box that can refuse the value typed into it and say why. See Controls
    export class EditBox : public TextBox
    {
    public:
        using TextBox::TextBox;
        // Where the first glyph is drawn, in the form's coordinates. Read out of the layout the
        // box was given rather than assembled from paddings, so an inset added anywhere between
        // the box's edge and its text is already counted.
        [[nodiscard]] FloatPoint textOriginInForm() const;
        // Records why the value was not taken. Recorded apart from being shown because the two
        // have different audiences: an ending that closes the window records a reason nobody
        // will ever see, and asking the sink twice to get one is not free.
        void setRefusal(std::wstring_view);
        // Puts the recorded reason on screen under the box at once, without waiting to be
        // hovered for. Nothing to show is not an error.
        void showRefusal();
    protected:
        void getTooltip(GetTooltipEvent&) override;
        void keyDown(KeyDownEvent&) override;
        void pressDown(PressDownEvent&) override;
        void charPress(CharPressEvent&) override;
    private:
        void clearRefusal();
    private:
        // Whether the line break of the press being answered belongs in the text. THE CHARACTER
        // OF A PRESS IS QUEUED BEFORE THE PRESS IS ANSWERED, so a Return the form takes for
        // itself would still break the line here. The press settles this and the character reads
        // it: Shift+Return is the box's, a plain Return is the form's.
        bool m_takeLineBreak{ false };
        bool m_sawPress{ false }; // whether a key press has reached the box yet
        // Empty while nothing has been refused. It is the whole of that state - what is shown,
        // and whether anything is.
        std::wstring m_refusal{};
    };

    class InPlaceEditRoot;

    // One row of the suggestion list - an item, named as the sink would take it.
    class SuggestionRow : public Button
    {
    public:
        SuggestionRow(const CreateParams&, InPlaceEditRoot&, std::size_t itemIndex);
        [[nodiscard]] std::size_t itemIndex() const { return m_itemIndex; }
        // What the row is matched by - the item's typed name, settled once.
        [[nodiscard]] const std::wstring& name() const { return m_name; }
    protected:
        void getText(GetTextEvent&) const override;
        void click(ClickEvent&) override;
    private:
        InPlaceEditRoot& m_root;
        std::size_t m_itemIndex;
        std::wstring m_name;
    };

    // The rows the typed text names, and the row the keys are on. See Controls#suggestionlist
    class SuggestionStack : public StackPanel
    {
    public:
        SuggestionStack(const CreateParams&, InPlaceEditRoot&);
        // Shows the rows the text names and hides the rest; answers how many, and none is current.
        std::size_t filter(std::wstring_view typed);
        // Moves the current row one shown row up or down. See Controls#suggestionlist
        void moveCurrent(ScrollDirection);
        // The item the current row stands for, while a row is current.
        [[nodiscard]] ItemIndexValue currentItemIndex() const;
    protected:
        // Every row may be current: the box moves it, and the base's answer would rule that out.
        bool defaultCanFocusItem(Control&) override;
        // The current row reads selected - the base says so only for a stack that follows the user.
        void getControlState(GetStateEvent&) const override;
    };

    // The suggestion list as it is dropped: a box that scrolls, holding the rows. See Controls
    using SuggestionList = ScrollBoxWith<SuggestionStack>;

    // The root of an in-place edit form: a frame with one text box in it. The box holds a COPY
    // of the text being edited - the control underneath keeps showing its own until the sink
    // takes the new one, and keeps it unchanged when nothing does.
    class InPlaceEditRoot : public WithBody<Panel, EditBox>
    {
    public:
        explicit InPlaceEditRoot(const CreateParams&);
    public:
        [[nodiscard]] EditResult result() const { return m_result; }
        // Shapes the box to the text it covers, fills it with a copy of that text and selects
        // the whole of it, so the first character typed replaces the value the way a rename
        // does.
        void setTarget(const EditTarget&, const Text& source, AcceptEditFunc);
        // Offers the text to the sink, and answers whether the edit is over. Called by the
        // endings the user drives - Enter, and a click outside.
        //
        // A REFUSAL DOES NOT SETTLE: it leaves an edit still to be corrected or abandoned, so a
        // later ending offers the corrected value again. The reason is recorded on the box
        // rather than shown, because only the caller knows whether there is still an editor to
        // show it over.
        bool offerText();
        // Offers the text only if the sink has never been asked. This is for the endings NOBODY
        // drove - a loop that ended underneath the editor - and it is the one place that must
        // not ask again: a sink is not obliged to be idempotent, and a rename that failed the
        // first time would otherwise get a retry the user never asked for, after they had
        // given up.
        void offerTextIfUnasked();
        // Puts the recorded reason on screen. Called by the endings that leave the editor
        // standing, and by no others.
        void showRefusal() { body().showRefusal(); }
        // The values the editor completes from, or null. The rows read their items off it.
        [[nodiscard]] const TextItems* suggestions() const { return m_suggestions; }
        // Puts the item's name in the box and offers it - a pick is a pick, however it was made.
        void takeSuggestion(std::size_t itemIndex);
    protected:
        [[nodiscard]] FloatPoint textOrigin() const override;
        void adjustNestedControlVisualState(const Control&, VisualState&) const override;
        void keyDown(KeyDownEvent&) override;
    private:
        using Base = WithBody<Panel, EditBox>;
    private:
        // Ends the edit with the text taken, or keeps the window up with the reason over it.
        void acceptText();
        [[nodiscard]] bool suggestionsShown() const;
        // Lists the items the box's text names, or takes the list down. See Controls#suggestionlist
        void updateSuggestions();
        // Lists every item, whatever is typed - what F4 and Alt+Down ask for.
        void showAllSuggestions();
        // Lists the items the text names, or takes the list down while none is.
        void showSuggestions(std::wstring_view typed);
        void hideSuggestions();
    private:
        AcceptEditFunc m_accept{};
        // Cancelled until something says otherwise: an edit nobody took is an edit that did not
        // happen.
        EditResult m_result{ EditResult::Cancelled };
        // Two separate facts. Settled means the edit is over - taken, or abandoned on Escape.
        // Asked means the sink has answered at least once, either way, which is what stops the
        // tidy-up offer running a sink that has already refused.
        bool m_settled{ false };
        bool m_asked{ false };
        const TextItems* m_suggestions{ nullptr };
        // The list, a window of its own on the box, shown and hidden as what is typed changes.
        std::optional<Form<SuggestionList>> m_list{};
        // The list asked for on an edit, answered once the input that made it has been delivered.
        UiTimer m_suggestionsRequest{};
        // Whether a request found the layout unsettled and waits on the pass that settles it.
        bool m_suggestionsWaitOnAlign{ false };
        ScopedEventConnection m_textEdit{};
        ScopedEventConnection m_aligned{};
    };

    // A text box in a window of its own, placed over the text it edits. See Controls
    export class InPlaceEditForm : public Form<InPlaceEditRoot>
    {
    public:
        InPlaceEditForm(const EditTarget&, const Text& source, AcceptEditFunc);
        [[nodiscard]] EditResult result() { return content().result(); }
        // Offers the text to the sink if nothing ever asked it. Called once the window is gone,
        // so a refusal here only records itself - there is nothing left to show it over. See
        // InPlaceEdit::run.
        void offerTextIfUnasked() { content().offerTextIfUnasked(); }
        // Clicking outside is an ending like any other, so it offers the text - and unlike the
        // window going away, it can be refused. A refused value keeps the editor up with the
        // reason over it, which is what stops an edit being lost to a click.
        [[nodiscard]] bool readyToClose() override;
    };

    // Runs an in-place editor, and says whether one is already on screen.
    export class InPlaceEdit
    {
    public:
        /// @brief Edits a copy of `source` in a box placed over the target's text, and waits for
        /// the user to finish. The value reaches the caller through `accept`, which runs while the
        /// editor is still up and may refuse it.
        ///
        /// @note COMMITTING IS THE DEFAULT. Enter offers the text, and so does every other way an
        /// edit ends - a click elsewhere, the window going away. Escape is the only thing that does
        /// not, so a user who finishes by walking off gets the same answer as one who presses
        /// Enter. Shift+Enter breaks the line and the edit goes on.
        ///
        /// @note A REFUSED VALUE KEEPS THE EDITOR OPEN. Enter and clicking away are both vetoable -
        /// a click on the form behind a popup goes through FormBase::readyToClose, which this form
        /// answers - so neither can lose an edit to a value nothing would take. The endings that
        /// cannot be vetoed are the window going away and the loop ending underneath it, and there
        /// a refusal means only what it always means: the value was not taken.
        ///
        /// @note This runs a message loop of its own and returns once the editor is gone, so
        /// anything the caller holds across the call - `accept` included - has to survive the user
        /// working elsewhere in the meantime. A sink may safely name the control the editor was
        /// opened over: that one is checked, and the sink is not called once it has gone.
        static EditResult run(const EditTarget&, const Text& source, AcceptEditFunc);
        /// @brief Whether an in-place editor is on screen.
        ///
        /// @note THERE IS ONE AT A TIME, APPLICATION-WIDE, and this is what a caller asks before
        /// opening one. An editor runs a message loop of its own, so a request made before it
        /// opened - a timer tick, a key already queued - is dispatched INSIDE that loop; and there
        /// is one caret and one focus, so a second editor standing over the first is one the user
        /// cannot see the end of. A request that arrives while one is up is dropped: the user is
        /// typing a name somewhere else, and the one they asked for before that is not the one in
        /// front of them.
        [[nodiscard]] static bool isRunning() { return s_editorOnScreen; }
    private:
        // Whether an editor is on screen. There is one of these for the application, because
        // there is one caret and one focus for it - see isRunning.
        inline static bool s_editorOnScreen{ false };
    };

    /// @brief Gives a control's OWN TEXT an in-place editor, and the gestures that open one.
    ///
    ///     class ThemeButton : public WithInPlaceEdit<Button> { ... };
    ///     tile.onAcceptEdit([](AcceptEditEvent& event) { ... });
    ///
    /// @note All the host supplies is somewhere for the value to go. The geometry is read off
    /// the host, because a control's text is laid out inside its own bounds: the rect that says
    /// where the text is says how wide it wraps as well, so the editor covers it and wraps the
    /// same way without being told either.
    ///
    /// @note The gestures are F2, and A CLICK ON THE TEXT OF A CONTROL THE USER IS ALREADY ON.
    /// The click that picks a control never edits it, or a control could never be picked
    /// without editing it, and neither does the trailing click of a double click.
    ///
    /// @note The host also answers StdActions::rename, so a menu item or a toolbar button
    /// presenting that command renames it with nothing written by the application.
    ///
    /// @note A host whose editorMode answers ReadOnly gets a READER instead: the same box
    /// over the same text, selected whole and refusing every change, which is how a caption is
    /// copied out of a control that only shows it. A reader needs no sink and claims no rename.
    ///
    /// @note A host that answers editorSuggestions gets a list under the box of the values whose
    /// names begin with what is typed - a combobox answers its items - and F4 or Alt+Down lists
    /// them all. Picking one, with Return or a click, puts its name in the box and offers it in
    /// the same press.
    export template <IsControl HostClass>
    class WithInPlaceEdit : public HostClass
    {
    public:
        template <typename... Args>
        explicit WithInPlaceEdit(const CreateParams&, Args&&...);
    public:
        // Written out rather than taken from DECLARE_EVENT: the macro's connect helper names
        // connectEvent unqualified, which does not find a dependent base.
        template <typename F> using OnAcceptEdit = OnEvent<F>;
        EventConnection onAcceptEdit(auto&& callback) {
            return this->template connectEvent<AcceptEditEvent>(
                std::forward<decltype(callback)>(callback));
        }
        /// Runs the editor over this control's text. False when there is nothing to edit, which
        /// is what lets the gesture that asked for it travel on. What was typed to ask for it
        /// replaces the value - see EditTarget::typed.
        bool openEditor(std::wstring_view typed = {});
    protected:
        /// Which editor opens now, if any, asked each time one would open. The default is Editable
        /// while anything listens for the accepted value and None otherwise. A control with
        /// conditions of its own answers None under them and the base's answer otherwise, and a
        /// reader answers ReadOnly - see the class note.
        [[nodiscard]] virtual EditorMode editorMode() const;
        /// The value the editor opens on. It is this control's own text, which is what a control
        /// that keeps one shows. A control that COMPOSES its caption - out of the data item it
        /// stands for, or out of parts it puts together as it is asked to draw - keeps no text of
        /// its own, and states that caption here instead.
        ///
        /// @note THE GEOMETRY IS NOT ASKED FOR AGAIN. The editor is placed over the rect this
        /// control lays ITS text out in, so what is stated here is the run of glyphs standing in
        /// that rect - the caption as it is read, and not some longer name behind it.
        virtual void getEditorText(Text&) const;
        /// Hands the value the editor was left with to whatever takes it, while the editor is still
        /// up. The default raises AcceptEditEvent; a control that takes the value itself answers
        /// here instead.
        virtual void acceptEditorText(AcceptEditEvent&);
        /// How far the editor may grow, measured on the TEXT rather than on the window. The
        /// default is this control's own text rect width and no limit in height - floor and
        /// ceiling one number - which is what keeps a caption that WRAPS wrapped: the rect the
        /// editor covers is where this control lays its text out, so it is also where that text
        /// breaks.
        ///
        /// A control whose text does NOT wrap - one line, trimmed at the edge - wants the
        /// opposite. A ceiling wider than the covered rect, or none at all, is how a value too
        /// long for the control is read whole while it is being typed. Zero on an axis says no
        /// limit, and the monitor is then the only bound left.
        [[nodiscard]] virtual FloatPoint editorMaxTextSize(const FloatRect& textRect) const;
        /// What the editor completes from - a combobox, its items. See Controls#suggestionlist
        [[nodiscard]] virtual const TextItems* editorSuggestions() const;
        /// Whether the click being answered opens the editor. It is asked once per click, before
        /// anything is dispatched, and only for a press on this control itself: a press on a part
        /// of it - a dropdown strip, a close button - is that part's.
        ///
        /// The default is A CLICK ON THE TEXT OF A CONTROL THE USER IS ALREADY ON: the click that
        /// PICKS a control never edits it, or a control could never be picked without editing it,
        /// and the pointer has to be over the text rather than over an icon or a check mark.
        ///
        /// A control a click cannot pick answers on the first press instead - it already stands
        /// for the one thing it names, so the press has no other meaning to be kept apart from.
        /// A control that wants a condition of its own ON TOP of the default says it with the
        /// base's answer as well, the way editorMode is written.
        [[nodiscard]] virtual bool clickOpensEditor() const;
        void keyDown(KeyDownEvent&) override;
        void nestedControlFocusing(FocusEvent&) override;
        void click(ClickEvent&) override;
        void doubleClick(DoubleClickEvent&) override;
    private:
        /// Claims StdActions::rename for this control and answers it. Connected from the
        /// constructor, the way TextBox connects the edit actions.
        void connectRenameAction();
        /// Asks for the editor once the command that asked for it has unwound. See the body.
        void openEditorLater();
    private:
        // Empty until a command asks for the editor. It is held by the control being renamed,
        // which is the control the editor is placed over, so it is there for exactly as long as
        // there is anything to rename - and ~UiTimer stops a request the control did not live
        // to answer.
        UiTimer m_openRequest{};
        // Held while the editor waits for the form's layout to settle - see openEditor. Dropped
        // there, once it has, and with this control if a rebuild takes it first.
        ScopedEventConnection m_alignedRequest{};
        // What was typed with a request that is waiting for the layout.
        std::wstring m_typed{};
        // Recorded as the press happens, because nothing afterwards can answer it:
        // Input::setMouseDown moves the focus BEFORE it dispatches the press, so by click() the
        // control reads focused whichever way the click went.
        bool m_wasCurrentBeforePress{ false };
        // The trailing release of a double click raises an ordinary click of its own,
        // indistinguishable from a lone one, and that pair means whatever double clicking means
        // here rather than an edit.
        bool m_doubleClicked{ false };
    };


    //-------------------------------------------------------------------------


    // WithInPlaceEdit

    template <IsControl HostClass>
    template <typename... Args>
    WithInPlaceEdit<HostClass>::WithInPlaceEdit(const CreateParams& params, Args&&... args)
        :
        HostClass{ params, std::forward<Args>(args)... }
    {
        // Connected here rather than given to the timer as a construction property: MSVC rejects
        // a this-capturing lambda in a default member initializer.
        m_openRequest.onTick([this](TimerEvent&){
            const std::wstring typed = std::exchange(m_typed, {});
            openEditor(typed);
        });
        connectRenameAction();
    }

    template <IsControl HostClass>
    bool WithInPlaceEdit<HostClass>::openEditor(const std::wstring_view typed)
    {
        // ONE EDITOR AT A TIME. Every route here can arrive while another is already up - a tick
        // is dispatched by the loop the editor runs, and so is a key press - and false is what
        // lets the gesture that asked travel on. See InPlaceEdit::isRunning.
        // A WAIT ENDS HERE WHATEVER IS DECIDED BELOW. A request dropped while another editor is
        // up stays dropped; one left connected would open an editor nobody asked for, from the
        // first pass after that editor closes. What such a request kept of the keys goes with it.
        m_alignedRequest = {};
        m_typed.clear();
        const EditorMode mode = editorMode();
        if (mode == EditorMode::None || InPlaceEdit::isRunning())
            return false;

        // THE RECT IS READ OFF A SETTLED LAYOUT. A rebuild that made this control asks for the
        // editor before the pass that lays it out has run, and a platform that paints when the
        // compositor asks rather than when the queue empties can deliver a 0 ms tick ahead of
        // that pass. Asked for again from the pass that settles the form, on a tick of its own:
        // the handler runs inside the emit, so the connection is dropped above and not there.
        FormBase& form = this->form();
        if (!form.contentAligned())
        {
            m_typed = typed;
            m_alignedRequest = form.onAligned([this](FormAlignedEvent&){
                openEditorLater();
            });
            return true;
        }

        const FloatRect textRect = this->textBounds(this->formContext(), this->boundsInForm());
        const EditTarget target{
            *this,
            textRect,
            this->horizontalTextAnchor(),
            editorMaxTextSize(textRect),
            mode == EditorMode::ReadOnly ? ReadOnly::Yes : ReadOnly::No,
            typed,
            editorSuggestions()
        };
        // The call below runs a message loop and returns only once the editor is gone, so a local
        // outlives every read of it.
        Text source{};
        getEditorText(source);
        // Naming this control from inside the sink is safe: the editor stops offering the text
        // once its target is gone - see InPlaceEditRoot::offerText.
        InPlaceEdit::run(target, source, [this](AcceptEditEvent& event){
            acceptEditorText(event);
        });
        return true;
    }

    template <IsControl HostClass>
    EditorMode WithInPlaceEdit<HostClass>::editorMode() const
    {
        if (this->template hasEventListeners<AcceptEditEvent>())
            return EditorMode::Editable;
        return EditorMode::None;
    }

    template <IsControl HostClass>
    void WithInPlaceEdit<HostClass>::getEditorText(Text& text) const
    {
        text << this->text();
    }

    template <IsControl HostClass>
    void WithInPlaceEdit<HostClass>::acceptEditorText(AcceptEditEvent& event)
    {
        this->emitEvent(event);
    }

    template <IsControl HostClass>
    FloatPoint WithInPlaceEdit<HostClass>::editorMaxTextSize(const FloatRect& textRect) const
    {
        return { textRect.width(), 0.0f };
    }

    template <IsControl HostClass>
    const TextItems* WithInPlaceEdit<HostClass>::editorSuggestions() const
    {
        return nullptr;
    }

    template <IsControl HostClass>
    bool WithInPlaceEdit<HostClass>::clickOpensEditor() const
    {
        return m_wasCurrentBeforePress && this->isTextHovered();
    }

    template <IsControl HostClass>
    void WithInPlaceEdit<HostClass>::keyDown(KeyDownEvent& event)
    {
        // THE KEY IS ANSWERED HERE RATHER THAN LEFT TO StdActions::rename's SHORTCUT. A key the
        // control handles never reaches the shortcut scopes, and this is the control that has to
        // answer: the subject walk starts at the focused control, and a container holding the
        // focus answers for itself rather than for the item it is on, so a tile inside a
        // StackView is not reached from a shortcut at all. The action states the same key so
        // that a menu line prints it.
        //
        // The editor opens from inside the press, which the focused-control walk allows for.
        if (event.key == Keys::F2 && openEditor())
        {
            event.handled = true;
            return;
        }
        HostClass::keyDown(event);
    }

    template <IsControl HostClass>
    void WithInPlaceEdit<HostClass>::nestedControlFocusing(FocusEvent& event)
    {
        // BEFORE the base, which is what carries the walk up to the container. isFocused()
        // answers for where the user WAS only until that container has recorded its new current
        // item, and it does that from inside the base call below.
        //
        // Control::setFocus raises this on EVERY press, including a press on the control that
        // already holds the focus, so the case being detected is reported rather than skipped.
        //
        // focusChanged cannot answer this. A container that owns its items retargets the event
        // to itself - StackPanelBase does - so the focus is written to the CONTAINER and an item
        // is only its focusDelegate. No such item is ever told its focus changed.
        if (event.control == this)
            m_wasCurrentBeforePress = this->isFocused();
        HostClass::nestedControlFocusing(event);
    }

    template <IsControl HostClass>
    void WithInPlaceEdit<HostClass>::click(ClickEvent& event)
    {
        // Read and cleared before anything else runs: the base may take the focus somewhere and
        // come back, and the editor below pumps messages of its own.
        const bool opensEditor = event.control == this && !m_doubleClicked && clickOpensEditor();
        m_wasCurrentBeforePress = false;
        m_doubleClicked = false;

        HostClass::click(event);
        if (opensEditor)
            openEditor();
    }

    template <IsControl HostClass>
    void WithInPlaceEdit<HostClass>::doubleClick(DoubleClickEvent& event)
    {
        m_doubleClicked = true;
        HostClass::doubleClick(event);
    }

    template <IsControl HostClass>
    void WithInPlaceEdit<HostClass>::connectRenameAction()
    {
        // CLAIMING SAYS THE COMMAND IS ABOUT THIS CONTROL; what is claimed says whether it can
        // run now. A host with nowhere for a new name to go has no rename at all - that is a
        // property of the control rather than a state it is in - so it says nothing and the walk
        // carries on up, and something above it may answer the same command for something else.
        // A host that has a sink and may not be renamed right now claims and reports disabled,
        // because the command is still about it: the line is greyed rather than dropped.
        this->onGetActionState([this](GetActionStateEvent& event){
            if (&event.action != &StdActions::rename)
                return;
            if (!this->template hasEventListeners<AcceptEditEvent>())
                return;
            event.claim({ .enabled = editorMode() == EditorMode::Editable });
        });

        this->onActionClick([this](ActionClickEvent& event){
            if (&event.action == &StdActions::rename)
                openEditorLater();
        });
    }

    // THE EDITOR CANNOT OPEN INSIDE THE MENU THAT ASKED FOR IT, and a menu item runs its command
    // before it closes, so a command answered on the spot would open one there.
    //
    // The editor is a popup owned by this control, so it registers on THIS control's form - which
    // is the form the menu is registered on - and takes the menu's place there: the menu is left
    // standing with nothing guarding it. It is placed over this control's text as well, and the
    // menu is what that text is under.
    //
    // WM_TIMER is behind every message already queued, so the tick arrives once the input that
    // asked for the rename has been answered. A command presented from anywhere else pays one
    // tick for it, which is the same tick a rename asked for by a rebuild already waits.
    //
    // IT DOES NOT PROMISE THE MENU HAS FINISHED CLOSING: a modal loop pumps WM_TIMER too, and the
    // queue can empty inside one. What keeps that from standing two editors on screen is the
    // one-at-a-time test in openEditor.
    // TODO: should the request wait on there being no popup over this control's form, rather than
    // on the queue being empty? A popup's registration is guarded by identity, so an editor
    // opening as a menu closes does not lose the menu - but it is placed over a menu that is
    // still on screen.
    template <IsControl HostClass>
    void WithInPlaceEdit<HostClass>::openEditorLater()
    {
        m_openRequest.start(MilliSeconds{ 0u });
    }
}
