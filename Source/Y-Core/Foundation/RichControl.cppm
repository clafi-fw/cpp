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
    //export class GetTextEvent;
    //export class GetChildTextEvent;
    //export class GetTooltipEvent;
    //export class DoubleClickEvent;
    //export class PressDownEvent;
    //export struct CreateParams;
    //export class AdjustMetricsEvent;
    //export class ContextPopupEvent;
    //export class ClickEvent;
    //export class GetStateEvent;
    ////

    // Adds to Control the stored appearance state - metrics, colors, text. See Control-Foundation
    export class RichControl : public Control
    {
    public:
        template <typename... Args>
        explicit RichControl(const CreateParams& params, Args&&... args);
    public:
        // How much of the input the control takes, and whether it can hold the focus.
        DECLARE_PROPERTY_STORAGE(Interactivity, interactivity, Interactivity::None)
    public:
        // color
        [[nodiscard]] OptionalUiElement colorRules() const { return m_colorRules; }
        void setColorRules(OptionalUiElement value) { m_colorRules = value; }
        // The control's own text, handed out live. See Control-Foundation#control-text
        ControlText& text() { return m_text; }
        const ControlText& text() const { return m_text; }
        TooltipText& tooltipText() { return m_tooltipText; }
        // metrics
        const ControlMetrics& metrics() const { return m_metrics; }
        void setMetrics(const ControlMetrics& value) { m_metrics = value; }
        //
        [[nodiscard]] Padding padding() const { return m_metrics.padding; }
        void setPadding(const Padding value) { m_metrics.padding = value; }
        void setPadding(const float x, const float y) { m_metrics.padding = { x, y }; }
        void setPadding(const float value) { setPadding({ value, value }); }
        //
        [[nodiscard]] Spacing spacing() const { return m_metrics.spacing; }
        void setSpacing(const Spacing value) { m_metrics.spacing = value; }
        void setSpacing(const float x, const float y) { setSpacing({ x, y }); }
        void setSpacing(const float value) { setSpacing({ value, value }); }
        //
        [[nodiscard]] MinSize minSize() const { return m_metrics.minSize; }
        void setMinSize(const MinSize value) { m_metrics.minSize = value; }
        void setMinSize(const float x, const float y) { setMinSize({ x, y }); }
        void setMinSize(const float value) { setMinSize({ value, value }); }
        //
        [[nodiscard]] PreferredSize preferredSize() const { return m_metrics.preferredSize; }
        void setPreferredSize(const PreferredSize value) { m_metrics.preferredSize = value; }
        void setPreferredWidth(const float value) { m_metrics.preferredSize.x = value; }
        void setPreferredHeight(const float value) { m_metrics.preferredSize.y = value; }
        //
        [[nodiscard]] MaxSize maxSize() const { return m_metrics.maxSize; }
        void setMaxSize(const MaxSize value) { m_metrics.maxSize = value; }
        //
        void setFixedWidth(float value) { m_metrics.setFixedWidth(value); }
        void setFixedHeight(float value) { m_metrics.setFixedHeight(value); }
        void setFixedSize(FixedSize value) { m_metrics.setFixedSize(value); }
        //
        [[nodiscard]] Thickness border() const { return m_metrics.border; }
        void setBorder(Thickness value) { m_metrics.border = value; }
        //
        [[nodiscard]] float radius() const { return m_metrics.radius; }
        void setRadius(const float value) { m_metrics.radius = value; }
        //
        [[nodiscard]] virtual Interactivity interactivity() const override final { return m_interactivity; }
        void setInteractivity(const Interactivity value) { m_interactivity = value; }
    protected:
        // Each override adds the state RichControl stores, then lets the base emit the event.
        void adjustMetrics(AdjustMetricsEvent&) const override final;
        void adjustPaint(AdjustPaintEvent&) override;
        void getText(GetTextEvent&) const override;
        void getTooltip(GetTooltipEvent&) override;
    private:
        ControlMetrics m_metrics{};
        OptionalUiElement m_colorRules{};
        ControlText m_text{};
        TooltipText m_tooltipText{};
    };


    template<typename ...Args>
    RichControl::RichControl(const CreateParams& params, Args&& ...args)
        :
        Control{ params, std::forward<Args>(args)... },
        INIT_PROPERTY(interactivity)
    {
        // The whole metric block at once, for a control handed a themed set.
        BIND_PROPERTY_MEMBER(ControlMetrics, m_metrics);
        BIND_PROPERTY_MEMBER(MaxSize, m_metrics.maxSize); // the largest the control may be made
        BIND_PROPERTY_MEMBER(MinSize, m_metrics.minSize); // the smallest the control may be made
        // What the control asks for when it is free to choose.
        BIND_PROPERTY_MEMBER(PreferredSize, m_metrics.preferredSize);
        // Both bounds at once, pinning the control to one size.
        BIND_PROPERTY_CALL(FixedSize, setFixedSize);
        // Space kept inside the control's border, around its content.
        BIND_PROPERTY_MEMBER(Padding, m_metrics.padding);
        // Space kept between the control's children.
        BIND_PROPERTY_MEMBER(Spacing, m_metrics.spacing);
        BIND_PROPERTY_ACTION(Border, m_metrics.border = p.value); // how heavy the control's border is
        // Corner radius of the control's box.
        BIND_PROPERTY_ACTION(Radius, m_metrics.radius = p.value);

        // A rule set or none. Bound first, so a plain UiElement anywhere in the pack wins.
        BIND_PROPERTY_MEMBER(OptionalUiElement, m_colorRules);
        // Which of the theme's rule sets the control paints from.
        BIND_PROPERTY_MEMBER(UiElement, m_colorRules);

        auto appendText = [&](const auto& p) { m_text << p; };
        BIND_PROPERTY_ACTION(Text, appendText(p)); // the control's own text
        // The text the control offers as its tooltip.
        BIND_PROPERTY_ACTION(TooltipText, m_tooltipText = p);
        // The control's own text, as a view.
        BIND_PROPERTY_ACTION(std::wstring_view, appendText(p));
        // The control's own text, as a literal.
        BIND_PROPERTY_ACTION(const wchar_t*, appendText(p));

        // The On... handler properties are bound by Control's constructor.
    }

}
