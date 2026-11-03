module;
#include "../System/EventBindings.h"

export module ClaFi.Core.Foundation :RichControl;

import :Control;
import :PaintEvent;

import ClaFi.Core.TextEngine.Text;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Props;

import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;

import ClaFi.StdLib;

namespace ClaFi
{
    //// fighting the red wiggles
    //export class HoverEnterEvent;
    //export class HoverLeaveEvent;
    //export class MouseMoveEvent;
    //export class AdjustTextEvent;
    //export class AdjustChildTextEvent;
    //export class AdjustTooltipEvent;
    //export class DoubleClickEvent;
    //export class PressDownEvent;
    //export struct CreateParams;
    //export class AdjustMetricsEvent;
    //export class ContextPopupEvent;
    //export class ClickEvent;
    //export class GetStateEvent;
    ////

    // Adds to Control the stored properties See Control-Foundation
    export class RichControl : public Control
    {
    public:
        template <typename... Args>
        explicit RichControl(const CreateParams& params, Args&&... args);
    public:
        // Whether Tab and the arrows can land on the control. See UI-Types
        DECLARE_PROPERTY_STORAGE(NavigationReach, navigationReach, NavigationReach::No)
        // What a mouse press on the control does with the focus. See UI-Types
        DECLARE_PROPERTY_STORAGE(MouseFocus, mouseFocus, MouseFocus::Parent)
        // Whether a mouse press is aimed at the control or at its parent. See UI-Types
        DECLARE_PROPERTY_STORAGE(ClickTarget, clickTarget, ClickTarget::Parent)
        // Whether the control holds the focus for its current item. See UI-Types
        DECLARE_PROPERTY_STORAGE(FocusScope, focusScope, FocusScope::No)
    public:
        // color
        [[nodiscard]] OptionalUiElement colorRules() const { return m_colorRules; }
        void setColorRules(OptionalUiElement value) { m_colorRules = value; }
        // The control's own text, handed out live. See Control-Foundation#control-text
        ControlText& text() { return m_text; }
        const ControlText& text() const { return m_text; }
        TooltipText& tooltipText() { return m_tooltipText; }
        // What the control's text is laid out over, or none.
        [[nodiscard]] const TextFormat* textFormat() const override;
        void setTextFormat(const TextFormat& value) { m_textFormat = value; }
        // metrics
        const ControlMetrics& metrics() const { return m_metrics; }
        // Sizes the control by the theme's set, which a theme switch brings up to date.
        void setMetrics(MetricsSet);
        // Sizes the control by a block of its own, which no theme switch touches.
        void setMetrics(const ControlMetrics&);
        // The theme's set sizing the control, none for a block of its own. See Control-Foundation
        [[nodiscard]] OptionalMetricsSet metricsSet() const { return m_metricsSet; }
        // The sizes the control's properties and setters stated. See AppTheme#metricsfields
        [[nodiscard]] MetricsFields statedMetrics() const { return m_statedMetrics; }
        //
        [[nodiscard]] Padding padding() const { return m_metrics.padding; }
        void setPadding(Padding);
        void setPadding(const float x, const float y) { setPadding({ x, y }); }
        void setPadding(const float value) { setPadding({ value, value }); }
        //
        [[nodiscard]] Spacing spacing() const { return m_metrics.spacing; }
        void setSpacing(Spacing);
        void setSpacing(const float x, const float y) { setSpacing({ x, y }); }
        void setSpacing(const float value) { setSpacing({ value, value }); }
        //
        [[nodiscard]] MinSize minSize() const { return m_metrics.minSize; }
        void setMinSize(MinSize);
        void setMinSize(const float x, const float y) { setMinSize({ x, y }); }
        void setMinSize(const float value) { setMinSize({ value, value }); }
        //
        [[nodiscard]] PreferredSize preferredSize() const { return m_metrics.preferredSize; }
        void setPreferredSize(PreferredSize);
        void setPreferredWidth(float value);
        void setPreferredHeight(float value);
        //
        [[nodiscard]] MaxSize maxSize() const { return m_metrics.maxSize; }
        void setMaxSize(MaxSize);
        //
        void setFixedWidth(float value);
        void setFixedHeight(float value);
        void setFixedSize(FixedSize);
        //
        [[nodiscard]] Thickness border() const { return m_metrics.border; }
        void setBorder(Thickness);
        //
        [[nodiscard]] float radius() const { return m_metrics.radius; }
        void setRadius(float value);
        //
        [[nodiscard]] NavigationReach navigationReach() const final { return m_navigationReach; }
        void setNavigationReach(const NavigationReach value) { m_navigationReach = value; }
        [[nodiscard]] MouseFocus mouseFocus() const final { return m_mouseFocus; }
        void setMouseFocus(const MouseFocus value) { m_mouseFocus = value; }
        [[nodiscard]] ClickTarget clickTarget() const final { return m_clickTarget; }
        void setClickTarget(const ClickTarget value) { m_clickTarget = value; }
        [[nodiscard]] FocusScope focusScope() const final { return m_focusScope; }
        void setFocusScope(const FocusScope value) { m_focusScope = value; }
    protected:
        // Each override adds the state RichControl stores, then lets the base emit the event.
        void adjustMetrics(AdjustMetricsEvent&) const override;
        void adjustPaint(AdjustPaintEvent&) override;
        void adjustText(AdjustTextEvent&) const override;
        void nestedAdjustTooltip(AdjustTooltipEvent&) override;
        void themeMetricsChanged() override;
    private:
        void takeMetricsSet(OptionalMetricsSet, const ThemeMetrics&); // none keeps what stands
    private:
        ControlMetrics m_metrics{}; // the set's sizes, with the stated ones over them
        OptionalMetricsSet m_metricsSet{};
        MetricsFields m_statedMetrics{};
        OptionalUiElement m_colorRules{};
        ControlText m_text{};
        TextFormat m_textFormat{};
        TooltipText m_tooltipText{};
    };

    export template<typename T>
        concept IsRichControl = std::derived_from<T, RichControl>;

    // Takes an action given to a control as a construction property. Declared here and defined
    // in the implementation unit, so this interface needs no more than the action's name.
    export void attachAction(Action&, RichControl&, PresenterRole = PresenterRole::Button);


    template<typename ...Args>
    RichControl::RichControl(const CreateParams& params, Args&& ...args)
        :
        Control{ params, std::forward<Args>(args)... },
        INIT_PROPERTY(navigationReach),
        INIT_PROPERTY(mouseFocus),
        INIT_PROPERTY(clickTarget),
        INIT_PROPERTY(focusScope)
    {
        // A theme's set or none. Bound first, so a plain MetricsSet anywhere in the pack wins.
        BIND_PROPERTY_ACTION(OptionalMetricsSet, takeMetricsSet(p, params.themeMetrics()));
        // The theme's set the control is sized by. See AppTheme#metricsset
        BIND_PROPERTY_ACTION(MetricsSet, takeMetricsSet(p, params.themeMetrics()));
        // The whole metric block at once, the control's own.
        BIND_PROPERTY_CALL(ControlMetrics, setMetrics);
        BIND_PROPERTY_CALL(MaxSize, setMaxSize); // the largest the control may be made
        BIND_PROPERTY_CALL(MinSize, setMinSize); // the smallest the control may be made
        // What the control asks for when it is free to choose.
        BIND_PROPERTY_CALL(PreferredSize, setPreferredSize);
        // Both bounds at once, pinning the control to one size.
        BIND_PROPERTY_CALL(FixedSize, setFixedSize);
        // Space kept inside the control's border, around its content.
        BIND_PROPERTY_CALL(Padding, setPadding);
        BIND_PROPERTY_CALL(Spacing, setSpacing); // space kept between the control's children
        BIND_PROPERTY_VALUE(Border, setBorder); // how heavy the control's border is
        BIND_PROPERTY_VALUE(Radius, setRadius); // corner radius of the control's box

        // A rule set or none. Bound first, so a plain UiElement anywhere in the pack wins.
        BIND_PROPERTY_MEMBER(OptionalUiElement, m_colorRules);
        // Which of the theme's rule sets the control paints from.
        BIND_PROPERTY_MEMBER(UiElement, m_colorRules);

        auto appendText = [&](const auto& p) { m_text << p; };
        // An empty text takes a Text whole, its sheet with it. See Control-Foundation#control-text
        const auto takeText = [&](const Text& p) {
            if (m_text.empty())
                m_text = p;
            else
                m_text << p;
        };
        BIND_PROPERTY_ACTION(Text, takeText(p)); // the control's own text
        // What the text is laid out over. See Control-Foundation#richcontrol-textformat
        BIND_PROPERTY_ACTION(TextFormat, m_textFormat << p);
        BIND_PROPERTY_ACTION(TooltipText, m_tooltipText = p); // the control's own tooltip
        // The control's own text, as a view.
        BIND_PROPERTY_ACTION(std::wstring_view, appendText(p));
        // The control's own text, as a literal.
        BIND_PROPERTY_ACTION(const wchar_t*, appendText(p));
        // What an Action given as a property takes this control for.
        const PresenterRole presenterRole = READ_PROPERTY(PresenterRole, PresenterRole::Button);
        // An action this control presents, taking it as a presenter. Bound after the text, since
        // the action writes its words only into a text with none. See Control-Foundation
        BIND_PROPERTY_ACTION(Action, attachAction(p, *this, presenterRole));

        // The On... handler properties are bound by Control's constructor.
    }

}
