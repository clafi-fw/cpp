export module ClaFi.Controls.FormTitle;

import ClaFi.Controls.Panel;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.Button;

import ClaFi.Icons.XMark;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.Foundation;

import ClaFi.Core.Context.FormContext;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    class SysMenuButton : public ToolButton
    {
    public:
        template <typename... Args>
        explicit SysMenuButton(const CreateParams&, Args&&...);
    };

    export class MinimizeButton : public SysMenuButton
    {
    public:
        using SysMenuButton::SysMenuButton;
    protected:
        void paintIcon(PaintIconEvent&) override;
        void click(ClickEvent&) override;
    };

    export class MaximizeButton : public SysMenuButton
    {
    public:
        using SysMenuButton::SysMenuButton;
    protected:
        void hitTest(HitTestEvent& event) const override { event.zone = HitTest::MaxButton; }
        void paintIcon(PaintIconEvent&) override;
        void click(ClickEvent&) override;
    };

    export class CloseButton : public SysMenuButton
    {
    public:
        using SysMenuButton::SysMenuButton;
    protected:
        void paintIcon(PaintIconEvent&) override;
        void click(ClickEvent&) override;
    };

    // The bar across the top of a window: its name, and the buttons that size it.
    export class FormTitle : public Panel
    {
    public:
        template<typename... Args>
        explicit FormTitle(const CreateParams&, Args&&...);
    protected:
        void adjustPaint(AdjustPaintEvent&) override;
        void hitTest(HitTestEvent& event) const override { event.zone = HitTest::Title; };
    private:
        StackPanel& m_sysButtons{ createRightBar<StackPanel>(
            Interactivity::ActiveContainer,
            Orientation::Horizontal,
            Padding{ 4.0f, 4.0f }
        ) };
        MinimizeButton& m_minimizeButton{ m_sysButtons.add<MinimizeButton>(L"Minimize")};
        MaximizeButton& m_maximizeButton{ m_sysButtons.add<MaximizeButton>() };
        CloseButton& m_closeButton{ m_sysButtons.add<CloseButton>(L"Close") };
    };


    //----------------------------------------------------------------------------


    constexpr IconSize k_iconSize{ 12.0f };

    // SysMenuButton

    // The strip these sit in is an ActiveContainer, so it keeps whichever button the user last
    // reached as its current item and reports that one as selected. A title-bar button wearing a
    // selected fill reads as a window state stuck on, so the fill is declined here; the focus
    // ring still says where the keyboard is.
    template<typename ...Args>
    SysMenuButton::SysMenuButton(const CreateParams& params, Args && ... args)
        :
        ToolButton{
            params,
            ButtonViewMode::IconOnly,
            k_iconSize,
            ShowSelectionOnSurface::No,
            std::forward<Args>(args)...
        }
    {
    }

    // MinimizeButton

    void MinimizeButton::paintIcon(PaintIconEvent& event)
    {
        float centerY = event.iconRect().centerY();
        event.canvas().drawLine(
            { event.iconRect().left, centerY },
            { event.iconRect().right, centerY },
            event.textRgb(InkGrade::Strongest),
            event.scaleF(1.0f)
            );
    }

    void MinimizeButton::click(ClickEvent& event)
    {
        event.form.minimize();
    }

    // MaximizeButton

    void MaximizeButton::paintIcon(PaintIconEvent& event)
    {
        FloatRect rect = event.iconRect();
        float strokeWidth = event.scale(1.0f);
        float radius = event.scale(2.0f);
        Color strokeColor = event.textRgb(InkGrade::Strongest);
        if (form().isMaximized())
        {
            float shift = event.scale(1.0f);
            radius -= shift / 2.0f;
            rect.inflate(-shift);
            rect.offset(shift, -shift);
            event.canvas().drawRoundedRectangle(rect, radius, radius, strokeColor, strokeWidth);

            shift *= 2;
            rect.offset(-shift, shift);
            // Masks the back square where the front one crosses it, so the two read as overlapping
            // sheets. That wants the colour behind this icon, which is what textColor().surface
            // is - the effective backdrop, already accumulated down the parent chain. The
            // control's border colour is not a substitute: it sits close enough to the surface
            // on the current themes to pass, and would drift apart on a theme where it does not.
            Color bgColor = event.surfaceRgb();
            event.canvas().fillRoundedRectangle(rect, radius, radius, bgColor);
        }
        event.canvas().drawRoundedRectangle(rect, radius, radius, strokeColor, strokeWidth);
    }

    void MaximizeButton::click(ClickEvent& event)
    {
        if (event.form.isMaximized())
            event.form.restore();
        else
            event.form.maximize();
    }

    // CloseButton

    void CloseButton::paintIcon(PaintIconEvent& event)
    {
        Icons::XMark::paint(event);
    }

    void CloseButton::click(ClickEvent& event)
    {
        event.closeForm();
    }

    // FormTitle

    template<typename ...Args>
    FormTitle::FormTitle(const CreateParams& params, Args&&... args)
        :
        Panel{
            params,
            UiElement::FormTitle,
            params.themeMetrics().formTitle,
            VerticalTextAnchor::Center,
            HorizontalTextAnchor::Center,
            WordWrap::No,
            std::forward<Args>(args)...
        }
    {
    }

    void FormTitle::adjustPaint(AdjustPaintEvent& event)
    {
        Panel::adjustPaint(event);
        event.setWindowSelectedAmount(1.0f);
    }

}
