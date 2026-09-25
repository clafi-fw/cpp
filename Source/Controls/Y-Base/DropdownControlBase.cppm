module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Base.DropdownControlBase;

export import ClaFi.Controls.Base.SplitButtonBase;
import ClaFi.Controls.Button;

import ClaFi.Icons.Chevron;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;
import ClaFi.Core.Context.FormContext;

namespace ClaFi::Controls
{
    export class DropdownControlBase;

    // DropdownPart

    /// The dropdown strip. It is a button in its own right for its colours, its states and its
    /// press.
    ///
    /// @note WHAT IT CARRIES DEPENDS ON WHETHER THE OWNER SENT ITS TEXT DOWN. A strip on its own
    /// is icon-only and the mark IS that icon, centred in the strip by the view mode. A ribbon
    /// strip - the owner's text moved onto it, see textOnDropdown - is a text label instead, and
    /// there the mark is the last glyph of the run so that the label and the mark centre together
    /// as one line.
    ///
    /// @note WHOSE AVAILABILITY THE STRIP READS IS THE OWNER'S CALL. Where the owner says the
    /// strip acts alone - see dropdownActsAlone - the strip stands for a second command beside
    /// the button's, a Save that has nothing to write over still has a Save as behind its strip,
    /// so the button being unavailable does not make the strip so. What the strip inherits even
    /// then is a genuinely disabled ANCESTOR: a page turned off turns off everything standing on
    /// it. Otherwise, which is the default, the two halves are one command and the strip is
    /// exactly as available as the control.
    ///
    /// The class stays complete here rather than moving to the implementation unit: the owner's
    /// constructor is a template and instantiates createSecondaryPart<DropdownPart> at every call
    /// site, so the definition has to be reachable from this interface.
    class DropdownPart : public ToolButton
    {
        friend DropdownControlBase;
    public:
        DropdownPart(const CreateParams&, DropdownControlBase&, ArrowPlacement);
        std::wstring_view diagnosticText() const override { return L"DropdownPart"; }
        [[nodiscard]] bool enabled(bool deep = false) const override;
    protected:
        void getControlState(GetStateEvent&) const override;
        void getText(GetTextEvent&) const override;
        void paintIcon(PaintIconEvent&) override;
        void adjustPaint(AdjustPaintEvent&) override;
        void paintSurface(PaintEvent&) override;
    private:
        [[nodiscard]] bool enabledAlone() const;
        void paintDivider(PaintEvent&) const;
    private:
        DropdownControlBase& m_owner;
    };

    // DropdownControlBase

    // A control that drops a popup, and the strip that says so. See Controls-Base
    export class DropdownControlBase : public SplitButtonBase
    {
        friend DropdownPart;
    public:
        template <typename... Args>
        DropdownControlBase(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] ArrowPlacement arrowPlacement() const { return m_config.placement; }
        /// Whether the control's own text has moved onto the strip, which is what a ribbon layout
        /// does. Read by the strip as it is built, to settle what it shows.
        [[nodiscard]] bool textOnDropdown() const { return m_config.textOnDropdown; }
        [[nodiscard]] float dropdownWidth() const { return m_config.dropdownWidth; }
        void setDropdownWidth(const float value);
        /// Whether the control's popup is up. True for every route into it - a press on the strip,
        /// a press on the control's own face, F4, dropDown() - rather than for whichever half of
        /// the control owns the popup.
        [[nodiscard]] bool droppedDown() const { return m_droppedDown; }
        /// How far the dropdown mark has turned: 0 with the popup closed, 1 with it open, animated
        /// between the two. A control painting a mark of its own follows the popup with this rather
        /// than with a selected state, which a container above it is free to write.
        [[nodiscard]] float droppedDownFactor() const { return m_droppedDownFactor; }
        /// Whether the control has a list to drop. A strip that is not shown says there is
        /// nothing under this control; one that was never made - an in-text mark - says nothing
        /// either way, and the control drops as it always does.
        [[nodiscard]] bool canDropDown() const;
        /// Opens the dropdown as if the control had been pressed. A control with nothing to drop
        /// answers nothing.
        void dropDown();
    protected:
        struct Config
        {
            ArrowPlacement placement{ ArrowPlacement::Right };
            ButtonViewMode viewMode{ ButtonViewMode::TextLabel };
            float dropdownWidth{ DropdownWidth{}.value };
            // A ribbon control hands its text down to the strip and keeps only the icon on top.
            bool textOnDropdown{ false };
        };
        // Runs the popup. The initiator is the control the press landed on, and it is what the
        // popup has to be owned by - see dropPopup().
        virtual void showDropdown(Control& initiator) = 0;
        // Whether pressing the control outside the strip opens the dropdown too. A combobox says
        // yes: every part of it opens the same list. A split button says no: its main half is a
        // separate action.
        [[nodiscard]] virtual bool dropOnPrimaryPress() const { return false; }
        // WHETHER THE STRIP IS A COMMAND OF ITS OWN, and so stands or falls apart from the
        // control's own face. A combobox says no - every part of it opens the same list, so a
        // combobox that cannot be used cannot be dropped either. A split button says yes: its
        // face carries one command and its strip another, and a Save with nothing to write over
        // still has a Save as behind it.
        //
        // A strip that acts alone is available whenever it is SHOWN. A control that wants it
        // unavailable hides it, which the base already reads as the strip not being there at all -
        // see shownSecondaryPart. There is no third answer to give: the state the control reports
        // is the one an attached action wrote into it, and the strip's command is not that one.
        [[nodiscard]] virtual bool dropdownActsAlone() const { return false; }
        // The control's own text, without the mark. Overridden by a control whose text is not
        // simply the text property - a combobox's is its selected item.
        virtual void getMainText(GetTextEvent&) const;
        // The mark an InText control ends its line with, put after the text given.
        void appendInTextMark(Text&) const;
        [[nodiscard]] virtual Ink dropdownMarkInk() const { return InkWell::textInk(InkGrade::Muted); }
        // Which way the mark points at each end of its turn. A control that means something by the
        // direction of its mark says so here - a breadcrumb crumb points along the path and turns
        // to point into the list it drops - and one that only means open or shut takes the
        // default, which reads as a list falling and being put back.
        [[nodiscard]] virtual DropdownMarkTurn dropdownMarkTurn() const { return {}; }
        // Draws the mark in whatever slot it was given - the strip's icon, or the box an in-text
        // mark holds in the control's own line. The two differ in where the slot comes from and in
        // nothing else, so the colour and the turn are settled here for both.
        void paintDropdownMark(PaintIconEvent&) const;

        // Drops a popup under this control and runs it.
        //
        // The owner must be the control the press actually landed on. FormBase::wnd_mouseUp only
        // reads a second press as "close me" when the popup's target is the control the mouse
        // went down on; owned by anything else, that press closes the popup and the click then
        // reopens it on the way back up.
        template <ClassOfFormControl ControlClass, typename... Args>
        int dropPopup(FormBase&, Control& owner, Args&&...);

        [[nodiscard]] SecondaryEdge secondaryEdge() const override;
        // The strip is the control's own edge on the side it sits on, so it takes the content
        // padding there with it.
        [[nodiscard]] bool secondaryReachesEdge() const override { return true; }
        void placeSecondaryPart(ScaledDimensions childArea) override;
        void secondaryClicked(ClickEvent&) override;

        void adjustChildInset(AdjustChildInsetEvent&) const override;
        void getText(GetTextEvent&) const override;
        void calculateChildren(FormBase&) override;
        void adjustChildMetrics(AdjustMetricsEvent&) const override;
        void adjustChildPaint(AdjustPaintEvent&) override;
        void click(ClickEvent&) override;
        void keyDown(KeyDownEvent&) override;
    private:
        template <typename... Args>
        static Config makeConfig(const Args&...);
        static ButtonViewMode mainViewMode(const Config&);
        // The text a ribbon strip carries: the control's own text, then the mark. A strip that
        // carries no text states nothing here - the mark is its icon.
        void getDropdownPartText(GetTextEvent&) const;
        void appendDropdownMark(Text&) const;
        // Opens the popup and holds the dropped down state for as long as it is up. showDropdown()
        // runs the popup modally, so it returns once the popup has closed.
        void runDropdown(Control& initiator);
        void setDroppedDown(const bool value);
        // The control the mark is drawn on: the strip when there is one, otherwise this control,
        // whose own text line carries it.
        [[nodiscard]] const Control& markHolder() const;
    private:
        // Design box the mark is drawn in: the strip's icon size, and the room an in-text mark
        // takes at the end of the text. The mark itself is smaller than its box.
        inline static constexpr float k_markBox{ 12.0f };
        inline static constexpr float k_markSpacing{ 4.0f };
        Config m_config;
        bool m_droppedDown{ false };
        float m_droppedDownFactor{ 0.0f };
    };


    //-------------------------------------------------------------------------


    // DropdownControlBase
    //
    // Only the templates stay here - the constructor, makeConfig() and dropPopup() - because every
    // caller instantiates them from this interface. Every other body lives in
    // DropdownControlBase.cpp.

    // makeConfig() is called twice rather than routed through a delegating constructor. A private
    // constructor taking the Config would be ambiguous with this one: a Config argument
    // binds to Config&& here and to const Config& there, both identity conversions, and the
    // reference binding tiebreaker picks the less cv-qualified one - this constructor - before
    // partial ordering is ever consulted. The delegation would then recurse, growing the pack by
    // one Config each round until the compiler gives up. Reading the pack after forwarding it is
    // safe because Props only ever copies out of it, which is what ButtonBase already relies on.
    template<typename ...Args>
    DropdownControlBase::DropdownControlBase(const CreateParams& params, Args && ...args)
        :
        SplitButtonBase{ params, std::forward<Args>(args)..., mainViewMode(makeConfig(args...)) },
        m_config{ makeConfig(args...) }
    {
        if (m_config.placement == ArrowPlacement::InText)
            return;

        createSecondaryPart<DropdownPart>(*this, m_config.placement);
    }

    template<ClassOfFormControl ControlClass, typename ...Args>
    int DropdownControlBase::dropPopup(FormBase& form, Control& owner, Args&&... args)
    {
        Form<ControlClass> popup = form.createPopup<ControlClass>(
            &owner,
            std::forward<Args>(args)...
        );
        popup.setDropdownClearance(1.0f);
        // Anchored to the whole control, which is what the user reads as the thing being dropped,
        // even when a strip of it is what owns the popup.
        popup.setPlacement(FormPlacement::Bottom, boundsInForm());
        // And at least as wide as it - a list narrower than the face it fell from reads as
        // belonging to something else. In design units, which a popup shares with its parent.
        popup.setMinWidth(width() / form.scaler().factor());
        return popup.execute();
    }

    template<typename ...Args>
    DropdownControlBase::Config DropdownControlBase::makeConfig(const Args&... args)
    {
        Config result;
        // How the control lays its icon out against its text.
        result.viewMode = READ_PROPERTY(ButtonViewMode, ButtonViewMode::TextLabel);
        // The design width of the dropdown strip.
        result.dropdownWidth = READ_PROPERTY(DropdownWidth, DropdownWidth{}).value;
        // Where the dropdown mark sits.
        result.placement = READ_PROPERTY(ArrowPlacement, ArrowPlacement::Auto);
        if (result.placement == ArrowPlacement::Auto)
        {
            if (hasTopIcon(result.viewMode))
                result.placement = ArrowPlacement::Bottom;
            else
                result.placement = ArrowPlacement::Right;
        }
        result.textOnDropdown =
            result.placement == ArrowPlacement::Bottom
            && hasTopIcon(result.viewMode);
        return result;
    }

}
