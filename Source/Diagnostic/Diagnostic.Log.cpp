module ClaFi.Diagnostic.Log;

import ClaFi.Diagnostic.FpsPage;
import ClaFi.Diagnostic.Options;

import ClaFi.Controls.TabbedBox;
import ClaFi.Controls.TabStrip;
import ClaFi.Controls.FormTitle;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.StackView;
import ClaFi.Controls.Checkbox;
import ClaFi.Controls.Spacer;
import ClaFi.Controls.Button;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Menu;

import ClaFi.StdActions;
import ClaFi.StdActions.Transfer;

import ClaFi.Core.TextEngine.Text;

import ClaFi.Core.Transfer.Clipboard;
import ClaFi.Core.Transfer.Source;
import ClaFi.Core.Transfer.Formats;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Dt;

import ClaFi.Core.Dom_StdSerializers;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.Utils;
import ClaFi.Core.System.UiTypes;

import ClaFi.Core.System.Props;
import ClaFi.StdLib;

namespace ClaFi
{
    using namespace Controls;
    // The config section of the window's own, and what it keeps.
    constexpr std::wstring_view k_sectionName = L"Diagnostic";
    constexpr std::wstring_view k_visibleName = L"Visible";
    constexpr std::wstring_view k_tabName = L"Tab";
    // THE TAB IS KEPT BY ITS CAPTION, which is what the config file then reads as. The captions
    // are stated once here and the tabs are built from them, so the name written and the name
    // matched cannot come apart.
    constexpr std::wstring_view k_outputTabCaption = L"Output";
    constexpr std::wstring_view k_fpsTabCaption = L"FPS";

    Hsl lastObjColor{ 0.6f, 1.0f, 0.66f };
    static std::unordered_map<const Control*, Color> colorMap{};

    // THE WHOLE PAGE, which is what a log is copied FOR - StdActions::copy beside it takes the
    // one row the menu was raised on. No shortcut and no icon: it is the second line of a menu
    // two lines long, and it is named in words.
    Action g_copyRows{ Text{ L"Copy all rows" } };

    class DiagnosticLogView : public StackView
    {
    public:
        using StackView::StackView;
    };

    // The Output page: the lines, newest at the bottom and scrolled to, under a bar saying whether
    // lines are being taken. They are not while the pointer is over the page, so what is being
    // read stays where it is.
    class OutputPage : public ScrollBoxWith<DiagnosticLogView>
    {
    public:
        using ScrollBoxWith<DiagnosticLogView>::ScrollBoxWith;
    public:
        std::wstring_view diagnosticText() const override { return L"OutputPage"; }
        bool stopped() const { return m_stopped; }
        RichControl& addAndSelect();
        // Called by the owner, which is what builds this page: the inherited constructors leave
        // no body of its own to connect from.
        void connectCopyActions();
    protected:
        void hoverEnter() override;
        void hoverLeave() override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&) override;
        void contextPopup(ContextPopupEvent&) override;
    private:
        void copyRows(const Control* single, InputStamp);
    private:
        Label& m_statusLabel{ createTopBar<Label>(
            OnEvent{ [this](GetTextEvent& event) {
                event.text << (m_stopped ? L"Stopped" : L"Active");
            } }
        ) };
        bool m_stopped{};
        RichControl* m_itemToSelect{};
    };

    // The diagnostic window: a tabbed box under a title bar of its own, the Output page holding
    // the log and the FPS page beside it, with the tabs along the bottom - the box is too narrow
    // to spend a column on them. Closing it hides it; the application's exit takes it down.
    struct DiagnosticWindow
    {
        explicit DiagnosticWindow(AppContext&);
        // Shows the window, or brings it to the front when it is already up.
        void showOrActivate(InputStamp);
        void restoreState();
        void storeState();
        [[nodiscard]] Dom::DomNodeBase& visibleNode() const;
        [[nodiscard]] Dom::DomNodeBase& tabNode() const;
        AppContext& appContext;
        Form<WithBody<Panel, TabbedBox>> form;
        FormTitle& title{ form.createTopBar<FormTitle>(
            Text{ L"Diagnostic - ", form.appContext().appName() }
            ) };
        // Held above the other windows. Hidden where the platform cannot do it - see
        // IPlatformWindow::canSetAlwaysOnTop.
        Checkbox& onTopCheck{ form.body().strip().add<Checkbox>(
            L"Always on top",
            Padding{ 8.0f, 0.0f },
            VerticalTextAnchor::Center
        ) };
        // The check stays at one end of the bar and the tabs at the other, whatever is between.
        FlexSpacer& stripSpacer{ form.body().strip().add<FlexSpacer>() };
        // NO BAR ON THE WIDTH, which is what makes a line wrap: a scrolled body keeps the width
        // it measured, and a line given somewhere to run to runs there instead of breaking. A
        // line nobody can read to the end of is a line that was not recorded.
        OutputPage& output{ form.body().pageControl().add<OutputPage>(
            HostProps{
                ScrollBars::Vertical
            },
            BodyProps{
                Orientation::Vertical,
                Padding{ 4.0f },
                SelectionMode::Multi,
                StackView::OnCanSelectItem{ [](CanSelectItemEvent& event) { event.canSelect = true; } }
            }
        ) };
        Diagnostic::FpsPage& fps{ form.body().pageControl().add<Diagnostic::FpsPage>() };
        // Held so that restoreState can open one of them by name. Neither is selected here -
        // which page the window opens on is the config's answer, and that is not read yet.
        Tab& outputTab{ form.body().strip().addTab(k_outputTabCaption, Page{ output }) };
        Tab& fpsTab{ form.body().strip().addTab(k_fpsTabCaption, Page{ fps }) };
    };

    static std::unique_ptr<DiagnosticWindow> g_diagnosticLogInstance{};

    void writeLine(const Text& text, const Control* ptr, bool showPtr);

    void diagnosticLog(const Text& text, const Control* ptr, bool showPtr)
    {
        if constexpr (Diagnostic::Options::enabled)
            writeLine(text, ptr, showPtr);
    }

    void initializeDiagnosticLog(AppContext& appContext)
    {
        if constexpr (Diagnostic::Options::enabled)
            g_diagnosticLogInstance = std::make_unique<DiagnosticWindow>(appContext);
    }

    void finalizeDiagnosticLog()
    {
        g_diagnosticLogInstance.reset();
    }

    void showDiagnosticWindow(const InputStamp stamp)
    {
        if constexpr (Diagnostic::Options::enabled)
            g_diagnosticLogInstance->showOrActivate(stamp);
    }

    Dom::Dt::Section createDiagnosticLogConfigSchema()
    {
        return Dom::Dt::Section{
            k_sectionName,
            Dom::Dt::Value{ k_visibleName, false },
            Dom::Dt::Value{ k_tabName, k_outputTabCaption }
        };
    }

    void restoreDiagnosticLogState()
    {
        if constexpr (Diagnostic::Options::enabled)
            g_diagnosticLogInstance->restoreState();
    }

    void storeDiagnosticLogState()
    {
        if constexpr (Diagnostic::Options::enabled)
            g_diagnosticLogInstance->storeState();
    }

    // One line into the Output page. A line about a control of the diagnostic window itself is
    // dropped: painting it would log, and logging would paint.
    void writeLine(const Text& text, const Control* ptr, bool showPtr)
    {
        static bool inDiagnosticLog{};
        if (inDiagnosticLog)
            return;
        inDiagnosticLog = true;

        OutputPage& output = g_diagnosticLogInstance->output;
        if (!output.stopped())
        {
            if (ptr && g_diagnosticLogInstance->form.content().containsNested(ptr))
            {
                inDiagnosticLog = false;
                return;
            }

            static int counter{};
            ++counter;
            RichControl& newItem = output.addAndSelect();
            newItem.text() << counter << L". " << text;
            if (showPtr)
            {
                newItem.text() << L": " << toHex(reinterpret_cast<std::uint64_t>(ptr));
                if (ptr)
                {
                    std::wstring_view s = ptr->diagnosticText();
                    Color color;
                    if (!colorMap.contains(ptr))
                    {
                        color = lastObjColor.toColor();
                        colorMap.insert({ ptr, color });
                        lastObjColor.offsetHue(1.0f / 6.0f);
                    }
                    else
                        color = colorMap[ptr];
                    if (!s.empty())
                        newItem.text() << color << L" (" << s << L")";
                }
            }
            // The same line where a debugger shows it, for the records the window is torn down
            // before it can paint - those of the exit save.
            Platform::debugOutput(newItem.text().plainText());
            g_diagnosticLogInstance->form.invalidateAlign();
        }
        inDiagnosticLog = false;
    }

    // OutputPage

    RichControl& OutputPage::addAndSelect()
    {
        static std::size_t count = 0;
        count++;
        if (count > 500)
        {
            body().controls().front()->deleteSelf();
        }

        m_itemToSelect = &body().add<ToolButton>();
        return *m_itemToSelect;
    }

    // Claiming says this page is what the command acts on; what it claims says whether it can act
    // right now. A page with no rows claims both and reports them disabled, which is not the same
    // as the commands being about something else.
    void OutputPage::connectCopyActions()
    {
        onGetActionState([this](GetActionStateEvent& event) {
            if (&event.action == &StdActions::copy)
                event.claim({ .enabled = body().currentItem() != nullptr });
            else if (&event.action == &g_copyRows)
                event.claim({ .enabled = !body().controls().empty() });
        });

        onActionClick([this](ActionClickEvent& event) {
            if (&event.action == &StdActions::copy)
                copyRows(body().currentItem(), event.stamp);
            else if (&event.action == &g_copyRows)
                copyRows(nullptr, event.stamp);
        });
    }

    void OutputPage::hoverEnter()
    {
        m_stopped = true;
        m_statusLabel.invalidate();
        ScrollBox::hoverEnter();
    }

    void OutputPage::hoverLeave()
    {
        m_stopped = false;
        m_statusLabel.invalidate();
        ScrollBox::hoverLeave();
    }

    void OutputPage::alignContent(AlignEvent& event, ScaledPosition pt1, ScaledDimensions& pt2)
    {
        ScrollBox::alignContent(event, pt1, pt2);
        if (m_itemToSelect)
        {
            m_itemToSelect->scrollIntoView();
            m_itemToSelect = nullptr;
        }
    }

    void OutputPage::contextPopup(ContextPopupEvent& event)
    {
        // The application gets first refusal, and a handler that stops the event has replaced the
        // menu outright.
        ScrollBox::contextPopup(event);
        if (event.propagationStopped())
            return;

        ActionList items{
            &StdActions::copy,
            &g_copyRows,
        };
        Menu menu{ *this };
        menu.add(items);
        menu.execute();
    }

    // Every row, or the one given, as one text with a line to each. A row is copied as its PLAIN
    // text: the colours a pointer line carries say nothing once it is out of this window, and a
    // reading pasted somewhere else is read as words.
    void OutputPage::copyRows(const Control* single, const InputStamp stamp)
    {
        std::wstring gathered{};
        for (const ControlPtr& row : body().controls())
        {
            if (single and row.get() != single)
                continue;
            if (!gathered.empty())
                gathered += L'\n';
            gathered += static_cast<const RichControl*>(row.get())->text().plainText();
        }
        if (gathered.empty())
            return;

        // ONE FORMAT IN. Plain text is advertised out of the conversion table, so an application
        // that has never heard of this framework pastes what it can read.
        Transfer::Source source{};
        source.add<Transfer::ClaFiText>(Text{ gathered });
        formContext().clipboard().set(std::move(source), stamp);
    }

    // DiagnosticWindow

    DiagnosticWindow::DiagnosticWindow(AppContext& appContext)
        :
        appContext{ appContext },
        form{
            appContext,
            nullptr,
            HostProps{
                appContext.themeMetrics().primaryWindow,
                appContext.themeMetrics().primaryWindowShadow,
                UiElement::Section,
                // Wide enough for the FPS page's four reading columns with their units - see FpsPage. The
                // height is the first run's; from then on the window is what the user left it.
                PreferredSize{ 400, 600 }
            },
            BodyProps{
                TabsOrientation::HorizontalBottom
            }
        }
    {
        // THE STRIP HAS TO SPAN THE BAR BEFORE ANYTHING IN IT CAN FILL. TabbedBox states
        // HorizontalAlign::Right on a bottom strip, and a control aligned to an edge is handed
        // the bar and hands back everything past its own content - see Control::align - so the
        // check and the tabs stood as one block at the right and the spacer between them had
        // nothing to take.
        form.body().strip().setHorizontalAlign(HorizontalAlign::Fill);

        output.connectCopyActions();

        onTopCheck.onGetState([this](GetStateEvent& event) {
            event.state.selected = form.isAlwaysOnTop();
        });
        onTopCheck.onClick([this](ClickEvent&) {
            form.setAlwaysOnTop(!form.isAlwaysOnTop());
            onTopCheck.invalidateState();
        });
        if (!form.canSetAlwaysOnTop())
            onTopCheck.hide();

        std::wstring windowTitle{ L"Diagnostic - " };
        windowTitle.append(appContext.appName().plainText());

        form.setWindowTitle(windowTitle);
        // The X hides: the lines keep coming, and Show diagnostic brings the same window back.
        form.setCloseAction(CloseAction::Hide);
        // Placed from the config under this name, and stored under it on every hide - see
        // FormBase::setConfigName. Nothing shows it here: whether it is up is the config's answer
        // too, and the config is not loaded yet - see restoreState.
        form.setConfigName(k_diagnosticFormName);
    }

    void DiagnosticWindow::showOrActivate(const InputStamp stamp)
    {
        if (!form.visible())
        {
            form.show();
            return;
        }
        if (form.isMinimized())
            form.restore();
        form.window().setFocus(stamp);
    }

    // THE TAB IS OPENED HERE AND NOWHERE ELSE, which is why the constructor selects none: the
    // page the window opens on is the config's answer, and the config is not loaded until now.
    // An unrecognised name reads as the Output page rather than as no page at all.
    void DiagnosticWindow::restoreState()
    {
        if (tabNode().get<std::wstring>() == k_fpsTabCaption)
            fpsTab.select();
        else
            outputTab.select();
        if (visibleNode().get<bool>())
            form.show();
    }

    // THE PLACEMENT IS WRITTEN HERE ONLY WHILE THE WINDOW IS UP: a hide has already written it,
    // and a window that is up when the config is saved would otherwise be saved where it stood
    // when it was last hidden.
    void DiagnosticWindow::storeState()
    {
        visibleNode().set(form.visible());
        // Whichever page the window is on, up or not: a hidden window is still standing on one,
        // and that is the page Show diagnostic brings back.
        const bool onFps = form.body().strip().currentItem() == &fpsTab;
        tabNode().set(std::wstring{ onFps ? k_fpsTabCaption : k_outputTabCaption });
        if (form.visible())
            appContext.storeFormPlacement(k_diagnosticFormName, form);
    }

    Dom::DomNodeBase& DiagnosticWindow::visibleNode() const
    {
        return appContext.config() / k_sectionName / k_visibleName;
    }

    Dom::DomNodeBase& DiagnosticWindow::tabNode() const
    {
        return appContext.config() / k_sectionName / k_tabName;
    }
}
