module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Base.ButtonBase;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{

    // A control that answers a click, with a face, an icon and a selection indicator.
    export class ButtonBase : public RichControl
    {
    public:
        template <typename... Args>
        explicit ButtonBase(const CreateParams&, Args&&... args);
        ~ButtonBase() override;
    public:
        // Whether the button shows its selection indicator, and when.
        DECLARE_WRITABLE_PROPERTY(IndicatorVisibility, indicatorVisibility, setIndicatorVisibility, IndicatorVisibility::None)
        // What that indicator is drawn as.
        DECLARE_WRITABLE_PROPERTY(IndicatorStyle, indicatorStyle, setIndicatorStyle, IndicatorStyle::Check)
        // Whether the selection is shown on the button's surface as well.
        DECLARE_WRITABLE_PROPERTY(ShowSelectionOnSurface, showSelectionOnSurface, setShowSelectionOnSurface, ShowSelectionOnSurface::No)
        // Where the indicator sits.
        DECLARE_WRITABLE_PROPERTY(IndicatorPlacement, indicatorPlacement, setIndicatorPlacement, IndicatorPlacement::LeftCenter)
        // The design size the button draws its icon at.
        DECLARE_WRITABLE_PROPERTY(IconSize, iconSize, setIconSize, 16.0f)
        // How the button lays its icon out against its text.
        DECLARE_WRITABLE_PROPERTY(ButtonViewMode, viewMode, setViewMode, ButtonViewMode::TextLabel)
        // The icon at the end of the line. See Controls-Base
        DECLARE_WRITABLE_PROPERTY(EndIcon, endIcon, setEndIcon, EndIcon::None)
    public:
        std::wstring_view diagnosticText() const override { return  L"ButtonBase"; }
        void setIndicatorVisibility(const IndicatorVisibility value);
        void setIndicatorStyle(const IndicatorStyle value);
        void setShowSelectionOnSurface(const ShowSelectionOnSurface value);
        void setIndicatorPlacement(const IndicatorPlacement value);
        void setIconSize(IconSize);
        void setViewMode(ButtonViewMode);
        void setEndIcon(EndIcon);
    protected:

        ControlSpan controls() override;
        virtual MinSize indicatorSize(const AppTheme&) const;
        // The selection indicator, and nullptr while none has been asked for.
        [[nodiscard]] const Control* indicator() const { return m_indicator; }

        void adjustChildMetrics(AdjustMetricsEvent& event) const override;
        void adjustChildPaint(AdjustPaintEvent&) override;
        void paintChildSurface(PaintEvent&) override;
        void adjustPaint(AdjustPaintEvent&) override;
        void paintSurface(PaintEvent&) override;
        // Draws the icon over whatever the surface left behind. It is reachable on its own so
        // that a control painting a surface of its own - a tab draws a silhouette instead of
        // the rounded rect Control paints - can still put an icon on top of it.
        void paintIconLayer(PaintEvent&);
        void paintText(PaintEvent&) override;
        virtual FloatRect iconRect(const PaintEvent&) const;
        void adjustTextRect(AdjustTextRectEvent&) const override;
        void getText(GetTextEvent&) const override;
        void nestedGetHint(GetHintEvent&) override;
        CalculatedDimensions measureText(AlignEvent&, ScaledDimensions asked, const Text&) override;
        void calculateChildren(FormBase& form) override;
        ScaledDimensions calculateContent(AlignEvent&) override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&) override;
        void nestedPressDown(PressDownEvent&) override;
        void nestedPressUp(PressUpEvent&) override;
        void nestedClick(ClickEvent&) override;
        void nestedControlFocusing(FocusEvent&) override;
        // A button's children are its parts, and there is room for two - see addChild.
        bool isChildPart(const Control&) const override { return true; }
        // Registers a child owned by a derived class. It shares one array with the selection
        // indicator, so controls() can hand out both in a single contiguous span. There is room
        // for exactly one - a button is not a container.
        Control& addChild(ControlPtr&&);
    private:
        Control& insertIndicator();
        void applyIndicatorVisibility();
        void injectCtrl(ClickEventBase&);
        void adjustForIndicator(ScaledSpacing spacing, FloatRect& rect) const;
        // Takes the indicator and a left icon off the box the text stands in, with their gaps.
        void adjustForSideParts(ScaledSpacing spacing, float iconWidth, FloatRect& rect) const;
        [[nodiscard]] bool showsIcon() const { return m_viewMode != ButtonViewMode::TextLabel; }
    private:
        
        // Both of them depend on text size - making them const is wrong:
        // How large the end icon is drawn, in design units.
        static constexpr float k_endIconSize = 18.0f;
        // The least room left between the caption and the end icon.
        static constexpr float k_endIconGap = 8.0f;
        
        static constexpr std::size_t k_maxChildren = 2;
        using ChildArray = std::array<ControlPtr, k_maxChildren>;
        // Occupied slots always form a prefix, so controls() never hands out an empty ControlPtr.
        // The indicator holds the first slot because it paints to the left of everything else.
        ChildArray m_children{};
        std::size_t m_childCount{ 0 };
        // Non-owning view into m_children.
        Control* m_indicator{ nullptr };
    };


    //-------------------------------------------------------------------------


    // ButtonBase
    //
    // The constructor is the one definition that has to stay here: it is a template, so every
    // caller instantiates it from this interface. Every other body lives in ButtonBase.cpp, which
    // keeps edits to them from rebuilding the modules that import this one.

    template<typename ...Args>
    ButtonBase::ButtonBase(const CreateParams& params, Args&& ...args)
        :
        RichControl{ params, std::forward<Args>(args)... },
        INIT_PROPERTY(indicatorVisibility),
        INIT_PROPERTY(indicatorStyle),
        INIT_PROPERTY(showSelectionOnSurface),
        INIT_PROPERTY(indicatorPlacement),
        INIT_PROPERTY(iconSize),
        INIT_PROPERTY(viewMode),
        INIT_PROPERTY(endIcon)
    {
        applyIndicatorVisibility();
    }

} // of namespace ClaFi
